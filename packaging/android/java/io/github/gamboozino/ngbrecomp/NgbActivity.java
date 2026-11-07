package io.github.gamboozino.ngbrecomp;

import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.provider.DocumentsContract;

import java.io.File;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import android.app.Presentation;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;


import org.libsdl.app.SDLActivity;

/* SDL's activity, loading the game (libmain.so, src/android_main.c) after SDL itself; and what the first start
 * needs to install the game from the player's disc image (src/android_install.c calls these): the system's file
 * picker, which gives the app the one file chosen (no storage permission), and the SD card. */
public class NgbActivity extends SDLActivity {
    private static final int PICK_DISC = 0x4E47;
    private volatile String mPicked;
    private volatile boolean mPickDone;
    private DisplayManager mDisplays;
    private QuickPresentation mQuickPresentation;
    private volatile boolean mQuickConnected;
    private final QuickPanelEvents mQuickEvents = new QuickPanelEvents();
    private final HashMap<String, PanelArt> mQuickArt = new HashMap<>();
    private volatile String mQuickState = "1\t1\t0\n";
    private final DisplayManager.DisplayListener mDisplayListener = new DisplayManager.DisplayListener() {
        public void onDisplayAdded(int id) { refreshQuickDisplay(); }
        public void onDisplayRemoved(int id) { refreshQuickDisplay(); }
        public void onDisplayChanged(int id) { refreshQuickDisplay(); }
    };

    @Override
    protected void onCreate(Bundle saved) {
        super.onCreate(saved);
        mDisplays = (DisplayManager)getSystemService(DISPLAY_SERVICE);
        if (mDisplays != null) mDisplays.registerDisplayListener(mDisplayListener, null);
    }

    private void closeQuickDisplay() {
        mQuickConnected = false;
        if (mQuickPresentation != null) {
            mQuickPresentation.panel.cancelTouch();
            mQuickPresentation.dismiss();
            mQuickPresentation = null;
        }
    }

    private void refreshQuickDisplay() {
        if (mDisplays == null || isFinishing() || !mIsResumedCalled) return;
        int main = getWindowManager().getDefaultDisplay().getDisplayId();
        Display target = null;
        for (Display display : mDisplays.getDisplays(DisplayManager.DISPLAY_CATEGORY_PRESENTATION)) {
            if (display.getDisplayId() != main && display.isValid()) { target = display; break; }
        }
        if (mQuickPresentation != null && target != null &&
            mQuickPresentation.getDisplay().getDisplayId() == target.getDisplayId() &&
            mQuickPresentation.isShowing()) return;
        closeQuickDisplay();
        if (target == null) return;
        try {
            QuickPresentation presentation = new QuickPresentation(this, target);
            presentation.show();
            mQuickPresentation = presentation;
            presentation.panel.setState(QuickPanelState.parse(mQuickState));
            mQuickConnected = true;
            android.util.Log.i("NGB-QUICK", "Inventory panel on display " + target.getDisplayId());
        } catch (WindowManager.InvalidDisplayException e) {
            closeQuickDisplay();
        }
    }

    @Override
    protected void onPause() {
        if (mQuickPresentation != null) mQuickPresentation.panel.cancelTouch();
        super.onPause();
    }

    @Override
    protected void onStop() {
        closeQuickDisplay();
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        closeQuickDisplay();
        if (mDisplays != null) mDisplays.unregisterDisplayListener(mDisplayListener);
        super.onDestroy();
    }

    /* C polls requests on the guest thread; the UI never calls an equip routine or reads game memory. */
    public boolean quickPanelConnected() { return mQuickConnected; }
    public long pollQuickPanelEvent() { return mQuickEvents.poll(); }

    public void setQuickPanelState(String state) {
        mQuickState = state;
        runOnUiThread(() -> {
            if (mQuickPresentation != null && state.equals(mQuickState))
                mQuickPresentation.panel.setState(QuickPanelState.parse(state));
        });
    }

