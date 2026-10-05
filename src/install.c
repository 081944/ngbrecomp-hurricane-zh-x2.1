/*
 * Finding the game's files, and installing them from the player's disc.
 *
 * The program carries no game data. It runs from the disc's files: the
 * directory named on the command line or in NGB_GAME_DIR, else the one it
 * installed earlier (<save dir>/game), else "game" beside the executable.
 * Given a disc image instead -- an Xbox ISO (a plain XDVDFS image or a full
 * Redump dump) or the .7z/.zip it came in -- it unpacks the game partition
 * into <save dir>/game once and runs from there.
 *
 * The code was recompiled from one particular default.xbe, so the installed
 * one is checked against it: a different release or a modified disc would
 * run this code against data it was not built for.
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#if !defined(_WIN32)
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

#include "install.h"

/* The retail USA default.xbe the code was recompiled from. */
#define NGB_XBE_SIZE  4571136u
#define NGB_XBE_FNV   0x3602ba6a6079c8cdull

#define SECTOR 2048u
static const char XDVDFS_MAGIC[20] = "MICROSOFT*XBOX*MEDIA";
/* Where the game partition starts: plain XISO, a few rebuilt-image layouts,
 * and a Redump full-disc dump (after the ~387 MB video partition). */
static const uint64_t BASES[] = { 0, 0xFD90, 0x30600, 0xFD90000, 0x18300000 };

static int is_dir(const char *p)
{
    struct stat st;
    return p && stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static int is_file(const char *p)
{
    struct stat st;
    return p && stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

static int has_xbe(const char *dir)
{
    char p[4096];
    snprintf(p, sizeof p, "%s/default.xbe", dir);
    return is_file(p);
}

static int mkdirs(const char *path)
{
    char tmp[4096];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char *q = tmp + 1; *q; q++) {
        if (*q != '/')
            continue;
        *q = 0;
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
            return -1;
        *q = '/';
    }
    return mkdir(tmp, 0755) == 0 || errno == EEXIST ? 0 : -1;
}

/* 0 when dir's default.xbe is the one this build was made from. */
int ngb_check_xbe(const char *dir)
{
    char p[4096];
    unsigned char buf[1 << 16];
    uint64_t h = 0xcbf29ce484222325ull, size = 0;
    size_t n;
    FILE *f;

    snprintf(p, sizeof p, "%s/default.xbe", dir);
    f = fopen(p, "rb");
    if (!f)
        return -1;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) {
        for (size_t i = 0; i < n; i++)
            h = (h ^ buf[i]) * 0x100000001b3ull;
        size += n;
    }
    fclose(f);
    return size == NGB_XBE_SIZE && h == NGB_XBE_FNV ? 0 : 1;
}

/* ---- XDVDFS ------------------------------------------------------------ */

typedef struct {
    FILE *f;
    uint64_t base;
    uint64_t files, bytes;
} Image;

static int read_at(Image *im, uint64_t sector, void *buf, size_t len)
{
    if (fseeko(im->f, (off_t)(im->base + sector * SECTOR), SEEK_SET) != 0)
        return -1;
    return fread(buf, 1, len, im->f) == len ? 0 : -1;
}

static int copy_out(Image *im, uint32_t sector, uint32_t size, const char *dest)
{
    static unsigned char buf[1 << 20];
    FILE *out = fopen(dest, "wb");
    uint32_t left = size;

    if (!out)
        return -1;
    if (fseeko(im->f, (off_t)(im->base + (uint64_t)sector * SECTOR), SEEK_SET) != 0) {
        fclose(out);
        return -1;
    }
    while (left) {
        size_t chunk = left < sizeof buf ? left : sizeof buf;
        if (fread(buf, 1, chunk, im->f) != chunk || fwrite(buf, 1, chunk, out) != chunk) {
            fclose(out);
            return -1;
        }
        left -= (uint32_t)chunk;
    }
    im->files++;
    im->bytes += size;
    if (im->files % 25 == 0)
        fprintf(stderr, "  %llu files, %llu MB\n", (unsigned long long)im->files,
                (unsigned long long)(im->bytes >> 20));
    return fclose(out) == 0 ? 0 : -1;
}

/* Every entry of a directory. Entries form a binary tree, but they also lie
 * contiguously within each sector, so a linear walk with a resync at every
 * sector boundary reads them all without trusting the tree links. */
