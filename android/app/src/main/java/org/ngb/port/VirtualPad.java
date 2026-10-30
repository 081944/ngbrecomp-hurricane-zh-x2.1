package org.ngb.port;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.SparseArray;
import android.view.MotionEvent;
import android.view.View;

/** Multi-touch Xbox pad. Inputs merge into port one alongside a physical pad. */
final class VirtualPad extends View {
    private static native void nativeState(int buttons, int lx, int ly, int rx, int ry);
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final SparseArray<Finger> fingers = new SparseArray<>();
    private final float[][] buttons = {
        {.91f,.77f,0}, {.97f,.62f,1}, {.85f,.62f,2}, {.91f,.47f,3},
        {.81f,.22f,4}, {.19f,.22f,5}, {.08f,.22f,6}, {.92f,.22f,7},
        {.31f,.58f,8}, {.31f,.82f,9}, {.26f,.70f,10}, {.36f,.70f,11},
        {.55f,.90f,12}, {.45f,.90f,13}, {.39f,.90f,14}, {.62f,.90f,15}
    };
    private final String[] labels = {"A","B","X","Y","黑","白","LT","RT","↑","↓","←","→","START","BACK","L3","R3"};
    private int held;
    private float lx, ly, rx, ry;
    private static final class Finger { int control; float x, y; Finger(int c) { control = c; } }
    VirtualPad(Context context) { super(context); setContentDescription("虚拟手柄"); }
    private float radius() { return getHeight() * .055f; }
    private int hit(float x, float y) {
        for (float[] button : buttons) {
            float dx = x - button[0] * getWidth(), dy = y - button[1] * getHeight();
            if (dx * dx + dy * dy < radius() * radius() * 1.7f) return (int)button[2];
        }
        if (Math.hypot(x - .13f * getWidth(), y - .72f * getHeight()) < getHeight() * .21f) return 16;
        if (Math.hypot(x - .71f * getWidth(), y - .72f * getHeight()) < getHeight() * .17f) return 17;
        return -1;
    }
    @Override public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked(), index = event.getActionIndex();
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            int control = hit(event.getX(index), event.getY(index));
            if (control >= 0) fingers.put(event.getPointerId(index), new Finger(control));
        }
        for (int i = 0; i < event.getPointerCount(); i++) {
            Finger finger = fingers.get(event.getPointerId(i));
            if (finger != null) { finger.x = event.getX(i); finger.y = event.getY(i); }
        }
        if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP)
            fingers.remove(event.getPointerId(index));
        if (action == MotionEvent.ACTION_CANCEL) fingers.clear();
        update();
        return true;
    }
    private void update() {
        held = 0; lx = ly = rx = ry = 0;
        for (int i = 0; i < fingers.size(); i++) {
            Finger finger = fingers.valueAt(i);
            if (finger.control < 16) held |= 1 << finger.control;
            else {
                boolean left = finger.control == 16;
                float x = (finger.x - (left ? .13f : .71f) * getWidth()) / (getHeight() * .13f);
                float y = (finger.y - .72f * getHeight()) / (getHeight() * .13f);
                float length = Math.max(1, (float)Math.hypot(x, y));
                x /= length; y /= length;
                if (left) { lx = x; ly = -y; } else { rx = x; ry = -y; }
            }
        }
        nativeState(held, (int)(lx * 32767), (int)(ly * 32767), (int)(rx * 32767), (int)(ry * 32767));
        invalidate();
    }
    void release() { fingers.clear(); update(); }
    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize(getHeight() * .025f);
        for (float[] button : buttons) {
            int id = (int)button[2];
            float x = button[0] * getWidth(), y = button[1] * getHeight();
            paint.setColor((held & (1 << id)) != 0 ? 0xA050A8E8 : 0x55303030);
            paint.setStyle(Paint.Style.FILL); canvas.drawCircle(x, y, radius(), paint);
            paint.setColor(0xA0FFFFFF); paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(2);
            canvas.drawCircle(x, y, radius(), paint);
            paint.setStyle(Paint.Style.FILL); paint.setColor(Color.WHITE);
            canvas.drawText(labels[id], x, y - (paint.ascent() + paint.descent()) / 2, paint);
        }
        stick(canvas, .13f, lx, ly); stick(canvas, .71f, rx, ry);
    }
    private void stick(Canvas canvas, float position, float x, float y) {
        float cx = getWidth() * position, cy = getHeight() * .72f, radius = getHeight() * .13f;
        paint.setStyle(Paint.Style.FILL); paint.setColor(0x30303030);
        canvas.drawCircle(cx, cy, radius, paint);
        paint.setColor(0x90FFFFFF); paint.setStyle(Paint.Style.STROKE);
        canvas.drawCircle(cx, cy, radius, paint);
        paint.setStyle(Paint.Style.FILL); paint.setColor(0x70FFFFFF);
        canvas.drawCircle(cx + x * radius * .65f, cy - y * radius * .65f, radius * .32f, paint);
    }
}
