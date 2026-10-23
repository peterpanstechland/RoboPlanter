"""Bounded, non-retained MQTT 5 transport for read-only RoboPlanter RPC.

Uses paho-mqtt==2.1.0 callback API VERSION2. Paho callbacks run on its
network thread; all application state is owned by the asyncio event loop.
"""
from __future__ import annotations

import asyncio
from dataclasses import dataclass, field
import json
import os
import re
import ssl
import time
from typing import Any, Callable
import uuid

import paho.mqtt.client as mqtt
from paho.mqtt.packettypes import PacketTypes
from paho.mqtt.properties import Properties
from paho.mqtt.subscribeoptions import SubscribeOptions

VERSION = 1
MAX_PAYLOAD = 32768
RPC_TIMEOUT_SECONDS = 6.0
HEARTBEAT_SECONDS = 5.0
PRESENCE_TTL_MS = 20000
RPC_TTL_MS = 6000
CLOCK_SKEW_MS = 2000
STALE_MS = 5000
NONCE = re.compile(r"[0-9a-f]{32}\Z")
REQUEST_ID = re.compile(r"[A-Za-z0-9_-]{1,32}\Z")
DEVICE_ID = re.compile(r"[A-Za-z0-9_-]{1,64}\Z")


def now_ms() -> int:
    return time.time_ns() // 1_000_000


def nonce(value: Any) -> bool:
    return isinstance(value, str) and NONCE.fullmatch(value) is not None


def valid_request_id(value: Any) -> bool:
    return isinstance(value, str) and REQUEST_ID.fullmatch(value) is not None


def live(envelope: dict, now: int, max_ttl_ms: int) -> bool:
    issued, expires = envelope.get("issued_at_ms"), envelope.get("expires_at_ms")
    return (type(issued) is int and type(expires) is int and
            0 < expires - issued <= max_ttl_ms and
            issued <= now + CLOCK_SKEW_MS and now < expires)


def _unique_object(pairs: list[tuple[str, Any]]) -> dict:
    result: dict = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("duplicate JSON key")
        result[key] = value
    return result


def _invalid_constant(value: str) -> None:
    raise ValueError("nonfinite JSON number")


def decode(payload: bytes) -> dict | None:
    if not payload or len(payload) > MAX_PAYLOAD:
        return None
    try:
        value = json.loads(payload, object_pairs_hook=_unique_object,
                           parse_constant=_invalid_constant)
    except (ValueError, UnicodeError, RecursionError):
        return None
    return value if isinstance(value, dict) else None


def encode(value: dict) -> bytes:
    data = json.dumps(value, ensure_ascii=False, allow_nan=False,
                      separators=(",", ":")).encode("utf-8")
    if len(data) > MAX_PAYLOAD:
        raise ValueError("MQTT payload too large")
    return data


@dataclass(frozen=True)
class MqttConfig:
    username: str
    password: str = field(repr=False)
    host: str = "mqtt.robomon.cn"
    port: int = 8883
    device_id: str = "roboplanter-01"
    ca_file: str | None = None

    def __post_init__(self) -> None:
        if not self.username or not self.password:
            raise ValueError("MQTT username and password are required")
        if not self.host or any(c.isspace() for c in self.host) or "/" in self.host:
            raise ValueError("MQTT host must be a hostname, not a URL")
        if type(self.port) is not int or not 1 <= self.port <= 65535:
            raise ValueError("MQTT port must be 1..65535")
        if DEVICE_ID.fullmatch(self.device_id) is None:
            raise ValueError("MQTT device ID must contain 1..64 ASCII letters, digits, _ or -")

    @classmethod
    def from_env(cls) -> MqttConfig:
        return cls(
            username=os.environ.get("ROBOPLANTER_MQTT_USERNAME", ""),
            password=os.environ.get("ROBOPLANTER_MQTT_PASSWORD", ""),
            host=os.environ.get("ROBOPLANTER_MQTT_HOST", "mqtt.robomon.cn"),
            port=int(os.environ.get("ROBOPLANTER_MQTT_PORT", "8883")),
            device_id=os.environ.get("ROBOPLANTER_MQTT_DEVICE_ID", "roboplanter-01"),
            ca_file=os.environ.get("ROBOPLANTER_MQTT_CA_FILE") or None,
        )

    def topic(self, suffix: str) -> str:
        if suffix not in {"query", "reply", "presence"}:
            raise ValueError("No action or arbitrary MQTT topics are supported")
        return f"roboplanter/v1/{self.device_id}/{suffix}"


