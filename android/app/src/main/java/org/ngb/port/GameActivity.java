package org.ngb.port;

import java.io.File;
import org.libsdl.app.SDLActivity;

public final class GameActivity extends SDLActivity {
    private VirtualPad pad;
    private TouchMenu touchMenu;
    private static native void surfaceAvailable(boolean available);
    @Override protected org.libsdl.app.SDLSurface createSDLSurface(android.content.Context context) {
        return new org.libsdl.app.SDLSurface(context) {
            @Override public void surfaceDestroyed(android.view.SurfaceHolder holder) {
                // Drain presentation before SDL releases the native window.
                surfaceAvailable(false);
                super.surfaceDestroyed(holder);
            }
            @Override public void surfaceChanged(android.view.SurfaceHolder holder, int format, int w, int h) {
                super.surfaceChanged(holder, format, w, h);
                surfaceAvailable(true);
            }
        };
    }
    @Override protected void onCreate(android.os.Bundle state) {
        super.onCreate(state);
        if (mLayout != null) {
            pad = new VirtualPad(this);
            mLayout.addView(pad, new android.widget.RelativeLayout.LayoutParams(
                    android.view.ViewGroup.LayoutParams.MATCH_PARENT, android.view.ViewGroup.LayoutParams.MATCH_PARENT));
            // One-touch access to the touch settings drawer: during gameplay
            // Start would open the pause menu and Back+Start is the game's
            // soft reset, so the touch overlay gets its own button.
            android.widget.Button menu = new android.widget.Button(this);
            menu.setText("☰");
            menu.setTextSize(16f);
            menu.setAllCaps(false);
            menu.setAlpha(0.5f);
            menu.setOnClickListener(v -> {
                if (touchMenu == null) touchMenu = new TouchMenu(this);
                if (touchMenu.getParent() == null)
                    mLayout.addView(touchMenu, new android.widget.RelativeLayout.LayoutParams(
                            android.view.ViewGroup.LayoutParams.MATCH_PARENT,
                            android.view.ViewGroup.LayoutParams.MATCH_PARENT));
                touchMenu.toggle();
            });
            android.widget.RelativeLayout.LayoutParams menuLayout =
                    new android.widget.RelativeLayout.LayoutParams(
                            android.view.ViewGroup.LayoutParams.WRAP_CONTENT,
                            android.view.ViewGroup.LayoutParams.WRAP_CONTENT);
            menuLayout.addRule(android.widget.RelativeLayout.ALIGN_PARENT_TOP);
            menuLayout.addRule(android.widget.RelativeLayout.ALIGN_PARENT_END);
            menuLayout.setMargins(0, 12, 12, 0);
            mLayout.addView(menu, menuLayout);
        }
    }
    @Override protected void onPause() {
        if (pad != null) pad.release();
        super.onPause();
    }
    @Override public void setOrientationBis(int width, int height, boolean resizable, String hint) {
        setRequestedOrientation(android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
    }
    private static native boolean configurePaths(String game, String userdata,
            String config, String cache, boolean silent);

    @Override protected String[] getArguments() {
        try { DriverStore.configure(this); }
        catch (Exception error) { throw new IllegalStateException("无法加载图形驱动", error); }
        File base = getExternalFilesDir(null);
        if (base == null) throw new IllegalStateException("游戏存储不可用");
        File game = new File(base, "game");
        boolean silentTest = getIntent().getBooleanExtra("silent_test", false);
        File userdata = silentTest ? new File(base, "test-frame-timing") : base;
        File saves = new File(userdata, "saves");
        File config = new File(userdata, "config");
        File cache = new File(getCacheDir(), "shaders");
        for (File directory : new File[] {saves, config, cache}) {
            if (!directory.isDirectory() && !directory.mkdirs())
                throw new IllegalStateException("无法创建目录：" + directory);
        }
        if (!configurePaths(game.getAbsolutePath(), saves.getAbsolutePath(),
                config.getAbsolutePath(), cache.getAbsolutePath(),
                silentTest))
            throw new IllegalStateException("无法初始化游戏路径");
        return new String[] {game.getAbsolutePath()};
    }

    /** Called by TouchMenu after DriverStore.select: relaunch this activity
     * in a fresh process (onDestroy kills this one) with the new driver. */
    void restartForDriver() {
        android.app.PendingIntent relaunch = android.app.PendingIntent.getActivity(this, 1001,
                getIntent(), android.app.PendingIntent.FLAG_IMMUTABLE);
        android.app.AlarmManager alarms =
                (android.app.AlarmManager) getSystemService(ALARM_SERVICE);
        if (alarms != null)
            alarms.set(android.app.AlarmManager.ELAPSED_REALTIME,
                    android.os.SystemClock.elapsedRealtime() + 600, relaunch);
        finishAffinity();
    }

    @Override protected void onDestroy() {
        super.onDestroy();
        // This Activity owns a separate process so a driver change starts with
        // fresh SDL, Vulkan and guest state instead of reusing native globals.
        android.os.Process.killProcess(android.os.Process.myPid());
    }
}
