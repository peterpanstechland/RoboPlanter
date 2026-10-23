"""Offline transport tests; no broker, device, or credentials are contacted."""
from __future__ import annotations

import asyncio
import copy
from pathlib import Path
import site
import ssl
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import AsyncMock, patch
import uuid

ROOT = Path(__file__).resolve().parents[2]
site.addsitedir(str(ROOT / ".tools" / "mcp-python"))
sys.path.insert(0, str(ROOT))

import httpx
import paho.mqtt.client as mqtt

from integrations.mcp.bridge import Config, PlanterBridge
from integrations.cloud.gateway import Gateway
from integrations.cloud.mqtt_bridge import MqttBridge
from integrations.cloud.protocol import (MqttConfig, MAX_PAYLOAD, RPC_TTL_MS,
    PRESENCE_TTL_MS, decode, encode, now_ms)


class Bus:
    def __init__(self):
        self.clients = []
        self.published = []

    def factory(self, **kwargs):
        client = FakeClient(self, kwargs)
        self.clients.append(client)
        return client


class FakeClient:
    def __init__(self, bus, options):
        self.bus, self.options = bus, options
        self.topics = []
        self.subscription_options = []
        self.started = False

    def tls_set_context(self, context):
        self.tls_context = context

    def username_pw_set(self, username, password):
        self.username, self.password = username, password

    def max_queued_messages_set(self, value):
        self.max_queue = value

    def reconnect_delay_set(self, **kwargs):
        pass

    def connect_async(self, host, port, **kwargs):
        self.connection = (host, port, kwargs)

    def loop_start(self):
        self.started = True
        asyncio.get_running_loop().call_soon(self.on_connect, self, None, None,
                                            SimpleNamespace(is_failure=False), None)

    def subscribe(self, subscriptions):
        self.topics = [topic for topic, options in subscriptions]
        self.subscription_options = [options for topic, options in subscriptions]
        asyncio.get_running_loop().call_soon(self.on_subscribe, self, None, 1,
            [SimpleNamespace(is_failure=False) for _ in subscriptions], None)
        return mqtt.MQTT_ERR_SUCCESS, 1

    def publish(self, topic, payload, **kwargs):
        self.bus.published.append((topic, decode(payload), kwargs))
        for client in self.bus.clients:
            if client.started and topic in client.topics:
                message = SimpleNamespace(topic=topic, payload=payload, retain=kwargs["retain"])
                asyncio.get_running_loop().call_soon(client.on_message, client, None, message)
        return SimpleNamespace(rc=mqtt.MQTT_ERR_SUCCESS)

    def disconnect(self):
        self.started = False

    def loop_stop(self):
        pass


async def settle():
    for _ in range(12):
        await asyncio.sleep(0)


def presence(session=None, **changes):
    issued = now_ms()
    return {"v": 1, "device_id": "roboplanter-01", "session": session or uuid.uuid4().hex,
            "seq": 1, "online": True, "issued_at_ms": issued,
            "expires_at_ms": issued + PRESENCE_TTL_MS, **changes}


def query(session, operation="get_state", args=None, **changes):
    issued = now_ms()
    return {"v": 1, "device_id": "roboplanter-01", "session": session,
            "client_session": uuid.uuid4().hex, "rpc_id": uuid.uuid4().hex,
            "op": operation, "args": {} if args is None else args,
            "issued_at_ms": issued, "expires_at_ms": issued + RPC_TTL_MS, **changes}


def reply(request, result, **changes):
    return {key: value for key, value in {
        **request, "issued_at_ms": now_ms(), "result": result, **changes,
    }.items() if key != "args"}


