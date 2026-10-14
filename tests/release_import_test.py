import importlib.util
from pathlib import Path
import unittest
import tempfile
import zipfile

SOURCE = Path(__file__).resolve().parents[1] / 'tools/release/import_assets.py'

class ImportTests(unittest.TestCase):
    def test_importer_exists(self):
        self.assertTrue(SOURCE.is_file(), 'The release needs a two-ROM extractor')

    def test_formats(self):
        if not SOURCE.is_file(): self.skipTest('implementation missing')
        spec=importlib.util.spec_from_file_location('clay_import',SOURCE)
        m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
        expected=bytes.fromhex('80371240')+bytes(range(12))
        work=SOURCE.parents[2]/'local/tests/import-formats';work.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='clay paths ',dir=work) as d:
            d=Path(d)
            for suffix,data in [('z64',expected),('v64',b''.join(expected[i:i+2][::-1] for i in range(0,16,2))),('n64',b''.join(expected[i:i+4][::-1] for i in range(0,16,4)))]:
                p=d/('test.'+suffix);p.write_bytes(data)
                self.assertEqual(m.load_rom(p),expected)
            p=d/'test.zip'
            with zipfile.ZipFile(p,'w') as z:z.writestr('dir/test.z64',expected)
            self.assertEqual(m.load_rom(p),expected)
            with zipfile.ZipFile(p,'a') as z:z.writestr('other.z64',expected)
            with self.assertRaisesRegex(ValueError,'exactly one'):m.load_rom(p)
            p=d/'bad.z64';p.write_bytes(b'bad!')
            with self.assertRaises(ValueError):m.load_rom(p)

if __name__=='__main__':unittest.main()
