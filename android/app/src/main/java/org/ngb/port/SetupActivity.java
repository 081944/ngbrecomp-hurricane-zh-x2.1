package org.ngb.port;

import android.app.Activity;
import android.app.AlertDialog;
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

/** The prototype imports an extracted disc folder; game assets are not in the APK. */
public final class SetupActivity extends Activity {
    private static final int PICK_GAME = 1;
    private static final int PICK_DRIVER = 2;
    private TextView status;
    private Button play, select, drivers, importDriver;
    private boolean importing;
    private File game;
    private File marker;
    private static native String checkGraphics();

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        File base = getExternalFilesDir(null);
        if (base == null) { finish(); return; }
        game = new File(base, "game");
        marker = new File(base, "game-import-complete");
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
        drivers = new Button(this);
        drivers.setOnClickListener(v -> chooseDriver());
        layout.addView(drivers);
        importDriver = new Button(this);
        importDriver.setText("导入驱动 ZIP");
        importDriver.setOnClickListener(v -> startActivityForResult(
                new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE), PICK_DRIVER));
        layout.addView(importDriver);
        play = new Button(this);
        play.setText("启动游戏");
        play.setOnClickListener(v -> checkAndStart());
        layout.addView(play);
        ScrollView scroll = new ScrollView(this);
        scroll.addView(layout);
        setContentView(scroll);
        refresh();
    }

    private void checkAndStart() {
        play.setEnabled(false);
        select.setEnabled(false);
        drivers.setEnabled(false);
        importDriver.setEnabled(false);
        status.setText("正在检查图形支持……");
        new Thread(() -> {
            String problem;
            try {
                DriverStore.configure(this);
                System.loadLibrary("SDL2");
                System.loadLibrary("main");
                problem = checkGraphics();
            } catch (Throwable error) { problem = error.toString(); }
            final String message = problem;
            runOnUiThread(() -> {
                refresh();
                if (message != null) {
                    status.setText("游戏文件已就绪，但当前图形驱动不满足运行要求。\n"
                            + "需要 Vulkan 1.3，请更新系统驱动或使用支持的设备。\n" + message);
                } else {
                    Intent intent = new Intent(this, GameActivity.class);
                    intent.putExtra("silent_test", getIntent().getBooleanExtra("silent_test", false));
                    startActivity(intent);
                }
            });
        }, "ngb-graphics-check").start();
    }

    private void chooseDriver() {
        java.util.List<DriverStore.Driver> list = DriverStore.list(this);
        String[] labels = new String[list.size() + 1];
        labels[0] = "系统驱动";
        int checked = 0;
        String selected = DriverStore.selected(this);
        for (int i = 0; i < list.size(); i++) {
            labels[i + 1] = list.get(i).name;
            if (list.get(i).id.equals(selected)) checked = i + 1;
        }
        new AlertDialog.Builder(this).setTitle("选择图形驱动")
                .setSingleChoiceItems(labels, checked, (dialog, index) -> {
                    try { DriverStore.select(this, index == 0 ? DriverStore.SYSTEM : list.get(index - 1).id); refresh(); }
                    catch (IOException error) { status.setText(error.getMessage()); }
                    dialog.dismiss();
                }).setNegativeButton("取消", null).show();
    }

    private void refresh() {
        boolean ready = new File(game, "default.xbe").isFile()
                && marker.isFile();
        play.setEnabled(ready && !importing);
        select.setEnabled(!importing);
        drivers.setEnabled(!importing);
        importDriver.setEnabled(!importing);
        drivers.setText("图形驱动：" + DriverStore.label(this));
        if (!importing) status.setText(ready
                ? "游戏文件已就绪。请连接手柄后启动。\n开发版本，兼容性与帧率仍在验证。"
                : "选择已提取的美版游戏文件夹，其中应包含 default.xbe。\n需要约 6 GB 可用空间，请使用你自己的游戏副本。\n当前版本使用手柄操作。");
    }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
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
                refresh();
                if (message != null) status.setText("导入失败：" + message + "\n请检查空间和文件夹后重试。");
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
