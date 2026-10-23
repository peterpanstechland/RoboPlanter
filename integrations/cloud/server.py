"""Authenticated cloud MCP entry point; no direct actuator transport.

Run from the repository root with ``python -m integrations.cloud.server``.
The reverse proxy terminates HTTPS and strips /roboplanter/ before forwarding.
The configured root path preserves that prefix in the SDK's SSE endpoint event.
"""
from __future__ import annotations

from contextlib import asynccontextmanager
from dataclasses import dataclass, field
import hashlib
import hmac
import os
from pathlib import Path
import re
import site
from typing import Any
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[2]
site.addsitedir(str(ROOT / ".tools" / "mcp-python"))

from mcp.server.transport_security import TransportSecuritySettings
from starlette.applications import Starlette
from starlette.middleware import Middleware
from starlette.requests import Request
from starlette.responses import JSONResponse
from starlette.routing import Route
from starlette.types import ASGIApp, Receive, Scope, Send

from integrations.mcp.server import create_server


@dataclass(frozen=True)
class CloudConfig:
    token: str = field(repr=False)
    allowed_hosts: tuple[str, ...] = (
        "emqx.robomon.cn", "emqx.robomon.cn:443",
        "127.0.0.1:18765", "localhost:18765", "127.0.0.1:8765",
    )
    allowed_origins: tuple[str, ...] = ("https://emqx.robomon.cn",)
    root_path: str = "/roboplanter"

    def __post_init__(self) -> None:
        # Token contents never appear in validation errors or dataclass repr.
        if not re.fullmatch(r"[A-Za-z0-9_-]{32,128}", self.token):
            raise ValueError("ROBOPLANTER_CLOUD_TOKEN must be a random 32..128 character URL-safe token")
        if not self.allowed_hosts or any(
            not re.fullmatch(r"[A-Za-z0-9.-]+(?::[0-9]{1,5})?", host)
            for host in self.allowed_hosts
        ):
            raise ValueError("ROBOPLANTER_ALLOWED_HOSTS requires explicit host[:port] values; wildcards are forbidden")
        for origin in self.allowed_origins:
            parsed = urlsplit(origin)
            if (parsed.scheme != "https" or not parsed.hostname or parsed.username is not None
                    or parsed.password is not None or parsed.path or parsed.query or parsed.fragment
                    or "*" in origin):
                raise ValueError("ROBOPLANTER_ALLOWED_ORIGINS requires exact HTTPS origins")
            try:
                parsed.port
            except ValueError:
                raise ValueError("ROBOPLANTER_ALLOWED_ORIGINS contains an invalid port") from None
        if self.root_path and not re.fullmatch(r"(?:/[A-Za-z0-9_-]+)+", self.root_path):
            raise ValueError("ROBOPLANTER_ROOT_PATH must be empty or a path without a trailing slash")

    @classmethod
    def from_env(cls) -> CloudConfig:
        def csv(name: str, default: tuple[str, ...]) -> tuple[str, ...]:
            raw = os.environ.get(name)
            return default if raw is None else tuple(part.strip() for part in raw.split(",") if part.strip())
        return cls(
            token=os.environ.get("ROBOPLANTER_CLOUD_TOKEN", ""),
            allowed_hosts=csv("ROBOPLANTER_ALLOWED_HOSTS", cls.__dataclass_fields__["allowed_hosts"].default),
            allowed_origins=csv("ROBOPLANTER_ALLOWED_ORIGINS", cls.__dataclass_fields__["allowed_origins"].default),
            root_path=os.environ.get("ROBOPLANTER_ROOT_PATH", "/roboplanter"),
        )


