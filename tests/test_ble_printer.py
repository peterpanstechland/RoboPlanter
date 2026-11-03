import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

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
            result = module.reserve_job(directory, "shot-01", {"bytes_acknowledged": 0})
            self.assertEqual(result.name, "shot-01.result.json")
            self.assertEqual(json.loads((directory / "shot-01.intent.json").read_text()), {"bytes_acknowledged": 0})
            with self.assertRaises(FileExistsError):
                module.reserve_job(directory, "shot-01", {})
            with self.assertRaises(ValueError):
                module.reserve_job(directory, "../shot-02", {})


if __name__ == "__main__":
    unittest.main()
