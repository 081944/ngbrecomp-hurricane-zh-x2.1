/*
 * The slice of GLib that xemu's NV2A code uses, for hosts without GLib (Android): memory, the
 * array, byte array, string and hash table types, a mutex, an async queue, a thread and a few
 * string and file helpers, each behaving as GLib's does for the calls made here. Built instead of
 * GLib when NV2A_GLIB_LITE is set (src/nv2a_vk/CMakeLists.txt); glib_lite.c has the rest.
 */
#ifndef NV2A_GLIB_LITE_H
#define NV2A_GLIB_LITE_H

#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef char gchar;
typedef unsigned char guchar;
typedef int gint;
typedef unsigned int guint;
typedef uint8_t guint8;
typedef uint16_t guint16;
typedef uint32_t guint32;
typedef int64_t gint64;
typedef uint64_t guint64;
typedef size_t gsize;
typedef int gboolean;
typedef void *gpointer;
typedef const void *gconstpointer;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define G_GNUC_PRINTF(f, a) __attribute__((format(printf, f, a)))
#define G_GNUC_UNUSED __attribute__((unused))
#define G_LIKELY(x) __builtin_expect(!!(x), 1)
#define G_UNLIKELY(x) __builtin_expect(!!(x), 0)

typedef void (*GDestroyNotify)(gpointer data);
typedef guint (*GHashFunc)(gconstpointer key);
typedef gboolean (*GEqualFunc)(gconstpointer a, gconstpointer b);
typedef gpointer (*GThreadFunc)(gpointer data);

/* ---- Memory: allocation failure aborts, as in GLib ---- */
gpointer g_malloc(gsize n);
gpointer g_malloc0(gsize n);
gpointer g_realloc(gpointer p, gsize n);
static inline void g_free(gpointer p) { free(p); }
static inline gpointer g_malloc_n(gsize count, gsize size) { return g_malloc(count * size); }
static inline gpointer g_malloc0_n(gsize count, gsize size) { return g_malloc0(count * size); }
#define g_new(T, n) ((T *)g_malloc_n((n), sizeof(T)))
#define g_new0(T, n) ((T *)g_malloc0_n((n), sizeof(T)))
gpointer g_memdup2(gconstpointer p, gsize n);

/* ---- Automatic cleanup ---- */
static inline void g_autoptr_cleanup_generic_gfree(void *p) { g_free(*(void **)p); }
#define g_autofree __attribute__((cleanup(g_autoptr_cleanup_generic_gfree)))
/* (through one more macro each, so a type name that is itself a macro is expanded first, as in GLib) */
#define G_DEFINE_AUTOPTR_CLEANUP_FUNC(T, f) G_DEFINE_AUTOPTR_CLEANUP_FUNC_(T, f)
#define G_DEFINE_AUTOPTR_CLEANUP_FUNC_(T, f)                                       \
    typedef T *glib_autoptr_##T;                                                   \
    static inline G_GNUC_UNUSED void glib_autoptr_cleanup_##T(T **p) { if (*p) f(*p); }
#define g_autoptr(T) g_autoptr_(T)
#define g_autoptr_(T) __attribute__((cleanup(glib_autoptr_cleanup_##T))) glib_autoptr_##T

/* ---- Strings ---- */
gchar *g_strdup(const gchar *s);
gchar *g_strdup_vprintf(const gchar *fmt, va_list ap) G_GNUC_PRINTF(1, 0);
gchar *g_strdup_printf(const gchar *fmt, ...) G_GNUC_PRINTF(1, 2);
gchar *g_strconcat(const gchar *first, ...) __attribute__((sentinel));
gsize g_strlcpy(gchar *dst, const gchar *src, gsize n);
static inline gboolean g_str_has_prefix(const gchar *s, const gchar *prefix)
{
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

typedef struct {
    gchar *str;
    gsize len;
    gsize allocated_len;
} GString;
GString *g_string_new(const gchar *init);
GString *g_string_new_take(gchar *init);
GString *g_string_append(GString *s, const gchar *text);
void g_string_append_vprintf(GString *s, const gchar *fmt, va_list ap) G_GNUC_PRINTF(2, 0);
gchar *g_string_free(GString *s, gboolean free_segment);

/* ---- Arrays ---- */
typedef struct {
    gchar *data;
    guint len;
} GArray;
GArray *g_array_new(gboolean zero_terminated, gboolean clear, guint element_size);
GArray *g_array_sized_new(gboolean zero_terminated, gboolean clear, guint element_size, guint reserved);
GArray *g_array_append_vals(GArray *a, gconstpointer data, guint len);
GArray *g_array_set_size(GArray *a, guint len);
gchar *g_array_free(GArray *a, gboolean free_segment);
void g_array_unref(GArray *a);
#define g_array_append_val(a, v) g_array_append_vals((a), &(v), 1)
#define g_array_index(a, T, i) (((T *)(void *)(a)->data)[(i)])

typedef struct {
    guint8 *data;
    guint len;
} GByteArray;
GByteArray *g_byte_array_new(void);
GByteArray *g_byte_array_sized_new(guint reserved);
GByteArray *g_byte_array_new_take(guint8 *data, gsize len);
GByteArray *g_byte_array_append(GByteArray *a, const guint8 *data, guint len);
GByteArray *g_byte_array_set_size(GByteArray *a, guint len);
guint8 *g_byte_array_free(GByteArray *a, gboolean free_segment);
void g_byte_array_unref(GByteArray *a);

/* ---- Hash tables ---- */
typedef struct GHashTable GHashTable;
GHashTable *g_hash_table_new_full(GHashFunc hash, GEqualFunc equal, GDestroyNotify key_destroy,
                                  GDestroyNotify value_destroy);
gboolean g_hash_table_replace(GHashTable *t, gpointer key, gpointer value);
gboolean g_hash_table_add(GHashTable *t, gpointer key);
gpointer g_hash_table_lookup(GHashTable *t, gconstpointer key);
gboolean g_hash_table_contains(GHashTable *t, gconstpointer key);
void g_hash_table_destroy(GHashTable *t);
guint g_int64_hash(gconstpointer v);
gboolean g_int64_equal(gconstpointer a, gconstpointer b);

/* ---- Threads: a zeroed GMutex is ready to use, as in GLib ---- */
typedef struct {
    pthread_mutex_t m;
} GMutex;
static inline void g_mutex_lock(GMutex *m) { pthread_mutex_lock(&m->m); }
static inline void g_mutex_unlock(GMutex *m) { pthread_mutex_unlock(&m->m); }

typedef struct GThread GThread;
GThread *g_thread_new(const gchar *name, GThreadFunc func, gpointer data);

typedef struct GAsyncQueue GAsyncQueue;
GAsyncQueue *g_async_queue_new(void);
void g_async_queue_push(GAsyncQueue *q, gpointer data);
gpointer g_async_queue_pop(GAsyncQueue *q);

/* GLib's own cleanups for its types */
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GArray, g_array_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GByteArray, g_byte_array_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GHashTable, g_hash_table_destroy)

/* ---- System ---- */
gint64 g_get_monotonic_time(void);
void g_usleep(unsigned long microseconds);
guint g_get_num_processors(void);
gint g_mkdir_with_parents(const gchar *path, gint mode);
typedef struct GError GError;
gboolean g_file_get_contents(const gchar *path, gchar **contents, gsize *length, GError **error);

#endif
