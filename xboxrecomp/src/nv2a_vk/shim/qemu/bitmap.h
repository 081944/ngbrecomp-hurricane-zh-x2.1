/* Just the bitmap operations the NV2A code calls, with QEMU's semantics:
 * unsigned long words, bit 0 of word 0 first. */
#ifndef NV2A_SHIM_BITMAP_H
#define NV2A_SHIM_BITMAP_H
#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define BITS_TO_LONGS(n) (((n) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define DECLARE_BITMAP(name, bits) unsigned long name[BITS_TO_LONGS(bits)]
static inline size_t bitmap_size(long nbits)
{
    return BITS_TO_LONGS(nbits) * sizeof(unsigned long);
}
static inline unsigned long *bitmap_new(long nbits)
{
    return g_malloc0(bitmap_size(nbits));
}
static inline bool test_bit(long nr, const unsigned long *addr)
{
    return (addr[nr / BITS_PER_LONG] >> (nr % BITS_PER_LONG)) & 1;
}
static inline void set_bit(long nr, unsigned long *addr)
{
    addr[nr / BITS_PER_LONG] |= 1UL << (nr % BITS_PER_LONG);
}
static inline void clear_bit(long nr, unsigned long *addr)
{
    addr[nr / BITS_PER_LONG] &= ~(1UL << (nr % BITS_PER_LONG));
}
static inline void bitmap_set(unsigned long *map, long start, long nr)
{
    for (long i = start; i < start + nr; i++) set_bit(i, map);
}
static inline void bitmap_clear(unsigned long *map, long start, long nr)
{
    for (long i = start; i < start + nr; i++) clear_bit(i, map);
}
static inline unsigned long find_next_bit(const unsigned long *addr,
                                          unsigned long size,
                                          unsigned long offset)
{
    for (; offset < size; offset++)
        if (test_bit(offset, addr)) return offset;
    return size;
}
static inline unsigned long find_first_bit(const unsigned long *addr,
                                           unsigned long size)
{
    return find_next_bit(addr, size, 0);
}
#endif
