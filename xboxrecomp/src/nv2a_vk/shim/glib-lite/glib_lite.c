/* The GLib slice of glib.h beside it (hosts without GLib). */
#define _GNU_SOURCE
#include "glib.h"

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

gpointer g_malloc(gsize n)
{
    if (!n)
        return NULL;
    void *p = malloc(n);
    if (!p)
        abort();
    return p;
}

gpointer g_malloc0(gsize n)
{
    if (!n)
        return NULL;
    void *p = calloc(1, n);
    if (!p)
        abort();
    return p;
}

gpointer g_realloc(gpointer p, gsize n)
{
    if (!n) {
        free(p);
        return NULL;
    }
    p = realloc(p, n);
    if (!p)
        abort();
    return p;
}

gpointer g_memdup2(gconstpointer p, gsize n)
{
    if (!p || !n)
        return NULL;
    void *q = g_malloc(n);
    memcpy(q, p, n);
    return q;
}

/* ---- Strings ---- */

gchar *g_strdup(const gchar *s)
{
    return s ? g_memdup2(s, strlen(s) + 1) : NULL;
}

gchar *g_strdup_vprintf(const gchar *fmt, va_list ap)
{
    char *s = NULL;
    if (vasprintf(&s, fmt, ap) < 0)
        abort();
    return s;
}

gchar *g_strdup_printf(const gchar *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    gchar *s = g_strdup_vprintf(fmt, ap);
    va_end(ap);
    return s;
}

gchar *g_strconcat(const gchar *first, ...)
{
    va_list ap;
    gsize n = 1;
    va_start(ap, first);
    for (const gchar *s = first; s; s = va_arg(ap, const gchar *))
        n += strlen(s);
    va_end(ap);
    gchar *out = g_malloc(n), *p = out;
    va_start(ap, first);
    for (const gchar *s = first; s; s = va_arg(ap, const gchar *)) {
        gsize l = strlen(s);
        memcpy(p, s, l);
        p += l;
    }
    va_end(ap);
    *p = 0;
    return out;
}

gsize g_strlcpy(gchar *dst, const gchar *src, gsize n)
{
    gsize l = strlen(src);
    if (n) {
        gsize c = l < n - 1 ? l : n - 1;
        memcpy(dst, src, c);
        dst[c] = 0;
    }
    return l;
}

static void string_room(GString *s, gsize more)
{
    if (s->len + more + 1 <= s->allocated_len)
        return;
    gsize a = s->allocated_len ? s->allocated_len : 16;
    while (a < s->len + more + 1)
        a *= 2;
    s->str = g_realloc(s->str, a);
    s->allocated_len = a;
}

GString *g_string_new(const gchar *init)
{
    GString *s = g_new0(GString, 1);
    string_room(s, 0);
    s->str[0] = 0;
    if (init)
        g_string_append(s, init);
    return s;
}

GString *g_string_new_take(gchar *init)
{
    if (!init)
        return g_string_new(NULL);
    GString *s = g_new0(GString, 1);
    s->str = init;
    s->len = strlen(init);
    s->allocated_len = s->len + 1;
    return s;
}

GString *g_string_append(GString *s, const gchar *text)
{
    gsize l = strlen(text);
    string_room(s, l);
    memcpy(s->str + s->len, text, l + 1);
    s->len += l;
    return s;
}

void g_string_append_vprintf(GString *s, const gchar *fmt, va_list ap)
{
    gchar *t = g_strdup_vprintf(fmt, ap);
    g_string_append(s, t);
    g_free(t);
}

gchar *g_string_free(GString *s, gboolean free_segment)
{
    gchar *str = s->str;
    g_free(s);
    if (free_segment) {
        g_free(str);
        return NULL;
    }
    return str;
}

/* ---- Arrays: the public part first, so a GArray * is the real one ---- */

typedef struct {
    GArray pub;
    guint elt, alloc;
    gboolean zero_terminated, clear;
} RealArray;

static void array_room(RealArray *a, guint len)
{
    guint want = len + (a->zero_terminated ? 1 : 0);
    if (want <= a->alloc)
        return;
    guint n = a->alloc ? a->alloc : 8;
    while (n < want)
        n *= 2;
    a->pub.data = g_realloc(a->pub.data, (gsize)n * a->elt);
    a->alloc = n;
}

static void array_terminate(RealArray *a)
{
    if (a->zero_terminated)
        memset(a->pub.data + (gsize)a->pub.len * a->elt, 0, a->elt);
}

GArray *g_array_sized_new(gboolean zero_terminated, gboolean clear, guint element_size, guint reserved)
{
    RealArray *a = g_new0(RealArray, 1);
    a->elt = element_size;
    a->zero_terminated = zero_terminated;
    a->clear = clear;
    array_room(a, reserved);
    if (a->pub.data)
        array_terminate(a);
    return &a->pub;
}

GArray *g_array_new(gboolean zero_terminated, gboolean clear, guint element_size)
{
    return g_array_sized_new(zero_terminated, clear, element_size, 0);
}

