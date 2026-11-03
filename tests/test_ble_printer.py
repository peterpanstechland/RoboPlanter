import importlib.util
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import AsyncMock, patch

spec = importlib.util.spec_from_file_location("ble_printer", Path(__file__).resolve().parents[1] / "tools/ble_printer.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def ticket(pixels=None):
    pixels = bytes(6144) if pixels is None else pixels
    return b"\x1b@\x1dv0\x00\x30\x00\x80\x00" + pixels + module.TAIL


class BlePrinterTests(unittest.TestCase):
    def test_cut_bytes_inside_raster_do_not_end_the_job(self):
        pixels = bytearray(6144)
        pixels[23:23 + len(module.TAIL)] = module.TAIL
        self.assertEqual(module.validate_receipt(ticket(pixels)), 1)

    def test_rejects_truncated_raster_extra_commands_and_wrong_geometry(self):
        valid = ticket()
        wrong_width = bytearray(valid)
        wrong_width[6] = 72
        for data in (valid[:100], valid[:-1], valid + b"\x1dV\x00", bytes(wrong_width)):
            with self.subTest(size=len(data)), self.assertRaises(ValueError):
                module.validate_receipt(data)

    def test_duplicate_and_unsafe_job_ids_cannot_reserve_output(self):
        with tempfile.TemporaryDirectory() as folder:
            directory = Path(folder)
            result = module.reserve_job(directory, "shot-01", {"bytes_written": 0})
            self.assertEqual(result.name, "shot-01.result.json")
            self.assertEqual(json.loads((directory / "shot-01.intent.json").read_text()), {"bytes_written": 0})
            with self.assertRaises(FileExistsError):
                module.reserve_job(directory, "shot-01", {})
            with self.assertRaises(ValueError):
                module.reserve_job(directory, "../shot-02", {})


class BleTransferTests(unittest.IsolatedAsyncioTestCase):
    async def exercise_transfer(self, directory, *, transfer="paced", fail_at=None):
        data = ticket(bytes(range(256)) * 24)
        receipt = directory / "receipt.bin"
        receipt.write_bytes(data)
        args = SimpleNamespace(command="print", file=receipt, name="test-printer",
                               address="test-address", job_id="transfer-01", journal=directory,
                               transfer=transfer, rate=9600)
        device = SimpleNamespace(name=args.name, address=args.address)
        scanner = SimpleNamespace(find_device_by_filter=AsyncMock(return_value=device))
        writes = []
        characteristic = SimpleNamespace(properties=["write", "write-without-response"],
                                         max_write_without_response_size=237)

        class Client:
            mtu_size = 240
            services = SimpleNamespace(get_service=lambda _: SimpleNamespace(
                get_characteristic=lambda _: characteristic))

            def __init__(self, *unused, **kwargs):
                pass

            async def __aenter__(self):
                return self

            async def __aexit__(self, *unused):
                pass

            async def write_gatt_char(self, char, chunk, *, response):
                writes.append((bytes(chunk), response))
                if len(writes) == fail_at:
                    raise OSError("simulated disconnect")

        fake_bleak = SimpleNamespace(BleakScanner=scanner, BleakClient=Client)
        with patch.dict(sys.modules, {"bleak": fake_bleak}), \
                patch.object(module.asyncio, "sleep", new=AsyncMock()), patch("builtins.print"):
            if fail_at is None:
                await module.run(args)
            else:
                with self.assertRaisesRegex(OSError, "simulated disconnect"):
                    await module.run(args)
            # Reusing a possibly printed job must fail before discovery or writes.
            with self.assertRaises(FileExistsError):
                await module.run(args)
        scanner.find_device_by_filter.assert_awaited_once()
        predicate = scanner.find_device_by_filter.call_args.args[0]
        self.assertTrue(predicate(device, SimpleNamespace(local_name=args.name)))
        self.assertFalse(predicate(SimpleNamespace(name=args.name, address="another-printer"),
                                   SimpleNamespace(local_name=args.name)))
        state = json.loads((directory / "transfer-01.result.json").read_text())
        return data, writes, state

    async def test_both_modes_preserve_all_bytes_and_cut_once(self):
        for transfer, limit in (("paced", 224), ("acknowledged", 128)):
            with self.subTest(transfer=transfer), tempfile.TemporaryDirectory() as folder:
                data, writes, state = await self.exercise_transfer(Path(folder), transfer=transfer)
                self.assertEqual(b"".join(chunk for chunk, _ in writes), data)
                self.assertTrue(all(len(chunk) <= limit for chunk, _ in writes))
                self.assertTrue(all(response == (transfer == "acknowledged") for _, response in writes))
                self.assertEqual(state["bytes_written"], len(data))
                self.assertEqual(state["status"], "bytes_sent_physical_confirmation_required")

    async def test_disconnect_stops_output_and_never_replays(self):
        with tempfile.TemporaryDirectory() as folder:
            _, writes, state = await self.exercise_transfer(Path(folder), fail_at=3)
            self.assertEqual(len(writes), 3)
            self.assertEqual(state["bytes_written"], 448)
            self.assertEqual(state["status"], "uncertain_do_not_retry")


if __name__ == "__main__":
    unittest.main()
