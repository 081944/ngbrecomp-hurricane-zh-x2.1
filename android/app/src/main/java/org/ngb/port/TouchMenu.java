package org.ngb.port;

import android.app.Activity;
import android.app.AlertDialog;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.util.List;

/**
 * The touch settings drawer: native Android widgets over the game, no pad
 * navigation and no frozen frame. Settings ride the same ngb_pc_setting
 * store as the game's own menus (changes apply at once and are saved);
 * the graphics driver switches need a restart, which GameActivity does.
 */
final class TouchMenu extends FrameLayout {
    private static native int nativeGetSetting(int which);
    private static native void nativeSetSetting(int which, int value);

    private static final int MATCH = ViewGroup.LayoutParams.MATCH_PARENT,
            WRAP = ViewGroup.LayoutParams.WRAP_CONTENT;
    private static final int PANEL_BG = 0xF0121418, TEXT = 0xFFE8EAED, DIM = 0xFFCFCFCF,
            SECTION = 0xFF8AB4E8, CHIP_OFF = 0x24FFFFFF, CHIP_ON = 0xFF3A6EA5;

    private final Activity activity;
    private ScrollView content;

    TouchMenu(Activity activity) {
        super(activity);
        this.activity = activity;
        setVisibility(GONE);
        View dim = new View(activity);
        dim.setBackgroundColor(0x88000000);
        dim.setOnClickListener(v -> hide());
        addView(dim, new LayoutParams(MATCH, MATCH));
        LinearLayout panel = new LinearLayout(activity);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setBackgroundColor(PANEL_BG);
        LayoutParams panelLayout = new LayoutParams(dp(310), MATCH, Gravity.END);
        addView(panel, panelLayout);
        LinearLayout header = new LinearLayout(activity);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);
        header.setPadding(dp(16), dp(12), dp(8), dp(12));
        TextView title = text("设置", 18, true, TEXT);
        header.addView(title, new LinearLayout.LayoutParams(0, WRAP, 1));
        Button close = chip("✕", false, this::hide);
        header.addView(close);
        panel.addView(header, new LinearLayout.LayoutParams(MATCH, WRAP));
        content = new ScrollView(activity);
        content.setVerticalScrollBarEnabled(true);
        panel.addView(content, new LinearLayout.LayoutParams(MATCH, 0, 1));
    }

    void toggle() {
        if (getVisibility() == VISIBLE) hide(); else show();
    }

    private void hide() {
        setVisibility(GONE);
    }

    private void show() {
        rebuild();
        setVisibility(VISIBLE);
    }

    /** The whole page is rebuilt on every open: settings and drivers both. */
    private void rebuild() {
        LinearLayout page = new LinearLayout(activity);
        page.setOrientation(LinearLayout.VERTICAL);
        page.setPadding(dp(16), 0, dp(16), dp(24));

        /* ---- 显示 ---- */
        page.addView(section("显示"));
        page.addView(sliderRow("渲染分辨率", 0, 1, 8, v -> v + "×"));
        page.addView(chipsRow("抗锯齿 MSAA", 10, new int[]{1, 2, 4, 8}, new String[]{"关", "2×", "4×", "8×"}));
        page.addView(chipsRow("宽高比", 7, new int[]{1, 0}, new String[]{"16:9", "4:3"}));
        page.addView(toggleRow("垂直同步", 4));
        page.addView(toggleRow("FPS 显示", 5));

        /* ---- 画质 ---- */
        page.addView(section("画质"));
        page.addView(chipsRow("纹理过滤", 1, new int[]{0, 2, 4, 8, 16}, new String[]{"关", "2×", "4×", "8×", "16×"}));
        page.addView(chipsRow("阴影分辨率", 2, new int[]{1, 2, 4, 8}, new String[]{"低", "中", "高", "极高"}));
        page.addView(toggleRow("平滑辉光", 18));
        page.addView(toggleRow("低负载自动降细节", 29));
        page.addView(toggleRow("显示 HUD", 24));

        /* ---- 音频 ---- */
        page.addView(section("音频"));
        page.addView(chipsRow("位置音频", 23, new int[]{0, 1, 2}, new String[]{"关", "HRTF", "声像"}));
        page.addView(chipsRow("输出声道", 25, new int[]{0, 1, 2}, new String[]{"自动", "立体声", "5.1"}));

        /* ---- 图形驱动 ---- */
        page.addView(section("图形驱动"));
        page.addView(text("当前：" + DriverStore.label(activity), 13, false, DIM));
        page.addView(spacer(6));
        List<DriverStore.Driver> drivers = DriverStore.list(activity);
        page.addView(driverRow("系统驱动", DriverStore.SYSTEM, DriverStore.SYSTEM.equals(DriverStore.selected(activity))));
        for (DriverStore.Driver driver : drivers)
            page.addView(driverRow(driver.name, driver.id, driver.id.equals(DriverStore.selected(activity))));
        page.addView(spacer(6));
        page.addView(driverImportRow());
        page.addView(spacer(10));
        page.addView(text("驱动选择后需要重启应用生效。", 12, false, DIM));

        content.removeAllViews();
        content.addView(page);
    }

    /* ---- drivers ---- */

    /** Opens the system document picker; GameActivity takes the result. */
    private View driverImportRow() {
        LinearLayout row = new LinearLayout(activity);
        row.setGravity(Gravity.CENTER);
        row.setPadding(dp(10), dp(10), dp(10), dp(10));
        GradientDrawable background = new GradientDrawable();
        background.setCornerRadius(dp(10));
        background.setColor(CHIP_OFF);
        row.setBackground(background);
        row.addView(text("＋ 导入驱动 ZIP…", 14, false, TEXT));
        row.setOnClickListener(v -> {
            hide();
            ((GameActivity) activity).importDriverZip();
        });
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(MATCH, WRAP);
        params.topMargin = dp(4);
        row.setLayoutParams(params);
        return row;
    }

    private View driverRow(String name, String id, boolean selected) {
        LinearLayout row = new LinearLayout(activity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(dp(10), dp(10), dp(10), dp(10));
        GradientDrawable background = new GradientDrawable();
        background.setCornerRadius(dp(10));
        background.setColor(selected ? CHIP_ON : CHIP_OFF);
        row.setBackground(background);
        TextView label = text(name, 14, selected, selected ? TEXT : DIM);
        row.addView(label, new LinearLayout.LayoutParams(0, WRAP, 1));
        if (selected) row.addView(text("✓", 14, true, TEXT));
        row.setOnClickListener(v -> {
            if (selected) return;
            new AlertDialog.Builder(activity)
                    .setMessage("切换到「" + name + "」需要重启应用，现在重启吗？")
                    .setPositiveButton("重启", (d, w) -> {
                        try {
                            DriverStore.select(activity, id);
                            ((GameActivity) activity).restartForDriver();
                        } catch (Exception error) {
                            Toast.makeText(activity, "切换失败：" + error.getMessage(), Toast.LENGTH_LONG).show();
                        }
                    })
                    .setNegativeButton("取消", null)
                    .show();
        });
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(MATCH, WRAP);
        params.topMargin = dp(4);
        row.setLayoutParams(params);
        return row;
    }

    /* ---- setting rows ---- */

    private View toggleRow(String name, int which) {
        LinearLayout row = rowBase();
        row.addView(text(name, 15, false, TEXT), new LinearLayout.LayoutParams(0, WRAP, 1));
        Switch toggle = new Switch(activity);
        toggle.setChecked(nativeGetSetting(which) != 0);
        toggle.setOnCheckedChangeListener((b, on) -> nativeSetSetting(which, on ? 1 : 0));
        row.addView(toggle);
        return row;
    }

    private View chipsRow(String name, int which, int[] values, String[] names) {
        LinearLayout column = columnBase();
        column.addView(text(name, 13, false, DIM));
        LinearLayout row = new LinearLayout(activity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        int current = nativeGetSetting(which);
        for (int i = 0; i < values.length; i++) {
            final int value = values[i];
            Button chip = chip(names[i], value == current, () -> {
                nativeSetSetting(which, value);
                restyleChips(row, value);
            });
            row.addView(chip);
        }
        column.addView(row);
        return column;
    }

    private void restyleChips(LinearLayout row, int selected) {
        for (int i = 0; i < row.getChildCount(); i++) {
            Button chip = (Button) row.getChildAt(i);
            boolean on = chip.getTag() != null && ((int) chip.getTag()) == selected;
            style(chip, on);
        }
    }

    private View sliderRow(String name, int which, int min, int max, java.util.function.IntFunction<String> format) {
        LinearLayout column = columnBase();
        LinearLayout head = new LinearLayout(activity);
        head.setOrientation(LinearLayout.HORIZONTAL);
        head.setGravity(Gravity.CENTER_VERTICAL);
        TextView label = text(name, 13, false, DIM);
        head.addView(label, new LinearLayout.LayoutParams(0, WRAP, 1));
        TextView value = text(format.apply(nativeGetSetting(which)), 13, true, TEXT);
        head.addView(value);
        column.addView(head);
        SeekBar slider = new SeekBar(activity);
        slider.setMax(max - min);
        slider.setProgress(Math.min(Math.max(nativeGetSetting(which), min), max) - min);
        slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar b, int progress, boolean fromUser) {
                value.setText(format.apply(progress + min));
            }
            @Override public void onStartTrackingTouch(SeekBar b) { }
            @Override public void onStopTrackingTouch(SeekBar b) {
                nativeSetSetting(which, b.getProgress() + min);
            }
        });
        column.addView(slider);
        return column;
    }

    /* ---- widget helpers ---- */

    private Button chip(String name, boolean selected, Runnable onClick) {
        Button chip = new Button(activity, null, 0);
        chip.setText(name);
        chip.setAllCaps(false);
        chip.setTextSize(13);
        chip.setTag(selected ? 1 : 0);
        chip.setPadding(dp(14), dp(6), dp(14), dp(6));
        style(chip, selected);
        chip.setOnClickListener(v -> onClick.run());
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(WRAP, WRAP);
        params.rightMargin = dp(6);
        chip.setLayoutParams(params);
        return chip;
    }

    private void style(Button chip, boolean selected) {
        GradientDrawable background = new GradientDrawable();
        background.setCornerRadius(dp(18));
        background.setColor(selected ? CHIP_ON : CHIP_OFF);
        chip.setBackground(background);
        chip.setTextColor(selected ? TEXT : DIM);
        chip.setTypeface(null, selected ? Typeface.BOLD : Typeface.NORMAL);
        chip.setTag(selected ? 1 : 0);
    }

    private LinearLayout rowBase() {
        LinearLayout row = new LinearLayout(activity);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(0, dp(10), 0, dp(10));
        return row;
    }

    private LinearLayout columnBase() {
        LinearLayout column = new LinearLayout(activity);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setPadding(0, dp(8), 0, dp(8));
        return column;
    }

    private TextView section(String name) {
        TextView view = text(name, 13, true, SECTION);
        view.setLetterSpacing(0.15f);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(MATCH, WRAP);
        params.topMargin = dp(18);
        params.bottomMargin = dp(2);
        view.setLayoutParams(params);
        return view;
    }

    private TextView text(String content, float sizeSp, boolean bold, int color) {
        TextView view = new TextView(activity);
        view.setText(content);
        view.setTextSize(sizeSp);
        view.setTypeface(null, bold ? Typeface.BOLD : Typeface.NORMAL);
        view.setTextColor(color);
        return view;
    }

    private View spacer(int heightDp) {
        View view = new View(activity);
        view.setLayoutParams(new LinearLayout.LayoutParams(1, dp(heightDp)));
        return view;
    }

    private int dp(float value) {
        return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
    }
}