class ProtocolTests(unittest.TestCase):
    def test_strict_payload_and_config(self):
        for payload in (b'{"v":1,"v":2}', b'{"v":NaN}', b'[]', b'\xff', b' ' * (MAX_PAYLOAD + 1)):
            self.assertIsNone(decode(payload))
        with self.assertRaises(ValueError):
            encode({"value": float("nan")})
        with self.assertRaises(ValueError):
            MqttConfig("user", "secret", device_id="other/#")
        self.assertNotIn("unique-secret", repr(MqttConfig("user", "unique-secret")))


class MqttTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.bus = Bus()
        self.config = MqttConfig("test-user", "test-secret")
        self.cloud = MqttBridge(self.config, client_factory=self.bus.factory)
        self.endpoints = [self.cloud]
        await self.cloud.start()
        await settle()

    async def asyncTearDown(self):
        for endpoint in reversed(self.endpoints):
            await endpoint.close()
        await settle()

    def deliver(self, endpoint, suffix, value, retained=False):
        endpoint._receive_message(self.config.topic(suffix), encode(value), retained)

    def online(self):
        heartbeat = presence()
        self.deliver(self.cloud, "presence", heartbeat)
        return heartbeat

    async def gateway(self, bridge):
        gateway = Gateway(self.config, bridge, client_factory=self.bus.factory)
        self.endpoints.append(gateway)
        await gateway.start()
        await settle()
        return gateway

    def queries(self):
        return [body for topic, body, _ in self.bus.published if topic.endswith("/query")]

    async def test_tls_mqtt5_ephemeral_nonretained_configuration(self):
        client = self.bus.clients[0]
        self.assertEqual(client.options["protocol"], mqtt.MQTTv5)
        self.assertEqual(client.options["callback_api_version"], mqtt.CallbackAPIVersion.VERSION2)
        self.assertEqual(client.connection[0:2], ("mqtt.robomon.cn", 8883))
        options = client.connection[2]
        self.assertIs(options["clean_start"], True)
        self.assertEqual(options["properties"].SessionExpiryInterval, 0)
        self.assertTrue(client.tls_context.check_hostname)
        self.assertEqual(client.tls_context.verify_mode, ssl.CERT_REQUIRED)
        self.assertEqual(set(client.topics), {self.config.topic("reply"), self.config.topic("presence")})
        for option in client.subscription_options:
            self.assertEqual(option.QoS, 0)
            self.assertTrue(option.retainAsPublished)
            self.assertEqual(option.retainHandling, 2)

    async def test_offline_is_real_unavailable_and_no_query_sent(self):
        state = await self.cloud.get_state()
        self.assertEqual(state["status"], "unavailable")
        result = await self.cloud.get_result("old_action")
        self.assertEqual(result["status"], "unknown")
        self.assertIn("Do not retry", result["next_step"])
        self.assertEqual(self.queries(), [])

    async def test_all_writes_rejected_without_publish_even_online(self):
        self.online()
        before = len(self.bus.published)
        for action, args in (("water", {"duration_ms": 500}), ("stop_water", {}),
                             ("light", {"on": True}), ("print", {"text": "hello"})):
            result = await self.cloud.act("unique_id", action, **args)
            self.assertEqual(result["error"], "cloud_writes_not_enabled")
            self.assertEqual(result["status"], "rejected")
            self.assertIs(result["sent"], False)
        self.assertEqual(len(self.bus.published), before)

    async def test_retained_expired_and_wrong_device_presence_rejected(self):
        self.deliver(self.cloud, "presence", presence(), retained=True)
        self.deliver(self.cloud, "presence", presence(issued_at_ms=now_ms()-30000, expires_at_ms=now_ms()-10000))
        self.deliver(self.cloud, "presence", presence(device_id="other-device"))
        self.assertFalse(self.cloud._online())
        self.assertEqual((await self.cloud.get_state())["status"], "unavailable")
        self.online()
        self.cloud._presence_received -= 21
        self.assertFalse(self.cloud._online())
        self.assertEqual((await self.cloud.get_state())["status"], "unavailable")
        self.assertFalse(self.queries())

    async def test_roundtrip_http_only_get_preserves_null_and_accounts_full_rpc(self):
        calls = []
        async def handle(request):
            calls.append(request)
            await asyncio.sleep(0.025)
            return httpx.Response(200, json={"schema_version": 1, "device_id": "RoboPlanter-ABCD",
                "sample_age_ms": 4980, "soil": {"valid": False, "value": None, "calibrated": False}})
        bridge = PlanterBridge(Config(token="local-device-secret-123"), transport=httpx.MockTransport(handle))
        await self.gateway(bridge)
        result = await self.cloud.get_state()
        self.assertEqual(len(calls), 1)
        self.assertEqual(calls[0].method, "GET")
        self.assertEqual(calls[0].url.path, "/api/v1/state")
        self.assertEqual(calls[0].headers["X-Device-Token"], "local-device-secret-123")
        self.assertIsNone(result["soil"]["value"])
        self.assertFalse(result["soil"]["calibrated"])
        self.assertGreaterEqual(result["_bridge"]["rpc_elapsed_ms"], 20)
        self.assertGreater(result["_bridge"]["effective_age_ms"], 5000)
        self.assertIs(result["_bridge"]["fresh"], False)
        self.assertFalse(result["_bridge"]["cloud_writes_enabled"])
        for topic, body, settings in self.bus.published:
            self.assertFalse(settings["retain"])
            self.assertEqual(settings["qos"], 0)
            self.assertGreater(settings["properties"].MessageExpiryInterval, 0)
            self.assertNotIn("local-device-secret-123", str(body))

    async def test_invalid_state_age_never_becomes_fresh(self):
        result = {"schema_version": 1, "device_id": "device", "soil": {"value": None},
                  "_bridge": {"fresh": True, "effective_age_ms": None}}
        with patch.object(self.cloud, "_rpc", AsyncMock(return_value=(result, 300))):
            state = await self.cloud.get_state()
        self.assertIsNone(state["_bridge"]["effective_age_ms"])
        self.assertFalse(state["_bridge"]["fresh"])

    async def test_gateway_rejects_retained_expired_other_session_and_actions(self):
        bridge = SimpleNamespace(get_state=AsyncMock(return_value={}), get_result=AsyncMock(return_value={}),
                                 act=AsyncMock())
        gateway = await self.gateway(bridge)
        request = query(gateway.session)
        self.deliver(gateway, "query", request, retained=True)
        self.deliver(gateway, "query", query("0" * 32))
        self.deliver(gateway, "query", query(gateway.session, issued_at_ms=now_ms()-7000, expires_at_ms=now_ms()-1000))
        self.deliver(gateway, "query", query(gateway.session, "water", {"duration_ms": 100}))
        self.deliver(gateway, "query", query(gateway.session, "get_result", {"request_id": "../bad"}))
        gateway._receive_message(self.config.topic("reply"), encode(request))
        await settle()
        bridge.get_state.assert_not_awaited()
        bridge.get_result.assert_not_awaited()
        bridge.act.assert_not_awaited()
        # Duplicate RPC IDs do not launch two local reads.
        self.deliver(gateway, "query", request)
        self.deliver(gateway, "query", request)
        await settle()
        bridge.get_state.assert_awaited_once()

    async def test_reply_bound_to_rpc_session_and_retained_flag(self):
        self.online()
        task = asyncio.create_task(self.cloud.get_result("job_1"))
        await settle()
        request = self.queries()[-1]
        result = {"ok": True, "request_id": "job_1", "status": "completed", "error": ""}
        self.deliver(self.cloud, "reply", reply(request, result), retained=True)
        self.deliver(self.cloud, "reply", reply(request, result, session="0" * 32))
        self.deliver(self.cloud, "reply", reply(request, result, client_session="0" * 32))
        self.deliver(self.cloud, "reply", reply(request, result, rpc_id="0" * 32))
        self.deliver(self.cloud, "reply", reply(request, result, expires_at_ms=now_ms()-1))
        await settle()
        self.assertFalse(task.done())
        self.deliver(self.cloud, "reply", reply(request, result))
        response = await task
        self.assertEqual(response["status"], "completed")
        self.assertTrue(response["read_only"])

    async def test_reconnect_fails_pending_and_changes_client_nonce(self):
        heartbeat = self.online()
        task = asyncio.create_task(self.cloud.get_state())
        await settle()
        previous = self.cloud.session
        self.cloud._handle_connect(True)
        await settle()
        self.assertNotEqual(previous, self.cloud.session)
        self.assertEqual((await task)["status"], "unavailable")
        self.assertFalse(self.cloud._online())
        # New gateway session invalidates a query already in flight.
        heartbeat["issued_at_ms"] += 1
        heartbeat["expires_at_ms"] += 1
        self.deliver(self.cloud, "presence", heartbeat)
        task = asyncio.create_task(self.cloud.get_state())
        await settle()
        self.deliver(self.cloud, "presence", presence(issued_at_ms=heartbeat["issued_at_ms"]+1,
            expires_at_ms=heartbeat["expires_at_ms"]+1))
        self.assertEqual((await task)["error"], "gateway_session_changed")

    async def test_timeout_does_not_republish(self):
        self.online()
        with patch("integrations.cloud.mqtt_bridge.RPC_TIMEOUT_SECONDS", 0.01):
            result = await self.cloud.get_state()
        self.assertEqual(result["error"], "gateway_query_timeout")
        self.assertEqual(len(self.queries()), 1)
        self.assertFalse(self.cloud._pending)

    async def test_gateway_drops_work_that_expires_while_http_is_running(self):
        started, finish = asyncio.Event(), asyncio.Event()
        async def state():
            started.set()
            await finish.wait()
            return {"schema_version": 1}
        gateway = await self.gateway(SimpleNamespace(get_state=state))
        request = query(gateway.session)
        self.deliver(gateway, "query", request)
        await started.wait()
        with patch("integrations.cloud.gateway.now_ms", return_value=request["expires_at_ms"]+1):
            finish.set()
            await settle()
        self.assertFalse([topic for topic, _, _ in self.bus.published if topic.endswith("/reply")])

    async def test_unknown_result_keeps_request_id_and_no_replay_instruction(self):
        bridge = SimpleNamespace(get_result=AsyncMock(return_value={
            "ok": False, "request_id": "lost_job", "status": "unknown", "error": "request_not_found"}))
        await self.gateway(bridge)
        result = await self.cloud.get_result("lost_job")
        self.assertEqual(result["status"], "unknown")
        self.assertEqual(result["request_id"], "lost_job")
        self.assertIn("Do not retry or replay", result["next_step"])
        bridge.get_result.assert_awaited_once_with("lost_job")

    async def test_malformed_result_is_unknown_instead_of_an_exception(self):
        malformed = {"ok": False, "request_id": "job", "status": [], "error": ""}
        with patch.object(self.cloud, "_rpc", AsyncMock(return_value=(malformed, 1))):
            result = await self.cloud.get_result("job")
        self.assertEqual(result["status"], "unknown")
        self.assertEqual(result["error"], "result_unverified")
        self.assertIn("Do not retry", result["next_step"])

    async def test_paho_thread_callback_hands_off_to_asyncio_loop(self):
        heartbeat = presence()
        message = SimpleNamespace(topic=self.config.topic("presence"), payload=encode(heartbeat), retain=False)
        await asyncio.to_thread(self.cloud._on_message, None, None, message)
        await settle()
        self.assertTrue(self.cloud._online())
        retained = copy.deepcopy(message)
        retained.retain = True
        retained.payload = encode(presence(online=False))
        await asyncio.to_thread(self.cloud._on_message, None, None, retained)
        await settle()
        self.assertTrue(self.cloud._online())


if __name__ == "__main__":
    unittest.main()
