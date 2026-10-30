package io.github.gamboozino.ngbrecomp;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;

import org.libsdl.app.SDLActivity;

/* SDL's activity, loading the game (libmain.so, src/android_main.c) after SDL itself; and the storage access the
 * first start needs to read the player's disc image (src/android_install.c calls these). */
public class NgbActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }

    /* Whether files anywhere on the shared storage can be read: Android 11 and later's "All files access", the
     * storage permission before */
    public boolean haveStorageAccess() {
        if (Build.VERSION.SDK_INT >= 30)
            return Environment.isExternalStorageManager();
        return checkSelfPermission(android.Manifest.permission.READ_EXTERNAL_STORAGE) ==
               PackageManager.PERMISSION_GRANTED;
    }

    /* The system's page to grant it (Android 11 and later), or its request (before) */
    public void askStorageAccess() {
        runOnUiThread(() -> {
            if (Build.VERSION.SDK_INT >= 30) {
                try {
                    startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                                             Uri.parse("package:" + getPackageName())));
                } catch (Exception e) {
                    startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
                }
            } else {
                requestPermissions(new String[] { android.Manifest.permission.READ_EXTERNAL_STORAGE }, 1);
            }
        });
    }
}
