"""Cloud boundary and official-SDK integration tests; no MQTT or device access.

Run: python -m unittest integrations.cloud.test_server -v
"""
from __future__ import annotations

import asyncio
from dataclasses import replace
import hashlib
from pathlib import Path
import site
import socket
import threading
import unittest
from unittest.mock import AsyncMock, Mock, patch

ROOT = Path(__file__).resolve().parents[2]
site.addsitedir(str(ROOT / ".tools" / "mcp-python"))

import httpx
from mcp import ClientSession
from mcp.client.sse import sse_client
from mcp.client.streamable_http import streamable_http_client
from starlette.testclient import TestClient
import uvicorn

from integrations.cloud.server import CloudConfig, create_app

TEST_TOKEN = "cloud-unit-test-only-token-1234567890"
AUTH = {"Authorization": "Bearer " + TEST_TOKEN}
TOOLS = {"get_planter_state", "water", "stop_water", "set_light", "print_note", "get_action_result"}


class FakeBridge:
    def __init__(self):
        self.started = self.closed = 0
        self.calls = []

    async def start(self):
        self.started += 1

    async def close(self):
        self.closed += 1

    async def get_state(self):
        self.calls.append("state")
        return {"_bridge": {"fresh": False}, "available": False, "error": "device_offline"}

    async def act(self, request_id, action, **kwargs):
        self.calls.append((request_id, action, kwargs))
        return {"ok": False, "request_id": request_id, "status": "rejected",
                "error": "cloud_writes_not_enabled", "sent": False}

    async def get_result(self, request_id):
        self.calls.append(("result", request_id))
        return {"ok": False, "request_id": request_id, "status": "unknown", "error": "device_offline"}


class ConfigTests(unittest.TestCase):
    def test_missing_or_short_token_refuses_startup(self):
        with patch.dict("os.environ", {}, clear=True):
            with self.assertRaises(ValueError):
                create_app(bridge=FakeBridge())
        for token in ("", "short", "bad token with spaces" * 3, "a" * 129):
            with self.subTest(length=len(token)), self.assertRaises(ValueError):
                CloudConfig(token)

    def test_config_repr_and_validation_do_not_reveal_secrets(self):
        self.assertNotIn(TEST_TOKEN, repr(CloudConfig(TEST_TOKEN)))
        secret = "invalid secret that must never be printed"
        with self.assertRaises(ValueError) as caught:
            CloudConfig(secret)
        self.assertNotIn(secret, str(caught.exception))

    def test_no_wildcard_hosts_or_insecure_browser_origins(self):
        for overrides in ({"allowed_hosts": ("*",)}, {"allowed_hosts": ()},
                          {"allowed_origins": ("http://emqx.robomon.cn",)},
                          {"allowed_origins": ("https://emqx.robomon.cn/path",)},
                          {"root_path": "/roboplanter/"}):
            with self.subTest(overrides=overrides), self.assertRaises(ValueError):
                CloudConfig(TEST_TOKEN, **overrides)