class Endpoint:
    """All subclass hooks execute on the owning asyncio loop, never Paho's thread."""

    def __init__(self, config: MqttConfig, role: str, subscriptions: tuple[str, ...],
                 *, client_factory: Callable[..., Any] = mqtt.Client):
        self.config = config
        self._role = role
        self._subscriptions = tuple(config.topic(x) for x in subscriptions)
        self._client_factory = client_factory
        self._client: Any = None
        self._loop: asyncio.AbstractEventLoop | None = None
        self._stopped = True
        self.connected = False
        self.session = ""
        self._subscription_mid: int | None = None

    async def start(self) -> None:
        if not self._stopped:
            return
        self._loop = asyncio.get_running_loop()
        client = self._client_factory(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id=f"rp-{self._role}-{uuid.uuid4().hex}", protocol=mqtt.MQTTv5,
        )
        # No insecure TLS override. The default context checks CA and hostname.
        context = ssl.create_default_context(cafile=self.config.ca_file)
        context.minimum_version = ssl.TLSVersion.TLSv1_2
        client.tls_set_context(context)
        client.username_pw_set(self.config.username, self.config.password)
        client.max_queued_messages_set(16)
        client.reconnect_delay_set(min_delay=1, max_delay=10)
        client.on_connect = self._on_connect
        client.on_disconnect = self._on_disconnect
        client.on_subscribe = self._on_subscribe
        client.on_message = self._on_message
        properties = Properties(PacketTypes.CONNECT)
        properties.SessionExpiryInterval = 0
        properties.MaximumPacketSize = MAX_PAYLOAD + 1024
        self._client = client
        self._stopped = False
        try:
            # Network/DNS/TLS work happens in Paho's thread, not the ASGI loop.
            client.connect_async(self.config.host, self.config.port, keepalive=15,
                                 clean_start=True, properties=properties)
            client.loop_start()
        except Exception:
            self._stopped = True
            self._client = None
            raise

    async def close(self) -> None:
        if self._stopped:
            return
        self._stopped = True
        self.connected = False
        self.session = ""
        self._session_lost()
        client, self._client = self._client, None
        if client is not None:
            client.disconnect()
            await asyncio.to_thread(client.loop_stop)

    def _dispatch(self, callback: Callable, *args: Any) -> None:
        loop = self._loop
        if loop is not None and not loop.is_closed() and not self._stopped:
            try:
                loop.call_soon_threadsafe(callback, *args)
            except RuntimeError:
                pass  # The owning application has already closed its loop.

    def _on_connect(self, client: Any, userdata: Any, flags: Any,
                    reason_code: Any, properties: Any) -> None:
        self._dispatch(self._handle_connect, not reason_code.is_failure)

    def _on_disconnect(self, client: Any, userdata: Any, flags: Any,
                       reason_code: Any, properties: Any) -> None:
        self._dispatch(self._handle_disconnect)

    def _on_subscribe(self, client: Any, userdata: Any, mid: int,
                      reason_codes: list, properties: Any) -> None:
        self._dispatch(self._handle_subscribe, mid,
                       bool(reason_codes) and not any(code.is_failure for code in reason_codes))

    def _on_message(self, client: Any, userdata: Any, message: Any) -> None:
        # Reject retained content even when forwarded live. Subscriptions set
        # retainAsPublished so the broker does not erase that flag.
        if (message.retain or message.topic not in self._subscriptions or
                len(message.payload) > MAX_PAYLOAD):
            return
        self._dispatch(self._receive_message, message.topic, bytes(message.payload), False)

    def _handle_connect(self, success: bool) -> None:
        if self._stopped:
            return
        self._handle_disconnect()
        if not success:
            return
        self.session = uuid.uuid4().hex
        options = SubscribeOptions(qos=0, noLocal=True, retainAsPublished=True, retainHandling=2)
        rc, mid = self._client.subscribe([(topic, options) for topic in self._subscriptions])
        if rc == mqtt.MQTT_ERR_SUCCESS:
            self._subscription_mid = mid

    def _handle_subscribe(self, mid: int, success: bool) -> None:
        if self._stopped or mid != self._subscription_mid:
            return
        self._subscription_mid = None
        if success:
            self.connected = True
            self._session_ready()
        else:
            self._handle_disconnect()

    def _handle_disconnect(self) -> None:
        self.connected = False
        self.session = ""
        self._subscription_mid = None
        self._session_lost()

    def _publish(self, suffix: str, envelope: dict, ttl_ms: int) -> bool:
        if self._stopped or not self.connected or self._client is None or ttl_ms <= 0:
            return False
        try:
            payload = encode(envelope)
            properties = Properties(PacketTypes.PUBLISH)
            properties.MessageExpiryInterval = max(1, (ttl_ms + 999) // 1000)
            # QoS 0 deliberately avoids Paho's QoS>0 replay queue. Read callers
            # may issue a new query after timeout; nothing retries an action.
            info = self._client.publish(self.config.topic(suffix), payload,
                                        qos=0, retain=False, properties=properties)
            return info.rc == mqtt.MQTT_ERR_SUCCESS
        except (ValueError, TypeError, RuntimeError, OSError):
            return False

    def _receive_message(self, topic: str, payload: bytes, retained: bool = False) -> None:
        if (self._stopped or not self.connected or retained or topic not in self._subscriptions):
            return
        envelope = decode(payload)
        if (envelope is None or type(envelope.get("v")) is not int or
                envelope["v"] != VERSION or envelope.get("device_id") != self.config.device_id):
            return
        self._receive(topic, envelope)

    def _session_ready(self) -> None:
        pass

    def _session_lost(self) -> None:
        pass

    def _receive(self, topic: str, envelope: dict) -> None:
        raise NotImplementedError
