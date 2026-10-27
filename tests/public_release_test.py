"""Public archive/version and portable importer regression fixtures. djultra64."""
import json
import contextlib
import io
from pathlib import Path
import shutil
import sys
import tarfile
import tempfile
import unittest
import zipfile
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.release import package, import_assets
from tools.release.public_files import release_version, source_files


class PublicReleaseTests(unittest.TestCase):
    def setUp(self):
        work=ROOT/'local/tests/release-policy';work.mkdir(parents=True,exist_ok=True)
        self.temp=tempfile.TemporaryDirectory(prefix='fixture ',dir=work)
        self.root=Path(self.temp.name)/'game';self.root.mkdir()
    def tearDown(self):self.temp.cleanup()
    def put(self,name,data=b'public fixture'):
        path=self.root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data);return path
    def test_versions_are_safe_artifact_components(self):
        for value in ['1.0','2.0','2.0-rc1']:
            self.assertEqual(release_version(value),value)
        for value in ['../2.0','2/0','v2.0','2.0\n','']:
            with self.assertRaises(ValueError):release_version(value)
    def test_source_archive_excludes_private_paths_and_preserves_notices(self):
        for name in ['README.md','LICENSE','host/example.cpp','licenses/ThirdParty/LICENSE',
                     'local/secret.json','analysis/evidence.json','generated/guest/funcs_0.c',
                     'host/__pycache__/secret.pyc','tests/private-snapshot.bin',
                     'docs/superpowers/plans/private.md']:
            self.put(name)
        out=self.root/'local/releases/2.0';out.mkdir(parents=True)
        with patch.object(package,'ROOT',self.root),patch.object(package,'OUT',out):
            archive=package.source_archive('2.0')
            with self.assertRaises(ValueError):package.source_archive('2.0')
        with tarfile.open(archive) as saved:
            names=saved.getnames()
            self.assertEqual(len(names),4)
            self.assertTrue(any(name.endswith('licenses/ThirdParty/LICENSE') for name in names))
            self.assertTrue(all(info.uname=='djultra64' and info.uid==0 for info in saved.getmembers()))
    def test_rom_suffix_header_and_symlink_fail_closed(self):
        suspect=self.put('host/leak.z64')
        with self.assertRaisesRegex(ValueError,'ROM excluded'):list(source_files(self.root))
        suspect.unlink();suspect=self.put('host/disguised.txt',bytes.fromhex('80371240')+bytes(12))
        with self.assertRaisesRegex(ValueError,'ROM header'):list(source_files(self.root))
        suspect.unlink();self.put('target.txt');(self.root/'host/link.txt').symlink_to(self.root/'target.txt')
        with self.assertRaisesRegex(ValueError,'symlinks'):list(source_files(self.root))
    def test_manifest_version_paths_checksums_and_private_state_guard(self):
        staged=self.root/'clayfighter64-2.0-linux-x86_64';staged.mkdir()
        (staged/'clayfighter64').write_bytes(b'executable fixture')
        package.write_manifest(staged,'Linux x86-64','2.0')
        manifest=json.loads((staged/'manifest.json').read_text())
        self.assertEqual(manifest['version'],'2.0');self.assertEqual(manifest['author'],'djultra64')
        self.assertEqual(manifest['portable_settings'],'local/settings')
        self.assertFalse(manifest['roms_included']);self.assertNotIn('validation',manifest)
        private=staged/'local/settings/training.cfg';private.parent.mkdir(parents=True);private.write_text('saved')
        with self.assertRaisesRegex(ValueError,'Private state'):package.write_manifest(staged,'Linux x86-64','2.0')
    def test_both_platform_packages_and_source_archive_with_stub_inputs(self):
        for name in ['README.md','LICENSE','NOTICE','CREDITS.md','docs/INSTALL.md','docs/BUILD.md',
                     'tools/release/import_assets.py','tools/release/install_assets_gui.ps1','tools/release/launch.py','tools/port_assets.py',
                     'tools/sc_import_assets.py','assets/branding/icon.png','config/controls.cfg',
                     'licenses/ThirdParty/LICENSE','local/audio/title64.s16le','build/claytality64',
                     'generated/release-decoder/font-decoder','local/releases/windows/claytality64.exe',
                     'local/releases/windows/SDL2.dll','local/font-decoder.exe']:
            self.put(name)
        for name in ['libdxcompiler.so','libdxil.so']:
            self.put('sdk/deps/rt64/src/contrib/dxc/lib/x64/'+name)
        for name in ['libSDL2-2.0.so.0','libSDL3.so.0','libdbus-1.so.3']:
            self.put('fixture-libs/'+name)
        with zipfile.ZipFile(self.root/'local/python-3.13.9-embed-amd64.zip','w') as archive:
            archive.writestr('LICENSE.txt','PSF authentic notice fixture')
            archive.writestr('python.exe','interpreter fixture')
        out=self.root/'local/releases/2.0'
        original_copy=package.copy
        def fixture_copy(source,destination):
            if str(source).startswith('/home/linuxbrew/'):
                source=self.root/'fixture-libs'/source.name
            original_copy(source,destination)
        with patch.object(package,'ROOT',self.root),patch.object(package,'OUT',out),\
             patch.object(package,'SDK',self.root/'sdk'),patch.object(package,'copy',fixture_copy):
            with contextlib.redirect_stdout(io.StringIO()):package.build('2.0')
        linux=out/'clayfighter64-2.0-linux-x86_64'
        windows=out/'clayfighter64-2.0-windows-x86_64'
        for staged in [linux,windows]:
            manifest=json.loads((staged/'manifest.json').read_text())
            self.assertEqual(manifest['version'],'2.0')
            self.assertNotIn('local/settings/training.cfg',manifest['files'])
            self.assertFalse((staged/'docs/VALIDATION_1.0.md').exists())
        self.assertTrue((out/(linux.name+'.tar.gz')).is_file())
        self.assertTrue((out/(windows.name+'.zip')).is_file())
        self.assertTrue((windows/'tools/release/install_assets_gui.ps1').is_file())
        for name in ['Install Assets.vbs', 'Play ClayFighter 64.vbs']:
            self.assertIn('pythonw.exe', (windows/name).read_text())
        self.assertTrue((out/'clayfighter64-2.0-source.tar.gz').is_file())
        self.assertEqual(len((out/'SHA256SUMS').read_text().splitlines()),3)
        with zipfile.ZipFile(out/(windows.name+'.zip')) as archive:
            self.assertTrue(all('\\' not in name for name in archive.namelist()))
    def test_importer_rejects_traversal_and_symlink_escape(self):
        self.assertEqual(import_assets.portable_path(self.root,'local/settings'),self.root/'local/settings')
        with self.assertRaises(ValueError):import_assets.portable_path(self.root,'../outside')
        external=self.root.parent/'outside';external.mkdir()
        (self.root/'local').symlink_to(external,target_is_directory=True)
        with self.assertRaises(ValueError):import_assets.portable_path(self.root,'local/import-work')


if __name__=='__main__':unittest.main()