static int extract_dir(Image *im, uint32_t sector, uint32_t size, const char *dest)
{
    unsigned char *data;
    int rc = 0;

    if (size == 0)
        return 0;
    data = malloc(size);
    if (!data || read_at(im, sector, data, size) != 0) {
        free(data);
        return -1;
    }
    for (uint32_t base = 0; base < size && rc == 0; base += SECTOR) {
        uint32_t off = 0, end = size - base < SECTOR ? size - base : SECTOR;
        while (off + 0x0E <= end && rc == 0) {
            const unsigned char *e = data + base + off;
            uint16_t l = (uint16_t)(e[0] | e[1] << 8), r = (uint16_t)(e[2] | e[3] << 8);
            uint32_t sec = e[4] | e[5] << 8 | e[6] << 16 | (uint32_t)e[7] << 24;
            uint32_t sz = e[8] | e[9] << 8 | e[10] << 16 | (uint32_t)e[11] << 24;
            unsigned attr = e[12], nlen = e[13];
            char name[256], path[4096];

            if ((l == 0xFFFF && r == 0xFFFF) || nlen == 0 || off + 0x0E + nlen > end)
                break;
            memcpy(name, e + 0x0E, nlen);
            name[nlen] = 0;
            if (strchr(name, '/') || !strcmp(name, ".") || !strcmp(name, "..")) {
                rc = -1;
                break;
            }
            snprintf(path, sizeof path, "%s/%s", dest, name);
            if (attr & 0x10) {
                rc = mkdirs(path) == 0 ? extract_dir(im, sec, sz, path) : -1;
            } else {
                rc = copy_out(im, sec, sz, path);
            }
            off += (0x0E + nlen + 3) & ~3u;
        }
    }
    free(data);
    return rc;
}

static int extract_iso(const char *iso, const char *dest)
{
    Image im = { 0 };
    unsigned char desc[SECTOR];
    int rc = -1, found = 0;

    im.f = fopen(iso, "rb");
    if (!im.f) {
        fprintf(stderr, "Cannot open %s\n", iso);
        return -1;
    }
    for (size_t i = 0; i < sizeof BASES / sizeof BASES[0]; i++) {
        im.base = BASES[i];
        if (read_at(&im, 32, desc, sizeof desc) == 0 &&
            !memcmp(desc, XDVDFS_MAGIC, sizeof XDVDFS_MAGIC)) {
            uint32_t root = desc[20] | desc[21] << 8 | desc[22] << 16 | (uint32_t)desc[23] << 24;
            uint32_t rsize = desc[24] | desc[25] << 8 | desc[26] << 16 | (uint32_t)desc[27] << 24;
            found = 1;
            fprintf(stderr, "Extracting the game from %s ...\n", iso);
            rc = mkdirs(dest) == 0 ? extract_dir(&im, root, rsize, dest) : -1;
            if (rc == 0)
                fprintf(stderr, "  done: %llu files, %llu MB\n",
                        (unsigned long long)im.files, (unsigned long long)(im.bytes >> 20));
            break;
        }
    }
    if (!found)
        fprintf(stderr, "%s is not an Xbox disc image\n", iso);
    fclose(im.f);
    return rc;
}

/* ---- archives ---------------------------------------------------------- */

#if !defined(_WIN32)
static int run(char *const argv[])
{
    pid_t pid = fork();
    int status;
    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }
    if (pid < 0 || waitpid(pid, &status, 0) < 0)
        return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static int find_iso(const char *dir, char *out, size_t n)
{
    DIR *d = opendir(dir);
    struct dirent *e;
    int found = 0;
    if (!d)
        return 0;
    while (!found && (e = readdir(d))) {
        const char *dot = strrchr(e->d_name, '.');
        if (dot && (!strcasecmp(dot, ".iso") || !strcasecmp(dot, ".xiso"))) {
            snprintf(out, n, "%s/%s", dir, e->d_name);
            found = 1;
        }
    }
    closedir(d);
    return found;
}

/* Unpack an archive with whatever the system has: bsdtar (libarchive, on
 * SteamOS and most desktops) reads .7z and .zip, else 7z/7za. */
static int unpack_archive(const char *archive, const char *tmp)
{
    char *bsdtar[] = { "bsdtar", "-xf", (char *)archive, "-C", (char *)tmp, NULL };
    char opt[4200];
    char *sevenz[] = { "7z", "x", "-y", opt, (char *)archive, NULL };
    char *sevenza[] = { "7za", "x", "-y", opt, (char *)archive, NULL };

    snprintf(opt, sizeof opt, "-o%s", tmp);
    fprintf(stderr, "Unpacking %s (this needs about 8 GB free for a while) ...\n", archive);
    if (run(bsdtar) == 0 || run(sevenz) == 0 || run(sevenza) == 0)
        return 0;
    fprintf(stderr, "Cannot unpack %s: install bsdtar (libarchive) or 7-Zip, "
                    "or extract the .iso yourself and pass that.\n", archive);
    return -1;
}