GArray *g_array_append_vals(GArray *pub, gconstpointer data, guint len)
{
    RealArray *a = (RealArray *)pub;
    array_room(a, pub->len + len);
    memcpy(pub->data + (gsize)pub->len * a->elt, data, (gsize)len * a->elt);
    pub->len += len;
    array_terminate(a);
    return pub;
}

GArray *g_array_set_size(GArray *pub, guint len)
{
    RealArray *a = (RealArray *)pub;
    array_room(a, len);
    if (len > pub->len && a->clear)
        memset(pub->data + (gsize)pub->len * a->elt, 0, (gsize)(len - pub->len) * a->elt);
    pub->len = len;
    if (pub->data)
        array_terminate(a);
    return pub;
}

gchar *g_array_free(GArray *pub, gboolean free_segment)
{
    gchar *data = pub->data;
    g_free(pub);
    if (free_segment) {
        g_free(data);
        return NULL;
    }
    return data;
}

void g_array_unref(GArray *a)
{
    g_array_free(a, TRUE);
}

typedef struct {
    GByteArray pub;
    guint alloc;
} RealBytes;

static void bytes_room(RealBytes *a, guint len)
{
    if (len <= a->alloc)
        return;
    guint n = a->alloc ? a->alloc : 16;
    while (n < len)
        n *= 2;
    a->pub.data = g_realloc(a->pub.data, n);
    a->alloc = n;
}

GByteArray *g_byte_array_sized_new(guint reserved)
{
    RealBytes *a = g_new0(RealBytes, 1);
    bytes_room(a, reserved);
    return &a->pub;
}

GByteArray *g_byte_array_new(void)
{
    return g_byte_array_sized_new(0);
}

GByteArray *g_byte_array_new_take(guint8 *data, gsize len)
{
    RealBytes *a = g_new0(RealBytes, 1);
    a->pub.data = data;
    a->pub.len = (guint)len;
    a->alloc = (guint)len;
    return &a->pub;
}

GByteArray *g_byte_array_append(GByteArray *pub, const guint8 *data, guint len)
{
    bytes_room((RealBytes *)pub, pub->len + len);
    memcpy(pub->data + pub->len, data, len);
    pub->len += len;
    return pub;
}

GByteArray *g_byte_array_set_size(GByteArray *pub, guint len)
{
    bytes_room((RealBytes *)pub, len);
    pub->len = len;
    return pub;
}

guint8 *g_byte_array_free(GByteArray *pub, gboolean free_segment)
{
    guint8 *data = pub->data;
    g_free(pub);
    if (free_segment) {
        g_free(data);
        return NULL;
    }
    return data;
}

void g_byte_array_unref(GByteArray *a)
{
    g_byte_array_free(a, TRUE);
}

/* ---- Hash tables: chained buckets, doubled at a load of 1 ---- */

typedef struct Node {
    struct Node *next;
    guint hash;
    gpointer key, value;
} Node;

struct GHashTable {
    Node **buckets;
    guint size, count;
    GHashFunc hash;
    GEqualFunc equal;
    GDestroyNotify key_destroy, value_destroy;
};

static guint direct_hash(gconstpointer v) { return (guint)(uintptr_t)v; }
static gboolean direct_equal(gconstpointer a, gconstpointer b) { return a == b; }

GHashTable *g_hash_table_new_full(GHashFunc hash, GEqualFunc equal, GDestroyNotify key_destroy,
                                  GDestroyNotify value_destroy)
{
    GHashTable *t = g_new0(GHashTable, 1);
    t->size = 16;
    t->buckets = g_new0(Node *, t->size);
    t->hash = hash ? hash : direct_hash;
    t->equal = equal ? equal : direct_equal;
    t->key_destroy = key_destroy;
    t->value_destroy = value_destroy;
    return t;
}

static Node **find(GHashTable *t, gconstpointer key, guint h)
{
    Node **n = &t->buckets[h & (t->size - 1)];
    while (*n && !((*n)->hash == h && t->equal((*n)->key, key)))
        n = &(*n)->next;
    return n;
}

static void grow(GHashTable *t)
{
    guint size = t->size * 2;
    Node **b = g_new0(Node *, size);
    for (guint i = 0; i < t->size; i++)
        for (Node *n = t->buckets[i], *next; n; n = next) {
            next = n->next;
            n->next = b[n->hash & (size - 1)];
            b[n->hash & (size - 1)] = n;
        }
    g_free(t->buckets);
    t->buckets = b;
    t->size = size;
}

gboolean g_hash_table_replace(GHashTable *t, gpointer key, gpointer value)
{
    guint h = t->hash(key);
    Node **n = find(t, key, h);
    if (*n) {   /* GLib's replace: the new key and value, the old ones destroyed */
        if (t->key_destroy && (*n)->key != key)
            t->key_destroy((*n)->key);
        if (t->value_destroy && (*n)->value != value)
            t->value_destroy((*n)->value);
        (*n)->key = key;
        (*n)->value = value;
        return FALSE;
    }
    Node *node = g_new(Node, 1);
    node->next = NULL;
    node->hash = h;
    node->key = key;
    node->value = value;
    *n = node;
    if (++t->count > t->size)
        grow(t);
    return TRUE;
}

