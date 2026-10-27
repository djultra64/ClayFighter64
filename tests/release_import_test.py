import importlib.util
from pathlib import Path
import unittest
import tempfile
import zipfile
from unittest.mock import patch
import subprocess

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


class DiscoveryTests(unittest.TestCase):
    def test_finds_verified_roms_in_game_and_roms_folders(self):
        spec = importlib.util.spec_from_file_location('clay_discovery', SOURCE)
        m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / 'Game'; root.mkdir()
            roms = Path(directory) / 'ROMs'; roms.mkdir()
            base = bytes.fromhex('80371240') + bytes(12)
            donor = bytes.fromhex('80371240') + bytes([1])*12
            m.BASE_SHA, m.SC_SHA = m.sha(base), m.sha(donor)
            (root / 'Clay Fighter 63 1-3.z64').write_bytes(base)
            (roms / 'ClayFighter Sculptors Cut.n64').write_bytes(donor)
            (root / 'ClayFighter fake.z64').write_bytes(b'bad!')
            found = m.discover_roms(root)
            self.assertEqual(found[m.BASE_SHA], root / 'Clay Fighter 63 1-3.z64')
            self.assertEqual(found[m.SC_SHA], roms / 'ClayFighter Sculptors Cut.n64')

    def test_zip_with_c64_rom_is_supported(self):
        spec = importlib.util.spec_from_file_location('clay_c64', SOURCE)
        m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
        expected = bytes.fromhex('80371240') + bytes(12)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'game.zip'
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr('game.c64', expected)
            self.assertEqual(m.load_rom(path), expected)

class LinuxSelectionTests(unittest.TestCase):
    def test_selects_only_missing_rom_with_graphical_dialog(self):
        spec = importlib.util.spec_from_file_location('clay_linux', SOURCE)
        m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
        existing = Path('/games/base.z64')
        with patch.object(m.shutil, 'which', side_effect=lambda name: '/usr/bin/zenity' if name == 'zenity' else None), \
             patch.object(m.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, '/games/sc.zip\n', '')) as dialog:
            paths = m.linux_dialog({m.BASE_SHA: existing})
        self.assertEqual(paths, [existing, Path('/games/sc.zip')])
        self.assertEqual(dialog.call_count, 1)
        self.assertIn('Sculptor', ' '.join(dialog.call_args.args[0]))

    def test_cancelling_dialog_does_not_install(self):
        spec = importlib.util.spec_from_file_location('clay_cancel', SOURCE)
        m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
        with patch.object(m.shutil, 'which', return_value='/usr/bin/zenity'), \
             patch.object(m.subprocess, 'run', return_value=subprocess.CompletedProcess([], 1, '', '')):
            self.assertEqual(m.linux_dialog({}), [])

if __name__=='__main__':unittest.main()
