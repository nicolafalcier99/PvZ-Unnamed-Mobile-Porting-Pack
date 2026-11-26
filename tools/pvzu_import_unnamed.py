"""Import the player's own Unnamed zip into a PVZU assets folder (desktop twin of the Android importer).
Copies only game asset folders from the zip's single top folder; never executes or keeps exe/dll/js.
Usage: python3 -I tools/pvzu_import_unnamed.py <unnamed.zip> <dest assets dir>
The same rules live in android/.../UnnamedImport.java: keep them in sync."""
import hashlib, json, os, shutil, sys, zipfile

ASSET_DIRS = {'images', 'reanim', 'sounds', 'compiled', 'particles', 'fonts', 'font', 'data', 'properties'}
TOP_FILE_EXTS = {'.mp3', '.ogg'}
BANNED_EXTS = {'.exe', '.dll', '.js', '.bat', '.cmd', '.vbs', '.ps1', '.lnk', '.scr', '.msi', '.com'}


def decode_name(info):
    if info.flag_bits & 0x800:
        return info.filename
    try:
        raw = info.filename.encode('cp437')
    except UnicodeEncodeError:
        return info.filename  # already real Unicode (zip "Unicode Path" extra field, read by newer Pythons)
    try:
        return raw.decode('gbk')
    except UnicodeDecodeError:
        return raw.decode('gb18030', errors='replace')


def target_for(name, top):
    """Relative destination for a zip entry, or None to skip it."""
    name = name.replace('\\', '/')
    if not top or not name.startswith(top + '/'):
        return None
    rel = name[len(top) + 1:]
    parts = [p for p in rel.split('/') if p not in ('', '.')]
    if not parts or any(p == '..' for p in parts) or rel.startswith('/'):
        return None
    ext = os.path.splitext(parts[-1])[1].lower()
    if ext in BANNED_EXTS:
        return None
    if len(parts) == 1:
        return parts[0] if ext in TOP_FILE_EXTS else None
    if parts[0].lower() not in ASSET_DIRS:
        return None
    return '/'.join(parts)


def find_top_folder(names):
    tops = {n.replace('\\', '/').split('/', 1)[0] for n in names if '/' in n.replace('\\', '/')}
    return tops.pop() if len(tops) == 1 else None


def import_unnamed_zip(zip_path, dest):
    h = hashlib.sha256()
    with open(zip_path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    os.makedirs(dest, exist_ok=True)
    dest_real = os.path.realpath(dest)
    copied, rejected = 0, []
    with zipfile.ZipFile(zip_path) as z:
        infos = [i for i in z.infolist() if not i.is_dir()]
        names = [decode_name(i) for i in infos]
        top = find_top_folder(names)
        for info, name in zip(infos, names):
            rel = target_for(name, top)
            if rel is None:
                if '..' in name.replace('\\', '/').split('/'):
                    rejected.append(name)
                continue
            out = os.path.realpath(os.path.join(dest, rel))
            if not out.startswith(dest_real + os.sep):
                rejected.append(name)
                continue
            os.makedirs(os.path.dirname(out), exist_ok=True)
            with z.open(info) as src, open(out, 'wb') as dst:
                shutil.copyfileobj(src, dst)
            copied += 1
    manifest = {'source_sha256': h.hexdigest(), 'top_folder': top, 'copied': copied, 'rejected': rejected}
    with open(os.path.join(dest, 'import-manifest.json'), 'w') as f:
        json.dump(manifest, f, ensure_ascii=False, indent=1)
    return manifest


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    r = import_unnamed_zip(sys.argv[1], sys.argv[2])
    print(f"copied {r['copied']} files from {r['top_folder']}; rejected {len(r['rejected'])}")