class BearerBoundary:
    """Pure ASGI guard, including both the SSE GET and its message POST.

    Do not log headers, supplied token, or request body. Fixed-size digests use
    compare_digest; duplicate Authorization/Host/Origin headers are rejected.
    Query parameters and cookies are never authentication sources.
    """

    def __init__(self, app: ASGIApp, *, config: CloudConfig):
        self.app = app
        self._token_digest = hashlib.sha256(config.token.encode("ascii")).digest()
        self._hosts = frozenset(config.allowed_hosts)
        self._origins = frozenset(config.allowed_origins)

    async def __call__(self, scope: Scope, receive: Receive, send: Send) -> None:
        if scope["type"] != "http":
            await self.app(scope, receive, send)
            return
        headers = scope.get("headers", [])

        def values(name: bytes) -> list[bytes]:
            return [value for key, value in headers if key.lower() == name]

        hosts, origins = values(b"host"), values(b"origin")
        if len(hosts) != 1 or hosts[0].decode("latin-1") not in self._hosts:
            await JSONResponse({"error": "invalid_host"}, status_code=421)(scope, receive, send)
            return
        if len(origins) > 1 or (origins and origins[0].decode("latin-1") not in self._origins):
            await JSONResponse({"error": "invalid_origin"}, status_code=403)(scope, receive, send)
            return
        # Uvicorn includes root_path in path; ASGI test clients may not.
        path = scope.get("path", "")
        prefix = scope.get("root_path", "").rstrip("/")
        if prefix and path.startswith(prefix + "/"):
            path = path[len(prefix):]
        if path == "/healthz" and scope["method"] in ("GET", "HEAD"):
            await self.app(scope, receive, send)
            return

        auth = values(b"authorization")
        candidate = b""
        if len(auth) == 1 and len(auth[0]) <= 136:
            scheme, separator, credential = auth[0].partition(b" ")
            if scheme.lower() == b"bearer" and separator:
                candidate = credential
        authenticated = hmac.compare_digest(hashlib.sha256(candidate).digest(), self._token_digest)
        if not authenticated:
            await JSONResponse(
                {"error": "unauthorized"}, status_code=401,
                headers={"WWW-Authenticate": 'Bearer realm="RoboPlanter"', "Cache-Control": "no-store"},
            )(scope, receive, send)
            return
        await self.app(scope, receive, send)


def create_app(config: CloudConfig | None = None, *, bridge: Any | None = None) -> Starlette:
    config = config or CloudConfig.from_env()  # Refuse startup before constructing MQTT without a token.
    if bridge is None:
        from .mqtt_bridge import MqttBridge
        bridge = MqttBridge.from_env()
    server = create_server(bridge=bridge, transport_security=TransportSecuritySettings(
        enable_dns_rebinding_protection=True,
        allowed_hosts=list(config.allowed_hosts), allowed_origins=list(config.allowed_origins),
    ))
    http_app = server.streamable_http_app()
    sse_app = server.sse_app(mount_path="/")

    @asynccontextmanager
    async def lifespan(app: Starlette):
        try:
            await bridge.start()
            async with server.session_manager.run():
                yield
        finally:
            await bridge.close()

    async def healthz(request: Request) -> JSONResponse:
        # Liveness only, never claims broker/device reachability or actuator safety.
        return JSONResponse({"status": "ok", "service": "roboplanter-mcp", "writes_enabled": False},
                            headers={"Cache-Control": "no-store"})

    # SDK 1.30 routes include their own security and request-body-limit wrappers.
    # One outer guard covers /mcp, /sse, /messages/ and future routes alike.
    app = Starlette(
        debug=False,
        routes=[Route("/healthz", healthz, methods=["GET"]), *http_app.routes, *sse_app.routes],
        middleware=[Middleware(BearerBoundary, config=config)], lifespan=lifespan,
    )
    app.state.mcp_server = server
    return app


def main() -> None:
    import uvicorn
    try:
        config = CloudConfig.from_env()
        app = create_app(config)
    except ValueError:
        # Configuration exceptions from dependencies must not expose broker credentials.
        raise SystemExit("Cloud configuration is invalid; check the required environment variable names and formats.") from None
    uvicorn.run(app, host="0.0.0.0", port=8765, root_path=config.root_path,
                access_log=False, log_level="warning", proxy_headers=False,
                limit_concurrency=32, timeout_keep_alive=5, timeout_graceful_shutdown=10)


if __name__ == "__main__":
    main()
