#!/usr/bin/env python3
"""PVZU: checks that every #include of a project header names the file with its exact case.

macOS (case-insensitive APFS) finds "Lawn/CutScene.h" when the file is Lawn/Cutscene.h; Linux, Android (NDK on
Linux) and WebAssembly CI do not, and stop with "file not found". This walks src/ and tests/ and resolves each
include the way the compiler does (the including file's folder first, then the include folders from CMakeLists.txt),
comparing names exactly. A header found only with a different case is a problem; a header not found at all is
assumed to be a system or generated header and skipped. A backslash in an include path is also a problem.

Usage: python3 -I tools/pvzu_check_includes.py [repo-root]     (exit 1 when a problem is found)
Tests: python3 -I -m unittest discover -s tools/tests -t tools
"""
import os, re, sys
from collections import namedtuple

SCAN_DIRS = ('src', 'tests')
# The include folders the game and test targets pass to the compiler (CMakeLists.txt, tests/CMakeLists.txt).
INCLUDE_DIRS = ('src', 'src/SexyAppFramework', 'third_party', 'tests/fixtures')
SOURCE_EXT = ('.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx', '.inc', '.inl', '.ipp', '.m', '.mm')
INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*(?:include|import|include_next)[ \t]*([<"])([^>"\n]+)[>"]', re.M)

Problem = namedtuple('Problem', 'file line include real why')


def strip_comments(text):
    """Blanks out comments (keeping newlines, so line numbers stay right) without touching string contents."""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and text.startswith('//', i):
            j = text.find('\n', i)
            j = n if j < 0 else j
            i = j
        elif c == '/' and text.startswith('/*', i):
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append('\n' * text.count('\n', i, j))
            i = j
        elif c in '"\'':  # string or character literal ('"' must not open a string)
            j = i + 1
            while j < n and text[j] not in c + '\n':
                j += 2 if text[j] == '\\' else 1
            out.append(text[i:j + 1])
            i = j + 1
        else:
            out.append(c)
            i += 1
    return ''.join(out)


class Tree:
    """Exact-case and case-insensitive lookups of relative paths, with directory listings cached."""
    def __init__(self):
        self._ls = {}

    def _entries(self, d):
        if d not in self._ls:
            try:
                self._ls[d] = os.listdir(d)
            except OSError:
                self._ls[d] = None
        return self._ls[d]

    def resolve(self, base, rel):
        """Returns (exact, real): exact is True when rel names an existing file with exact case below base;
        real is the correctly-cased rel when only a case-insensitive match exists, else None."""
        cur, real_parts, exact = base, [], True
        parts = [p for p in rel.split('/') if p not in ('', '.')]
        for k, part in enumerate(parts):
            if part == '..':
                cur = os.path.dirname(cur)
                real_parts.append(part)
                continue
            entries = self._entries(cur)
            if entries is None:
                return False, None
            if part in entries:
                hit = part
            else:
                low = [e for e in entries if e.lower() == part.lower()]
                if not low:
                    return False, None
                hit, exact = low[0], False
            cur = os.path.join(cur, hit)
            real_parts.append(hit)
        if not os.path.isfile(cur):
            return False, None
        return (True, None) if exact else (False, '/'.join(real_parts))


def check_tree(root, scan_dirs=SCAN_DIRS, include_dirs=INCLUDE_DIRS):
    root = os.path.abspath(root)
    tree = Tree()
    inc_dirs = [os.path.join(root, d) for d in include_dirs if os.path.isdir(os.path.join(root, d))]
    found = []
    for scan in scan_dirs:
        for dirpath, dirnames, filenames in os.walk(os.path.join(root, scan)):
            dirnames.sort()
            for name in sorted(filenames):
                if not name.endswith(SOURCE_EXT):
                    continue
                path = os.path.join(dirpath, name)
                rel_file = os.path.relpath(path, root)
                with open(path, encoding='utf-8', errors='replace') as f:
                    text = strip_comments(f.read())
                for m in INCLUDE_RE.finditer(text):
                    line = text.count('\n', 0, m.start()) + 1
                    quote, inc = m.group(1), m.group(2).strip()
                    if '\\' in inc:
                        found.append(Problem(rel_file, line, inc, inc.replace('\\', '/'), 'backslash in path'))
                        inc = inc.replace('\\', '/')  # and still check its case below
                    bases = ([dirpath] if quote == '"' else []) + inc_dirs
                    real = None
                    for base in bases:
                        exact, r = tree.resolve(base, inc)
                        if exact:
                            real = None
                            break
                        if r and real is None:
                            real = r
                    if real:
                        found.append(Problem(rel_file, line, inc, real, 'wrong case'))
    return found


def main(argv):
    root = argv[1] if len(argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    found = check_tree(root)
    for p in found:
        print(f'{p.file}:{p.line}: #include "{p.include}": {p.why}, the file is "{p.real}"')
    print(f'{len(found)} include problem(s)')
    return 1 if found else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
