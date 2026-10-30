package org.ngb.port;

import android.content.Context;
import android.os.Build;
import org.json.JSONObject;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/** AdrenoTools ZIP packages live in private, executable app storage. */
final class DriverStore {
    static final String SYSTEM = "system";
    static final class Driver {
        final String id, name, library;
        final File directory;
        Driver(String id, String name, String library, File directory) {
            this.id = id; this.name = name; this.library = library; this.directory = directory;
        }
    }
    private static native String configureNative(String hooks, String directory, String library, String temporary);
    static File root(Context context) { return new File(context.getFilesDir(), "drivers"); }
    static String selected(Context context) {
        return context.getSharedPreferences("graphics", 0).getString("driver", SYSTEM);
    }
    static void select(Context context, String id) throws IOException {
        if (!context.getSharedPreferences("graphics", 0).edit().putString("driver", id).commit())
            throw new IOException("无法保存驱动选择");
    }
    static Driver read(File directory) throws Exception {
        File meta = new File(directory, "meta.json");
        if (!meta.isFile() || meta.length() > 65536) throw new IOException("驱动包缺少有效的 meta.json");
        JSONObject json = new JSONObject(new String(java.nio.file.Files.readAllBytes(meta.toPath()), StandardCharsets.UTF_8));
        if (json.optInt("minApi", 0) > Build.VERSION.SDK_INT)
            throw new IOException("此驱动需要更新的 Android 版本");
        String library = json.getString("libraryName");
        if (!library.matches("[A-Za-z0-9_.+-]+\\.so") || library.contains(".."))
            throw new IOException("驱动库名称无效");
        File file = new File(directory, library);
        byte[] header = new byte[20];
        try (DataInputStream input = new DataInputStream(new FileInputStream(file))) { input.readFully(header); }
        if (header[0] != 127 || header[1] != 'E' || header[2] != 'L' || header[3] != 'F'
                || header[4] != 2 || header[5] != 1 || header[16] != 3 || header[17] != 0
                || (header[18] & 255) != 183 || header[19] != 0)
            throw new IOException("驱动库必须是 Android ARM64 共享库");
        String name = json.optString("name", directory.getName());
        return new Driver(directory.getName(), name, library, directory);
    }
    static List<Driver> list(Context context) {
        List<Driver> result = new ArrayList<>();
        File[] directories = root(context).listFiles();
        if (directories != null) for (File directory : directories) {
            if (!directory.isDirectory() || directory.getName().startsWith(".partial-")) continue;
            try { result.add(read(directory)); } catch (Exception ignored) { }
        }
        result.sort(Comparator.comparing(driver -> driver.name));
        return result;
    }
    static Driver current(Context context) throws IOException {
        String id = selected(context);
        if (SYSTEM.equals(id)) return null;
        for (Driver driver : list(context)) if (driver.id.equals(id)) return driver;
        throw new IOException("所选驱动不可用，请重新选择或导入");
    }
    static String label(Context context) {
        try { Driver driver = current(context); return driver == null ? "系统驱动" : driver.name; }
        catch (IOException error) { return "所选驱动不可用"; }
    }
    static void configure(Context context) throws Exception {
        Driver driver = current(context);
        System.loadLibrary("ngb_vulkan");
        String error = configureNative(context.getApplicationInfo().nativeLibraryDir,
                driver == null ? "" : driver.directory.getAbsolutePath() + "/",
                driver == null ? "" : driver.library, context.getCacheDir().getAbsolutePath());
        if (error != null) throw new IOException(error);
    }
    static Driver importZip(Context context, InputStream stream) throws Exception {
        File base = root(context);
        if (!base.isDirectory() && !base.mkdirs()) throw new IOException("无法创建驱动目录");
        String id = UUID.randomUUID().toString();
        File partial = new File(base, ".partial-" + id);
        if (!partial.mkdir()) throw new IOException("无法创建驱动导入目录");
        boolean committed = false;
        try {
            String prefix = partial.getCanonicalPath() + File.separator;
            Set<String> names = new HashSet<>();
            long total = 0;
            int files = 0;
            try (ZipInputStream zip = new ZipInputStream(new BufferedInputStream(stream))) {
                ZipEntry entry;
                byte[] buffer = new byte[65536];
                while ((entry = zip.getNextEntry()) != null) {
                    if (++files > 128) throw new IOException("驱动包包含过多文件");
                    String name = entry.getName();
                    File output = new File(partial, name);
                    if (name.contains("\\") || name.startsWith("/") || !output.getCanonicalPath().startsWith(prefix)
                            || !names.add(output.getCanonicalPath())) throw new IOException("驱动包文件路径无效");
                    if (entry.isDirectory()) {
                        if (!output.isDirectory() && !output.mkdirs()) throw new IOException("无法创建驱动子目录");
                    } else {
                        File parent = output.getParentFile();
                        if (!parent.isDirectory() && !parent.mkdirs()) throw new IOException("无法创建驱动子目录");
                        try (OutputStream target = new FileOutputStream(output)) {
                            int count;
                            while ((count = zip.read(buffer)) != -1) {
                                total += count;
                                if (total > 256L * 1024 * 1024) throw new IOException("驱动包解压大小超过 256 MB");
                                target.write(buffer, 0, count);
                            }
                        }
                    }
                    zip.closeEntry();
                }
            }
            read(partial);
            File installed = new File(base, id);
            if (!partial.renameTo(installed)) throw new IOException("无法完成驱动导入");
            committed = true;
            Driver driver = read(installed);
            select(context, driver.id);
            return driver;
        } finally {
            if (!committed) removePartial(partial);
        }
    }
    private static void removePartial(File directory) {
        File[] files = directory.listFiles();
        if (files != null) for (File file : files) {
            if (file.isDirectory()) removePartial(file); else file.delete();
        }
        directory.delete();
    }
}
