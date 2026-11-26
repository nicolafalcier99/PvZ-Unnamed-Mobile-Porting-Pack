"""Tests for tools/pvzu_import_unnamed.py. Run: python3 -I -m unittest discover -s tools/tests -t tools"""
import io, json, os, sys, tempfile, unittest, zipfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
import pvzu_import_unnamed as imp  # noqa: E402


class RawNameInfo(zipfile.ZipInfo):
    """Stores the name bytes as given, without the UTF-8 flag (what Chinese zip tools write)."""
    def __init__(self, raw):
        super().__init__(raw.decode('cp437'))
        self._raw = raw

    def _encodeFilenameFlags(self):
        return self._raw, self.flag_bits & ~0x800


def make_zip(path, entries):
    """entries: list of (raw_name_bytes_or_str, data). bytes names are stored without the UTF-8 flag (GBK zip)."""
    with zipfile.ZipFile(path, 'w') as z:
        for name, data in entries:
            info = RawNameInfo(name) if isinstance(name, bytes) else zipfile.ZipInfo(name)
            z.writestr(info, data)


class ImporterTests(unittest.TestCase):
    def test_ImporterHandlesGbkNamesAndSkipsBinaries(self):
        with tempfile.TemporaryDirectory() as d:
            zpath = os.path.join(d, 'u.zip')
            gbk_help = '需要帮助.png'.encode('gbk')
            gbk_img = '[1.3beta2]/images/'.encode() + '僵尸.png'.encode('gbk')
            make_zip(zpath, [
                (gbk_help, b'top-level help'),
                ('使用必看和免责声明.txt'.encode('gbk'), b'disclaimer'),
                ('[1.3beta2]/PlantsVsZombies.exe', b'MZ'),
                ('[1.3beta2]/gdi42.dll', b'MZ'),
                ('[1.3beta2]/reanim/AC_RunActiveContent.js', b'js'),
                (gbk_img, b'png'),
                ('[1.3beta2]/images/Zombie.png', b'png'),
                ('[1.3beta2]/properties/LawnStrings.txt', b'[A]\nb\n'),
                ('[1.3beta2]/lawnbgm(1).mp3', b'mp3'),
                ('[1.3beta2]/../evil.txt', b'no'),
                ('[1.3beta2]/notes/readme.txt', b'not an asset folder'),
            ])
            out = os.path.join(d, 'assets')
            result = imp.import_unnamed_zip(zpath, out)
            got = sorted(os.path.relpath(os.path.join(r, f), out).replace(os.sep, '/')
                         for r, _, fs in os.walk(out) for f in fs)
            self.assertEqual(got, sorted([
                'images/僵尸.png', 'images/Zombie.png', 'import-manifest.json',
                'lawnbgm(1).mp3', 'properties/LawnStrings.txt',
            ]))
            self.assertEqual(result['copied'], 4)
            self.assertEqual(result['top_folder'], '[1.3beta2]')
            manifest = json.load(open(os.path.join(out, 'import-manifest.json')))
            self.assertEqual(len(manifest['source_sha256']), 64)
            self.assertIn('[1.3beta2]/../evil.txt', manifest['rejected'])

    def test_target_rules(self):
        t = imp.target_for
        self.assertEqual(t('[1.3beta2]/images/a.png', '[1.3beta2]'), 'images/a.png')
        self.assertEqual(t('[1.3beta2]/Compiled/particles/x.compiled', '[1.3beta2]'), 'Compiled/particles/x.compiled')
        self.assertIsNone(t('[1.3beta2]/gdi42.dll', '[1.3beta2]'))
        self.assertIsNone(t('loose.txt', '[1.3beta2]'))
        self.assertIsNone(t('[1.3beta2]/images/../../x.png', '[1.3beta2]'))
        self.assertIsNone(t('[1.3beta2]/IMAGES/evil.EXE', '[1.3beta2]'))


if __name__ == '__main__':
    unittest.main()
