"""Cloud MCP adapter: live MQTT queries only; hardware writes are disabled."""
from __future__ import annotations

import asyncio
from dataclasses import dataclass
import math
import time
from typing import Any
import uuid

from .protocol import (Endpoint, MqttConfig, VERSION, RPC_TTL_MS, RPC_TIMEOUT_SECONDS,
                       PRESENCE_TTL_MS, STALE_MS, live, nonce, now_ms, valid_request_id)

NO_RETRY = "Query this same request ID or inspect the planter. Do not retry or replay the action."


class Unavailable(Exception):
    pass


@dataclass
class Pending:
    future: asyncio.Future
    session: str
    client_session: str
    operation: str
    expires_at_ms: int


class MqttBridge(Endpoint):
    def __init__(self, config: MqttConfig, **kwargs: Any):
        super().__init__(config, "cloud", ("reply", "presence"), **kwargs)
        self._gateway_session = ""
        self._presence_received = 0.0
        self._presence_expires = 0
        self._presence_issued = -1
        self._presence_seq = -1
        self._pending: dict[str, Pending] = {}

    @classmethod
    def from_env(cls) -> MqttBridge:
        return cls(MqttConfig.from_env())

    def _session_lost(self) -> None:
        self._gateway_session = ""
        self._presence_received = 0.0
        self._presence_expires = 0
        self._presence_issued = -1
        self._presence_seq = -1
        self._fail_pending("mqtt_disconnected")

    def _fail_pending(self, error: str) -> None:
        for pending in self._pending.values():
            if not pending.future.done():
                pending.future.set_exception(Unavailable(error))

    def _online(self) -> bool:
        return (self.connected and bool(self._gateway_session) and
                now_ms() < self._presence_expires and
                time.monotonic() - self._presence_received < PRESENCE_TTL_MS / 1000)

    def _receive(self, topic: str, envelope: dict) -> None:
        if topic == self.config.topic("presence"):
            expected = {"v", "device_id", "session", "seq", "issued_at_ms", "expires_at_ms", "online"}
            if (set(envelope) != expected or not nonce(envelope.get("session")) or
                    type(envelope.get("seq")) is not int or envelope["seq"] < 0 or
                    type(envelope.get("online")) is not bool or
                    not live(envelope, now_ms(), PRESENCE_TTL_MS)):
                return
            issued, seq = envelope["issued_at_ms"], envelope["seq"]
            if (issued < self._presence_issued or
                    (issued == self._presence_issued and
                     (envelope["session"] != self._gateway_session or seq <= self._presence_seq))):
                return
            if self._gateway_session and self._gateway_session != envelope["session"]:
                self._fail_pending("gateway_session_changed")
            self._presence_issued, self._presence_seq = issued, seq
            self._gateway_session = envelope["session"] if envelope["online"] else ""
            self._presence_expires = envelope["expires_at_ms"]
            self._presence_received = time.monotonic()
            if not envelope["online"]:
                self._fail_pending("gateway_offline")
            return
        expected = {"v", "device_id", "session", "client_session", "rpc_id", "op",
                    "issued_at_ms", "expires_at_ms", "result"}
        if set(envelope) != expected or not nonce(envelope.get("rpc_id")):
            return
        pending = self._pending.get(envelope["rpc_id"])
        if (pending is None or pending.future.done() or not self._online() or
                envelope.get("session") != pending.session or
                envelope.get("session") != self._gateway_session or
                envelope.get("client_session") != pending.client_session or
                envelope.get("client_session") != self.session or
                envelope.get("op") != pending.operation or
                envelope.get("expires_at_ms") != pending.expires_at_ms or
                not live(envelope, now_ms(), RPC_TTL_MS) or
                not isinstance(envelope.get("result"), dict)):
            return
        pending.future.set_result(envelope["result"])

    async def _rpc(self, operation: str, args: dict) -> tuple[dict, int]:
        if not self._online():
            raise Unavailable("gateway_unavailable")
        if len(self._pending) >= 32:
            raise Unavailable("query_capacity_reached")
        started = time.monotonic()
        issued = now_ms()
        rpc_id = uuid.uuid4().hex
        future = asyncio.get_running_loop().create_future()
        pending = Pending(future, self._gateway_session, self.session, operation, issued + RPC_TTL_MS)
        self._pending[rpc_id] = pending
        query = {"v": VERSION, "device_id": self.config.device_id, "session": pending.session,
                 "client_session": pending.client_session, "rpc_id": rpc_id, "op": operation,
                 "args": args, "issued_at_ms": issued, "expires_at_ms": pending.expires_at_ms}
        try:
            if not self._publish("query", query, RPC_TTL_MS):
                raise Unavailable("mqtt_publish_failed")
            result = await asyncio.wait_for(future, RPC_TIMEOUT_SECONDS)
            return result, math.ceil((time.monotonic() - started) * 1000)
        except TimeoutError:
            raise Unavailable("gateway_query_timeout") from None
        finally:
            self._pending.pop(rpc_id, None)
            if not future.done():
                future.cancel()

    async def get_state(self) -> dict[str, Any]:
        try:
            result, elapsed_ms = await self._rpc("get_state", {})
        except Unavailable as error:
            return {"ok": False, "status": "unavailable", "error": str(error),
                    "source": "cloud", "read_only": True}
        if result.get("status") == "unavailable" and result.get("ok") is False:
            return {**result, "read_only": True}
        metadata = result.get("_bridge")
        if (type(result.get("schema_version")) is not int or result["schema_version"] != 1 or
                not isinstance(result.get("device_id"), str) or not isinstance(metadata, dict)):
            return {"ok": False, "status": "unavailable", "error": "invalid_gateway_state", "read_only": True}
        age = metadata.get("effective_age_ms")
        effective_age = age + elapsed_ms if type(age) is int and age >= 0 else None
        return {**result, "_bridge": {**metadata, "read_only": True,
                "fresh": metadata.get("fresh") is True and effective_age is not None and effective_age <= STALE_MS,
                "effective_age_ms": effective_age, "rpc_elapsed_ms": elapsed_ms,
                "transport": "mqtt-read-only", "cloud_writes_enabled": False}}

    async def act(self, request_id: str, action: str, **values: Any) -> dict[str, Any]:
        # Intentionally no transport call, including STOP. The device has no
        # execution-deadline protocol yet, so cloud writes must remain disabled.
        return {"ok": False, "request_id": request_id, "status": "rejected",
                "error": "cloud_writes_not_enabled", "source": "cloud", "sent": False,
                "next_step": "Cloud control is not enabled. No hardware command was sent."}

    async def get_result(self, request_id: str) -> dict[str, Any]:
        if not valid_request_id(request_id):
            return {"ok": False, "request_id": request_id, "status": "rejected",
                    "error": "invalid_request_id", "source": "cloud", "sent": False}
        try:
            result, _ = await self._rpc("get_result", {"request_id": request_id})
        except Unavailable as error:
            result = {"ok": False, "request_id": request_id, "status": "unknown", "error": str(error)}
        statuses = {"accepted", "completed", "rejected", "failed", "cancelled", "stopped", "unknown"}
        if (result.get("request_id") != request_id or type(result.get("ok")) is not bool or
                not isinstance(result.get("status"), str) or result["status"] not in statuses or
                not isinstance(result.get("error"), str) or
                result["ok"] != (result["status"] in {"accepted", "completed"})):
            result = {"ok": False, "request_id": request_id, "status": "unknown", "error": "result_unverified"}
        result = {**result, "read_only": True}
        if result["status"] == "unknown":
            result["next_step"] = NO_RETRY
        return result
