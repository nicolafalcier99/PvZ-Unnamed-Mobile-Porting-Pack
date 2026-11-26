package io.github.wszqkzqk.pvzportable;

// PVZU: Unnamed zip import + bundled pack install. Same rules as tools/pvzu_import_unnamed.py (keep in sync):
// only asset folders from the zip's single top folder, never exe/dll/js, no paths escaping the target,
// zip names decoded as GBK unless flagged UTF-8.

import android.content.Context;
import android.content.res.AssetManager;
import android.net.Uri;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.Charset;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

final class UnnamedImport {
    private UnnamedImport() {}

    static final Set<String> ASSET_DIRS = new HashSet<>(Arrays.asList(
        "images", "reanim", "sounds", "compiled", "particles", "fonts", "font", "data", "properties"));
    static final Set<String> TOP_FILE_EXTS = new HashSet<>(Arrays.asList(".mp3", ".ogg"));
    static final Set<String> BANNED_EXTS = new HashSet<>(Arrays.asList(
        ".exe", ".dll", ".js", ".bat", ".cmd", ".vbs", ".ps1", ".lnk", ".scr", ".msi", ".com"));

    static File packDir(File gameDir) { return new File(new File(gameDir, "mods"), "unnamed"); }
    static File assetsDir(File gameDir) { return new File(packDir(gameDir), "assets"); }

    private static String ext(String name) {
        int dot = name.lastIndexOf('.');
        return dot < 0 ? "" : name.substring(dot).toLowerCase(Locale.ROOT);
    }

    /** Relative destination for a zip entry inside the top folder, or null to skip it. */
    static String targetFor(String name, String top) {
        name = name.replace('\\', '/');
        if (top == null || !name.startsWith(top + "/")) return null;
        String rel = name.substring(top.length() + 1);
        StringBuilder out = new StringBuilder();
        String[] parts = rel.split("/");
        int kept = 0;
        String first = null, last = null;
        for (String p : parts) {
            if (p.isEmpty() || p.equals(".")) continue;
            if (p.equals("..")) return null;
            if (kept++ > 0) out.append('/');
            out.append(p);
            if (first == null) first = p;
            last = p;
        }
        if (kept == 0) return null;
        String e = ext(last);
        if (BANNED_EXTS.contains(e)) return null;
        if (kept == 1) return TOP_FILE_EXTS.contains(e) ? out.toString() : null;
        if (!ASSET_DIRS.contains(first.toLowerCase(Locale.ROOT))) return null;
        return out.toString();
    }

    private static String topFolderOf(String name) {
        name = name.replace('\\', '/');
        int slash = name.indexOf('/');
        return slash > 0 ? name.substring(0, slash) : null;
    }

    /** Imports the zip into <gameDir>/mods/unnamed/assets. Returns the number of files copied. */
    static int importZip(Context ctx, Uri uri, File gameDir) throws IOException {
        // Pass 1: find the single top folder (entry names decoded as GBK; UTF-8-flagged names decode the same for ASCII).
        String top = null;
        boolean single = true;
        Charset gbk = Charset.forName("GBK");
        try (InputStream is = ctx.getContentResolver().openInputStream(uri);
             ZipInputStream zis = new ZipInputStream(is, gbk)) {
            ZipEntry e;
            while ((e = zis.getNextEntry()) != null) {
                if (e.isDirectory()) continue;
                String t = topFolderOf(e.getName());
                if (t == null) continue; // loose top-level file (mirror site extras)
                if (top == null) top = t; else if (!top.equals(t)) single = false;
            }
        }
        if (top == null || !single) throw new IOException("not an Unnamed zip (needs one top folder like [1.3beta2])");

        File dest = assetsDir(gameDir);
        deleteRecursive(dest);
        if (!dest.mkdirs() && !dest.isDirectory()) throw new IOException("cannot create " + dest);
        String destCanon = dest.getCanonicalPath() + File.separator;
        int copied = 0;
        byte[] buf = new byte[256 * 1024];
        try (InputStream is = ctx.getContentResolver().openInputStream(uri);
             ZipInputStream zis = new ZipInputStream(is, gbk)) {
            ZipEntry e;
            while ((e = zis.getNextEntry()) != null) {
                if (e.isDirectory()) continue;
                String rel = targetFor(e.getName(), top);
                if (rel == null) continue;
                File out = new File(dest, rel);
                if (!out.getCanonicalPath().startsWith(destCanon)) continue; // zip-slip guard
                File parent = out.getParentFile();
                if (parent != null && !parent.isDirectory()) parent.mkdirs();
                try (OutputStream os = new BufferedOutputStream(new FileOutputStream(out), buf.length)) {
                    int n;
                    while ((n = zis.read(buf)) > 0) os.write(buf, 0, n);
                }
                copied++;
            }
        }
        try (OutputStream os = new FileOutputStream(new File(dest, "import-manifest.json"))) {
            os.write(("{\"top_folder\": \"" + top.replace("\"", "") + "\", \"copied\": " + copied + "}\n").getBytes("UTF-8"));
        }
        return copied;
    }

    /** Copies the pack bundled in the APK (assets/pvzu-pack) to <gameDir>/mods/unnamed, keeping imported assets. */
    static boolean installBundledPack(Context ctx, File gameDir) {
        AssetManager am = ctx.getAssets();
        try {
            String[] top = am.list("pvzu-pack");
            if (top == null || top.length == 0) return false;
            copyAssetTree(am, "pvzu-pack", packDir(gameDir));
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    private static void copyAssetTree(AssetManager am, String path, File dest) throws IOException {
        String[] children = am.list(path);
        if (children != null && children.length > 0) {
            if (!dest.isDirectory()) dest.mkdirs();
            for (String c : children) copyAssetTree(am, path + "/" + c, new File(dest, c));
            return;
        }
        File parent = dest.getParentFile();
        if (parent != null && !parent.isDirectory()) parent.mkdirs();
        try (InputStream in = am.open(path); OutputStream os = new FileOutputStream(dest)) {
            byte[] buf = new byte[64 * 1024];
            int n;
            while ((n = in.read(buf)) > 0) os.write(buf, 0, n);
        }
    }

    private static void deleteRecursive(File f) {
        if (!f.exists()) return;
        File[] kids = f.listFiles();
        if (kids != null) for (File k : kids) deleteRecursive(k);
        f.delete();
    }
}