class BoundaryTests(unittest.TestCase):
    def setUp(self):
        self.bridge = FakeBridge()
        self.app = create_app(CloudConfig(TEST_TOKEN), bridge=self.bridge)
        self.client = TestClient(self.app, base_url="http://localhost:18765", root_path="/roboplanter")
        self.client.__enter__()

    def tearDown(self):
        self.client.__exit__(None, None, None)
        self.assertEqual(self.bridge.started, 1)
        self.assertEqual(self.bridge.closed, 1)

    def test_health_is_liveness_only_no_credentials_or_device_claim(self):
        response = self.client.get("/healthz")
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.json(), {"status": "ok", "service": "roboplanter-mcp", "writes_enabled": False})
        self.assertEqual(self.bridge.calls, [])
        self.assertNotIn(TEST_TOKEN, response.text)

    def test_every_transport_and_unknown_path_requires_bearer(self):
        for method, path in (("POST", "/mcp"), ("GET", "/sse"), ("POST", "/messages/"),
                             ("GET", "/unknown"), ("POST", "/healthz")):
            with self.subTest(path=path, method=method):
                response = self.client.request(method, path)
                self.assertEqual(response.status_code, 401)
                self.assertIn("Bearer", response.headers["www-authenticate"])
        self.assertEqual(self.bridge.calls, [])

    def test_wrong_query_cookie_and_duplicate_tokens_do_not_authenticate(self):
        for kwargs in ({"headers": {"Authorization": "Bearer wrong"}},
                       {"params": {"access_token": TEST_TOKEN}},
                       {"headers": {"Cookie": "access_token=" + TEST_TOKEN}},
                       {"headers": [("Authorization", AUTH["Authorization"]),
                                    ("Authorization", AUTH["Authorization"])]}):
            with self.subTest(kind=tuple(kwargs)):
                self.assertEqual(self.client.post("/mcp", **kwargs).status_code, 401)

    def test_bearer_scheme_is_case_insensitive_but_credentials_are_not(self):
        headers = {"Authorization": "bEaReR " + TEST_TOKEN,
                   "Accept": "application/json, text/event-stream"}
        response = self.client.post("/mcp", headers=headers, json={"jsonrpc": "2.0", "id": 1, "method": "tools/list"})
        self.assertEqual(response.status_code, 200)
        self.assertEqual(self.client.post("/mcp", headers={"Authorization": "Bearer " + TEST_TOKEN.upper()}).status_code, 401)

    def test_fixed_length_digest_uses_constant_time_comparison(self):
        with patch("integrations.cloud.server.hmac.compare_digest", wraps=__import__("hmac").compare_digest) as compare:
            self.client.post("/mcp", headers={"Authorization": "Bearer wrong"})
            self.assertEqual(compare.call_count, 1)
            first, second = compare.call_args.args
            self.assertEqual(len(first), hashlib.sha256().digest_size)
            self.assertEqual(len(second), hashlib.sha256().digest_size)

    def test_host_and_origin_are_exact_allowlists(self):
        for headers, expected in (({**AUTH, "Host": "attacker.example"}, 421),
                                  ({**AUTH, "Origin": "https://attacker.example"}, 403),
                                  ({**AUTH, "Origin": "null"}, 403),
                                  ([("Host", "localhost:18765"), ("Host", "attacker.example")], 421)):
            with self.subTest(expected=expected):
                self.assertEqual(self.client.get("/healthz", headers=headers).status_code, expected)
        self.assertEqual(self.client.get("/healthz", headers={"Origin": "https://emqx.robomon.cn"}).status_code, 200)

    def test_authenticated_oversized_body_is_rejected_before_bridge(self):
        response = self.client.post("/mcp", headers={**AUTH, "Content-Type": "application/json"}, content=b"x" * 16385)
        self.assertEqual(response.status_code, 413)
        self.assertEqual(self.bridge.calls, [])


