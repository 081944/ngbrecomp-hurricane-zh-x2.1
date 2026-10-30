/* JNI glue for the setup UI's process: it must not load the full game
 * library (libmain.so), so the SAF ISO import lives in its own small
 * library (libngbsetup.so) built from src/install.c. */
#include <jni.h>
#include <stdint.h>

#include "install.h"

static volatile uint64_t s_import_done, s_import_total;

static void import_progress(uint64_t done, uint64_t total, void *ud)
{
    (void)ud;
    s_import_done = done;
    s_import_total = total;
}

/* Installs the picked image (an Xbox ISO reached through an SAF fd) into
 * dest. Runs on the Java import thread; progress is exposed through
 * nativeInstallProgress for the UI thread to poll. */
JNIEXPORT jint JNICALL
Java_org_ngb_port_SetupActivity_nativeImportGame(JNIEnv *env, jclass cls,
    jint fd, jstring dest)
{
    (void)env; (void)cls;
    const char *d = (*env)->GetStringUTFChars(env, dest, NULL);
    if (!d) return -1;
    char out[4096];
    ngb_install_set_progress(import_progress, NULL);
    s_import_done = s_import_total = 0;
    int rc = ngb_install_iso_fd((int)fd, d, out, sizeof out);
    (*env)->ReleaseStringUTFChars(env, dest, d);
    return rc;
}

/* 0 when dir/default.xbe is the very XBE this build was recompiled from, so
 * the setup screen can tell "imported, of the wrong version" (a US folder
 * left by another build) apart from "imported". */
JNIEXPORT jint JNICALL
Java_org_ngb_port_SetupActivity_nativeCheckGame(JNIEnv *env, jclass cls,
    jstring dir)
{
    (void)cls;
    const char *d = (*env)->GetStringUTFChars(env, dir, NULL);
    if (!d) return -1;
    int rc = ngb_check_xbe(d);
    (*env)->ReleaseStringUTFChars(env, dir, d);
    return rc;
}

/* { bytes done, bytes total } of the running/last import. */
JNIEXPORT jlongArray JNICALL
Java_org_ngb_port_SetupActivity_nativeInstallProgress(JNIEnv *env, jclass cls)
{
    (void)cls;
    jlong v[2] = { (jlong)s_import_done, (jlong)s_import_total };
    jlongArray a = (*env)->NewLongArray(env, 2);
    if (a) (*env)->SetLongArrayRegion(env, a, 0, 2, v);
    return a;
}

JNIEXPORT jstring JNICALL
Java_org_ngb_port_SetupActivity_nativeInstallError(JNIEnv *env, jclass cls)
{
    (void)cls;
    const char *e = ngb_install_error();
    return (*env)->NewStringUTF(env, e && e[0] ? e : "unknown error");
}
