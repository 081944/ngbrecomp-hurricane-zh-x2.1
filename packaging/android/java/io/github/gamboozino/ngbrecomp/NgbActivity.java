package io.github.gamboozino.ngbrecomp;

import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.provider.DocumentsContract;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import org.json.JSONObject;
import org.libsdl.app.SDLActivity;

/* SDL's activity, loading the game (libmain.so, src/android_main.c) after SDL itself; and what the first start
 * needs to install the game from the player's disc image (src/android_install.c calls these): the system's file
 * picker, which gives the app the one file chosen (no storage permission), and the SD card. Also the Vulkan
 * driver packages (src/android_gpu.c): the zips of the emulator kind -- meta.json and the driver's .so -- that
 * the player copies into the app's own files folder's drivers/ (Android/data/.../files/drivers), unpacked here
 * into files/drivers/<name>/ for the game to load. */
public class NgbActivity extends SDLActivity {
    private static final int PICK_DISC = 0x4E47;
    private volatile String mPicked;
    private volatile boolean mPickDone;

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }

    /* The system bars, hidden: the status bar otherwise draws over the picture's top (a black band above,
     * the picture looking dropped). Again whenever the window regains focus (the system brings them back
     * on some devices, as at the start). A swipe shows them for a moment. */
    private void hideSystemBars() {
        runOnUiThread(() -> {
            try {
                android.view.View decor = getWindow().getDecorView();
                if (android.os.Build.VERSION.SDK_INT >= 30) {
                    getWindow().setDecorFitsSystemWindows(false);
                    android.view.WindowInsetsController c = decor.getWindowInsetsController();
                    if (c != null) {
                        c.hide(android.view.WindowInsets.Type.statusBars() |
                               android.view.WindowInsets.Type.navigationBars());
                        c.setSystemBarsBehavior(
                            android.view.WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                    }
                } else {
                    decor.setSystemUiVisibility(
                        android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                        android.view.View.SYSTEM_UI_FLAG_FULLSCREEN |
                        android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                        android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                        android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                        android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
                }
            } catch (Exception ignored) {
            }
        });
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            hideSystemBars();
    }

    @Override
    protected void onResume() {
        super.onResume();
        hideSystemBars();
    }

    /* The system's file picker, for the disc image: pickDone() turns true when it closes, pickedPath() is then
     * the file's path, or null */
    public void pickDisc() {
        mPicked = null;
        mPickDone = false;
        runOnUiThread(() -> {
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType("*/*");
            try {
                startActivityForResult(i, PICK_DISC);
            } catch (Exception e) {
                mPickDone = true;
            }
        });
    }

    public boolean pickDone() {
        return mPickDone;
    }

    public String pickedPath() {
        return mPicked;
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        if (request == PICK_DISC) {   /* (off the UI thread: a cloud file is fetched whole when opened) */
            final Uri uri = result == RESULT_OK && data != null ? data.getData() : null;
            new Thread(() -> {
                mPicked = uri != null ? pathOf(uri) : null;
                mPickDone = true;
            }).start();
            return;
        }
        super.onActivityResult(request, result, data);
    }

    /* The driver zips (the emulators' kind: meta.json -- name, vendor, libraryName -- and the driver's .so) that
     * the player copies into the app's own files folder's drivers/ (Android/data/.../files/drivers), unpacked
     * into files/drivers/<zip's name>/ for src/android_gpu.c to list and load. A zip already unpacked (its .src
     * marker agrees) is left as it is. */
    public void syncDrivers() {
        try {
            File store = new File(getFilesDir(), "drivers");
            store.mkdirs();
            File inbox = new File(getExternalFilesDir(null), "drivers");
            inbox.mkdirs();
            File[] zips = inbox.listFiles((d, n) -> n.toLowerCase().endsWith(".zip"));
            if (zips == null)
                return;
            for (File zip : zips) {
                try {
                    installDriverZip(zip, store);
                } catch (Exception e) {   /* (one bad zip does not stop the others) */
                    android.util.Log.i("NGB", "syncDrivers: " + zip.getName() + ": " + e);
                }
            }
        } catch (Exception ignored) {
        }
    }

    private void installDriverZip(File zip, File store) throws Exception {
        File out = new File(store, zip.getName().replaceAll("(?i)\\.zip$", ""));
        String want = zip.getName() + " " + zip.lastModified() + " " + zip.length();
        File marker = new File(out, ".src");
        if (out.isDirectory() && marker.isFile() && readFile(marker).equals(want))
            return;
        File staging = new File(store, ".staging");
        deleteTree(staging);
        staging.mkdirs();
        String libName = null;
        InputStream in = new java.io.FileInputStream(zip);
        ZipInputStream zp = new ZipInputStream(in);
        for (ZipEntry e; (e = zp.getNextEntry()) != null; ) {
            if (e.isDirectory())
                continue;
            String fn = new File(e.getName()).getName();   /* (flat: no zip-slip) */
            if (fn.equals("meta.json"))
                copy(zp, new File(staging, "meta.json"));
            else if (fn.endsWith(".so")) {
                libName = fn;
                copy(zp, new File(staging, fn));
            }
        }
        zp.close();
        File meta = new File(staging, "meta.json");
        if (!meta.isFile() || libName == null) {
            deleteTree(staging);                           /* (not a driver zip: left as it is) */
            android.util.Log.i("NGB", "syncDrivers: " + zip.getName() + ": not a driver zip");
            return;
        }
        JSONObject m = new JSONObject(readFile(meta));
        String named = m.optString("libraryName", "");
        if (!named.isEmpty())
            libName = named;
        if (!new File(staging, libName).isFile()) {
            deleteTree(staging);
            android.util.Log.i("NGB", "syncDrivers: " + zip.getName() + ": no " + libName + " in it");
            return;
        }
        deleteTree(out);
        if (!staging.renameTo(out))
            return;
        OutputStream mk = new FileOutputStream(marker);
        mk.write(want.getBytes("UTF-8"));
        mk.close();
        android.util.Log.i("NGB", "syncDrivers: installed " + out.getName() + " (" + libName + ")");
    }

    private static void copy(InputStream in, File out) throws Exception {
        OutputStream f = new FileOutputStream(out);
        byte[] buf = new byte[65536];
        for (int n; (n = in.read(buf)) > 0; )
            f.write(buf, 0, n);
        f.close();
    }

    private static String readFile(File f) throws Exception {
        InputStream in = new java.io.FileInputStream(f);
        java.io.ByteArrayOutputStream b = new java.io.ByteArrayOutputStream();
        byte[] buf = new byte[65536];
        for (int n; (n = in.read(buf)) > 0; )
            b.write(buf, 0, n);
        in.close();
        return b.toString("UTF-8");
    }

    private static void deleteTree(File f) {
        File[] c = f.listFiles();
        if (c != null)
            for (File x : c)
                deleteTree(x);
        f.delete();
    }

    /* The file by its path where the app can read it there; else (most often, with no storage permission) an open
     * descriptor's, /proc/self/fd/N, which the installer takes for a disc image */
    private String pathOf(Uri uri) {
        try {
            if (DocumentsContract.isDocumentUri(this, uri)) {
                String id = DocumentsContract.getDocumentId(uri);
                if ("com.android.externalstorage.documents".equals(uri.getAuthority()) && id.indexOf(':') > 0) {
                    String volume = id.substring(0, id.indexOf(':')), rel = id.substring(id.indexOf(':') + 1);
                    File f = new File("primary".equalsIgnoreCase(volume)
                                          ? Environment.getExternalStorageDirectory().getPath()
                                          : "/storage/" + volume, rel);
                    if (f.canRead())
                        return f.getPath();
                }
                if (id.startsWith("raw:") && new File(id.substring(4)).canRead())
                    return id.substring(4);
                if (id.startsWith("home:")) {   /* (the Documents folder) */
                    File f = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS),
                                      id.substring(5));
                    if (f.canRead())
                        return f.getPath();
                }
            }
            ParcelFileDescriptor p = getContentResolver().openFileDescriptor(uri, "r");
            return p == null ? null : "/proc/self/fd/" + p.detachFd();
        } catch (Exception e) {
            return null;
        }
    }

    /* The app's own folder on a removable SD card, if one is in (no permission needed there), or null */
    public String sdCardFolder() {
        File[] dirs = getExternalFilesDirs(null);
        for (int i = 1; dirs != null && i < dirs.length; i++)
            if (dirs[i] != null && Environment.isExternalStorageRemovable(dirs[i]) &&
                Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(dirs[i])))
                return dirs[i].getPath();
        return null;
    }

    /* The phone's own motor, standing in for a pad's two (src/android_haptic.c): the game's force feedback as
     * an amplitude of 1..255 held for durationMs, or 0 to stop. Nothing happens on a phone without one. */
    public void vibrate(int amplitude, long durationMs) {
        Vibrator v;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            VibratorManager manager = (VibratorManager) getSystemService(VIBRATOR_MANAGER_SERVICE);
            v = manager != null ? manager.getDefaultVibrator() : null;
        } else {
            v = (Vibrator) getSystemService(VIBRATOR_SERVICE);
        }
        if (v == null || !v.hasVibrator())
            return;
        if (amplitude <= 0 || durationMs <= 0) {
            v.cancel();
            return;
        }
        int amp = Math.max(1, Math.min(255, amplitude));
        if (!v.hasAmplitudeControl())
            amp = VibrationEffect.DEFAULT_AMPLITUDE;
        v.vibrate(VibrationEffect.createOneShot(durationMs, amp));
    }
}