static void remove_tree(const char *path)
{
    DIR *d = opendir(path);
    struct dirent *e;
    if (d) {
        while ((e = readdir(d))) {
            char p[4096];
            if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
                continue;
            snprintf(p, sizeof p, "%s/%s", path, e->d_name);
            if (is_dir(p))
                remove_tree(p);
            else
                unlink(p);
        }
        closedir(d);
    }
    rmdir(path);
}
#endif

/* ---- the picker -------------------------------------------------------- */

#if !defined(_WIN32)
/* Ask with a desktop file dialog when there is one (zenity, kdialog). */
static int pick_disc(char *out, size_t n)
{
    const char *cmds[] = {
        "zenity --file-selection --title='Ninja Gaiden Black: choose your disc image "
        "(.iso, .7z, .zip) or its default.xbe' 2>/dev/null",
        "kdialog --title 'Ninja Gaiden Black: choose your disc image' "
        "--getopenfilename \"$HOME\" '*.iso *.xiso *.7z *.zip default.xbe' 2>/dev/null",
    };
    for (size_t i = 0; i < sizeof cmds / sizeof cmds[0]; i++) {
        FILE *p = popen(cmds[i], "r");
        if (!p)
            continue;
        if (fgets(out, (int)n, p)) {
            out[strcspn(out, "\r\n")] = 0;
            if (pclose(p) == 0 && *out)
                return 1;
        } else {
            pclose(p);
        }
    }
    return 0;
}
#endif

/* Install from src (an image, an archive, or a directory/default.xbe) into
 * dest. 0 on success. */
static int install_from(const char *src, const char *dest)
{
    char dir[4096];
    const char *dot = strrchr(src, '.');

    /* A default.xbe or a folder of extracted files: run from there. */
    if (is_dir(src) && has_xbe(src))
        return 1;
    snprintf(dir, sizeof dir, "%s", src);
    if (dot && !strcasecmp(dot, ".xbe")) {
        char *slash = strrchr(dir, '/');
        if (slash) *slash = 0;
        return has_xbe(dir) ? 1 : -1;
    }
    if (dot && (!strcasecmp(dot, ".iso") || !strcasecmp(dot, ".xiso")))
        return extract_iso(src, dest);
#if !defined(_WIN32)
    if (dot && (!strcasecmp(dot, ".7z") || !strcasecmp(dot, ".zip"))) {
        char tmp[4200], iso[4400];
        int rc = -1;
        snprintf(tmp, sizeof tmp, "%s.unpack", dest);
        remove_tree(tmp);
        if (mkdirs(tmp) == 0 && unpack_archive(src, tmp) == 0) {
            if (find_iso(tmp, iso, sizeof iso))
                rc = extract_iso(iso, dest);
            else
                fprintf(stderr, "No .iso inside %s\n", src);
        }
        remove_tree(tmp);
        return rc;
    }
#endif
    fprintf(stderr, "%s: not a disc image, archive or game folder\n", src);
    return -1;
}

int ngb_find_game(const char *arg, const char *save_dir, const char *exe_dir,
                  char *out, size_t n)
{
    char installed[4096], beside[4096], picked[4096];
    const char *env = getenv("NGB_GAME_DIR");
    const char *src = arg && *arg ? arg : NULL;

    snprintf(installed, sizeof installed, "%s/game", save_dir);
    snprintf(beside, sizeof beside, "%sgame", exe_dir);

    if (!src && env && *env)
        src = env;
    if (!src) {
        if (has_xbe(installed)) { snprintf(out, n, "%s", installed); goto check; }
        if (has_xbe(beside))    { snprintf(out, n, "%s", beside);    goto check; }
#if !defined(_WIN32)
        if (pick_disc(picked, sizeof picked))
            src = picked;
#endif
        if (!src) {
            fprintf(stderr,
                "No game files found. Run it once with your disc image:\n"
                "  NinjaGaidenBlack /path/to/disc.iso   (or the .7z, or a folder "
                "of extracted files)\n"
                "It is installed into %s and found there from then on.\n", installed);
            return -1;
        }
    }

    if (is_dir(src) && has_xbe(src)) {
        snprintf(out, n, "%s", src);
    } else {
        int rc = install_from(src, installed);
        if (rc < 0)
            return -1;
        if (rc == 1) {   /* a folder or default.xbe: use it in place */
            snprintf(out, n, "%s", src);
            if (!is_dir(src)) {
                char *slash = strrchr(out, '/');
                if (slash) *slash = 0;
            }
        } else {
            snprintf(out, n, "%s", installed);
        }
    }

check:
    switch (ngb_check_xbe(out)) {
    case 0:
        return 0;
    case 1:
        fprintf(stderr, "%s/default.xbe is not the retail USA Ninja Gaiden Black "
                        "this build was made from.\n", out);
        if (getenv("NGB_ANY_XBE"))
            return 0;
        return -1;
    default:
        fprintf(stderr, "No default.xbe in %s\n", out);
        return -1;
    }
}
