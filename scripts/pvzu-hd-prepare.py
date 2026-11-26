#!/usr/bin/env python3
"""PVZU: build an HD art pack from AI-upscaled copies of the player's own PC images (see src/Mod/HdArt.h).

Private by design: the pack holds derived copies of EA's art. Never commit it, never share it. This script holds
no assets; it reads the PC files and the 4x upscales from your disk and writes the pack folder.

    pvzu-hd-prepare.py [--scale 2] [--big-scale N --big-pixels P] [out_dir]

Inputs (defaults: the DEFAULT_* paths below):
  --pc   the extracted main.pak (reanim/, images/, particles/, properties/resources.xml)
  --src  the 4x upscales, same relative names, every file .png (jpg sources became png)
Output: <out>/<reanim|images|particles>/<PC name>.png and <out>/hd.json {"scale", "count", "source"}.

What it does for each image the engine loads:
  * resize 4x -> scale with Lanczos on premultiplied alpha (no dark halos), then bleed colour into the fully
    transparent pixels (the engine's own "sanding" does the same for PC images under linear filtering);
  * split alpha: PvZ keeps some images as a colour file plus a separate alpha file ("<dir>/_name" or
    "<dir>/name_", ImageLib composes alpha = the alpha file's blue channel; an alpha file without a colour file
    becomes white + alpha), and resources.xml can name an alphaimage / alphagrid (one alpha cel repeated over the
    image's rows x cols). The engine attaches the twin after that composition, so the pack holds the composed
    RGBA under the colour image's name and no separate alpha files;
  * images the upscaler could not read (GIFs) get no twin: the engine keeps the PC pixels;
  * --override <dir> (default hd-upscale/seedvr-bg when it exists): <name>.png there replaces the a4x colour image
    of the image with that base name (e.g. SeedVR2 lawns at 2x: no resize at factor 2, else resized from its size).
Requires Pillow and numpy (e.g. python3).

--unnamed (plan 9, U1): builds the hd-unnamed pack (default pvz-game-files/hd-unnamed-2x; phone <files>/hd-unnamed/)
for the art the Unnamed overlay serves (pvz-unnamed-pack/assets minus data/overlay-skip-*.txt). Each image the game
loads is resolved the way ImageLib + PakInterface do it (per extension png, jpg, gif, tga: overlay file first,
then PC; alpha companions found by the PC's file list), in Chinese and English mode. If what it shows is
pixel-identical to the PC image, its key goes to hd.json "samePc" (the engine then uses the PC pack's twin);
otherwise a twin is composed from the 4x upscales: PC pieces from --src (a4x), Unnamed pieces that differ from PC
from --unnamed-src (pvz-game-files/hd-upscale/unnamed-a4x, made by pvz-game-files/hd-upscale/run-unnamed.py), and
background colour images from --unnamed-bg (U2's SeedVR2 output at 2x, pvz-game-files/hd-upscale/unnamed-seedvr-bg).
Also writes <out>/classes.tsv (every Unnamed image: class, status vs PC, served, need) and, with
--list-upscale FILE, the Unnamed files still to upscale (path<TAB>class; class zombie = model A + outline band).
"""
import argparse
import json
import os
import re
import sys
import xml.etree.ElementTree as ET
from concurrent.futures import ProcessPoolExecutor

import numpy as np
from PIL import Image

HOME = os.path.expanduser("~")
GAME = os.path.join(HOME, "pvz-game-files")
DEFAULT_PC = os.path.join(GAME, "windows-steam-3591", "pak-x")
DEFAULT_SRC = os.path.join(GAME, "hd-upscale", "a4x")
DEFAULT_OUT = os.path.join(GAME, "hd-pack-2x")
DEFAULT_OVERRIDE = os.path.join(GAME, "hd-upscale", "seedvr-bg")  # SeedVR2 lawns at 2x, when present
FOLDERS = ("reanim", "images", "particles")  # data/ = bitmap fonts: never upscaled
EXTS = (".png", ".jpg", ".jpeg", ".gif", ".bmp", ".tga")  # ImageLib's order: the first existing one loads
SRC_FACTOR = 4


