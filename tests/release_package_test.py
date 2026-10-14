"""Integration tests with locally provided ROMs; no game data is redistributed."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('import_assets',ROOT/'tools/release/import_assets.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

@unittest.skipUnless((ROOT/'probe-output/base.z64').exists() and (ROOT/'probe-output/sc.z64').exists() and (ROOT/'generated/release-decoder/font-decoder').exists(),'Requires both local ROMs and generated decoder')
class ReleaseIntegration(unittest.TestCase):
    def setUp(self):
        work=ROOT/'local/tests/release-import';work.mkdir(parents=True,exist_ok=True)
        self.temp=tempfile.TemporaryDirectory(prefix='clay release paths ',dir=work)
        self.root=Path(self.temp.name);(self.root/'bin').mkdir()
        shutil.copy2(ROOT/'generated/release-decoder/font-decoder',self.root/'bin/font-decoder')
    def tearDown(self):self.temp.cleanup()
    def test_one_rom_and_duplicate_rom_rejected_before_writes(self):
        for paths in [[ROOT/'probe-output/base.z64'],[ROOT/'probe-output/base.z64']*2]:
            with self.assertRaisesRegex(ValueError,'Both different USA ROMs'):module.install(self.root,paths)
            self.assertFalse((self.root/'probe-output').exists())
    def test_wrong_rom_rejected_before_writes(self):
        bad=self.root/'bad.z64';bad.write_bytes(bytes.fromhex('80371240')+bytes(12))
        with self.assertRaisesRegex(ValueError,'Unsupported ROM'):module.install(self.root,[bad,ROOT/'probe-output/sc.z64'])
        self.assertFalse((self.root/'probe-output').exists())
    def test_reversed_order_matches_final_private_assets_and_detects_corruption(self):
        module.install(self.root,[ROOT/'probe-output/sc.z64',ROOT/'probe-output/base.z64'])
        self.assertTrue(module.installed(self.root))
        for name in ['local/sc_bios/bios.bin','local/sc_roster/asset-image-roster.z64']:
            self.assertEqual((self.root/name).read_bytes(),(ROOT/name).read_bytes())
        (self.root/'local/sc_bios/bios.bin').write_bytes(b'broken')
        self.assertFalse(module.installed(self.root))
        self.assertTrue((self.root/'ClayFighter 64.desktop').exists())

if __name__=='__main__':unittest.main()