gboolean g_hash_table_add(GHashTable *t, gpointer key)
{
    return g_hash_table_replace(t, key, key);
}

gpointer g_hash_table_lookup(GHashTable *t, gconstpointer key)
{
    Node *n = *find(t, key, t->hash(key));
    return n ? n->value : NULL;
}

gboolean g_hash_table_contains(GHashTable *t, gconstpointer key)
{
    return *find(t, key, t->hash(key)) != NULL;
}

void g_hash_table_destroy(GHashTable *t)
{
    if (!t)
        return;
    for (guint i = 0; i < t->size; i++)
        for (Node *n = t->buckets[i], *next; n; n = next) {
            next = n->next;
            if (t->key_destroy)
                t->key_destroy(n->key);
            if (t->value_destroy && !(t->key_destroy && n->value == n->key))
                t->value_destroy(n->value);
            g_free(n);
        }
    g_free(t->buckets);
    g_free(t);
}

guint g_int64_hash(gconstpointer v)
{
    gint64 x = *(const gint64 *)v;
    return (guint)(x ^ (x >> 32));
}

gboolean g_int64_equal(gconstpointer a, gconstpointer b)
{
    return *(const gint64 *)a == *(const gint64 *)b;
}

/* ---- Threads ---- */

struct GThread {
    pthread_t id;
    GThreadFunc func;
    gpointer data;
};

static void *thread_start(void *p)
{
    GThread *t = p;
    return t->func(t->data);
}

GThread *g_thread_new(const gchar *name, GThreadFunc func, gpointer data)
{
    GThread *t = g_new0(GThread, 1);
    t->func = func;
    t->data = data;
    if (pthread_create(&t->id, NULL, thread_start, t))
        abort();
#if defined(__linux__)
    if (name) {
        char n[16];
        g_strlcpy(n, name, sizeof n);
        pthread_setname_np(t->id, n);
    }
#else
    (void)name;
#endif
    return t;
}

typedef struct QNode {
    struct QNode *next;
    gpointer data;
} QNode;

struct GAsyncQueue {
    pthread_mutex_t m;
    pthread_cond_t c;
    QNode *head, *tail;
};

GAsyncQueue *g_async_queue_new(void)
{
    GAsyncQueue *q = g_new0(GAsyncQueue, 1);
    pthread_mutex_init(&q->m, NULL);
    pthread_cond_init(&q->c, NULL);
    return q;
}

void g_async_queue_push(GAsyncQueue *q, gpointer data)
{
    QNode *n = g_new(QNode, 1);
    n->next = NULL;
    n->data = data;
    pthread_mutex_lock(&q->m);
    if (q->tail)
        q->tail->next = n;
    else
        q->head = n;
    q->tail = n;
    pthread_cond_signal(&q->c);
    pthread_mutex_unlock(&q->m);
}

gpointer g_async_queue_pop(GAsyncQueue *q)
{
    pthread_mutex_lock(&q->m);
    while (!q->head)
        pthread_cond_wait(&q->c, &q->m);
    QNode *n = q->head;
    q->head = n->next;
    if (!q->head)
        q->tail = NULL;
    pthread_mutex_unlock(&q->m);
    gpointer data = n->data;
    g_free(n);
    return data;
}

/* ---- System ---- */

gint64 g_get_monotonic_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (gint64)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void g_usleep(unsigned long microseconds)
{
    struct timespec ts = { (time_t)(microseconds / 1000000), (long)(microseconds % 1000000) * 1000 };
    while (nanosleep(&ts, &ts) && errno == EINTR) {
    }
}

guint g_get_num_processors(void)
{
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 ? (guint)n : 1;
}

gint g_mkdir_with_parents(const gchar *path, gint mode)
{
    if (!path || !*path) {
        errno = EINVAL;
        return -1;
    }
    gchar *p = g_strdup(path);
    for (gchar *c = p + 1;; c++) {
        if (*c == '/' || !*c) {
            gchar keep = *c;
            *c = 0;
            if (mkdir(p, (mode_t)mode) && errno != EEXIST) {
                g_free(p);
                return -1;
            }
            *c = keep;
            if (!keep)
                break;
        }
    }
    g_free(p);
    return 0;
}

gboolean g_file_get_contents(const gchar *path, gchar **contents, gsize *length, GError **error)
{
    (void)error;
    FILE *f = fopen(path, "rb");
    if (!f)
        return FALSE;
    gsize cap = 1 << 16, len = 0;
    gchar *buf = g_malloc(cap);
    for (size_t r; (r = fread(buf + len, 1, cap - len - 1, f)) > 0;) {
        len += r;
        if (len + 1 == cap)
            buf = g_realloc(buf, cap *= 2);
    }
    fclose(f);
    buf[len] = 0;
    *contents = buf;
    if (length)
        *length = len;
    return TRUE;
}
