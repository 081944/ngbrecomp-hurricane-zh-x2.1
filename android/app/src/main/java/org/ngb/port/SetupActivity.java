package org.ngb.port;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.provider.DocumentsContract;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.ScrollView;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * The root activity. With the game imported this is a launcher only: the
 * import UI never shows (the game relaunches right away after exiting),
 * except as a last resort after repeated failed starts, so a broken
 * driver can still be swapped from here. Without a game it shows the
 * first-time import screen.
 */
public final class SetupActivity extends Activity {
    private static final int PICK_GAME = 1;
    private static final int PICK_DRIVER = 2;
    private static final int GAME = 3;
    private static final int PICK_ISO = 4;
    /** After the game exits, stay here this long before auto-relaunching. */
    private static final long RELAUNCH_GRACE_MS = 5000;
    /** A start that ends this quickly counts as a failed launch. */
    private static final long FAILED_LAUNCH_MS = 15000;
    private TextView status;
    private Button select, selectIso, importDriver, launch;
    private boolean importing, launching, uiReady, importingIso;
    private int failCount;
    private long gameExitAt, gameStartAt;
    private File game;
    private File marker;

    /* Extraction of a ~7 GB image runs on the import thread inside the JNI
     * call; the C side publishes byte counters the UI thread polls here. */
    private final android.os.Handler ui = new android.os.Handler();
    private final Runnable progressTick = new Runnable() {
        @Override public void run() {
            if (!importingIso) return;
            long[] p = nativeInstallProgress();
            if (p[1] > 0)
                status.setText(String.format("正在解压 ISO…… %.2f GB / %.2f GB",
                        p[0] / 1e9, p[1] / 1e9));
            ui.postDelayed(this, 250);
        }
    };