class SdkTests(unittest.IsolatedAsyncioTestCase):
    async def exercise_session(self, session, bridge):
        initialized = await session.initialize()
        self.assertEqual(initialized.serverInfo.name, "RoboPlanter")
        discovered = await session.list_tools()
        self.assertEqual({tool.name for tool in discovered.tools}, TOOLS)
        water = next(tool for tool in discovered.tools if tool.name == "water")
        self.assertEqual(water.inputSchema["properties"]["duration_ms"]["maximum"], 30000)
        self.assertTrue(water.annotations.destructiveHint)
        state = await session.call_tool("get_planter_state")
        self.assertFalse(state.isError)
        self.assertFalse(state.structuredContent["_bridge"]["fresh"])
        # Tools remain discoverable/importable while the cloud bridge denies all writes.
        result = await session.call_tool("water", {"request_id": "sdk_test_1", "duration_ms": 2000})
        self.assertEqual(result.structuredContent["status"], "rejected")
        self.assertFalse(result.structuredContent["sent"])
        self.assertEqual(result.structuredContent["error"], "cloud_writes_not_enabled")
        bad_duration = await session.call_tool("water", {"request_id": "sdk_bad", "duration_ms": 30001})
        self.assertTrue(bad_duration.isError)
        self.assertEqual(len(bridge.calls), 2)

    async def test_official_streamable_http_client_initialize_list_and_call(self):
        bridge = FakeBridge()
        app = create_app(CloudConfig(TEST_TOKEN), bridge=bridge)
        async with app.router.lifespan_context(app):
            async with httpx.AsyncClient(transport=httpx.ASGITransport(app), base_url="http://localhost:18765",
                                         headers=AUTH) as client:
                async with streamable_http_client("http://localhost:18765/mcp", http_client=client) as (read, write, _):
                    async with ClientSession(read, write) as session:
                        await self.exercise_session(session, bridge)
        self.assertEqual((bridge.started, bridge.closed), (1, 1))

    async def test_official_sse_client_with_public_prefix_initialize_list_and_call(self):
        bridge = FakeBridge()
        listener = socket.socket()
        listener.bind(("127.0.0.1", 0))
        host = f"127.0.0.1:{listener.getsockname()[1]}"
        config = replace(CloudConfig(TEST_TOKEN), allowed_hosts=(host,))
        app = create_app(config, bridge=bridge)

        async def proxy_app(scope, receive, send):
            # Simulates the proxy+Uvicorn public-prefix scope without contacting a proxy.
            if scope["type"] == "http":
                scope = dict(scope, root_path="/roboplanter")
            await app(scope, receive, send)

        server = uvicorn.Server(uvicorn.Config(proxy_app, host="127.0.0.1", port=0, log_level="error",
                                               access_log=False, timeout_graceful_shutdown=2))
        thread = threading.Thread(target=server.run, kwargs={"sockets": [listener]}, daemon=True)
        thread.start()
        try:
            for _ in range(250):
                if server.started:
                    break
                if not thread.is_alive():
                    self.fail("Test server exited before startup")
                await asyncio.sleep(.02)
            self.assertTrue(server.started)
            async with asyncio.timeout(15):
                async with sse_client(f"http://{host}/roboplanter/sse", headers=AUTH) as (read, write):
                    async with ClientSession(read, write) as session:
                        await self.exercise_session(session, bridge)
        finally:
            server.should_exit = True
            await asyncio.to_thread(thread.join, 5)
            listener.close()
        self.assertFalse(thread.is_alive())
        self.assertEqual((bridge.started, bridge.closed), (1, 1))

    async def test_bridge_is_closed_when_start_fails(self):
        bridge = FakeBridge()

        async def failed_start():
            raise RuntimeError("simulated startup failure")

        bridge.start = failed_start
        app = create_app(CloudConfig(TEST_TOKEN), bridge=bridge)
        with self.assertRaisesRegex(RuntimeError, "simulated startup failure"):
            async with app.router.lifespan_context(app):
                pass
        self.assertEqual(bridge.closed, 1)

    async def test_real_mqtt_bridge_rejects_every_write_without_publication(self):
        from integrations.cloud.mqtt_bridge import MqttBridge
        from integrations.cloud.protocol import MqttConfig
        bridge = MqttBridge(MqttConfig(username="unit-test", password="unit-test-only"))
        bridge.start, bridge.close = AsyncMock(), AsyncMock()
        bridge._publish = Mock(side_effect=AssertionError("No cloud write may publish MQTT"))
        app = create_app(CloudConfig(TEST_TOKEN), bridge=bridge)
        async with app.router.lifespan_context(app):
            async with httpx.AsyncClient(transport=httpx.ASGITransport(app), base_url="http://localhost:18765",
                                         headers=AUTH) as client:
                async with streamable_http_client("http://localhost:18765/mcp", http_client=client) as (read, write, _):
                    async with ClientSession(read, write) as session:
                        await session.initialize()
                        for name, arguments in (
                            ("water", {"request_id": "real_water", "duration_ms": 2000}),
                            ("stop_water", {"request_id": "real_stop"}),
                            ("set_light", {"request_id": "real_light", "on": True}),
                            ("print_note", {"request_id": "real_print", "text": "Test"}),
                        ):
                            result = await session.call_tool(name, arguments)
                            self.assertEqual(result.structuredContent["status"], "rejected")
                            self.assertEqual(result.structuredContent["error"], "cloud_writes_not_enabled")
                            self.assertFalse(result.structuredContent["sent"])
        bridge._publish.assert_not_called()
        bridge.start.assert_awaited_once()
        bridge.close.assert_awaited_once()


if __name__ == "__main__":
    unittest.main()
