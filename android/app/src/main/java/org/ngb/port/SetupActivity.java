package org.ngb.port;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.ScrollView;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * A launcher, not a setup screen: once the game folder is imported the app
 * goes straight into the game. The setup UI only appears while there is no
 * game (import the folder, optionally a driver ZIP); the in-game touch menu
 * handles driver switching afterwards.
 */
public final class SetupActivity extends Activity {
    private static final int PICK_GAME = 1;
    private static final int PICK_DRIVER = 2;
    private static final int GAME = 3;
    private TextView status;
    private Button select, importDriver;
    private boolean importing;
    private File game;
    private File marker;

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        File base = getExternalFilesDir(null);
        if (base == null) { finish(); return; }
        game = new File(base, "game");
        marker = new File(base, "game-import-complete");
        if (gameReady()) { startGame(); return; }

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
        importDriver = new Button(this);
        importDriver.setText("导入驱动 ZIP");
        importDriver.setOnClickListener(v -> startActivityForResult(
                new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE), PICK_DRIVER));
        layout.addView(importDriver);
        ScrollView scroll = new ScrollView(this);
        scroll.addView(layout);
        setContentView(scroll);
        refresh();
    }

    private boolean gameReady() {
        return new File(game, "default.xbe").isFile() && marker.isFile();
    }

    private void startGame() {
        Intent intent = new Intent(this, GameActivity.class);
        intent.putExtra("silent_test", getIntent().getBooleanExtra("silent_test", false));
        startActivityForResult(intent, GAME);
    }

    private void refresh() {
        boolean ready = gameReady();
        select.setEnabled(!importing);
        importDriver.setEnabled(!importing);
        if (!importing) status.setText(ready
                ? "游戏文件已就绪。"
                : "选择已提取的美版游戏文件夹，其中应包含 default.xbe。\n需要约 6 GB 可用空间，请使用你自己的游戏副本。\n导入完成后游戏将自动启动。");
    }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request == GAME) {
            // The game owns a process that exits on exit; a cancelled result
            // (back, or a failed start) lands back here, where the driver can
            // be changed before the next attempt.
            if (result != RESULT_OK && !gameReady())
                status.setText("游戏未启动。请检查驱动或重新导入游戏文件夹。");
            return;
        }
        if (result != RESULT_OK || data == null || data.getData() == null) return;
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
                startGame();   // first import: straight into the game
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
