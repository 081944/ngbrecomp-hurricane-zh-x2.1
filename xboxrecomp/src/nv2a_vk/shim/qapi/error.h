#ifndef NV2A_SHIM_ERROR_H
#define NV2A_SHIM_ERROR_H
typedef struct Error { char *msg; } Error;
extern Error *error_fatal;
extern Error *error_abort;
void error_setg_internal(Error **errp, const char *fmt, ...) G_GNUC_PRINTF(2, 3);
#define error_setg(errp, ...) error_setg_internal(errp, __VA_ARGS__)
void error_prepend(Error **errp, const char *fmt, ...) G_GNUC_PRINTF(2, 3);
static inline const char *error_get_pretty(const Error *err) { return err->msg; }
static inline void error_free(Error *err) { if (err) { g_free(err->msg); g_free(err); } }
static inline void error_propagate(Error **dst, Error *src)
{
    if (dst) *dst = src; else error_free(src);
}
#endif
