/*
 * Android: the phone vibrates where the game expects a pad to. Force feedback comes in as a USB output report
 * (xboxrecomp/src/usb/usb_gamepad.c, usb_gamepad_rumble) and ends at xbox_InputSetState, which rumbles the SDL
 * controller in the port -- and on a phone there is usually no such controller to rumble: the on-screen pad is
 * not one, and a Bluetooth pad's own motors are often unsupported. So the phone's motor takes it, through
 * NgbActivity.vibrate() and the system Vibrator.
 *
 * The title re-sends the report every frame while a motor is on, so the bursts are not repeated blindly: a
 * change is sent as it arrives, a steady speed is re-armed before the last burst runs out, and a drop to zero
 * cancels at once instead of waiting for the burst to end.
 */
#include <jni.h>
#include <SDL.h>
#include <SDL_system.h>
#include <time.h>

/* void ngb_android_haptic(unsigned left, unsigned right): the game's force feedback, the left (low) motor
 * speed and the right (high) one, 0..65535 each, called from xbox_InputSetState (which declares a weak empty
 * default; this is the strong definition Android builds link) on the thread that runs the game, as often as
 * the title sends the report. */

#define BURST_MS   1000   /* one vibration; the title calls back ~60 times a second while it lasts */
#define REFRESH_MS 600    /* re-arm with this much of the burst left, so a held motor never stutters */

static jclass s_cls;              /* NgbActivity, held for the life of the process */
static jmethodID s_vibrate;
static int s_no_method;           /* looked up and not found: do not look again */
static int s_amp = -1;            /* the amplitude last sent, -1 before the first call */
static long long s_sent_ms;       /* when it was sent */

static long long now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (long long)t.tv_sec * 1000 + (long long)(t.tv_nsec / 1000000);
}

/* The game's two motors on one phone motor: the stronger of them, 0..65535 scaled to 1..255 */
static int amplitude_of(unsigned left, unsigned right)
{
    unsigned speed = left > right ? left : right;
    int amp = (int)((speed * 255u + 32767u) / 65535u);
    return speed && !amp ? 1 : amp;
}

void ngb_android_haptic(unsigned left, unsigned right)
{
    int amp = amplitude_of(left, right);
    long long t = now_ms();

    if (amp == s_amp && (amp == 0 || t - s_sent_ms < REFRESH_MS))
        return;                       /* nothing changed, and the burst has time left */
    if (amp == 0 && s_amp <= 0) {
        s_amp = 0;                    /* already still */
        return;
    }

    JNIEnv *env = SDL_AndroidGetJNIEnv();
    if (!env)
        return;
    jobject act = SDL_AndroidGetActivity();
    if (!act)
        return;
    if (!s_cls && !s_no_method) {
        jclass c = (*env)->GetObjectClass(env, act);
        if (c)
            s_cls = (jclass)(*env)->NewGlobalRef(env, c);
        if (c)
            (*env)->DeleteLocalRef(env, c);
        s_vibrate = s_cls ? (*env)->GetMethodID(env, s_cls, "vibrate", "(IJ)V") : NULL;
        if (!s_vibrate) {
            if ((*env)->ExceptionCheck(env))
                (*env)->ExceptionClear(env);
            s_no_method = 1;
        }
    }
    if (s_vibrate)
        (*env)->CallVoidMethod(env, act, s_vibrate, (jint)amp, (jlong)BURST_MS);
    if ((*env)->ExceptionCheck(env))   /* a phone that refuses the request is not a reason to stop the game */
        (*env)->ExceptionClear(env);
    (*env)->DeleteLocalRef(env, act);
    s_amp = amp;
    s_sent_ms = t;
}