def key_of(rel):
    """Same as pvzu::hd::Key: lower case, '/' separators, image extension dropped."""
    k = rel.replace("\\", "/").lower()
    while k.startswith("./"):
        k = k[2:]
    stem, ext = os.path.splitext(k)
    return stem if ext in EXTS else k


def alpha_companions(key):
    d, n = os.path.split(key)
    return [os.path.join(d, "_" + n) if d else "_" + n, key + "_"]


def is_companion(key):
    n = os.path.basename(key)
    return n.startswith("_") or n.endswith("_")


def companion_bases(key):
    d, n = os.path.split(key)
    out = []
    if n.startswith("_"):
        out.append(n[1:])
    if n.endswith("_"):
        out.append(n[:-1])
    return [(d + "/" + b) if d else b for b in out if b]


def index_pc(pc_dir):
    """key -> PC relative path, using ImageLib's extension order when two files share a stem."""
    found = {}
    for folder in FOLDERS:
        base = os.path.join(pc_dir, folder)
        for name in sorted(os.listdir(base)):
            stem, ext = os.path.splitext(name)
            if ext.lower() not in EXTS or not os.path.isfile(os.path.join(base, name)):
                continue
            rel = folder + "/" + name
            k = key_of(rel)
            if k in found and EXTS.index(os.path.splitext(found[k])[1].lower()) <= EXTS.index(ext.lower()):
                continue
            found[k] = rel
    return found


def resource_alpha(pc_dir):
    """key of an image resource -> (alphaimage key or None, alphagrid key or None, rows, cols)."""
    path = os.path.join(pc_dir, "properties", "resources.xml")
    out = {}
    if not os.path.exists(path):
        return out
    root = ET.parse(path).getroot()
    for res in root.iter("Resources"):
        prefix = ""
        for el in res:
            if el.tag == "SetDefaults":
                prefix = el.get("path", "")
                continue
            if el.tag != "Image":
                continue
            p = el.get("path", "")
            if not p:
                continue
            full = p if "/" in p or not prefix else prefix + "/" + p
            ai, ag = el.get("alphaimage"), el.get("alphagrid")
            if not ai and not ag:
                continue
            join = lambda n: key_of(n if "/" in n or not prefix else prefix + "/" + n)
            out[key_of(full)] = (join(ai) if ai else None, join(ag) if ag else None,
                                 int(el.get("rows", "1")), int(el.get("cols", "1")))
    return out


def src_png(src_dir, pc_rel):
    stem, _ = os.path.splitext(pc_rel)
    p = os.path.join(src_dir, stem + ".png")
    return p if os.path.exists(p) else None


def load_rgba(path):
    im = Image.open(path)
    im.load()
    if im.mode != "RGBA":
        im = im.convert("RGBA")
    return np.asarray(im, dtype=np.uint8).copy()


def pc_size(pc_dir, rel):
    with Image.open(os.path.join(pc_dir, rel)) as im:
        return im.size