    private static native int nativeImportGame(int fd, String dest);
    private static native long[] nativeInstallProgress();
    private static native String nativeInstallError();

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        File base = getExternalFilesDir(null);
        if (base == null) { finish(); return; }
        game = new File(base, "game");
        marker = new File(base, "game-import-complete");
        buildUi();
    }

    @Override protected void onResume() {
        super.onResume();
        maybeStartGame();
    }

    private boolean gameReady() {
        return new File(game, "default.xbe").isFile() && marker.isFile();
    }

    /** The window always has content: a blank root is what reads as a black screen. */
    private void buildUi() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        int padding = (int)(24 * getResources().getDisplayMetrics().density);
        layout.setPadding(padding, padding, padding, padding);
        TextView title = new TextView(this);
        title.setText("Ninja Gaiden Black");
        title.setTextSize(28);
        layout.addView(title);
        status = new TextView(this);
        status.setTextSize(17);
        layout.addView(status);
        select = new Button(this);
        select.setText("导入游戏文件夹");
        select.setOnClickListener(v -> startActivityForResult(
                new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE), PICK_GAME));
        layout.addView(select);
        selectIso = new Button(this);
        selectIso.setText("导入游戏 ISO");
        selectIso.setOnClickListener(v -> startActivityForResult(
                new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*")
                        .addCategory(Intent.CATEGORY_OPENABLE), PICK_ISO));
        layout.addView(selectIso);
        importDriver = new Button(this);
        importDriver.setText("导入驱动 ZIP");
        importDriver.setOnClickListener(v -> startActivityForResult(
                new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE), PICK_DRIVER));
        layout.addView(importDriver);
        launch = new Button(this);
        launch.setText("启动游戏");
        launch.setOnClickListener(v -> startGame());
        layout.addView(launch);
        ScrollView scroll = new ScrollView(this);
        scroll.addView(layout);
        setContentView(scroll);
        uiReady = true;
        refresh();
    }

    private void maybeStartGame() {
        if (launching || importing || !gameReady()) return;
        if (failCount >= 2) return;   // repeated failures: wait for the button
        if (SystemClock.elapsedRealtime() - gameExitAt < RELAUNCH_GRACE_MS) return;
        startGame();
    }

    private void startGame() {
        gameStartAt = SystemClock.elapsedRealtime();
        launching = true;
        refresh();
        startActivityForResult(gameIntent(), GAME);
    }

    private Intent gameIntent() {
        Intent intent = new Intent(this, GameActivity.class);
        intent.putExtra("silent_test", getIntent().getBooleanExtra("silent_test", false));
        return intent;
    }

    private void refresh() {
        if (!uiReady) return;
        boolean ready = gameReady();
        // The import UI exists only while the game folder is missing or
        // incomplete (or after repeated failed starts); once the folder is
        // there this is a plain launcher with a manual start fallback.
        boolean showImport = !ready || failCount >= 2;
        select.setVisibility(showImport ? View.VISIBLE : View.GONE);
        selectIso.setVisibility(showImport ? View.VISIBLE : View.GONE);
        importDriver.setVisibility(showImport ? View.VISIBLE : View.GONE);
        launch.setVisibility(ready ? View.VISIBLE : View.GONE);
        select.setEnabled(!importing);
        selectIso.setEnabled(!importing);
        importDriver.setEnabled(!importing);
        launch.setEnabled(!importing && !launching);
        if (!importing) status.setText(launching ? "正在启动游戏……"
                : !ready ? "选择已提取的美版游戏文件夹，其中应包含 default.xbe。\n需要约 6 GB 可用空间，请使用你自己的游戏副本。\n导入完成后游戏将自动启动。"
                : failCount >= 2 ? "游戏连续启动失败。可尝试重新导入游戏文件夹或导入其他驱动 ZIP，或点“启动游戏”重试。"
                : "游戏文件已就绪。");
    }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request == GAME) {
            // The game exited (back / in-game quit) or could not start.
            launching = false;
            gameExitAt = SystemClock.elapsedRealtime();
            long ranFor = gameExitAt - gameStartAt;
            boolean backgroundKill = getSharedPreferences("graphics", 0)
                    .getBoolean("auto_relaunch", false);
            if (backgroundKill) {
                // The system finished the backgrounded game (OEM cleanup):
                // bring it right back via an alarm, which may start
                // activities although this process is in the background.
                getSharedPreferences("graphics", 0).edit().remove("auto_relaunch").commit();
                status.setText("游戏在后台被系统结束，正在重新启动……");
                android.app.PendingIntent relaunch = android.app.PendingIntent.getActivity(
                        this, 1002, gameIntent(), android.app.PendingIntent.FLAG_IMMUTABLE);
                android.app.AlarmManager alarms =
                        (android.app.AlarmManager) getSystemService(ALARM_SERVICE);
                if (alarms != null)
                    alarms.set(android.app.AlarmManager.ELAPSED_REALTIME,
                            SystemClock.elapsedRealtime() + 1500, relaunch);
                return;
            }
            if (ranFor >= FAILED_LAUNCH_MS) failCount = 0;
            if (ranFor < FAILED_LAUNCH_MS && ++failCount >= 2) {
                // Repeated failed starts: stop auto-relaunching (that would
                // just loop) and show the import screen again so the game
                // folder or driver can be swapped from here.
                buildUi();
                return;
            }
            // The game folder exists: straight back into the game.
            refresh();
            startGame();
            return;
        }
        if (result != RESULT_OK || data == null || data.getData() == null) return;
        if (request == PICK_ISO) {
            importing = true;
            importingIso = true;
            refresh();
            status.setText("正在解压 ISO……");
            ui.postDelayed(progressTick, 250);
            Uri image = data.getData();
            new Thread(() -> {
                String error = null;
                try (android.os.ParcelFileDescriptor pfd =
                         getContentResolver().openFileDescriptor(image, "r")) {
                    if (pfd == null) throw new IOException("无法打开所选文件");
                    if (nativeImportGame(pfd.getFd(), game.getAbsolutePath()) != 0)
                        throw new IOException(nativeInstallError());
                } catch (Exception e) { error = e.getMessage(); }
                final String message = error;
                runOnUiThread(() -> {
                    importingIso = false;
                    importing = false;
                    ui.removeCallbacks(progressTick);
                    if (message != null) {
                        refresh();
                        status.setText("ISO 导入失败：" + message);
                        return;
                    }
                    try { if (!marker.exists()) marker.createNewFile(); }
                    catch (IOException ignored) {}
                    startGame();   // straight into the game after extraction
                });
            }, "ngb-iso-import").start();
            return;
        }
        if (request == PICK_DRIVER) {
            importing = true;
            refresh();
            status.setText("正在导入驱动……");
            new Thread(() -> {
                String message;
                try (InputStream input = getContentResolver().openInputStream(data.getData())) {
                    if (input == null) throw new IOException("无法读取驱动 ZIP");
                    DriverStore.Driver driver = DriverStore.importZip(this, input);
                    message = "已选择驱动：" + driver.name + "。下次启动游戏时生效。";
                } catch (Exception error) { message = "驱动导入失败：" + error.getMessage(); }
                final String text = message;
                runOnUiThread(() -> { importing = false; refresh(); status.setText(text); });
            }, "ngb-driver-import").start();
            return;
        }
        if (request != PICK_GAME) return;
        importing = true;
        refresh();
        status.setText("正在导入，请保持应用打开……");
        Uri tree = data.getData();
        new Thread(() -> {
            String error = null;
            try {
                if (!game.isDirectory() && !game.mkdirs()) throw new IOException("无法创建游戏目录");
                if (marker.exists() && !marker.delete()) throw new IOException("无法更新导入状态");
                copyTree(tree, DocumentsContract.getTreeDocumentId(tree), game);
                if (!new File(game, "default.xbe").isFile()) throw new IOException("所选文件夹中没有 default.xbe");
                if (!marker.createNewFile()) throw new IOException("无法保存导入状态");
            } catch (Exception e) { error = e.getMessage(); }
            final String message = error;
            runOnUiThread(() -> {
                importing = false;
                if (message != null) {
                    refresh();
                    status.setText("导入失败：" + message + "\n请检查空间和文件夹后重试。");
                    return;
                }
                launching = true;      // first import: straight into the game
                refresh();
                startGame();
            });
        }, "ngb-import").start();
    }

    private void copyTree(Uri tree, String documentId, File destination) throws IOException {
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, documentId);
        String[] columns = {DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME, DocumentsContract.Document.COLUMN_MIME_TYPE};
        try (Cursor cursor = getContentResolver().query(children, columns, null, null, null)) {
            if (cursor == null) throw new IOException("无法读取所选文件夹");
            while (cursor.moveToNext()) {
                String id = cursor.getString(0), name = cursor.getString(1), mime = cursor.getString(2);
                if (name == null || name.isEmpty() || name.equals(".") || name.equals("..")
                        || name.contains("/") || name.contains("\\")) throw new IOException("无效文件名");
                File output = new File(destination, name);
                if (!output.getCanonicalPath().startsWith(game.getCanonicalPath() + File.separator))
                    throw new IOException("无效文件路径");
                if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mime)) {
                    if (!output.isDirectory() && !output.mkdirs()) throw new IOException("无法创建子文件夹");
                    copyTree(tree, id, output);
                } else {
                    Uri item = DocumentsContract.buildDocumentUriUsingTree(tree, id);
                    try (InputStream input = getContentResolver().openInputStream(item);
                         FileOutputStream stream = new FileOutputStream(output)) {
                        if (input == null) throw new IOException("无法读取 " + name);
                        byte[] buffer = new byte[1024 * 1024];
                        int count;
                        while ((count = input.read(buffer)) != -1) stream.write(buffer, 0, count);
                    }
                }
            }
        }
    }
}
