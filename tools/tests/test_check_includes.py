"""Tests for tools/pvzu_check_includes.py. Run: python3 -I -m unittest discover -s tools/tests -t tools"""
import os, sys, tempfile, unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
import pvzu_check_includes as chk  # noqa: E402


def write(root, rel, text=''):
    path = os.path.join(root, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as f:
        f.write(text)


def problems(root, include_dirs=('src',)):
    return chk.check_tree(root, scan_dirs=('src', 'tests'), include_dirs=include_dirs)


class CheckIncludesTests(unittest.TestCase):
    def test_ExactCaseIncludesPass(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, 'src/Lawn/Cutscene.h')
            write(d, 'src/Lawn/Board.h', '#include "Cutscene.h"\n')
            write(d, 'src/Mod/A.cpp', '#include "Lawn/Cutscene.h"\n#include "../Lawn/Board.h"\n#include <vector>\n')
            self.assertEqual(problems(d), [])

    def test_WrongCaseInIncludeDirIsReportedWithRealName(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, 'src/Lawn/Cutscene.h')
            write(d, 'src/Mod/Engine/NightLevels.cpp', '// x\n#include "Lawn/CutScene.h"\n')
            found = problems(d)
            self.assertEqual(len(found), 1)
            p = found[0]
            self.assertEqual((p.file, p.line), (os.path.join('src', 'Mod', 'Engine', 'NightLevels.cpp'), 2))
            self.assertEqual(p.include, 'Lawn/CutScene.h')
            self.assertEqual(p.real, 'Lawn/Cutscene.h')

    def test_WrongCaseDirectoryComponentIsReported(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, 'src/Lawn/Widget/X.h')
            write(d, 'tests/t.cpp', '#include "lawn/widget/X.h"\n')
            found = problems(d)
            self.assertEqual([(p.include, p.real) for p in found], [('lawn/widget/X.h', 'Lawn/Widget/X.h')])

    def test_WrongCaseRelativeToIncludingFile(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, 'src/Mod/Hud.h')
            write(d, 'src/Mod/Hud.cpp', '#include "HUD.h"\n')
            self.assertEqual([p.real for p in problems(d)], ['Hud.h'])

    def test_AngleIncludeOfProjectHeaderIsChecked(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, 'src/Mod/GameData.h')
            write(d, 'src/x.cpp', '#include <mod/GameData.h>\n#include <SDL3/SDL.h>\n')
            self.assertEqual([p.include for p in problems(d)], ['mod/GameData.h'])

    def test_BackslashInIncludeIsReported(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, 'src/Lawn/Board.h')
            write(d, 'src/x.cpp', '#include "Lawn\\Board.h"\n')
            self.assertEqual(len(problems(d)), 1)

    def test_UnknownHeadersAreNotProblems(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, 'src/x.cpp', '#include "generated_config.h"\n#include <windows.h>\n')
            self.assertEqual(problems(d), [])

    def test_ExactMatchInLaterIncludeDirWins(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, 'third_party/doctest.h')
            write(d, 'tests/t.cpp', '#include "doctest.h"\n')
            self.assertEqual(problems(d, include_dirs=('src', 'third_party')), [])

    def test_BackslashIncludeIsAlsoCheckedForCase(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, 'src/Lawn/Board.h')
            write(d, 'src/x.cpp', '#include "lawn\\Board.h"\n')
            self.assertEqual(sorted(p.why for p in problems(d)), ['backslash in path', 'wrong case'])

    def test_QuoteCharacterLiteralDoesNotHideAComment(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, 'src/Lawn/Cutscene.h')
            write(d, 'src/x.cpp', "char q = '\"'; /*\n#include \"Lawn/CutScene.h\"\n*/\n")
            self.assertEqual(problems(d), [])

    def test_CommentedOutIncludeIsIgnored(self):
        with tempfile.TemporaryDirectory() as d:
            write(d, 'src/Lawn/Cutscene.h')
            write(d, 'src/x.cpp', '// #include "Lawn/CutScene.h"\n/* #include "Lawn/CutScene.h" */\n')
            self.assertEqual(problems(d), [])


if __name__ == '__main__':
    unittest.main()