    public void setQuickPanelIcon(String name, int[] pixels, int width, int height) {
        if (width < 1 || height < 1 || width > 1024 || height > 1024 || pixels.length != width * height) return;
        Bitmap bitmap = Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888);
        int left = width, top = height, right = 0, bottom = 0;
        for (int y = 0; y < height; y++) for (int x = 0; x < width; x++) {
            if ((pixels[y * width + x] >>> 24) <= 16) continue;
            left = Math.min(left, x); top = Math.min(top, y);
            right = Math.max(right, x + 1); bottom = Math.max(bottom, y + 1);
        }
        final PanelArt art = new PanelArt(bitmap, new Rect(left, top, right, bottom));
        runOnUiThread(() -> {
            mQuickArt.put(name, art);
            if (mQuickPresentation != null) mQuickPresentation.panel.invalidate();
        });
    }

    private static final class PanelArt {
        final Bitmap bitmap;
        final Rect bounds;
        PanelArt(Bitmap b, Rect r) { bitmap = b; bounds = r; }
    }

    private final class QuickPresentation extends Presentation {
        QuickPanelView panel;
        QuickPresentation(Context context, Display display) { super(context, display); }
        @Override protected void onCreate(Bundle saved) {
            super.onCreate(saved);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
                                 WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL);
            getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
            panel = new QuickPanelView(getContext());
            setContentView(panel);
        }
    }

    private final class QuickPanelView extends View {
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
        private final float density;
        private QuickPanelState state = QuickPanelState.parse("1\t1\t0\n");
        private final QuickPanelUi ui = new QuickPanelUi();
        private QuickPanelLayout main, drawer;
        private QuickPanelLayout.Cell touched;
        private long touchEpoch;
        private int pointer = -1;
        private float startX, startY, reveal;
        private android.animation.ValueAnimator animator;
        private Bitmap drawerSnapshot;
        private final android.graphics.Path revealPath = new android.graphics.Path();
        private final int background = Color.rgb(12, 16, 22), card = Color.rgb(27, 34, 43);
        private final int accent = Color.rgb(221, 191, 127), white = Color.rgb(236, 239, 242);
        private final int muted = Color.rgb(135, 149, 165);
        QuickPanelView(Context context) {
            super(context);
            density = getResources().getDisplayMetrics().density;
            setContentDescription("快捷装备：主武器、投掷武器、道具；右下角展开忍法、腕轮、服装");
        }
        private float dp(float value) { return value * density; }
        void setState(QuickPanelState next) {
            if (next.error != null) android.util.Log.e("NGB-QUICK", "Inventory snapshot rejected: " + next.error);
            if (next.epoch != state.epoch || next.mode != 2) cancelTouch();
            if (next.epoch != state.epoch || next.mode <= 1) {
                ui.reset(); animateDrawer(false);
            }
            if(next.mapId<0) ui.mapOpen=false;
            state = next;
            buildLayout(); invalidate();
        }
        private void buildLayout() {
            if (getWidth() <= 0 || getHeight() <= 0) return;
            main = new QuickPanelLayout(state, getWidth(), getHeight(), density, false);
            drawer = new QuickPanelLayout(state, getWidth(), getHeight(), density, true);
            if (touched != null) {
                QuickPanelLayout next = ui.expanded ? drawer : main;
                QuickPanelLayout.Cell replacement = next.find(touched.category(), touched.id(), touched.action);
                if (replacement == null || !replacement.sameBounds(touched)) cancelTouch();
            }
        }
        @Override protected void onSizeChanged(int w, int h, int oldW, int oldH) {
            cancelTouch();
            if(animator!=null) animator.cancel();
            drawerSnapshot=null; reveal=ui.expanded ? 1f : 0f;
            buildLayout();
        }
        void cancelTouch() {
            if (touched != null && touched.entry != null) mQuickEvents.add(touchEpoch, 3, 0, 0);
            touched = null; pointer = -1; invalidate();
        }
        private void animateDrawer(boolean open) {
            if (animator != null) animator.cancel();
            if (main == null || drawer == null || reveal == (open ? 1f : 0f)) return;
            // Rasterize once: animation frames only composite the page, rather than
            // measuring labels and rescaling every game sprite on each frame.
            drawerSnapshot = Bitmap.createBitmap(getWidth(), getHeight(), Bitmap.Config.ARGB_8888);
            Canvas snapshot = new Canvas(drawerSnapshot);
            snapshot.drawColor(background); drawPage(snapshot, drawer, true);
            animator = android.animation.ValueAnimator.ofFloat(reveal, open ? 1f : 0f);
            animator.setDuration((long)((open ? 320 : 260)*Math.abs((open ? 1f : 0f)-reveal)));
            animator.setInterpolator(new android.view.animation.PathInterpolator(.2f, 0f, 0f, 1f));
            animator.addUpdateListener(a -> { reveal = (Float)a.getAnimatedValue(); postInvalidateOnAnimation(); });
            animator.addListener(new android.animation.AnimatorListenerAdapter() {
                @Override public void onAnimationEnd(android.animation.Animator animation) {
                    drawerSnapshot = null; invalidate();
                }
            });
            animator.start();
        }
        @Override protected void onDetachedFromWindow() {
            cancelTouch(); if (animator != null) animator.cancel(); drawerSnapshot=null; super.onDetachedFromWindow();
        }
        private void text(Canvas canvas, String value, float x, float y, float size, int color) {
            paint.setStyle(Paint.Style.FILL); paint.setColor(color); paint.setTextSize(size);
            paint.setTypeface(android.graphics.Typeface.create("sans-serif-medium", android.graphics.Typeface.NORMAL));
            canvas.drawText(value, x, y, paint);
        }
        private RectF rect(QuickPanelLayout.Cell c) { return new RectF(c.x, c.y, c.x+c.w, c.y+c.h); }
        private void drawPage(Canvas canvas, QuickPanelLayout page, boolean expanded) {
            String[] titles = {"", "主武器", "投掷武器", "忍法", "道具", "服装", "腕轮"};
            for (QuickPanelLayout.Region area : page.regions) {
                paint.setStyle(Paint.Style.FILL); paint.setColor(Color.rgb(18, 24, 32));
                canvas.drawRoundRect(new RectF(area.x, area.y, area.x+area.w, area.y+area.h), dp(13), dp(13), paint);
                text(canvas, titles[area.category], area.x+dp(9), area.y+dp(16), dp(12), white);
            }
            for (QuickPanelLayout.Cell c : page.cells) drawTile(canvas, c, expanded);
        }
        private void drawTile(Canvas canvas, QuickPanelLayout.Cell c, boolean expanded) {
            RectF r = rect(c);
            QuickPanelState.Entry e = c.entry;
            boolean active = touched != null && touched.category() == c.category() && touched.id() == c.id() && touched.action == c.action;
            boolean enabled = e == null ? (c.action!=2 || state.mapId>=0) : state.mode == 2 && e.usable;
            boolean selected = e != null ? e.equipped : c.action==2 && ui.mapOpen;
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(active ? Color.rgb(70, 74, 80) : selected ? Color.rgb(60, 49, 32) : card);
            float radius=dp(e==null ? 16 : 11);
            if(e==null) {
                paint.setColor(Color.rgb(5,8,12));
                canvas.drawRoundRect(new RectF(r.left,r.top+dp(3),r.right,r.bottom+dp(3)),radius,radius,paint);
                paint.setColor(!enabled ? Color.rgb(30,35,42) : active ? Color.rgb(92,78,51) : Color.rgb(64,55,39));
            }
            canvas.drawRoundRect(r, radius, radius, paint);
            paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(dp(selected ? 2.6f : 1.3f));
            paint.setColor(!enabled ? Color.rgb(64,73,85) : selected || e == null ? accent : Color.rgb(102, 119, 138));
            canvas.drawRoundRect(r, radius, radius, paint);
            paint.setStyle(Paint.Style.FILL);
            if(e==null) {
                float cx=r.left+dp(17), cy=r.centerY(), s=dp(14);
                paint.setColor(enabled ? accent : muted);
                if(c.action==2) {
                    paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(dp(1.5f));
                    canvas.drawRect(cx-s*.45f,cy-s*.45f,cx+s*.45f,cy+s*.45f,paint);
                    canvas.drawLine(cx-s*.15f,cy-s*.45f,cx-s*.15f,cy+s*.45f,paint);
                    canvas.drawLine(cx+s*.15f,cy-s*.45f,cx+s*.15f,cy+s*.45f,paint);
                    paint.setStyle(Paint.Style.FILL);
                } else if(expanded) {
                    paint.setStrokeWidth(dp(1.8f)); paint.setStrokeCap(Paint.Cap.ROUND);
                    canvas.drawLine(cx-s*.35f,cy-s*.35f,cx+s*.35f,cy+s*.35f,paint);
                    canvas.drawLine(cx+s*.35f,cy-s*.35f,cx-s*.35f,cy+s*.35f,paint);
                    paint.setStrokeCap(Paint.Cap.BUTT);
                } else {
                    for(int y=0;y<2;y++) for(int x=0;x<2;x++) {
                        float a=cx+(x-.5f)*s*.65f,b=cy+(y-.5f)*s*.65f;
                        canvas.drawRoundRect(new RectF(a-s*.21f,b-s*.21f,a+s*.21f,b+s*.21f),dp(1.5f),dp(1.5f),paint);
                    }
                }
                String label=c.action==2 ? (ui.mapOpen ? "关闭" : "地图") : expanded ? "收起" : "更多装备";
                float labelFont=Math.min(dp(16),(r.width()-dp(39))/label.length());
                text(canvas,label,r.left+dp(32),cy+labelFont*.35f,labelFont,enabled ? white : muted);
                return;
            }
            float font = Math.min(dp(13), Math.max(dp(11), r.height()*.21f));
            float top = r.top+dp(10), bottom = r.bottom-font-dp(12);
            if (e.category == 6) {
                // A compact cuff/knot symbol, drawn at panel resolution rather than upscaled text.
                float s = Math.min(r.width()*.45f, (bottom-top)*.75f), cx=r.centerX(), cy=(top+bottom)/2;
                paint.setColor(enabled ? accent : muted); paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(Math.max(dp(2),s*.09f));
                canvas.drawOval(new RectF(cx-s*.48f,cy-s*.36f,cx+s*.48f,cy+s*.36f),paint);
                canvas.drawArc(new RectF(cx-s*.34f,cy-s*.24f,cx+s*.34f,cy+s*.24f),20,280,false,paint);
                paint.setStyle(Paint.Style.FILL);
                canvas.drawCircle(cx,cy-s*.35f,s*.12f,paint);
            } else {
                PanelArt art = mQuickArt.get(e.art);
                if (art != null && !art.bounds.isEmpty()) {
                    float maxW=r.width()-dp(14), maxH=Math.max(dp(8),bottom-top);
                    float scale=Math.min(maxW/art.bounds.width(),maxH/art.bounds.height());
                    float cx=r.centerX(), cy=(top+bottom)/2;
                    RectF dest=new RectF(cx-art.bounds.exactCenterX()*scale,cy-art.bounds.exactCenterY()*scale,
                        cx+(art.bitmap.getWidth()-art.bounds.exactCenterX())*scale,cy+(art.bitmap.getHeight()-art.bounds.exactCenterY())*scale);
                    int save=canvas.save(); canvas.clipRect(r);
                    paint.setAlpha(enabled ? 255 : 95); canvas.drawBitmap(art.bitmap,null,dest,paint);
                    paint.setAlpha(255); canvas.restoreToCount(save);
                }
            }
            String label = e.name;
            paint.setTextSize(font);
            label=android.text.TextUtils.ellipsize(label,new android.text.TextPaint(paint),r.width()-dp(10),android.text.TextUtils.TruncateAt.END).toString();
            float labelW=paint.measureText(label);
            text(canvas,label,r.centerX()-labelW/2,r.bottom-dp(7),font,enabled ? white : muted);
            if (e != null && (e.quantity || e.level>0)) {
                String badge=e.quantity ? "×"+e.count : "Lv."+e.level;
                text(canvas,badge,r.left+dp(6),r.top+dp(15),dp(11),selected ? accent : white);
            }
            if (selected) {
                paint.setColor(accent); canvas.drawCircle(r.right-dp(9),r.top+dp(9),dp(4),paint);
            }
        }
        @Override protected void onDraw(Canvas canvas) {
            canvas.drawColor(background);
            if (main == null) return;
            if(ui.mapOpen) {
                drawMap(canvas);
                drawTile(canvas,main.mapButton,false); drawTile(canvas,main.toggle,false);
                return;
            }
            if(reveal<1) drawPage(canvas,main,false);
            if (reveal > 0) {
                paint.setStyle(Paint.Style.FILL); paint.setColor(Color.argb((int)(170*reveal),0,0,0));
                canvas.drawRect(0,0,getWidth(),getHeight(),paint);
                QuickPanelLayout.Cell anchor=main.toggle;
                float left=anchor.x*(1-reveal), top=anchor.y*(1-reveal);
                float right=anchor.x+anchor.w+(getWidth()-anchor.x-anchor.w)*reveal;
                float bottom=anchor.y+anchor.h+(getHeight()-anchor.y-anchor.h)*reveal;
                int save=canvas.save();
                revealPath.reset();
                revealPath.addRoundRect(new RectF(left,top,right,bottom),dp(16)*(1-reveal),dp(16)*(1-reveal),android.graphics.Path.Direction.CW);
                canvas.clipPath(revealPath);
                if(drawerSnapshot!=null) {
                    float scale=.94f+.06f*reveal;
                    canvas.scale(scale,scale,anchor.x+anchor.w/2,anchor.y+anchor.h/2);
                    paint.setAlpha((int)(255*Math.min(1,reveal*2)));
                    canvas.drawBitmap(drawerSnapshot,0,0,paint); paint.setAlpha(255);
                } else { canvas.drawColor(background); drawPage(canvas,drawer,true); }
                canvas.restoreToCount(save);
            }
            if (state.mode <= 1 && state.entries.isEmpty()) {
                paint.setColor(Color.argb(225,12,16,22)); canvas.drawRect(0,0,getWidth(),getHeight(),paint);
                String message=state.mode==0 ? "在游戏选项中开启快速菜单" : "进入游戏后显示装备";
                paint.setTextSize(dp(18));
                text(canvas,message,(getWidth()-paint.measureText(message))/2,getHeight()/2,dp(18),white);
            }
        }
        private void drawMap(Canvas canvas) {
            PanelArt art=mQuickArt.get(state.mapArt);
            if(art==null || art.bounds.isEmpty()) {
                String message="正在读取地图…";
                paint.setTextSize(dp(18));
                text(canvas,message,(getWidth()-paint.measureText(message))/2,getHeight()/2,dp(18),white);
                return;
            }
            float availableH=getHeight()-dp(70), margin=dp(12);
            float scale=Math.min((getWidth()-2*margin)/art.bounds.width(),(availableH-2*margin)/art.bounds.height());
            float w=art.bounds.width()*scale,h=art.bounds.height()*scale;
            RectF dest=new RectF((getWidth()-w)/2,(availableH-h)/2,(getWidth()+w)/2,(availableH+h)/2);
            paint.setStyle(Paint.Style.FILL); paint.setColor(white); paint.setAlpha(255);
            canvas.drawBitmap(art.bitmap,art.bounds,dest,paint);
            if(state.mapPosition) {
                float x=dest.left+(state.mapX*art.bitmap.getWidth()-art.bounds.left)*scale;
                float y=dest.top+(state.mapY*art.bitmap.getHeight()-art.bounds.top)*scale;
                if(!dest.contains(x,y)) return;
                paint.setColor(Color.rgb(7,12,18)); canvas.drawCircle(x,y,dp(6),paint);
                paint.setColor(accent); canvas.drawCircle(x,y,dp(4),paint);
                paint.setColor(white); canvas.drawCircle(x,y,dp(1.5f),paint);
            }
        }
        @Override public boolean onTouchEvent(MotionEvent event) {
            int action=event.getActionMasked();
            if (action==MotionEvent.ACTION_DOWN) {
                cancelTouch();
                if (main == null || (animator != null && animator.isRunning())) return false;
                QuickPanelLayout.Cell c=(ui.expanded ? drawer : main).hit(event.getX(),event.getY());
                if(ui.mapOpen && c!=null && c.entry!=null) return false;
                if (c==null || c.entry!=null && (state.mode!=2 || !c.entry.usable)) return false;
                if (c.entry==null && state.mode<=1) return false;
                if(c.action==2 && state.mapId<0) return false;
                touched=c; touchEpoch=state.epoch; pointer=event.getPointerId(0);
                startX=event.getX(); startY=event.getY();
                if(c.entry!=null) mQuickEvents.add(touchEpoch,1,c.category(),c.id());
                invalidate(); return true;
            }
            if(action==MotionEvent.ACTION_CANCEL || action==MotionEvent.ACTION_POINTER_DOWN) { cancelTouch(); return true; }
            int index=event.findPointerIndex(pointer);
            if(index<0) { cancelTouch(); return true; }
            if(action==MotionEvent.ACTION_MOVE && touched!=null &&
                (!rect(touched).contains(event.getX(index),event.getY(index)) ||
                 Math.hypot(event.getX(index)-startX,event.getY(index)-startY)>dp(12))) cancelTouch();
            if(action==MotionEvent.ACTION_UP) {
                QuickPanelLayout.Cell c=touched;
                if(c!=null && rect(c).contains(event.getX(index),event.getY(index))) {
                    touched=null; pointer=-1;
                    if(c.action==2) {
                        ui.toggleMap(state.mapId>=0);
                        if(animator!=null) animator.cancel();
                        reveal=0;
                    }
                    else if(c.entry==null) { ui.toggle(); animateDrawer(ui.expanded); }
                    else {
                        mQuickEvents.add(touchEpoch,2,c.category(),c.id());
                        if(ui.selected(c.category())) animateDrawer(false);
                    }
                    performClick(); invalidate();
                } else cancelTouch();
            }
            return true;
        }
        @Override public boolean performClick() { super.performClick(); return true; }
    }
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }

    /* The system bars, hidden: the status bar otherwise draws over the picture's top (a black band above,
     * the picture looking dropped). Again whenever the window regains focus (the system brings them back
     * on some devices, as at the start). A swipe shows them for a moment. */
    private void hideSystemBars() {
        runOnUiThread(() -> {
            try {
                android.view.View decor = getWindow().getDecorView();
                if (android.os.Build.VERSION.SDK_INT >= 30) {
                    getWindow().setDecorFitsSystemWindows(false);
                    android.view.WindowInsetsController c = decor.getWindowInsetsController();
                    if (c != null) {
                        c.hide(android.view.WindowInsets.Type.statusBars() |
                               android.view.WindowInsets.Type.navigationBars());
                        c.setSystemBarsBehavior(
                            android.view.WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                    }
                } else {
                    decor.setSystemUiVisibility(
                        android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                        android.view.View.SYSTEM_UI_FLAG_FULLSCREEN |
                        android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                        android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                        android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                        android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
                }
            } catch (Exception ignored) {
            }
        });
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            hideSystemBars();
    }

    @Override
    protected void onResume() {
        super.onResume();
        hideSystemBars();
        refreshQuickDisplay();
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

    /* The phone's own motor, standing in for a pad's two (src/android_haptic.c): the game's force feedback as
     * an amplitude of 1..255 held for durationMs, or 0 to stop. Nothing happens on a phone without one. */
    public void vibrate(int amplitude, long durationMs) {
        Vibrator v;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            VibratorManager manager = (VibratorManager) getSystemService(VIBRATOR_MANAGER_SERVICE);
            v = manager != null ? manager.getDefaultVibrator() : null;
        } else {
            v = (Vibrator) getSystemService(VIBRATOR_SERVICE);
        }
        if (v == null || !v.hasVibrator())
            return;
        if (amplitude <= 0 || durationMs <= 0) {
            v.cancel();
            return;
        }
        int amp = Math.max(1, Math.min(255, amplitude));
        if (!v.hasAmplitudeControl())
            amp = VibrationEffect.DEFAULT_AMPLITUDE;
        v.vibrate(VibrationEffect.createOneShot(durationMs, amp));
    }
}

/* Plain data and request encoding are also exercised by the host-side checks. */
final class QuickPanelState {
    String error;
    long epoch;
    int mode;
    boolean freeze;
    int mapId=-1;
    String mapArt="";
    boolean mapPosition;
    float mapX,mapY;
    final List<Entry> entries = new ArrayList<>();
    static final class Entry {
        int category, id, count, level;
        boolean usable, quantity, equipped;
        String art, name;
    }
    List<Entry> category(int category) {
        List<Entry> result = new ArrayList<>();
        for (Entry e : entries) if (e.category == category) result.add(e);
        return result;
    }
    static QuickPanelState parse(String packet) {
        QuickPanelState state = new QuickPanelState();
        try {
            String[] rows = packet.split("\n");
            String[] head = rows[0].split("\t");
            state.epoch = Long.parseLong(head[0]);
            state.mode = Integer.parseInt(head[1]);
            state.freeze = "1".equals(head[2]);
            if(head.length>=8) {
                state.mapId=Integer.parseInt(head[3]); state.mapArt=head[4];
                state.mapPosition="1".equals(head[5]);
                state.mapX=Float.parseFloat(head[6]); state.mapY=Float.parseFloat(head[7]);
                if(state.mapId < -1 || state.mapId>=52 || !Float.isFinite(state.mapX) || !Float.isFinite(state.mapY) ||
                    state.mapPosition && (state.mapX<0 || state.mapX>1 || state.mapY<0 || state.mapY>1)) throw new IllegalArgumentException();
            }
            if (state.epoch < 1 || state.epoch > 0xFFFFFF || state.mode < 0 || state.mode > 3 || rows.length > 769)
                throw new IllegalArgumentException();
            for (int i = 1; i < rows.length; i++) {
                String[] v = rows[i].split("\t", 9);
                Entry e = new Entry();
                e.category = Integer.parseInt(v[0]); e.id = Integer.parseInt(v[1]);
                e.count = Integer.parseInt(v[2]); e.usable = "1".equals(v[3]);
                e.quantity = "1".equals(v[4]); e.level = Integer.parseInt(v[5]);
                e.equipped = "1".equals(v[6]); e.art = v[7]; e.name = label(v[8]);
                if (e.category < 1 || e.category > 6 || e.id < 0 || e.id > 65535 || e.count < 0 || e.count > 255)
                    throw new IllegalArgumentException();
                state.entries.add(e);
            }
        } catch (RuntimeException e) { state.error = e.toString(); state.mode = 1; state.mapId=-1; state.entries.clear(); }
        return state;
    }
    private static String label(String name) {
        switch (name) {
        case "Dragon Sword": return "龙剑";
        case "True Dragon Sword": return "真龙剑";
        case "Nunchaku": return "双节棍";
        case "Lunar": return "无想新月棍";
        case "War Hammer": return "战锤";
        case "Dabilahro": return "大刀";
        case "Vigoorian Flail": return "双拐";
        case "Wooden Sword": return "木刀";
        case "Unlabored Flawlessness": return "天衣无缝";
        case "Kitetsu": return "鬼彻";
        case "Dark Dragon Blade": return "黑龙丸";
        case "Plasma Saber": return "等离子军刀";
        case "Plasma Saber Mk.II": return "等离子军刀Ⅱ";
        case "Shuriken": return "手里剑";
        case "Incendiary Shuriken": return "爆破手里剑";
        case "Windmill Shuriken": return "风车手里剑";
        case "Smoke Bomb": return "烟玉";
        case "Bow": return "弓";
        case "Bow (APFSDS)": return "弓 · 穿甲箭";
        case "Bow (Arrows)": return "弓 · 普通箭";
        case "Bow (Explosive)": return "弓 · 爆破箭";
        case "Strongbow": return "强弓";
        case "Spear Gun": return "水中枪";
        case "The Art of the Fire Wheels": return "鬼火之术";
        case "The Art of the Inferno": return "火炎龙之术";
        case "The Art of the Ice Storm": return "冰刃杀之术";
        case "The Art of the Inazuma": return "百雷锤之术";
        case "Elixir of Spiritual Life": return "灵命药";
        case "Great Spirit Elixir": return "灵命仙药";
        case "Elixir of the Devil Way": return "鬼道药";
        case "Great Devil Elixir": return "鬼道仙药";
        case "Fragrance of Dayflower": return "露草馨香";
        case "Fragrance of Hydrangea": return "紫阳馨香";
        case "Default Costume": return "剧情默认";
        case "Costume 1": return "服装 1";
        case "Costume 2": return "服装 2";
        case "Costume 3": return "服装 3";
        case "Costume 4": return "服装 4";
        case "Costume 5": return "服装 5";
        case "Costume 6": return "服装 6";
        case "Costume 7": return "服装 7";
        case "Costume 8": return "服装 8";
        default: return name;
        }
    }
}

final class QuickPanelEvents {
    private final ArrayDeque<Long> events = new ArrayDeque<>();
    synchronized void add(long epoch, int action, int category, int item) {
        if (events.size() >= 32) { events.clear(); action = 3; category = item = 0; }
        events.add((epoch << 32) | ((long)action << 24) | ((long)category << 16) | item);
    }
    synchronized long poll() { return events.isEmpty() ? 0 : events.removeFirst(); }
}

final class QuickPanelUi {
    boolean expanded,mapOpen;
    void toggle() { mapOpen=false; expanded=!expanded; }
    void toggleMap(boolean available) { if(available) { expanded=false; mapOpen=!mapOpen; } }
    void reset() { expanded=false; mapOpen=false; }
    boolean selected(int category) {
        if(expanded && (category==3 || category==5 || category==6)) { expanded=false; return true; }
        return false;
    }
}

final class QuickPanelLayout {
    static final class Cell {
        final QuickPanelState.Entry entry;
        final float x,y,w,h;
        final int action;
        Cell(QuickPanelState.Entry e,float x,float y,float w,float h,int action) {
            entry=e; this.x=x; this.y=y; this.w=w; this.h=h; this.action=action;
        }
        int category() { return entry==null ? 0 : entry.category; }
        int id() { return entry==null ? 0 : entry.id; }
        boolean contains(float a,float b) { return a>=x && a<x+w && b>=y && b<y+h; }
        boolean sameBounds(Cell c) { return x==c.x && y==c.y && w==c.w && h==c.h; }
    }
    static final class Region {
        final int category;
        final float x,y,w,h;
        Region(int c,float x,float y,float w,float h) { category=c; this.x=x; this.y=y; this.w=w; this.h=h; }
    }
    final List<Cell> cells=new ArrayList<>();
    final List<Region> regions=new ArrayList<>();
    final float height;
    Cell toggle,mapButton;
    private final float density,gap;
    private final QuickPanelState state;
    QuickPanelLayout(QuickPanelState state,int width,int viewport,float density,boolean expanded) {
        this.state=state; this.density=density; gap=6*density; height=viewport;
        float m=6*density, w=Math.max(1,width-2*m), h=Math.max(1,viewport-2*m);
        if(!expanded) {
            float left=(w-gap)*.56f;
            float right=w-gap-left, rightX=m+left+gap;
            float top=(h-gap)*.57f;
            region(1,m,m,left,h,false);
            region(2,rightX,m,right,top,false);
            region(4,rightX,m+top+gap,right,h-top-gap,true);
        } else {
            float top=.26f*(h-gap);
            float lowerY=m+top+gap, lowerH=h-top-gap;
            float left=(w-gap)*.56f;
            region(3,m,m,w,top,false);
            region(6,m,lowerY,left,lowerH,false);
            region(5,m+left+gap,lowerY,w-gap-left,lowerH,true);
        }
        // Equal footer buttons fill the right-hand category's width.
        Region footer=regions.get(regions.size()-1);
        float buttonW=(footer.w-gap)/2;
        toggle=new Cell(null,footer.x+buttonW+gap,viewport-m-52*density,buttonW,52*density,1);
        cells.add(toggle);
        mapButton=new Cell(null,footer.x,toggle.y,buttonW,52*density,2);
        cells.add(mapButton);
    }
    private void region(int category,float x,float y,float w,float h,boolean control) {
        if(control) h-=58*density;
        regions.add(new Region(category,x,y,w,h));
        List<QuickPanelState.Entry> entries=state.category(category);
        int n=entries.size();
        if(n==0) return;
        float inset=5*density, title=20*density;
        x+=inset; y+=title; w-=2*inset; h-=title+inset;
        int rows=1;
        double best=Double.MAX_VALUE;
        for(int r=1;r<=n;r++) {
            int cols=(n+r-1)/r;
            float cw=(w-gap*(cols-1))/cols, ch=(h-gap*(r-1))/r;
            if(cw<=0 || ch<=0) continue;
            double score=Math.pow(Math.log(cw/ch),2)+(r*cols-n)*.045;
            if(Math.min(cw,ch)<44*density) score+=20+Math.pow(44*density-Math.min(cw,ch),2);
            if(score<best) { best=score; rows=r; }
        }
        float cellH=(h-gap*(rows-1))/rows;
        int index=0;
        for(int row=0;row<rows;row++) {
            int count=(n+rows-1-row)/rows;
            float cellW=(w-gap*(count-1))/count;
            for(int col=0;col<count;col++,index++) {
                Cell c=new Cell(entries.get(index),
                    x+col*(cellW+gap),y+row*(cellH+gap),cellW,cellH,0);
                cells.add(c);
            }
        }
    }
    Cell hit(float x,float y) { for(Cell c:cells) if(c.contains(x,y)) return c; return null; }
    Cell find(int category,int id,int action) {
        for(Cell c:cells) if(c.category()==category && c.id()==id && c.action==action) return c;
        return null;
    }
}
