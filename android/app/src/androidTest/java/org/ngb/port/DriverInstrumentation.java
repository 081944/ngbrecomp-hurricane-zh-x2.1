package org.ngb.port;

import android.app.Instrumentation;
import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import java.io.*;
import java.lang.reflect.Method;
import java.util.zip.*;

/** Uses the real ZIP importer and native driver without opening another app. */
public final class DriverInstrumentation extends Instrumentation {
    @Override public void onCreate(Bundle arguments) { super.onCreate(arguments); start(); }
    @Override public void onStart() {
        Bundle result = new Bundle();
        try {
            Context context = getTargetContext();
            String before = DriverStore.selected(context);
            reject(context, "../outside", new byte[] {1});
            if (!before.equals(DriverStore.selected(context))) throw new AssertionError("Invalid ZIP changed selection");
            reject(context, "meta.json", "{\"minApi\":999,\"libraryName\":\"driver.so\"}".getBytes("UTF-8"));
            DriverStore.Driver driver;
            try (InputStream input = getContext().getAssets().open("driver.zip")) {
                driver = DriverStore.importZip(context, input);
            }
            if (!driver.id.equals(DriverStore.selected(context))) throw new AssertionError("Selection was not saved");
            DriverStore.configure(context);
            System.loadLibrary("SDL2");
            System.loadLibrary("main");
            Method check = SetupActivity.class.getDeclaredMethod("checkGraphics");
            check.setAccessible(true);
            String problem = (String)check.invoke(null);
            if (problem != null) throw new AssertionError(problem);
            result.putString("driver", driver.name);
            result.putString("result", "ZIP validation, import, persistence and Vulkan 1.3 feature check passed");
            finish(Activity.RESULT_OK, result);
        } catch (Throwable error) {
            result.putString("failure", android.util.Log.getStackTraceString(error));
            finish(Activity.RESULT_CANCELED, result);
        }
    }
    private static void reject(Context context, String name, byte[] content) throws Exception {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        try (ZipOutputStream zip = new ZipOutputStream(bytes)) {
            zip.putNextEntry(new ZipEntry(name)); zip.write(content); zip.closeEntry();
        }
        try {
            DriverStore.importZip(context, new ByteArrayInputStream(bytes.toByteArray()));
            throw new AssertionError("Invalid ZIP accepted: " + name);
        } catch (IOException expected) { }
    }
}
