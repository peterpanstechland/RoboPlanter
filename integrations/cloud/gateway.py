#!/usr/bin/env python3
"""Local dual-network gateway: MQTT queries -> existing XIAO HTTP GET only.

Run with python -m integrations.cloud.gateway. Keep ROBOPLANTER_TOKEN here,
never in the cloud MQTT credentials. The computer needs both XIAO AP access
and Internet access (for example Wi-Fi plus Ethernet).
"""
from __future__ import annotations

import asyncio
from pathlib import Path
import signal
import site
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
site.addsitedir(str(ROOT / ".tools" / "mcp-python"))
if not __package__:
    sys.path.insert(0, str(ROOT))

from integrations.mcp.bridge import Config, PlanterBridge
from integrations.cloud.protocol import (Endpoint, MqttConfig, VERSION, RPC_TTL_MS,
    PRESENCE_TTL_MS, HEARTBEAT_SECONDS, live, nonce, now_ms, valid_request_id)


class Gateway(Endpoint):
    def __init__(self, config: MqttConfig, bridge: PlanterBridge, **kwargs: Any):
        super().__init__(config, "gateway", ("query",), **kwargs)
        self.bridge = bridge
        self._heartbeat: asyncio.Task | None = None
        self._queries: dict[tuple[str, str], asyncio.Task] = {}
        self._seen: dict[tuple[str, str], int] = {}
        self._seq = 0

    @classmethod
    def from_env(cls) -> Gateway:
        config = Config.from_env()
        if not config.writes_configured:
            raise ValueError("Configure a nondefault ROBOPLANTER_TOKEN of at least 16 characters locally")
        return cls(MqttConfig.from_env(), PlanterBridge(config))

    def _session_ready(self) -> None:
        self._seq = 0
        self._heartbeat = asyncio.create_task(self._heartbeat_loop())

    def _session_lost(self) -> None:
        if self._heartbeat is not None:
            self._heartbeat.cancel()
            self._heartbeat = None
        for task in self._queries.values():
            task.cancel()
        self._queries.clear()
        self._seen.clear()

    async def close(self) -> None:
        tasks = list(self._queries.values())
        if self._heartbeat is not None:
            tasks.append(self._heartbeat)
        await super().close()
        if tasks:
            await asyncio.gather(*tasks, return_exceptions=True)

    async def _heartbeat_loop(self) -> None:
        while self.connected:
            issued = now_ms()
            self._seq += 1
            self._publish("presence", {"v": VERSION, "device_id": self.config.device_id,
                "session": self.session, "seq": self._seq, "online": True,
                "issued_at_ms": issued, "expires_at_ms": issued + PRESENCE_TTL_MS}, PRESENCE_TTL_MS)
            await asyncio.sleep(HEARTBEAT_SECONDS)

    def _receive(self, topic: str, envelope: dict) -> None:
        expected = {"v", "device_id", "session", "client_session", "rpc_id", "op",
                    "args", "issued_at_ms", "expires_at_ms"}
        now = now_ms()
        if (set(envelope) != expected or envelope.get("session") != self.session or
                not nonce(envelope.get("client_session")) or not nonce(envelope.get("rpc_id")) or
                not live(envelope, now, RPC_TTL_MS)):
            return
        operation, args = envelope.get("op"), envelope.get("args")
        if operation == "get_state":
            if args != {}:
                return
        elif operation == "get_result":
            if (not isinstance(args, dict) or set(args) != {"request_id"} or
                    not valid_request_id(args["request_id"])):
                return
        else:
            return  # No action dispatch, even if a caller invents an operation.
        self._seen = {key: expiry for key, expiry in self._seen.items() if expiry > now}
        key = envelope["client_session"], envelope["rpc_id"]
        if key in self._seen or len(self._queries) >= 16 or len(self._seen) >= 256:
            return
        self._seen[key] = envelope["expires_at_ms"]
        task = asyncio.create_task(self._answer(envelope))
        self._queries[key] = task
        task.add_done_callback(lambda done: self._finish_query(key, done))

    def _finish_query(self, key: tuple[str, str], task: asyncio.Task) -> None:
        if self._queries.get(key) is task:
            self._queries.pop(key, None)
        if not task.cancelled():
            task.exception()  # Retrieve exceptions without logging credentials or payloads.

    async def _answer(self, query: dict) -> None:
        try:
            remaining = (query["expires_at_ms"] - now_ms()) / 1000
            if remaining <= 0:
                return
            async with asyncio.timeout(remaining):
                if query["op"] == "get_state":
                    result = await self.bridge.get_state()
                else:
                    result = await self.bridge.get_result(query["args"]["request_id"])
        except (TimeoutError, OSError):
            result = {"ok": False, "status": "unavailable", "error": "device_query_unavailable"}
            if query["op"] == "get_result":
                result.update(status="unknown", request_id=query["args"]["request_id"])
        if (not self.connected or query["session"] != self.session or
                not live(query, now_ms(), RPC_TTL_MS)):
            return
        issued = now_ms()
        self._publish("reply", {"v": VERSION, "device_id": self.config.device_id,
            "session": self.session, "client_session": query["client_session"],
            "rpc_id": query["rpc_id"], "op": query["op"], "result": result,
            "issued_at_ms": issued, "expires_at_ms": query["expires_at_ms"]},
            query["expires_at_ms"] - issued)


async def run() -> None:
    gateway = Gateway.from_env()
    stop = asyncio.Event()
    loop = asyncio.get_running_loop()
    for sig in (signal.SIGINT, signal.SIGTERM):
        try:
            loop.add_signal_handler(sig, stop.set)
        except NotImplementedError:
            pass  # Windows asyncio.run handles Ctrl+C by cancelling the task.
    await gateway.start()
    try:
        await stop.wait()
    finally:
        await gateway.close()


if __name__ == "__main__":
    try:
        asyncio.run(run())
    except KeyboardInterrupt:
        pass
