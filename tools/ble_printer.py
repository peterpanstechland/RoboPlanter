#!/usr/bin/env python3
"""Send one validated RoboPlanter raster receipt to an explicitly named BLE printer.

Requires bleak==1.1.1. Each job ID is durably reserved before the first byte;
an interrupted or completed job is never automatically replayed.
"""
from __future__ import annotations

import argparse
import asyncio
import hashlib
import json
from pathlib import Path
import re
import time

SERVICE = "000018f0-0000-1000-8000-00805f9b34fb"
WRITE = "00002af1-0000-1000-8000-00805f9b34fb"
TAIL = b"\x1bd\x06\x1dV\x00"
MAX_BYTES = 80000


def validate_receipt(data: bytes) -> int:
    """Parse commands by length; raster pixels may contain any command bytes."""
    if len(data) > MAX_BYTES or not data.startswith(b"\x1b@"):
        raise ValueError("Expected a RoboPlanter ESC/POS raster receipt under 80 KB")
    offset, bands = 2, 0
    while data[offset:offset + 4] == b"\x1dv0\x00":
        if len(data) - offset < 8:
            raise ValueError("Truncated raster header")
        width = int.from_bytes(data[offset + 4:offset + 6], "little")
        rows = int.from_bytes(data[offset + 6:offset + 8], "little")
        if width != 48 or rows != 128:
            raise ValueError("This EM5826H profile requires 384 dots and 128-row bands")
        offset += 8 + width * rows
        if offset > len(data):
            raise ValueError("Truncated raster pixels")
        bands += 1
    if not bands or data[offset:] != TAIL:
        raise ValueError("Expected raster bands followed by feed and one full cut")
    return bands


def reserve_job(directory: Path, job_id: str, state: dict) -> Path:
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,64}", job_id):
        raise ValueError("Job ID must be 1-64 ASCII letters, digits, hyphens or underscores")
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / f"{job_id}.intent.json"
    with path.open("x", encoding="utf-8") as stream:
        json.dump(state, stream, ensure_ascii=False, indent=2)
        stream.flush()
        # Flush the reservation before any physical output.
        import os
        os.fsync(stream.fileno())
    return directory / f"{job_id}.result.json"


async def run(args: argparse.Namespace) -> None:
    from bleak import BleakClient, BleakScanner

    data = None
    if args.command == "print":
        if args.file.stat().st_size > MAX_BYTES:
            raise ValueError("Receipt exceeds 80 KB")
        data = args.file.read_bytes()
        validate_receipt(data)
        if not re.fullmatch(r"[A-Za-z0-9_-]{1,64}", args.job_id):
            raise ValueError("Invalid job ID")
        if (args.journal / f"{args.job_id}.intent.json").exists():
            raise FileExistsError("Job already attempted; inspect its result, do not replay")

    discovered = await BleakScanner.discover(timeout=8, return_adv=True)
    matches = [device for device, advertisement in discovered.values()
               if (advertisement.local_name or device.name) == args.name
               and (not args.address or device.address.casefold() == args.address.casefold())]
    if len(matches) != 1:
        raise RuntimeError(f"Expected exactly one matching printer, found {len(matches)}")
    device = matches[0]
    async with BleakClient(device, timeout=25, winrt={"use_cached_services": False}) as client:
        service = client.services.get_service(SERVICE)
        characteristic = service.get_characteristic(WRITE) if service else None
        if characteristic is None or "write" not in characteristic.properties:
            raise RuntimeError("Printer does not expose the verified 18F0/2AF1 write service")
        if data is None:
            print(json.dumps({"name": device.name, "address": device.address,
                              "mtu": client.mtu_size, "service": SERVICE,
                              "write": WRITE, "properties": characteristic.properties}))
            return

        chunk_size = min(128, client.mtu_size - 3)
        if chunk_size < 20:
            raise RuntimeError("Unexpected BLE MTU")
        state = {"name": args.name, "address": device.address, "job_id": args.job_id,
                 "sha256": hashlib.sha256(data).hexdigest(), "bytes_requested": len(data),
                 "bytes_acknowledged": 0, "chunk_bytes": chunk_size, "status": "sending"}
        result = reserve_job(args.journal, args.job_id, state)
        start = time.monotonic()
        last_progress = start
        deadline = min(180, 20 + len(data) / 500)
        try:
            for offset in range(0, len(data), chunk_size):
                remaining = deadline - (time.monotonic() - start)
                if remaining <= 0:
                    raise TimeoutError("BLE receipt deadline exceeded")
                chunk = data[offset:offset + chunk_size]
                await asyncio.wait_for(client.write_gatt_char(characteristic, chunk, response=True),
                                       timeout=min(5, remaining))
                state["bytes_acknowledged"] += len(chunk)
                await asyncio.sleep(.025)
                if time.monotonic() - last_progress >= 5:
                    print(f"BLE {state['bytes_acknowledged']}/{len(data)} bytes", flush=True)
                    last_progress = time.monotonic()
            await asyncio.sleep(1)
            state["status"] = "bytes_sent_physical_confirmation_required"
        except BaseException as error:
            state.update(status="uncertain_do_not_retry", error=str(error))
            raise
        finally:
            state["elapsed_seconds"] = round(time.monotonic() - start, 2)
            result.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")
            print(json.dumps(state, ensure_ascii=False), flush=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["inspect", "print"])
    parser.add_argument("--name", required=True, help="Exact name, e.g. EM5826H_1C0E")
    parser.add_argument("--address", help="Optionally require this exact Bluetooth address")
    parser.add_argument("--file", type=Path, help="RoboPlanter .bin from the shared raster encoder")
    parser.add_argument("--job-id", help="Unique ID for this deliberate physical print")
    parser.add_argument("--journal", type=Path, default=Path(__file__).resolve().parents[1] / "build/ble-printer")
    args = parser.parse_args()
    if args.command == "print" and (args.file is None or args.job_id is None):
        parser.error("print requires --file and --job-id")
    asyncio.run(run(args))


if __name__ == "__main__":
    main()