def resize_premultiplied(rgba, w, h):
    """Lanczos on premultiplied alpha, back to straight alpha. rgba: HxWx4 uint8."""
    if rgba.shape[1] == w and rgba.shape[0] == h:
        return rgba
    f = rgba.astype(np.float32) / 255.0
    a = f[..., 3]
    opaque = bool(np.all(rgba[..., 3] == 255))
    chans = []
    for c in range(4):
        ch = f[..., c] * (1.0 if (c == 3 or opaque) else a)
        chans.append(np.asarray(Image.fromarray(ch, mode="F").resize((w, h), Image.LANCZOS), dtype=np.float32))
    na = np.clip(chans[3], 0.0, 1.0)
    out = np.empty((h, w, 4), dtype=np.float32)
    for c in range(3):
        if opaque:
            out[..., c] = chans[c]
        else:
            with np.errstate(divide="ignore", invalid="ignore"):
                out[..., c] = np.where(na > 1e-4, chans[c] / np.maximum(na, 1e-4), 0.0)
    out[..., 3] = 1.0 if opaque else na
    return (np.clip(out, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)


def fit_channel(ch, w, h):
    """An alpha channel resized (Lanczos) to w x h when its source was a different size."""
    if ch.shape == (h, w):
        return ch
    f = Image.fromarray(ch.astype(np.float32), mode="F").resize((w, h), Image.LANCZOS)
    return np.clip(np.asarray(f) + 0.5, 0, 255).astype(np.uint8)


def bleed(rgba, rings=6):
    """Fully transparent pixels take the mean colour of their non-transparent neighbours, ring by ring
    (so linear filtering never mixes in black)."""
    a = rgba[..., 3]
    known = a > 0
    if known.all() or not known.any():
        return rgba
    rgb = rgba[..., :3].astype(np.float32)
    rgb[~known] = 0
    for _ in range(rings):
        acc = np.zeros_like(rgb)
        cnt = np.zeros(known.shape, np.float32)
        kp = np.pad(known, 1)
        rp = np.pad(rgb, ((1, 1), (1, 1), (0, 0)))
        H, W = known.shape
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dy == 0 and dx == 0:
                    continue
                k = kp[1 + dy:1 + dy + H, 1 + dx:1 + dx + W]
                acc += rp[1 + dy:1 + dy + H, 1 + dx:1 + dx + W] * k[..., None]
                cnt += k
        grow = (~known) & (cnt > 0)
        if not grow.any():
            break
        rgb[grow] = acc[grow] / cnt[grow][:, None]
        known = known | grow
    out = rgba.copy()
    out[..., :3] = np.clip(rgb + 0.5, 0, 255).astype(np.uint8)
    out[..., 3] = a
    return out


def build_one(job):
    """job: dict -> (status, key, message, out_rel, out_bytes)"""
    key, scale = job["key"], job["scale"]
    lw, lh = job["size"]
    w, h = lw * scale, lh * scale
    try:
        color = job.get("override") or job["color"]  # an override (e.g. SeedVR2 lawn) replaces the a4x colour
        if color:
            rgba = load_rgba(color)
        else:  # alpha only: ImageLib makes white (gAlphaComposeColor) + alpha
            rgba = None
        if job["alpha"]:
            alpha = load_rgba(job["alpha"])[..., 2]  # ImageLib takes the alpha file's blue channel
            if rgba is None:
                rgba = np.empty(alpha.shape + (4,), np.uint8)
                rgba[..., :3] = 255
            rgba[..., 3] = fit_channel(alpha, rgba.shape[1], rgba.shape[0])
        for grid, rows, cols in job["grids"]:  # resources.xml alphaimage (1x1) / alphagrid (rows x cols)
            ga = load_rgba(grid)[..., 2]
            th, tw = ga.shape[0] * rows, ga.shape[1] * cols
            if th % lh or tw % lw or th // lh != tw // lw:
                return ("skip", key, f"alphagrid cel {ga.shape} x {rows}x{cols} does not match {lw}x{lh}", None, 0)
            rgba[..., 3] = fit_channel(np.tile(ga, (rows, cols)), rgba.shape[1], rgba.shape[0])
        fw, fh = rgba.shape[1] / lw, rgba.shape[0] / lh
        if fw != fh or fw < 1 or fw != int(fw):
            return ("skip", key, f"source {rgba.shape[1]}x{rgba.shape[0]} is not a whole multiple of {lw}x{lh}", None, 0)
        out = bleed(resize_premultiplied(rgba, w, h))
        if np.all(out[..., 3] == 255):
            img = Image.fromarray(out[..., :3], mode="RGB")
        else:
            img = Image.fromarray(out, mode="RGBA")
        dst = os.path.join(job["out"], job["out_rel"])
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        img.save(dst, compress_level=6)
        return ("ok", key, "", job["out_rel"], os.path.getsize(dst))
    except Exception as e:  # keep going: one bad file must not stop the pack
        return ("skip", key, f"error {e}", None, 0)


def index_override(override_dir):
    """lower-case stem -> file, for the override folder's PNGs (matched on the colour image's base name)."""
    out = {}
    if override_dir and os.path.isdir(override_dir):
        for name in os.listdir(override_dir):
            stem, ext = os.path.splitext(name)
            if ext.lower() == ".png":
                out[stem.lower()] = os.path.join(override_dir, name)
    return out


def plan(pc_dir, src_dir, out_dir, scale, big_scale, big_pixels, only, override_dir=None):
    pc = index_pc(pc_dir)
    overrides = index_override(override_dir)
    res_alpha = resource_alpha(pc_dir)
    jobs, skipped = [], []
    for key, rel in sorted(pc.items()):
        color_rel, alpha_rel, out_rel = rel, None, rel
        if is_companion(key):
            bases = companion_bases(key)
            if any(b in pc for b in bases):
                continue  # composed into its colour image below
            # An alpha file with no colour file: ImageLib loads it as white + alpha under the base name.
            key, color_rel, alpha_rel = bases[0], None, rel
            out_rel = os.path.dirname(rel) + "/" + os.path.basename(key) + ".png"
        else:
            for comp in alpha_companions(key):  # ImageLib: "_name" first, then "name_"
                if comp in pc:
                    alpha_rel = pc[comp]
                    break
            out_rel = os.path.dirname(rel) + "/" + os.path.splitext(os.path.basename(rel))[0] + ".png"
        if only and not re.search(only, key):
            continue
        size = pc_size(pc_dir, color_rel or alpha_rel)
        color = src_png(src_dir, color_rel) if color_rel else None
        alpha = src_png(src_dir, alpha_rel) if alpha_rel else None
        if (color_rel and not color) or (alpha_rel and not alpha):
            skipped.append((key, "no upscale (" + (color_rel or alpha_rel) + ")"))
            continue
        grids = []
        if key in res_alpha:
            ai, ag, rows, cols = res_alpha[key]
            for gkey, r, c in ((ai, 1, 1), (ag, rows, cols)):
                if gkey:
                    g = src_png(src_dir, pc[gkey]) if gkey in pc else None
                    if not g:
                        skipped.append((key, f"no upscale of its alpha {gkey}"))
                        grids = None
                        break
                    grids.append((g, r, c))
            if grids is None:
                continue
        s = big_scale if (big_scale and size[0] * size[1] >= big_pixels) else scale
        override = overrides.get(os.path.basename(key)) if color_rel else None
        jobs.append({"key": key, "scale": s, "size": size, "color": color, "alpha": alpha, "grids": grids,
                     "out": out_dir, "out_rel": out_rel, "override": override})
    return jobs, skipped


# ---------------------------------------------------------------------------------------------------------------
# --unnamed: the hd-unnamed pack (plan 9, U1). See the module docstring.

DEFAULT_PACK = os.path.join(HOME, "pvz-unnamed-pack")
DEFAULT_UNNAMED_OUT = os.path.join(GAME, "hd-unnamed-2x")
DEFAULT_UNNAMED_SRC = os.path.join(GAME, "hd-upscale", "unnamed-a4x")
DEFAULT_UNNAMED_BG = os.path.join(GAME, "hd-upscale", "unnamed-seedvr-bg")
ENGINE_EXTS = (".png", ".jpg", ".gif", ".tga")  # ImageLib kImageExts: the order TryLoadByExt tries
APP = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Full-screen scenes: SeedVR2 (agent U2), never Real-ESRGAN. Their alpha companions stay Real-ESRGAN model A.
# (Not SelectorScreen_BG: 134x75, smaller than one SeedVR2 tile; model A like its PC twin.)
BACKGROUND_RE = re.compile(r"^(background\d(unsodded|boss)?|awardscreen_back|selectorscreen_bg_(center|left)|"
                           r"titlescreen|tree_bg|challenge_background|background_(greenhouse|mushroomgarden)|"
                           r"aquarium\d|almanac_(index|plant|zombie)back|store_background(night)?)$")
# Zombie art outside the vanilla zombie reanims (Unnamed's Football reskins, the green "lvmao" head). Not
# "zombie" alone: ZombieNote*, ZombiesWon, Zombie_questionmark are UI (text) and stay clean model A.
ZOMBIE_NAME_RE = re.compile(r"^(zombie_(?!questionmark)|black_football|green_football|lvmao)")
MOD_PLANT_RE = re.compile(r"^(hurrikale|ytfs)")


def index_dir(root):
    """lower relative path (reanim|images|particles/...) -> real path, image files only."""
    out = {}
    for folder in FOLDERS:
        base = os.path.join(root, folder)
        if not os.path.isdir(base):
            continue
        for name in os.listdir(base):
            if os.path.splitext(name)[1].lower() in EXTS and os.path.isfile(os.path.join(base, name)):
                out[(folder + "/" + name).lower()] = os.path.join(base, name)
    return out


def read_skip(path):
    out = set()
    if os.path.exists(path):
        for line in open(path, encoding="utf-8"):
            line = line.strip()
            if line and not line.startswith("#"):
                out.add(line.replace("\\", "/").lower())
    return out


def reanim_classes(pc_dir):
    """upper image stem (IMAGE_REANIM_<X> -> X) -> 'plant' | 'zombie', from the vanilla tables in the app source."""
    def table(path, pattern):
        try:
            return set(re.findall(pattern, open(os.path.join(APP, path), encoding="utf-8").read()))
        except OSError:
            return set()
    plants = table("src/Lawn/Plant.cpp", r"mReanimationType = ReanimationType::(REANIM_\w+)")
    zombies = table("src/Lawn/Zombie.cpp", r"mReanimationType = (REANIM_\w+)")
    files = {}
    try:
        src = open(os.path.join(APP, "src/PvzpLib/Reanimator.cpp"), encoding="utf-8").read()
        for t, f in re.findall(r"ReanimationType::(REANIM_\w+), \.mReanimFileName = \"([^\"]+)\"", src):
            files[t] = f
    except OSError:
        pass
    out = {}
    for kind, types in (("zombie", zombies), ("plant", plants)):
        for t in types:
            f = files.get(t)
            p = os.path.join(pc_dir, f) if f else None
            if not p or not os.path.exists(p):
                continue
            for img in re.findall(r"IMAGE_REANIM_(\w+)", open(p, encoding="utf-8", errors="replace").read()):
                out.setdefault(img.upper(), kind)
                if kind == "plant" and "_" in img:
                    out.setdefault("PREFIX:" + img.split("_")[0].upper(), kind)
    return out


def classify(rel, reanim_kind):
    folder, name = rel.split("/", 1)
    stem = os.path.splitext(name)[0].lower()
    base = stem.strip("_")
    if folder == "particles":
        return "particle"
    if BACKGROUND_RE.match(base):
        return "background"
    if ZOMBIE_NAME_RE.match(base) or reanim_kind.get(base.upper()) == "zombie":
        return "zombie"
    if MOD_PLANT_RE.match(base) or reanim_kind.get(base.upper()) == "plant":
        return "plant"
    if "_" in base and reanim_kind.get("PREFIX:" + base.split("_")[0].upper()) == "plant":
        return "plant"  # a new part of a vanilla plant (PeaShooter_Head1, SnowPea_mouth1)
    return "ui"


class Resolver:
    """What ImageLib loads for a key in one language: overlay served files win per extension; alpha companions
    are probed on the PC's file list (ImageLib's FastFileExists never sees the overlay)."""

    def __init__(self, pc, served):
        self.pc, self.served = pc, served

    def file(self, key):
        for ext in ENGINE_EXTS:
            if key + ext in self.served:
                return ("ov", self.served[key + ext], key + ext)
            if key + ext in self.pc:
                return ("pc", self.pc[key + ext], key + ext)
        return None

    def pc_exists(self, key):
        return any(key + ext in self.pc for ext in ENGINE_EXTS)

    def spec(self, key, res_alpha):
        """(colour, alpha, grids) of the image the game composes for key, or None. Pieces: (where, path, rel)."""
        colour = self.file(key)
        alpha = None
        for comp in alpha_companions(key):
            if self.pc_exists(comp):
                alpha = self.file(comp)
                if alpha:
                    break
        grids = []
        if key in res_alpha:
            ai, ag, rows, cols = res_alpha[key]
            for gkey, r, c in ((ai, 1, 1), (ag, rows, cols)):
                if gkey:
                    g = self.file(gkey)
                    if g:
                        grids.append((g, r, c))
        if colour is None and alpha is None:
            return None
        return (colour, alpha, tuple(grids))


_pixel_cache = {}


def pixels(path):
    if path not in _pixel_cache:
        _pixel_cache[path] = load_rgba(path)
    return _pixel_cache[path]


def same_pixels(a, b):
    pa, pb = pixels(a), pixels(b)
    return pa.shape == pb.shape and bool(np.array_equal(pa, pb))


def drop_mismatched_alpha(spec):
    """ImageLib ComposeAlpha skips an alpha image of another size (the colour image keeps its own alpha)."""
    colour, alpha, grids = spec
    if colour and alpha and pixels(colour[1]).shape[:2] != pixels(alpha[1]).shape[:2]:
        alpha = None
    return (colour, alpha, grids)


def same_spec(a, b):
    """Both specs compose the same pixels: same pieces, each pixel-identical."""
    if a is None or b is None:
        return False
    pa = [a[0], a[1]] + [g[0] for g in a[2]]
    pb = [b[0], b[1]] + [g[0] for g in b[2]]
    if len(pa) != len(pb) or [g[1:] for g in a[2]] != [g[1:] for g in b[2]]:
        return False
    for x, y in zip(pa, pb):
        if (x is None) != (y is None):
            return False
        if x is not None and x[1] != y[1] and not same_pixels(x[1], y[1]):
            return False
    return True


def unnamed_plan(args):
    pc_dir, pack = args.pc, args.pack
    pc = index_dir(pc_dir)
    ov = index_dir(os.path.join(pack, "assets"))
    always = read_skip(os.path.join(pack, "data", "overlay-skip-always.txt"))
    en_skip = read_skip(os.path.join(pack, "data", "overlay-en-skip.txt"))
    served = {"zh": {k: v for k, v in ov.items() if k not in always}}
    served["en"] = {k: v for k, v in served["zh"].items() if k not in en_skip}
    res_alpha = resource_alpha(pc_dir)
    pc_only = Resolver(pc, {})
    kinds = reanim_classes(pc_dir)

    # The keys the engine asks the hd-unnamed pack about: OverlayServes(key) = the key or a companion is served.
    keys = set()
    for rel in served["zh"]:
        k = key_of(rel)
        keys.update(companion_bases(k) if is_companion(k) else [k])
    rows, jobs, same_pc, same_pc_en, upscale, waiting, conflicts = {}, [], [], [], {}, [], []
    for key in sorted(keys):
        specs = {}
        for lang in ("zh", "en"):
            if any(key + e in served[lang] or c + e in served[lang]
                   for c in [key] + alpha_companions(key) for e in EXTS):
                s = Resolver(pc, served[lang]).spec(key, res_alpha)
                if s:
                    specs[lang] = drop_mismatched_alpha(s)
        if not specs:
            continue
        spec = specs.get("zh") or specs["en"]
        if all(p is None or p[0] == "pc" for p in [spec[0], spec[1]] + [g[0] for g in spec[2]]):
            continue  # nothing from the overlay ends up in the image: the PC twin is right without a list entry
        pc_spec = pc_only.spec(key, res_alpha)
        if pc_spec:
            pc_spec = drop_mismatched_alpha(pc_spec)
        if same_spec(spec, pc_spec):
            same_pc.append(key)
            continue
        if "en" in specs and specs["en"] != spec:
            # A piece is on overlay-en-skip.txt: the pack's file is the Chinese image; English needs another one.
            if same_spec(specs["en"], pc_spec):
                same_pc_en.append(key)  # English shows the PC image: the engine uses the PC twin there
            else:
                conflicts.append(key)  # would need an English twin too: not supported, the Chinese one is used
        colour, alpha, grids = spec
        cls = classify((colour or alpha)[2], kinds)

        def source(piece, is_colour):
            where, path, rel = piece
            if where == "ov" and rel in pc and same_pixels(path, pc[rel]):
                where = "pc"
            if where == "pc":
                return src_png(args.src, pc[rel][len(pc_dir) + 1:]), None
            stem_rel = os.path.relpath(os.path.splitext(path)[0], os.path.join(pack, "assets"))
            if is_colour and cls == "background":
                bg = os.path.join(args.unnamed_bg, stem_rel + ".png")
                return None, (bg if os.path.exists(bg) else "")
            upscale[stem_rel] = "zombie" if cls == "zombie" else "a"
            p = os.path.join(args.unnamed_src, stem_rel + ".png")
            return (p if os.path.exists(p) else None), None

        c_src, bg = source(colour, True) if colour else (None, None)
        a_src = source(alpha, False)[0] if alpha else None
        g_src = [(source(g, False)[0], r, c) for g, r, c in grids]
        out_rel = os.path.dirname((colour or alpha)[2]) + "/" + os.path.basename(
            os.path.splitext((colour or alpha)[1])[0]) + ".png"
        size = pc_size(os.path.dirname((colour or alpha)[1]), os.path.basename((colour or alpha)[1]))
        rows[key] = cls
        if bg == "":  # background without SeedVR2 output yet: keep a finished twin U2 put in place, else wait
            if os.path.exists(os.path.join(args.out, out_rel)):
                jobs.append({"key": key, "keep": out_rel})
            else:
                waiting.append(key)
            continue
        if (colour and not (c_src or bg)) or (alpha and not a_src) or any(g[0] is None for g in g_src):
            waiting.append(key)
            continue
        jobs.append({"key": key, "scale": args.scale, "size": size, "color": c_src, "alpha": a_src,
                     "grids": g_src, "out": args.out, "out_rel": out_rel, "override": bg})

    # classes.tsv: every Unnamed image (data/ = bitmap fonts, never upscaled).
    table = []
    for rel, path in sorted(ov.items()):
        real_rel = os.path.relpath(path, os.path.join(pack, "assets"))
        pc_twin = pc[rel] if rel in pc else (pc_only.file(key_of(rel)) or (None, None))[1]  # same name, any ext
        if pc_twin:
            status = "same" if same_pixels(path, pc_twin) else "diff"
        else:
            status = "new"
        srv = "en+zh" if rel in served["en"] else ("zh" if rel in served["zh"] else "never")
        stem_rel = os.path.splitext(real_rel)[0]
        if srv == "never":
            need = "none"
        elif stem_rel in upscale or (classify(real_rel, kinds) == "background" and status != "same"
                                     and key_of(rel) in rows):
            need = "upscale"
        else:
            need = "pc-twin"
        table.append((real_rel, classify(real_rel, kinds), status, srv, need))
    return jobs, same_pc, same_pc_en, upscale, waiting, conflicts, table


def main_unnamed(args):
    jobs, same_pc, same_pc_en, upscale, waiting, conflicts, table = unnamed_plan(args)
    if args.list_upscale:
        with open(args.list_upscale, "w") as f:
            for rel, kind in sorted(upscale.items()):
                f.write(f"{rel}\t{kind}\n")
        print(f"{len(upscale)} Unnamed files to upscale -> {args.list_upscale}")
    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "classes.tsv"), "w") as f:
        f.write("path\tclass\tstatus\tserved\tneed\n")
        for row in table:
            f.write("\t".join(row) + "\n")
    build = [j for j in jobs if "keep" not in j]
    if args.only:
        build = [j for j in build if re.search(args.only, j["key"])]
    ok, total, skipped = 0, 0, []
    with ProcessPoolExecutor(max_workers=args.jobs) as ex:
        for status, key, msg, rel, size in ex.map(build_one, build, chunksize=2):
            if status == "ok":
                ok += 1
                total += size
            else:
                skipped.append((key, msg))
    # count = twins on disk (also those built by an earlier run or kept for U2), so a partial run stays truthful
    on_disk = sum(1 for k, _ in index_dir(args.out).items() if not is_companion(key_of(k)))
    info = {"scale": args.scale, "count": on_disk, "source": "realesrgan-x4plus-anime",
            "plants": "realesrgan-x4plus-anime", "ui": "realesrgan-x4plus-anime",
            "zombies": "mix: realesrgan-x4plus-anime inside, realesr-animevideov3 outline",
            "backgrounds": "seedvr2", "samePc": same_pc, "samePcEn": same_pc_en}
    with open(os.path.join(args.out, "hd.json"), "w") as f:
        json.dump(info, f, indent=0)
        f.write("\n")
    for key, why in skipped:
        print(f"skip {key}: {why}")
    for key in waiting:
        print(f"waiting for an upscale: {key}")
    for key in conflicts:
        print(f"English image differs from PC and from the Chinese twin (Chinese twin used): {key}")
    print(f"hd-unnamed {args.out}: built {ok} ({total / 1e6:.1f} MB), {on_disk} twins on disk, "
          f"{len(same_pc)} samePc, {len(waiting)} waiting, {len(skipped)} skipped")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out", nargs="?", default=None)
    ap.add_argument("--unnamed", action="store_true", help="build the hd-unnamed pack (see above)")
    ap.add_argument("--pack", default=DEFAULT_PACK, help="--unnamed: the Unnamed pack (assets/, data/)")
    ap.add_argument("--unnamed-src", default=DEFAULT_UNNAMED_SRC, help="--unnamed: 4x upscales of Unnamed files")
    ap.add_argument("--unnamed-bg", default=DEFAULT_UNNAMED_BG, help="--unnamed: SeedVR2 backgrounds (2x)")
    ap.add_argument("--list-upscale", default="", help="--unnamed: write the Unnamed files still to upscale")
    ap.add_argument("--pc", default=DEFAULT_PC)
    ap.add_argument("--src", default=DEFAULT_SRC)
    ap.add_argument("--scale", type=int, default=2, choices=(2, 4))
    ap.add_argument("--big-scale", type=int, default=0, choices=(0, 2), help="factor for big images (budget)")
    ap.add_argument("--big-pixels", type=int, default=300000, help="logical w*h from which --big-scale applies")
    ap.add_argument("--override", default=DEFAULT_OVERRIDE if os.path.isdir(DEFAULT_OVERRIDE) else "",
                    help="folder of <name>.png that replace the a4x colour image (any whole multiple of the PC size)")
    ap.add_argument("--plants-model", default="realesrgan-x4plus-anime", help="recorded in hd.json")
    ap.add_argument("--zombies-model", default="mix: realesrgan-x4plus-anime inside, realesr-animevideov3 outline",
                    help="recorded in hd.json (zombie* files)")
    ap.add_argument("--only", default="", help="regex on keys (tests)")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 4) - 2))
    args = ap.parse_args()
    if args.unnamed:
        args.out = args.out or DEFAULT_UNNAMED_OUT
        if args.override == DEFAULT_OVERRIDE:
            args.override = ""  # the PC lawn overrides never apply to Unnamed art
        return main_unnamed(args)
    args.out = args.out or DEFAULT_OUT

    jobs, skipped = plan(args.pc, args.src, args.out, args.scale, args.big_scale, args.big_pixels, args.only,
                         args.override)
    used = sorted(j["key"] for j in jobs if j.get("override"))
    if used:
        print(f"override from {args.override}: {', '.join(used)}")
    os.makedirs(args.out, exist_ok=True)
    ok, total = 0, 0
    with ProcessPoolExecutor(max_workers=args.jobs) as ex:
        for status, key, msg, rel, size in ex.map(build_one, jobs, chunksize=4):
            if status == "ok":
                ok += 1
                total += size
            else:
                skipped.append((key, msg))
    # Which upscaler made what (2026-11-23: zombies keep a rougher stroke, lawns from SeedVR2 when present).
    info = {"scale": args.scale, "count": ok, "source": "realesrgan-x4plus-anime",
            "plants": args.plants_model, "zombies": args.zombies_model,
            "lawns": "seedvr2" if used else args.plants_model}
    if args.big_scale:
        info["bigScale"] = args.big_scale
        info["bigPixels"] = args.big_pixels
    with open(os.path.join(args.out, "hd.json"), "w") as f:
        json.dump(info, f)
        f.write("\n")
    for key, why in skipped:
        print(f"skip {key}: {why}")
    print(f"HD pack {args.out}: {ok} images, {total / 1e6:.1f} MB, {len(skipped)} skipped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
