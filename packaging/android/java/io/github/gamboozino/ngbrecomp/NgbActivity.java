package io.github.gamboozino.ngbrecomp;

import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;

import java.io.File;

import org.libsdl.app.SDLActivity;

/* SDL's activity, loading the game (libmain.so, src/android_main.c) after SDL itself; and what the first start
 * needs to install the game from the player's disc image (src/android_install.c calls these): the system's file
 * picker, which gives the app the one file chosen (no storage permission), and the SD card. */
public class NgbActivity extends SDLActivity {
    private static final int PICK_DISC = 0x4E47;
    private volatile String mPicked;
    private volatile boolean mPickDone;

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
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
}
