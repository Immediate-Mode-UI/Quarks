#include "qk.h"

/* system includes */
#include <assert.h> /* assert */
#include <stdlib.h> /* calloc, free */
#include <string.h> /* memcpy, memset */
#include <inttypes.h> /* PRIu64 */
#include <limits.h> /* INT_MAX */
#include <stdio.h> /* fprintf, fputc */

/* constants */
#define VERSION 1
#define UTF_SIZE 4
#define UTF_INVALID 0xFFFD

/* utf-8 */
static const unsigned char utf_byte[UTF_SIZE+1] = {0x80,0,0xC0,0xE0,0xF0};
static const unsigned char utf_mask[UTF_SIZE+1] = {0xC0,0x80,0xE0,0xF0,0xF8};
static const unsigned long utf_min[UTF_SIZE+1] = {0,0,0x80,0x800,0x10000};
static const unsigned long utf_max[UTF_SIZE+1] = {0x10FFFF,0x7F,0x7FF,0xFFFF,0x10FFFF};

/* memory */
#define qalloc(a,sz) (a)->alloc((a)->usr, sz, __FILE__, __LINE__)
#define qdealloc(a,ptr) (a)->dealloc((a)->usr, ptr, __FILE__, __LINE__)
static void *dalloc(void *usr, int s, const char *file, int line){return malloc((size_t)s);}
static void dfree(void *usr, void *d, const char *file, int line){free(d);}
static const struct allocator default_allocator = {0,dalloc,dfree};

/* root tables */
static const struct element g_root_elements[] = {
    /* type             id, parent, wid,            depth,flags */
    {WIDGET_ROOT,       0,  0,  WIDGET_ROOT,        0,0},
    {WIDGET_OVERLAY,    1,  0,  WIDGET_OVERLAY,     1,0},
    {WIDGET_POPUP,      2,  1,  WIDGET_POPUP,       2,0},
    {WIDGET_CONTEXTUAL, 3,  2,  WIDGET_CONTEXTUAL,  3,0},
    {WIDGET_UNBLOCKING, 4,  3,  WIDGET_UNBLOCKING,  4,0},
    {WIDGET_BLOCKING,   5,  4,  WIDGET_BLOCKING,    5,0},
    {WIDGET_UI,         6,  5,  WIDGET_UI,          6,0},
};
static union param g_root_params[1];
static const unsigned char g_root_data[1];
static const uiid g_root_table_keys[] = {0,1,2,3,4,5,6,0};
static const int g_root_table_vals[] = {0,1,2,3,4,5,6,0};
static const struct component g_root = {
    VERSION, 0, 0, 7,
    g_root_elements, cntof(g_root_elements),
    g_root_table_vals, g_root_table_keys, 8,
    g_root_data, 0, g_root_params, 0,
    0, 0, 0, 0
};
#define OPCODES(OP)\
    OP(BUF_BEGIN,       1,  MIDFMT)\
    OP(BUF_ULINK,       2,  MIDFMT" "IDFMT)\
    OP(BUF_DLINK,       1,  MIDFMT)\
    OP(BUF_CONECT,      2,  MIDFMT" %d")\
    OP(BUF_END,         1,  MIDFMT)\
    OP(WIDGET_BEGIN,    2,  "%d %d")\
    OP(WIDGET_END,      0,  "")  \
    OP(BOX_PUSH,        2,  IDFMT" "IDFMT)\
    OP(BOX_POP,         0,  "")\
    OP(PROPERTY_SET,    1,  "%u")\
    OP(PROPERTY_CLR,    1,  "%u")\
    OP(PUSH_FLOAT,      1,  "%f")\
    OP(PUSH_INT,        1,  "%d")\
    OP(PUSH_UINT,       1,  "%u")\
    OP(PUSH_ID,         1,  IDFMT)\
    OP(PUSH_MID,        1,  MIDFMT)\
    OP(PUSH_STR,        1,  "%s")\
    OP(NEXT_BUF,        1,  "%p")\
    OP(EOF,             0,  "")
enum opcodes {
    #define OP(a,b,c) OP_ ## a,
    OPCODES(OP)
    #undef OP
    OPCNT
};
/* ---------------------------------------------------------------------------
 *                                  Math
 * --------------------------------------------------------------------------- */
api int
npow2(int n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return ++n;
}
api int
floori(float x)
{
    x = cast(float,(cast(int,x) - ((x < 0.0f) ? 1 : 0)));
    return cast(int,x);
}
api int
ceili(float x)
{
    if (x < 0) {
        int t = cast(int,x);
        float r = x - cast(float,t);
        return (r > 0.0f) ? (t+1): t;
    } else {
        int i = cast(int,x);
        return (x > i) ? (i+1): i;
    }
}
api float
roundf(float x)
{
    int e = 0;
    float y = 0;
    static const float toint = 1.0f/(1.1920928955078125e-07F);
    union {float f; unsigned long i;} u;

    u.f = x;
    e = (u.i >> 23) & 0xff;
    if (e >= 0x7f+23) return x;
    if (u.i >> 31) x = -x;
    if (e < 0x7f-1)
        return 0*u.f;

    y = x + toint - toint - x;
    if (y > 0.5f)
        y = y + x - 1;
    else if (y <= -0.5f)
        y = y + x + 1;
    else y = y + x;
    if (u.i >> 31)
        y = -y;
    return y;
}
api int
roundi(float x)
{
    return cast(int, roundf(x));
}
api int
strn(const char *s)
{
    const char *a = s;
    const unsigned *w;
    if (!s) return 0;

    #define UONES (((unsigned)-1)/UCHAR_MAX)
    #define UHIGHS (UONES * (UCHAR_MAX/2+1))
    #define UHASZERO(x) ((x)-UONES & ~(x) & UHIGHS)

    for (;ucast(s)&3; ++s) if (!*s) return (int)(s-a);
    for (w = (const void*)s; !UHASZERO(*w); ++w);
    for (s = (const void*)w; *s; ++s);
    return (int)(s-a);

    #undef UONES
    #undef UHIGHS
    #undef UHASZERO
}
/* ---------------------------------------------------------------------------
 *                                  Overflow
 * --------------------------------------------------------------------------- */
intern int
size_add_valid(int a, int b)
{
    if (a < 0 || b < 0) return 0;
    return a <= (INT_MAX-b);
}
intern int
size_add2_valid(int a, int b, int c)
{
    return size_add_valid(a, b) && size_add_valid(a+b, c);
}
intern int
size_mul_valid(int a, int b)
{
    if (a < 0 || b < 0) return 0;
    if (b == 0) return 1;
    return a <= (INT_MAX/b);
}
intern int
size_add_mul2_valid(int mula, int a, int mulb, int b)
{
    /* mula * a + mulb * b */
    return size_mul_valid(mula, a) && size_mul_valid(mulb, b) && size_add_valid(mula*a,mulb*b);
}
intern int
size_mul_add2_valid(int mul, int a, int b, int c)
{
    /* mul*a + b + c */
    return size_mul_valid(a,mul) && size_add2_valid(a*mul,b,c);
}
intern int
size_mul2_add_valid(int mula, int a, int mulb, int b, int c)
{
    /* mula*a + mulb*b + c */
    return size_add_mul2_valid(mula,a,mulb,b) && size_add_valid(mula*a+mulb*b,c);
}
intern int
size_mul2_add2_valid(int mula, int a, int mulb, int b, int c, int d)
{
    /* mula*a + mulb*b + c + d */
    return size_mul2_add_valid(mula,a,mulb,b,c) && size_add_valid(mula*a+mulb*b+c,d);
}
/* ---------------------------------------------------------------------------
 *                                  UTF-8
 * --------------------------------------------------------------------------- */
intern int
utf_validate(unsigned long *u, int i)
{
    assert(u);
    if (!u) return 0;
    if (!between(*u, utf_min[i], utf_max[i]) ||
         between(*u, 0xD800, 0xDFFF))
        *u = UTF_INVALID;
    for (i = 1; *u > utf_max[i]; ++i);
    return i;
}
intern unsigned long
utf_decode_byte(char c, int *i)
{
    assert(i);
    if (!i) return 0;
    for(*i = 0; *i < cntof(utf_mask); ++(*i)) {
        if (((unsigned char)c & utf_mask[*i]) == utf_byte[*i])
            return (unsigned char)(c & ~utf_mask[*i]);
    } return 0;
}
api int
utf_decode(unsigned long *u, const char *s, int slen)
{
    int i,j, len, type = 0;
    unsigned long udecoded;
    assert(s); assert(u);
    if (!s || !u || !slen)
        return 0;

    *u = UTF_INVALID;
    udecoded = utf_decode_byte(s[0], &len);
    if (!between(len, 1, UTF_SIZE))
        return 1;

    for (i = 1, j = 1; i < slen && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | utf_decode_byte(s[i], &type);
        if (type != 0) return j;
    } if (j < len) return 0;

    *u = udecoded;
    utf_validate(u, len);
    return len;
}
intern char
utf_encode_byte(unsigned long u, int i)
{
    return (char)((utf_byte[i]) | ((unsigned char)u & ~utf_mask[i]));
}
api int
utf_encode(char *s, int cap, unsigned long u)
{
    int len, i;
    len = utf_validate(&u, 0);
    if (cap < len || !len || len > UTF_SIZE)
        return 0;

    for (i = len - 1; i != 0; --i) {
        s[i] = utf_encode_byte(u, 0);
        u >>= 6;
    } s[0] = utf_encode_byte(u, len);
    return len;
}
api int
utf_len(const char *s, int len)
{
    int result = 0;
    int rune_len = 0;
    unsigned long rune = 0;
    if (!s) return 0;

    while ((rune_len = utf_decode(&rune, s, len))) {
        len = max(0,len-rune_len);
        s += rune_len;
        result++;
    } return result;
}
api const char*
utf_at(unsigned long *rune, int *rune_len,
    const char *s, int len, int idx)
{
    int runes = 0;
    assert(s);
    assert(rune);
    assert(rune_len);

    if (!s || !rune || !rune_len) return 0;
    while ((*rune_len = utf_decode(rune, s, len))) {
        if (runes++ == idx) return s;
        len = max(0, len - *rune_len);
        s += *rune_len;
    } return 0;
}
/* ---------------------------------------------------------------------------
 *                                  List
 * --------------------------------------------------------------------------- */
api void
list_init(struct list_hook *list)
{
    list->next = list->prev = list;
}
intern void
list__add(struct list_hook *n,
    struct list_hook *prev, struct list_hook *next)
{
    next->prev = n;
    n->next = next;
    n->prev = prev;
    prev->next = n;
}
api void
list_add_head(struct list_hook *list, struct list_hook *n)
{
    list__add(n, list, list->next);
}
api void
list_add_tail(struct list_hook *list, struct list_hook *n)
{
    list__add(n, list->prev, list);
}
intern void
list__del(struct list_hook *prev, struct list_hook *next)
{
    next->prev = prev;
    prev->next = next;
}
api void
list_del(struct list_hook *entry)
{
    list__del(entry->prev, entry->next);
    entry->next = entry;
    entry->prev = entry;
}
api void
list_move_head(struct list_hook *list, struct list_hook *entry)
{
    list_del(entry);
    list_add_head(list, entry);
}
api void
list_move_tail(struct list_hook *list, struct list_hook *entry)
{
    list_del(entry);
    list_add_tail(list, entry);
}
intern void
list__splice(const struct list_hook *list,
    struct list_hook *prev, struct list_hook *next)
{
    struct list_hook *first = list->next;
    struct list_hook *last = list->prev;

    first->prev = prev;
    prev->next = first;

    last->next = next;
    next->prev = last;
}
intern void
list_splice_head(struct list_hook *dst,
    struct list_hook *list)
{
    if (!list_empty(list)) {
        list__splice(list, dst, dst->next);
        list_init(list);
    }
}
intern void
list_splice_tail(struct list_hook *dst,
    struct list_hook *list)
{
    if (!list_empty(list)) {
        list__splice(list, dst->prev, dst);
        list_init(list);
    }
}
/* ---------------------------------------------------------------------------
 *                              Block-Alloator
 * --------------------------------------------------------------------------- */
intern void
block_alloc_init(struct block_allocator *a)
{
    list_init(&a->blks);
    list_init(&a->freelist);
}
intern struct memory_block*
block_alloc(struct block_allocator *a, int blksz)
{
    struct memory_block *blk = 0;
    assert(a);
    assert(a->mem);
    assert(a->mem->alloc);

    blksz = max(blksz, DEFAULT_MEMORY_BLOCK_SIZE);
    if (blksz == DEFAULT_MEMORY_BLOCK_SIZE && !list_empty(&a->freelist)) {
        /* allocate from freelist */
        blk = list_entry(a->freelist.next, struct memory_block, hook);
        list_del(&blk->hook);
    } else blk = qalloc(a->mem, blksz);
    assert(blk);

    /* setup block */
    zero(blk, szof(*blk));
    blk->size = blksz - szof(*blk);
    blk->base = cast(unsigned char*, (blk+1));

    /* add block into list */
    a->blkcnt++;
    list_init(&blk->hook);
    list_add_head(&a->blks, &blk->hook);
    return blk;
}
intern void
block_dealloc(struct block_allocator *a, struct memory_block *blk)
{
    assert(a && blk);
    if (!a || !blk) return;
    list_del(&blk->hook);
    if ((blk->size + szof(*blk)) == DEFAULT_MEMORY_BLOCK_SIZE)
        list_add_head(&a->freelist, &blk->hook);
    else qdealloc(a->mem, blk);
    a->blkcnt--;
}
intern void
free_blocks(struct block_allocator *a)
{
    struct list_hook *i, *n = 0;
    assert(a);
    assert(a->mem);
    assert(a->mem->alloc);

    list_foreach_s(i, n, &a->freelist) {
        struct memory_block *blk = 0;
        blk = list_entry(i, struct memory_block, hook);
        list_del(&blk->hook);
        qdealloc(a->mem, blk);
    }
    list_foreach_s(i, n, &a->blks) {
        struct memory_block *blk = 0;
        blk = list_entry(i, struct memory_block, hook);
        list_del(&blk->hook);
        qdealloc(a->mem, blk);
    } a->blkcnt = 0;
}
/* ---------------------------------------------------------------------------
 *                                  Arena
 * --------------------------------------------------------------------------- */
intern void
arena_grow(struct memory_arena *a, int minsz)
{
    /* allocate new memory block */
    int blksz = max(minsz, DEFAULT_MEMORY_BLOCK_SIZE);
    struct memory_block *blk = block_alloc(a->mem, blksz);
    assert(blk);

    /* link memory block into list */
    blk->prev = a->blk;
    a->blk = blk;
    a->blkcnt++;
}
api void*
arena_push(struct memory_arena *a, int cnt, int size, int align)
{
    int valid = 0;
    assert(a);
    if (!a) return 0;

    /* validate enough space */
    valid = a->blk && size_mul_add2_valid(size, cnt, a->blk->used, align);
    if (!valid || ((cnt*size) + a->blk->used + align) > a->blk->size) {
        if (!size_mul_add2_valid(size, cnt, szof(struct memory_block), align))
            return 0;
        arena_grow(a, cnt*size + szof(struct memory_block) + align);
    }
    /* allocate memory from block */
    align = max(align,1);
    {struct memory_block *blk = a->blk;
    unsigned char *raw = blk->base + blk->used;
    unsigned char *res = align(raw, align);

    blk->used += (cnt*size) + (res-raw);
    zero(res, (cnt*size));
    return res;}
}
intern void
arena_free_last_blk(struct memory_arena *a)
{
    struct memory_block *blk = a->blk;
    a->blk = blk->prev;
    block_dealloc(a->mem, blk);
    a->blkcnt--;
}
intern void
arena_clear(struct memory_arena *a)
{
    assert(a);
    if (!a) return;
    while (a->blk)
        arena_free_last_blk(a);
}
intern struct temp_memory
temp_memory_begin(struct memory_arena *a)
{
    struct temp_memory res;
    assert(a);
    res.used = a->blk ? a->blk->used: 0;
    res.blk = a->blk;
    res.arena = a;
    a->tmpcnt++;
    return res;
}
intern void
temp_memory_end(struct temp_memory tmp)
{
    struct memory_arena *a = tmp.arena;
    assert(a);
    while (a->blk != tmp.blk)
        arena_free_last_blk(a);
    if (a->blk) a->blk->used = tmp.used;
    a->tmpcnt--;
}
/* ---------------------------------------------------------------------------
 *                              Hash-Table
 * --------------------------------------------------------------------------- */
intern void
insert(struct table *tbl, uiid id, int val)
{
    uiid cnt = cast(uiid, tbl->cnt);
    uiid idx = id & (cnt-1), begin = idx;
    do {uiid key = tbl->keys[idx];
        if (key) continue;
        tbl->keys[idx] = id;
        tbl->vals[idx] = val; return;
    } while ((idx = ((idx+1) & (cnt-1))) != begin);
}
intern int
lookup(struct table *tbl, uiid id)
{
    uiid key, cnt = cast(uiid, tbl->cnt);
    uiid idx = id & (cnt-1), begin = idx;
    do {if (!(key = tbl->keys[idx])) return 0;
        if (key == id) return tbl->vals[idx];
    } while ((idx = ((idx+1) & (cnt-1))) != begin);
    return 0;
}
/* ---------------------------------------------------------------------------
 *                                  Box
 * --------------------------------------------------------------------------- */
api void
box_shrink(struct box *d, const struct box *s, int pad)
{
    assert(d && s);
    if (!d || !s) return;
    d->x = s->x + pad;
    d->y = s->y + pad;
    d->w = max(0, s->w - 2*pad);
    d->h = max(0, s->h - 2*pad);
}
api void
box_pad(struct box *d, const struct box *s, int padx, int pady)
{
    assert(d && s);
    if (!d || !s) return;
    d->x = s->x + padx;
    d->y = s->y + pady;
    d->w = max(0, s->w - 2*padx);
    d->h = max(0, s->h - 2*pady);
}
api int
box_intersect(const struct box *a, const struct box *b)
{
    return intersect(a->x, a->y, a->w, a->h, b->x, b->y, b->w, b->h);
}
/* ---------------------------------------------------------------------------
 *                                  Buffer
 * --------------------------------------------------------------------------- */
intern union param*
op_add(struct state *s, const union param *p, int cnt)
{
    int i = 0;
    assert(s && p);
    assert(cnt > 0);
    assert(cnt < MAX_OPS-2);

    {struct param_buffer *ob = s->opbuf;
    if ((s->op_idx + cnt) >= (MAX_OPS-2)) {
        /* allocate new param buffer */
        struct param_buffer *b = 0;
        b = arena_push(&s->arena, 1, szof(*ob), 0);
        ob->ops[s->op_idx + 0].op = OP_NEXT_BUF;
        ob->ops[s->op_idx + 1].p = b;
        s->opbuf = ob = b;
        s->op_idx = 0;
    } assert(s->op_idx + cnt < (MAX_OPS-2));

    /* copy params into buffer */
    for (i = 0; i < cnt; ++i)
        ob->ops[s->op_idx++] = p[i];
    return ob->ops + (s->op_idx - cnt);}
}
intern const char*
store(struct state *s, const char *str, int len)
{
    assert(s && str);
    assert(s->buf);
    assert(len < MAX_STR_BUF-1);

    {struct buffer *ob = s->buf;
    if ((s->buf_off + len) > MAX_STR_BUF-1) {
        /* allocate new data buffer */
        struct buffer *b = arena_push(&s->arena, 1, szof(*ob), 0);
        s->buf_off = 0;
        ob->next = b;
        s->buf = ob = b;
    } assert((s->buf_off + len) <= MAX_STR_BUF-1);
    copy(ob->buf + s->buf_off, str, len);

    /* store zero-terminated string */
    {int off = s->buf_off;
    ob->buf[s->buf_off + len] = 0;
    s->total_buf_size += len + 1;
    s->buf_off += len + 1;
    return ob->buf + off;}}
}
/* ---------------------------------------------------------------------------
 *                                  IDs
 * --------------------------------------------------------------------------- */
api void
pushid(struct state *s, unsigned id)
{
    struct idrange *idr = 0;
    assert(s);
    assert(s->stkcnt < cntof(s->idstk));
    if (!s || s->stkcnt >= cntof(s->idstk))
        return;

    idr = &s->idstk[s->stkcnt++];
    idr->base = id, idr->cnt = 0;
    s->lastid = (idr->base & 0xFFFFFFFFlu) << 32lu;
}
api void
setid(struct state *s, uiid id)
{
    assert(s);
    assert(s->stkcnt < cntof(s->idstk));
    if (!s) return;
    /* set one time only ID */
    s->idstate = ID_GEN_ONE_TIME;
    s->otid = id;
}
api uiid
genwid(struct state *s)
{
    uiid id = 0;
    assert(s);
    if (!s) return 0;

    /* generate widget ID */
    {int idx = max(0, s->stkcnt-1);
    struct idrange *idr = &s->idstk[idx];
    id |= (idr->base & 0xFFFFFFFFlu) << 32lu;
    id |= (++idr->cnt);
    s->lastid = id;
    return id;}
}
api uiid
genbid(struct state *s)
{
    /* generate box ID */
    switch (s->idstate) {
    case ID_GEN_DEFAULT:
        return genwid(s);
    case ID_GEN_ONE_TIME: {
        s->idstate = ID_GEN_DEFAULT;
        return s->otid;
    }}
}
api void
popid(struct state *s)
{
    assert(s);
    assert(s->stkcnt);
    if (!s || !s->stkcnt) return;
    s->stkcnt--;
}
/* ---------------------------------------------------------------------------
 *                                  Repository
 * --------------------------------------------------------------------------- */
api struct box*
find(struct repository *repo, uiid id)
{
    int idx = lookup(&repo->tbl, id);
    if (!idx) return 0;
    return repo->boxes + idx;
}

/* ---------------------------------------------------------------------------
 *                                  State
 * --------------------------------------------------------------------------- */
intern struct state*
state_find(struct context *ctx, uiid id)
{
    struct list_hook *i = 0;
    assert(ctx);
    if (!ctx) return 0;
    list_foreach(i, &ctx->states) {
        struct state *s = list_entry(i, struct state, hook);
        if (s->id == id) return s;
    } return 0;
}
api struct box*
polls(struct state *s, uiid id)
{
    struct repository *repo = 0;
    repo = s->repo;
    if (!repo) return 0;
    return find(repo, id);
}

/* ---------------------------------------------------------------------------
 *                                  Module
 * --------------------------------------------------------------------------- */
intern struct module*
module_find(struct context *ctx, mid id)
{
    struct list_hook *i = 0;
    assert(ctx);
    if (!ctx) return 0;
    list_foreach(i, &ctx->mod) {
        struct module *m = list_entry(i, struct module, hook);
        if (m->id == id) return m;
    } return 0;
}
api struct state*
module_begin(struct context *ctx, mid id,
    enum relationship rel, mid parent, mid owner, uiid bid)
{
    struct state *s = 0;
    assert(ctx);
    if (!ctx) return 0;

    /* pick up previous state if possible */
    s = state_find(ctx, id);
    if (s) {s->op_idx -= 2; return s;}
    s = arena_push_type(&ctx->arena, struct state);
    assert(s);
    if (!s) return 0;

    /* setup state */
    s->id = id;
    s->ctx = ctx;
    s->cfg = &ctx->cfg;
    s->arena.mem = &ctx->blkmem;
    s->param_list = s->opbuf = arena_push(&s->arena, 1, szof(struct param_buffer), 0);
    s->buf_list = s->buf = arena_push(&s->arena, 1, szof(struct buffer), 0);
    s->mod = module_find(ctx, id);
    if (s->mod) s->repo = s->mod->repo[s->mod->repoid];
    list_init(&s->hook);
    list_add_tail(&ctx->states, &s->hook);
    pushid(s, id);

    /* serialize api call */
    {union param p[8];
    p[0].op = OP_BUF_BEGIN;
    p[1].mid = id;
    p[2].op = OP_BUF_ULINK;
    p[3].mid = parent;
    p[4].id = bid;
    p[5].op = OP_BUF_CONECT;
    p[6].mid = owner;
    p[7].i = rel;
    op_add(s, p, cntof(p));}
    return s;
}
api void
module_end(struct state *s)
{
    assert(s);
    if (!s) return;
    {union param p[2];
    p[0].op = OP_BUF_END;
    p[1].id = s->id;
    op_add(s, p, cntof(p));}
    popid(s);

    assert(!s->wtop);
    assert(!s->depth);
}
api int
module_destroy(struct context *ctx, mid id)
{
    struct list_hook tmp;
    struct module *m = 0;
    struct list_hook *i = 0;
    struct list_hook *n = 0;
    struct repository *repo = 0;

    assert(ctx);
    assert(id);
    if (!ctx || !id)
        return 0;

    /* find and unlink module */
    m = module_find(ctx, id);
    if (!m) return 0;
    repo = m->repo[m->repoid];
    if (repo) {
        /* unlink root box */
        struct box *root = m->root;
        list_del(&root->node);
    }
    list_del(&m->hook);
    list_move_tail(&tmp, &m->hook);

    /* remove sub-tree */
    list_foreach_s(i, n, &ctx->mod) {
        struct list_hook *k = 0;
        struct module *sub = list_entry(i, struct module, hook);
        list_foreach(k, &tmp) {
            struct module *gb = list_entry(k, struct module, hook);
            if (sub->parent == gb) {
                list_move_tail(&tmp, &sub->hook);
                break;
            }
        }
    } list_splice_tail(&ctx->garbage, &tmp);
    return 1;
}
api struct state*
section_begin(struct state *s, mid id)
{
    assert(s);
    assert(id && "Section are not allowed to overwrite root");
    if (!s || !id) return 0;
    return module_begin(s->ctx, id, RELATIONSHIP_INDEPENDENT, s->id, s->id, s->lastid);
}
api void
section_end(struct state *s)
{
    module_end(s);
}
api void
link(struct state *s, mid id, enum relationship rel)
{
    assert(s);
    if (!s) return;
    {union param p[5];
    p[0].op = OP_BUF_DLINK;
    p[1].mid = id;
    p[2].op = OP_BUF_CONECT;
    p[3].mid = id;
    p[4].i = rel;
    op_add(s, p, cntof(p));}
}
api struct state*
begin(struct context *ctx, mid id)
{
    assert(ctx);
    if (!ctx) return 0;
    return module_begin(ctx, id, RELATIONSHIP_INDEPENDENT,
        0, 0, WIDGET_UI - WIDGET_LAYER_BEGIN);
}
api void
end(struct state *s)
{
    assert(s);
    if (!s) return;
    module_end(s);
}
api void
slot(struct state *s, uiid id)
{
    union param p[4];
    assert(s);
    assert(s->wtop > 0);
    if (!s || s->wtop <= 0) return;

    widget_begin(s, WIDGET_SLOT);
    p[0].op = OP_BOX_PUSH;
    p[1].id = id;
    p[2].id = s->wstk[s->wtop-1].id;
    p[3].op = OP_BOX_POP;
    op_add(s, p, cntof(p));
    widget_end(s);
}
/* ---------------------------------------------------------------------------
 *                                  Popup
 * --------------------------------------------------------------------------- */
api struct state*
popup_begin(struct state *s, mid id, enum popup_type type)
{
    unsigned bid = 0;
    switch (type) {
    case POPUP_BLOCKING:
        bid = WIDGET_POPUP-WIDGET_LAYER_BEGIN; break;
    case POPUP_NON_BLOCKING:
        bid = WIDGET_CONTEXTUAL-WIDGET_LAYER_BEGIN; break;}
    return module_begin(s->ctx, id, RELATIONSHIP_INDEPENDENT, 0, s->id, bid);
}
api void
popup_end(struct state *s)
{
    union param p[2];
    p[0].op = OP_BUF_END;
    p[1].id = s->id;
    op_add(s, p, cntof(p));
}
api struct box*
popup_find(struct context *ctx, mid id)
{
    struct module *m = module_find(ctx, id);
    if (!m || !m->root) return 0;
    return m->root;
}
intern int
popup_is_active(struct context *ctx, enum popup_type type)
{
    int active = 0;
    struct list_hook *i = 0;
    struct box *layer = 0;
    struct box *skip = 0;

    switch (type) {
    default: assert(layer != 0); break;
    case POPUP_BLOCKING:
        layer = ctx->popup;
        skip = ctx->contextual; break;
    case POPUP_NON_BLOCKING:
        layer = ctx->contextual;
        skip = ctx->unblocking; break;}

    assert(layer);
    list_foreach(i, &layer->lnks) {
        struct box *b = list_entry(i, struct box, node);
        if (b == skip) continue;
        if (!(b->flags & BOX_HIDDEN))
            return 1;
    } return active;
}
api void
popup_show(struct context *ctx, mid id, enum visibility vis)
{
    struct box *pop = popup_find(ctx, id);
    if (!pop) return;
    if (vis == VISIBLE) {
        pop->flags &= ~(unsigned)BOX_HIDDEN;
    } else pop->flags |= BOX_HIDDEN;
    ctx->blocking->flags |= BOX_IMMUTABLE;
}
/* ---------------------------------------------------------------------------
 *                                  Widget
 * --------------------------------------------------------------------------- */
api void
widget_begin(struct state *s, int type)
{
    struct widget *w;
    union param p[3], *q = 0;
    assert(s);
    if (!s) return;

    p[0].op = OP_WIDGET_BEGIN;
    p[1].type = type;
    p[2].i = 0;
    q = op_add(s, p, cntof(p));
    assert(q);

    assert(s->wtop < cntof(s->wstk));
    w = s->wstk + s->wtop++;
    w->id = genwid(s);
    w->argc = &q[2].i;
    w->type = type;
}
api void
widget_end(struct state *s)
{
    union param p[1];
    assert(s);
    assert(s->wtop > 0);
    if (!s || s->wtop <= 0) return;

    p[0].op = OP_WIDGET_END;
    op_add(s, p, cntof(p));
    s->wtop = max(s->wtop-1, 0);
}
api uiid
widget_box_push(struct state *s)
{
    union param p[3];
    assert(s);
    if (!s) return 0;

    p[0].op = OP_BOX_PUSH;
    p[1].id = genbid(s);
    p[2].id = s->wstk[s->wtop-1].id;
    op_add(s, p, cntof(p));

    s->depth++;
    s->boxcnt++;
    s->tree_depth = max(s->depth, s->tree_depth);
    return p[1].id;
}
api uiid
widget_box(struct state *s)
{
    uiid id = widget_box_push(s);
    widget_box_pop(s);
    return id;
}
api void
widget_box_pop(struct state *s)
{
    union param p[1];
    assert(s);
    if (!s) return;

    p[0].op = OP_BOX_POP;
    op_add(s, p, cntof(p));
    assert(s->depth);
    s->depth = max(s->depth-1, 0);
}
api void
widget_box_property_set(struct state *s, enum properties prop)
{
    union param p[2];
    p[0].op = OP_PROPERTY_SET;
    p[1].u = prop;
    op_add(s, p, cntof(p));
}
api void
widget_box_property_clear(struct state *s, enum properties prop)
{
    union param p[2];
    p[0].op = OP_PROPERTY_CLR;
    p[1].u = prop;
    op_add(s, p, cntof(p));
}
/* ---------------------------------------------------------------------------
 *                                  Parameter
 * --------------------------------------------------------------------------- */
intern union param*
widget_push_param(struct state *s, union param *p)
{
    struct widget *w = 0;
    assert(s->wtop > 0);
    w = &s->wstk[s->wtop-1];
    *w->argc +=1 ; s->argcnt++;
    return op_add(s, p, 2);
}
api float*
widget_param_float(struct state *s, float f)
{
    assert(s);
    if (!s) return 0;
    {union param p[2], *q;
    p[0].op = OP_PUSH_FLOAT;
    p[1].f = f;
    q = widget_push_param(s, p);
    return &q[1].f;}
}
api int*
widget_param_int(struct state *s, int i)
{
    assert(s);
    if (!s) return 0;
    {union param p[2], *q;
    p[0].op = OP_PUSH_INT;
    p[1].i = i;
    q = widget_push_param(s, p);
    return &q[1].i;}
}
api unsigned*
widget_param_uint(struct state *s, unsigned u)
{
    assert(s);
    if (!s) return 0;
    {union param p[2], *q;
    p[0].op = OP_PUSH_UINT;
    p[1].u = u;
    q = widget_push_param(s, p);
    return &q[1].u;}
}
api uiid*
widget_param_id(struct state *s, uiid id)
{
    assert(s);
    if (!s) return 0;
    {union param p[2], *q;
    p[0].op = OP_PUSH_ID;
    p[1].id = id;
    q = widget_push_param(s, p);
    return &q[1].id;}
}
api mid*
widget_param_mid(struct state *s, mid id)
{
    assert(s);
    if (!s) return 0;
    {union param p[2], *q;
    p[0].op = OP_PUSH_MID;
    p[1].mid = id;
    q = widget_push_param(s, p);
    return &q[1].mid;}
}
api const char*
widget_param_str(struct state *s, const char *str, int len)
{
    assert(s);
    if (!s) return 0;
    {union param p[2];
    p[0].op = OP_PUSH_STR;
    p[1].i = len + 1;
    widget_push_param(s, p);}
    return store(s, str, len);
}
api float*
widget_modifier_float(struct state *s, float *f)
{
    assert(s && f);
    assert(s->ctx);
    assert(s->ctx->active);
    assert(s->wtop > 0);
    assert(s->ctx);
    if (!s || !f) return 0;

    {struct repository *repo = s->repo;
    if (repo) {
        /* try to find previous state and set if box is active */
        const struct context *ctx = s->ctx;
        const struct box *act = ctx->active;
        struct widget w = s->wstk[s->wtop-1];
        if (act->wid == w.id && act->type == w.type)
            *f = act->params[*w.argc].f;
    } return widget_param_float(s, *f);}
}
api int*
widget_modifier_int(struct state *s, int *i)
{
    assert(s && i);
    assert(s->ctx);
    assert(s->ctx->active);
    assert(s->wtop > 0);
    assert(s->ctx);
    if (!s || !i) return 0;

    {struct repository *repo = s->repo;
    if (repo) {
        /* try to find previous state and set if box is active */
        const struct context *ctx = s->ctx;
        const struct box *act = ctx->active;
        struct widget w = s->wstk[s->wtop-1];
        if (act->wid == w.id && act->type == w.type)
            *i = act->params[*w.argc].i;
    } return widget_param_int(s, *i);}
}
api unsigned*
widget_modifier_uint(struct state *s, unsigned *u)
{
    assert(s && u);
    assert(s->ctx);
    assert(s->ctx->active);
    assert(s->wtop > 0);
    assert(s->ctx);
    if (!s || !u) return 0;

    {struct repository *repo = s->repo;
    if (repo) {
        /* try to find previous state and set if box is active */
        const struct context *ctx = s->ctx;
        const struct box *act = ctx->active;
        struct widget w = s->wstk[s->wtop-1];
        if (act->wid == w.id && act->type == w.type)
            *u = act->params[*w.argc].u;
    } return widget_param_uint(s, *u);}
}
api float*
widget_state_float(struct state *s, float f)
{
    const struct box *b;
    struct widget w;

    assert(s);
    assert(s->ctx);
    assert(s->wtop > 0);
    if (!s || s->wtop < 1) return 0;

    /* try to find and set previous state */
    w = s->wstk[s->wtop-1];
    b = polls(s, w.id + 1);
    if (!b || b->type != w.type)
        return widget_param_float(s, f);
    return widget_param_float(s, b->params[*w.argc].f);
}
api int*
widget_state_int(struct state *s, int i)
{
    const struct box *b = 0;
    struct widget w;

    assert(s);
    assert(s->ctx);
    assert(s->wtop > 0);
    if (!s || s->wtop < 1) return 0;

    /* try to find and set previous state */
    w = s->wstk[s->wtop-1];
    b = polls(s, w.id + 1);
    if (!b || b->type != w.type)
        return widget_param_int(s, i);
    return widget_param_int(s, b->params[*w.argc].i);
}
api unsigned*
widget_state_uint(struct state *s, unsigned u)
{
    const struct box *b;
    struct widget w;

    assert(s);
    assert(s->ctx);
    assert(s->wtop > 0);
    if (!s || s->wtop < 1) return 0;

    /* try to find and set previous state */
    w = s->wstk[s->wtop-1];
    b = polls(s, w.id + 1);
    if (!b || b->type != w.type)
        return widget_param_uint(s, u);
    return widget_param_uint(s, b->params[*w.argc].u);
}
api uiid*
widget_state_id(struct state *s, uiid u)
{
    const struct box *b;
    struct widget w;

    assert(s);
    assert(s->ctx);
    assert(s->wtop > 0);
    if (!s || s->wtop < 1) return 0;

    /* try to find and set previous state */
    w = s->wstk[s->wtop-1];
    b = polls(s, w.id + 1);
    if (!b || b->type != w.type)
        return widget_param_id(s, u);
    return widget_param_id(s, b->params[*w.argc].id);
}
api union param*
widget_get_param(struct box *b, int idx)
{
    assert(b);
    return b->params + idx;
}
api float*
widget_get_float(struct box *b, int idx)
{
    union param *p = 0;
    assert(b);
    p = widget_get_param(b, idx);
    return &p->f;
}
api int*
widget_get_int(struct box *b, int idx)
{
    union param *p = 0;
    assert(b);
    p = widget_get_param(b, idx);
    return &p->i;
}
api unsigned*
widget_get_uint(struct box *b, int idx)
{
    union param *p = 0;
    assert(b);
    p = widget_get_param(b, idx);
    return &p->u;
}
api uiid*
widget_get_id(struct box *b, int idx)
{
    union param *p = 0;
    assert(b);
    p = widget_get_param(b, idx);
    return &p->id;
}
api mid*
widget_get_mid(struct box *b, int idx)
{
    union param *p = 0;
    assert(b);
    p = widget_get_param(b, idx);
    return &p->mid;
}
api const char*
widget_get_str(struct box *b, int idx)
{
    union param *p = 0;
    assert(b);
    p = widget_get_param(b, idx);
    return b->buf + p[0].i;
}
/* ---------------------------------------------------------------------------
 *                                  Process
 * --------------------------------------------------------------------------- */
intern struct box**
bfs(struct box **buf, struct box *root)
{
    struct list_hook *i = 0;
    unsigned long head = 0, tail = 1;
    struct box **que = buf; que[tail] = root;
    while (head < tail) {
        struct box *b = que[++head];
        if (b != root && b->type == WIDGET_TREE_ROOT)
            continue;
        list_foreach(i, &b->lnks)
            que[++tail] = list_entry(i,struct box,node);
    } return que+1;
}
intern int
dfs(struct box **buf, struct box **stk, struct box *root)
{
    int tail = 0;
    unsigned long head = 0;
    struct list_hook *i = 0;

    stk[head++] = root;
    while (head > 0) {
        struct box *b = stk[--head];
        buf[tail++] = b;
        list_foreach_rev(i, &b->lnks) {
            struct box *s = list_entry(i,struct box,node);
            if (s->flags & BOX_HIDDEN /*|| !box_intersect(b, s) */) continue;
            stk[head++] = s;
        }
    } return tail;
}
intern void
operation_begin(union process *p, enum process_type type,
    struct context *ctx, struct memory_arena *arena)
{
    assert(p);
    assert(ctx);
    assert(arena);
    memset(p, 0, sizeof(*p));

    p->type = type;
    list_init(&p->input.evts);
    p->hdr.tmp = temp_memory_begin(arena);
    p->hdr.arena = arena;
    p->hdr.ctx = ctx;
}
intern void
operation_end(union process *p)
{
    assert(p);
    if (!p) return;
    temp_memory_end(p->hdr.tmp);
}
api void
box_blueprint(struct box *b, int padx, int pady)
{
    assert(b);
    if (!b) return;
    {struct list_hook *i = 0;
    list_foreach(i, &b->lnks) {
        const struct box *n = list_entry(i, struct box, node);
        b->dw = max(b->dw, n->dw + 2*padx);
        b->dh = max(b->dh, n->dh + 2*pady);
    }}
}
api void
box_layout(struct box *b, int pad)
{
    assert(b);
    if (!b) return;
    {struct list_hook *i = 0;
    list_foreach(i, &b->lnks) {
        struct box *n = list_entry(i, struct box, node);
        n->x = b->x + pad;
        n->y = b->y + pad;
        n->w = max(b->w - 2*pad, 0);
        n->h = max(b->h - 2*pad, 0);
    }}
}
intern struct box*
at(struct box *b, int mx, int my)
{
    struct list_hook *i = b->lnks.prev;
    while (1) {
        while (&b->lnks != i) {
            struct box *sub = list_entry(i, struct box, node);
            if (!(sub->flags & BOX_IMMUTABLE) && !(sub->flags & BOX_HIDDEN)) {
                if (inbox(mx, my, sub->x, sub->y, sub->w, sub->h)) {
                    b = sub;
                    i = b->lnks.prev;
                    continue;
                }
            } i = i->prev;
        }
        if (b->flags & BOX_UNSELECTABLE){
            i = b->node.prev;
            b = b->parent;
        } else break;
    } return b;
}
intern void
event_add_box(union event *evt, struct box *box)
{
    assert(evt);
    assert(box);
    assert(evt->hdr.cnt < evt->hdr.cap);
    if (evt->hdr.cnt >= evt->hdr.cap) return;
    evt->hdr.boxes[evt->hdr.cnt++] = box;
}
intern union event*
event_begin(union process *p, enum event_type type, struct box *orig)
{
    union event *res = 0;
    assert(p);
    assert(orig);

    /* allocate event and link into list */
    res = arena_push_type(p->hdr.arena, union event);
    assert(res);
    list_init(&res->hdr.hook);
    list_add_tail(&p->input.evts, &res->hdr.hook);

    /* setup event */
    res->type = type;
    res->hdr.input = p->input.state;
    res->hdr.origin = orig;
    res->hdr.cap = orig->tree_depth + 1;
    res->hdr.boxes = arena_push_array(p->hdr.arena, res->hdr.cap, struct box*);
    event_add_box(res, orig);
    return res;
}
intern void
event_end(union event *evt)
{
    unused(evt);
}
intern struct list_hook*
it(struct list_hook *list, struct list_hook *iter)
{
    /* list iteration */
    if (list_empty(list)) return 0;
    else if (iter && iter->next == list)
        iter = 0;
    else if (!iter)
        iter = list->next;
    else iter = iter->next;
    return iter;
}
intern struct list_hook*
itr(struct list_hook *list, struct list_hook *iter)
{
    /* list reverse iteration */
    if (list_empty(list)) return 0;
    else if (iter && iter->prev == list)
        iter = 0;
    else if (!iter)
        iter = list->prev;
    else iter = iter->prev;
    return iter;
}
api union process*
process_begin(struct context *ctx, unsigned flags)
{
    /* repository member object alignment */
    static const int uint_align = alignof(unsigned);
    static const int uiid_align = alignof(uiid);
    static const int int_align = alignof(int);
    static const int ptr_align = alignof(void*);
    static const int box_align = alignof(struct box);
    static const int param_align = alignof(union param);
    static const int repo_align = alignof(struct repository);

    /* repository member object size */
    static const int int_size = szof(int);
    static const int ptr_size = szof(void*);
    static const int uint_size = szof(unsigned);
    static const int uiid_size = szof(uiid);
    static const int box_size = szof(struct box);
    static const int param_size = szof(union param);
    static const int repo_size = szof(struct repository);

    #define jmpto(ctx,s) do{(ctx)->state = (s); goto r;}while(0)
    enum processing_states {
        STATE_DISPATCH,
        STATE_COMMIT, STATE_CALC_REQ_MEMORY, STATE_ALLOC_PERSISTENT,
        STATE_ALLOC_TEMPORARY, STATE_COMPILE,
        STATE_BLUEPRINT, STATE_LAYOUTING, STATE_TRANSFORM,
        STATE_INPUT, STATE_PAINT,
        STATE_TRACE_DISPATCH, STATE_TRACE,
        STATE_SERIALIZE, STATE_SERIALIZE_DISPATCH,
        STATE_SERIALIZE_TABLE, STATE_SERIALIZE_BINARY,
        STATE_CLEAR, STATE_GC, STATE_CLEANUP, STATE_DONE
    };
    static const struct opdef {
        enum opcodes type;
        int argc;
        const char *name;
        const char *fmt;
        const char *str;
    } opdefs[] = {
        #define OP(a,b,c) {OP_ ## a, b, #a, c, #a " " c},
        OPCODES(OP)
        #undef OP
        {OPCNT,0,0}
    };
    int i = 0;
    assert(ctx);
    if (!ctx) return 0;
    r:switch (ctx->state) {
    case STATE_DISPATCH: {
        if (flags & flag(PROC_TRACE))
            jmpto(ctx, STATE_TRACE_DISPATCH);
        else if (flags & flag(PROC_CLEANUP))
            jmpto(ctx, STATE_CLEANUP);
        else if (flags & flag(PROC_CLEAR))
            jmpto(ctx, STATE_GC);
        else if (flags & flag(PROC_COMMIT))
            jmpto(ctx, STATE_COMMIT);
        else jmpto(ctx, STATE_DONE);
    }
    case STATE_COMMIT: {
        struct state *s = 0;
        union process *p = &ctx->proc;
        ctx->iter = it(&ctx->states, ctx->iter);
        if (!ctx->iter) {
            if (!list_empty(&ctx->states)) {
                struct list_hook *it = 0;
                list_foreach(it, &ctx->mod) {
                    struct module *m = list_entry(it, struct module, hook);
                    struct repository *r = m->repo[m->repoid];
                    for (i = 0; i < r->boxcnt; ++i) {
                        /* reset box input state */
                        struct box *b = r->boxes + i;
                        b->hovered = b->entered = b->exited = 0;
                        b->drag_end = b->moved = 0;
                        b->pressed = b->released = 0;
                        b->clicked = b->scrolled = 0;
                        b->drag_begin = b->dragged = 0;
                        /* calculate box tree depth */
                        b->tree_depth = cast(unsigned short, b->depth + r->tree_depth);
                    }
                }
            }
            /* free states */
            {struct list_hook *si = 0;
            list_foreach(si, &ctx->states) {
                s = list_entry(si, struct state, hook);
                arena_clear(&s->arena);
            } list_init(&ctx->states);}

            /* state transition table */
            if ((flags & flag(PROC_BLUEPRINT)) && ctx->unbalanced)
                jmpto(ctx, STATE_BLUEPRINT);
            else if (flags & flag(PROC_INPUT))
                jmpto(ctx, STATE_INPUT);
            else if (flags & flag(PROC_PAINT))
                jmpto(ctx, STATE_PAINT);
            else if (flags & flag(PROC_SERIALIZE))
                jmpto(ctx, STATE_SERIALIZE);
            else jmpto(ctx, STATE_DONE);
        }
        s = list_entry(ctx->iter, struct state, hook);
        if (s->mod) {
            s->mod->seq = ctx->seq;
            jmpto(ctx, STATE_CALC_REQ_MEMORY);
        }
        /* allocate new module for state */
        operation_begin(p, PROC_ALLOC, ctx, &ctx->arena);
        p->mem.size = szof(struct module);
        ctx->state = STATE_ALLOC_PERSISTENT;
        return p;
    }
    case STATE_ALLOC_PERSISTENT: {
        struct state *s = list_entry(ctx->iter, struct state, hook);
        union process *p = &ctx->proc;
        if (!p->mem.ptr) return p;
        assert(p->mem.ptr);

        /* setup new module */
        {struct module *m = cast(struct module*, p->mem.ptr);
        assert(type_aligned(m, struct module));
        zero(m, szof(*m));
        list_init(&m->hook);
        m->id = s->id;
        m->seq = ctx->seq;
        m->size = szof(struct module);
        s->mod = m;
        list_add_tail(&ctx->mod, &m->hook);}
        jmpto(ctx, STATE_CALC_REQ_MEMORY);
        return p;
    }
    case STATE_CALC_REQ_MEMORY: {
        struct state *s = list_entry(ctx->iter, struct state, hook);
        union process *p = &ctx->proc;
        operation_begin(p, PROC_ALLOC_FRAME, ctx, &ctx->arena);

        s->boxcnt++;
        s->tblcnt = cast(int, cast(float, s->boxcnt) * 1.35f);
        s->tblcnt = npow2(s->tblcnt);

        /* calculate required repository memory */
        p->mem.size = s->total_buf_size;
        p->mem.size += box_align + param_align;
        p->mem.size += repo_size + repo_align;
        p->mem.size += int_align + uint_align + uiid_align;
        p->mem.size += ptr_size * (s->boxcnt + 1);
        p->mem.size += box_size * s->boxcnt;
        p->mem.size += uint_size * s->boxcnt;
        p->mem.size += uiid_size * s->tblcnt;
        p->mem.size += int_size * s->tblcnt;
        p->mem.size += param_size * s->argcnt;
        jmpto(ctx, STATE_ALLOC_TEMPORARY);
    }
    case STATE_ALLOC_TEMPORARY: {
        union process *p = &ctx->proc;
        if (p->mem.ptr)
            jmpto(ctx, STATE_COMPILE);
        return p;
    }
    case STATE_COMPILE: {
        union process *p = &ctx->proc;
        struct state *s = list_entry(ctx->iter, struct state, hook);
        struct module *m = s->mod;

        struct repository *old = m->repo[m->repoid];
        struct repository *repo = p->mem.ptr;
        zero(repo, szof(repo));
        m->repoid = !m->repoid;
        m->repo[m->repoid] = repo;

        /* I.) Setup repository memory layout */
        repo->boxcnt = s->boxcnt;
        repo->boxes = (struct box*)align(repo + 1, box_align);
        repo->bfs = (struct box**)align(repo->boxes + repo->boxcnt, ptr_align);
        repo->tbl.cnt = s->tblcnt;
        repo->tbl.keys = (uiid*)align(repo->bfs + repo->boxcnt + 1, uiid_align);
        repo->tbl.vals = (int*)align(repo->tbl.keys + repo->tbl.cnt, int_align);
        repo->params = (union param*)align(repo->tbl.vals + repo->tbl.cnt, param_align);
        repo->buf = (char*)(repo->params + s->argcnt);
        repo->depth = s->tree_depth + 1;
        repo->tree_depth = s->tree_depth + 1;
        repo->bufsiz = s->total_buf_size;
        repo->boxcnt = 1;
        repo->size = p->mem.size;

        /* setup sub-tree root box */
        m->root = repo->boxes;
        m->root->buf = repo->buf;
        m->root->params = repo->params;
        m->root->flags = 0;
        m->root->type = WIDGET_TREE_ROOT;
        list_init(&m->root->node);
        list_init(&m->root->lnks);

        /* II.) Setup repository data */
        {struct gizmo {int type, params, argi, argc;} gstk[MAX_TREE_DEPTH];
        struct box *boxstk[MAX_TREE_DEPTH];
        int gtop = 0, depth = 1;
        int buf_off = 0, buf_size = 0;

        struct buffer *buf = s->buf_list;
        struct param_buffer *ob = s->param_list;
        union param *op = &ob->ops[s->op_begin];
        boxstk[0] = repo->boxes;
        while (1) {
            switch (op[0].op) {
            /* ---------------------------- Buffer -------------------------- */
            case OP_BUF_END:
                goto eol0;
            case OP_NEXT_BUF:
                ob = (struct param_buffer*)op[1].p;
                op = ob->ops;
                continue;
            case OP_BUF_BEGIN: {
                assert(op[1].mid == s->id);
            } break;
            case OP_BUF_ULINK: {
                /* find parent module */
                struct repository *prepo = 0;
                struct module *pm = module_find(ctx, op[1].mid);
                if (!pm) break;
                prepo = pm->repo[pm->repoid];
                if (!prepo) break;

                /* link module root box into parent module box */
                {int idx = lookup(&prepo->tbl, op[2].id);
                struct box *pb = prepo->boxes + idx;
                struct box *b = m->root;
                list_del(&b->node);
                list_add_tail(&pb->lnks, &b->node);
                repo->tree_depth = prepo->tree_depth + pb->depth + 1;
                b->parent = pb;}

                /* relink child module directly after parent */
                list_del(&m->hook);
                list_add_tail(&ctx->mod, &m->hook);
                m->parent = pm;
            } break;
            case OP_BUF_DLINK: {
                /* find child module */
                struct repository *crepo = 0;
                struct module *cm = module_find(ctx, op[1].mid);
                assert(cm);
                if (!cm || !cm->root) break;
                crepo = cm->repo[cm->repoid];
                assert(crepo);
                if (!crepo) break;

                /* link referenced module root box into current box in module */
                assert(depth > 0);
                {struct box *pb = boxstk[depth-1];
                struct box *b = cm->root;
                list_del(&b->node);
                list_add_tail(&pb->lnks, &b->node);
                crepo->tree_depth = repo->tree_depth + depth + 1;
                b->parent = pb;}

                /* relink child module directly after parent */
                list_del(&cm->hook);
                list_add_tail(&ctx->mod, &cm->hook);
                cm->parent = m;
            } break;
            case OP_BUF_CONECT: {
                /* setup lifetime connection */
                struct module *pm = module_find(ctx, op[1].mid);
                assert(pm);
                if (!pm) break;
                m->owner = pm;
                m->rel = (enum relationship)op[2].i;
            } break;

            /* --------------------------- Widgets -------------------------- */
            case OP_WIDGET_BEGIN: {
                /* push new widet on stack */
                struct gizmo *g = &gstk[gtop++];
                assert(gtop < MAX_TREE_DEPTH);
                g->type = op[1].type;
                g->params = repo->argcnt;
                g->argi = 0, g->argc = op[2].i;
                repo->argcnt += g->argc;
            } break;
            case OP_WIDGET_END:
                assert(gtop > 0); gtop--; break;

            /* -------------------------- Parameter ------------------------- */
            case OP_PUSH_STR:
            case OP_PUSH_FLOAT:
            case OP_PUSH_INT:
            case OP_PUSH_UINT:
            case OP_PUSH_ID:
            case OP_PUSH_MID: {
                struct gizmo *g = &gstk[max(0,gtop-1)];
                assert(gtop > 0);
                assert(g->argi < g->argc);
                if (g->argi >= g->argc) break;

                {int idx = g->params + g->argi++;
                switch (op[0].op) {
                    case OP_PUSH_STR: {
                        int len = op[1].i;
                        repo->params[idx].i = buf_off;
                        assert(buf_off < repo->bufsiz);
                        if (buf_off + len > MAX_STR_BUF) {
                            assert(buf->next);
                            buf = buf->next;
                            buf_off = 0;
                        } copy(repo->buf + buf_size, buf->buf + buf_off, len);
                        buf_size += len;
                        buf_off += len;
                    } break;
                    case OP_PUSH_FLOAT:
                        repo->params[idx].f = op[1].f; break;
                    case OP_PUSH_INT:
                        repo->params[idx].i = op[1].i; break;
                    case OP_PUSH_UINT:
                        repo->params[idx].u = op[1].u; break;
                    case OP_PUSH_ID:
                        repo->params[idx].id = op[1].id; break;
                    case OP_PUSH_MID:
                        repo->params[idx].mid = op[1].mid; break;
                }}
            } break;

            /* ---------------------------- Boxes ----------------------------*/
            case OP_BOX_POP:
                assert(depth > 1); depth--; break;
            case OP_BOX_PUSH: {
                struct gizmo *g = 0;
                uiid id = op[1].id;
                int idx = repo->boxcnt++;
                struct box *pb = boxstk[depth-1];
                insert(&repo->tbl, id, idx);
                assert(gtop > 0);
                g = &gstk[gtop-1];

                /* setup box */
                {struct box *b = repo->boxes + idx;
                b->id = id;
                b->flags = 0;
                b->parent = pb;
                b->type = g->type;
                b->wid = op[2].id;
                b->buf = repo->buf;
                b->params = repo->params + g->params;
                b->depth = cast(unsigned short, depth);

                /* link box into parent */
                list_init(&b->node);
                list_init(&b->lnks);

                /* update tracked hot boxes */
                list_add_tail(&pb->lnks, &b->node);
                if (b->id == ctx->active->id)
                    ctx->active = b;
                if (b->id == ctx->origin->id)
                    ctx->origin = b;
                if (b->id == ctx->hot->id)
                    ctx->hot = b;

                /* push box into stack */
                assert(depth < MAX_TREE_DEPTH);
                boxstk[depth++] = b;}
            } break;
            /* -------------------------- Properties -------------------------*/
            case OP_PROPERTY_SET: {
                assert(depth > 0);
                {struct box *b = boxstk[depth-1];
                b->flags |= op[1].u;}
            } break;
            case OP_PROPERTY_CLR: {
                assert(depth > 0);
                {struct box *b = boxstk[depth-1];
                b->flags &= ~op[1].u;}
            } break;}
            op += opdefs[op[0].op].argc + 1;
        } eol0:;}

        repo->bfs = bfs(repo->bfs, repo->boxes);
        ctx->unbalanced = 1;
        if (old) list_del(&old->boxes[0].node);
        jmpto(ctx, STATE_COMMIT);
        return p;
    }
    case STATE_BLUEPRINT: {
        struct module *m = 0;
        struct repository *r = 0;
        union process *p = &ctx->proc;

        ctx->iter = itr(&ctx->mod, ctx->iter);
        if (!ctx->iter) {
            /* state transition table */
            if (flags & flag(PROC_LAYOUT))
                jmpto(ctx, STATE_LAYOUTING);
            else jmpto(ctx, STATE_DONE);
        } m = list_entry(ctx->iter, struct module, hook);
        assert(m);
        r = m->repo[m->repoid];
        assert(r);

        /* blueprint request of current module repository */
        operation_begin(p, PROC_BLUEPRINT, ctx, &ctx->arena);
        p->layout.repo = r;
        p->layout.end = p->layout.inc = -1;
        p->layout.begin = r->boxcnt-1;
        p->layout.boxes = r->bfs;
        return p;
    }
    case STATE_LAYOUTING: {
        struct module *m = 0;
        struct repository *r = 0;
        union process *p = &ctx->proc;

        ctx->iter = it(&ctx->mod, ctx->iter);
        if (!ctx->iter) {
            ctx->unbalanced = 0;
            /* state transition table */
            if (flags & flag(PROC_INPUT))
                jmpto(ctx, STATE_INPUT);
            else if (flags & flag(PROC_PAINT))
                jmpto(ctx, STATE_PAINT);
            else if (flags & flag(PROC_SERIALIZE))
                jmpto(ctx, STATE_SERIALIZE);
            else jmpto(ctx, STATE_DONE);
        } m = list_entry(ctx->iter, struct module, hook);
        assert(m);
        r = m->repo[m->repoid];
        assert(r);

        /* layout request of current module repository */
        operation_begin(p, PROC_LAYOUT, ctx, &ctx->arena);
        p->layout.end = max(0,r->boxcnt);
        p->layout.begin = 0, p->layout.inc = 1;
        p->layout.boxes = r->bfs;
        p->layout.repo = r;
        return p;
    }
    case STATE_INPUT: {
        union process *p = &ctx->proc;
        struct input *in = &ctx->input;
        if (in->resized) {
            /* window resize */
            struct box *root = ctx->tree;
            int w = root->w, h = root->h;
            root->w = in->width;
            root->h = in->height;

            in->resized = 0;
            if (w != in->width || h != in->height)
                jmpto(ctx, STATE_BLUEPRINT);
        } operation_begin(p, PROC_INPUT, ctx, &ctx->arena);
        p->input.state = in;

        in->mouse.dx = in->mouse.x - in->mouse.lx;
        in->mouse.dy = in->mouse.y - in->mouse.ly;
        if (in->mouse.dx || in->mouse.dy) {
            /* motion - entered, exited, dragged, moved */
            int mx = in->mouse.x, my = in->mouse.y;
            int lx = in->mouse.lx, ly = in->mouse.ly;

            struct box *last = ctx->hot;
            ctx->hot = at(ctx->tree, mx, my);
            if (ctx->hot != last) {
                /* hot - entered, exited */
                union event *evt;
                struct box *prev = last;
                struct box *cur = ctx->hot;

                {const struct box *c = cur, *l = prev;
                cur->entered = !inbox(lx, ly, c->x, c->y, c->w, c->h);
                prev->exited = !inbox(mx, my, l->x, l->y, l->w, l->h);}

                /* exited */
                evt = event_begin(p, EVT_EXITED, prev);
                evt->entered.cur = ctx->hot;
                evt->entered.last = last;
                while ((prev = prev->parent)) {
                    const struct box *l = prev;
                    int was = inbox(lx, ly, l->x, l->y, l->w, l->h);
                    int isnt = !inbox(mx, my, l->x, l->y, l->w, l->h);
                    prev->exited = was && isnt;
                    event_add_box(evt, prev);
                } event_end(evt);

                /* entered */
                evt = event_begin(p, EVT_ENTERED, cur);
                evt->exited.cur = ctx->hot;
                evt->exited.last = last;
                while ((cur = cur->parent)) {
                    const struct box *c = cur;
                    int wasnt = !inbox(lx, ly, c->x, c->y, c->w, c->h);
                    int is = inbox(mx, my, c->x, c->y, c->w, c->h);
                    cur->entered = wasnt && is;
                    event_add_box(evt, cur);
                } event_end(evt);
            }
            if (ctx->active == ctx->origin) {
                /* dragged, moved */
                struct box *a = ctx->active;
                union event *evt = 0;
                struct box *act = 0;

                /* moved */
                if ((a->flags & BOX_MOVABLE_X) || (a->flags & BOX_MOVABLE_Y)) {
                    a->moved = 1;
                    if (a->flags & BOX_MOVABLE_X)
                        a->x += in->mouse.dx;
                    if (a->flags & BOX_MOVABLE_Y)
                        a->y += in->mouse.dy;
                    ctx->unbalanced = 1;

                    evt = event_begin(p, EVT_MOVED, act = a);
                    evt->moved.x = in->mouse.dx;
                    evt->moved.y = in->mouse.dy;
                    while ((act = act->parent))
                        event_add_box(evt, act);
                    event_end(evt);
                }
                /* dragged */
                evt = event_begin(p, EVT_DRAGGED, act = a);
                evt->dragged.x = in->mouse.dx;
                evt->dragged.y = in->mouse.dy;
                while ((act = act->parent))
                    event_add_box(evt, act);
                event_end(evt);
                a->dragged = 1;
            }
            /* reset */
            in->mouse.dx = in->mouse.dy = 0;
            in->mouse.lx = in->mouse.x;
            in->mouse.ly = in->mouse.y;
        }
        for (i = 0; i < cntof(in->mouse.btn); ++i) {
            /* button - pressed, released, clicked */
            struct key *btn = in->mouse.btn + i;
            struct box *a = ctx->active;
            struct box *o = ctx->origin;
            struct box *h = ctx->hot;
            union event *evt = 0;
            struct box *act = 0;

            if (!btn->transitions) continue;
            ctx->active = (btn->down && btn->transitions) ? ctx->hot: ctx->active;
            ctx->origin = (btn->down && btn->transitions) ? ctx->hot: ctx->tree;

            /* pressed */
            h->pressed = btn->down && btn->transitions;
            if (h->pressed) {
                evt = event_begin(p, EVT_PRESSED, act = h);
                evt->pressed.x = in->mouse.x;
                evt->pressed.y = in->mouse.y;
                while ((act = act->parent)) {
                    event_add_box(evt, act);
                    act->pressed = 1;
                } event_end(evt);

                /* drag_begin */
                evt = event_begin(p, EVT_DRAG_BEGIN, act = h);
                evt->drag_begin.x = in->mouse.x;
                evt->drag_begin.y = in->mouse.y;
                while ((act = act->parent))
                    event_add_box(evt, act);
                event_end(evt);
                a->drag_begin = 1;
            }
            /* released */
            h->released = !btn->down && btn->transitions;
            if (h->released) {
                evt = event_begin(p, EVT_RELEASED, act = a);
                evt->released.x = in->mouse.x;
                evt->released.y = in->mouse.y;
                while ((act = act->parent)) {
                    event_add_box(evt, act);
                    act->released = 1;
                } event_end(evt);

                if (ctx->hot == ctx->active) {
                    /* clicked */
                    a->clicked = 1;
                    evt = event_begin(p, EVT_CLICKED, act = a);
                    evt->clicked.x = in->mouse.x;
                    evt->clicked.y = in->mouse.y;
                    while ((act = act->parent)) {
                        event_add_box(evt, act);
                        act->clicked = 1;
                    } event_end(evt);
                }
                /* drag_end */
                evt = event_begin(p, EVT_DRAG_END, act = o);
                evt->drag_end.x = in->mouse.x;
                evt->drag_end.y = in->mouse.y;
                while ((act = act->parent))
                    event_add_box(evt, act);
                event_end(evt);
                o->drag_end = 1;
            } btn->transitions = 0;
        }
        if (in->mouse.wheelx || in->mouse.wheely) {
            /* scroll */
            union event *evt = 0;
            struct box *a = ctx->active;
            evt = event_begin(p, EVT_SCROLLED, a);
            evt->scroll.x = in->mouse.wheelx;
            evt->scroll.y = in->mouse.wheely;
            while ((a = a->parent)) {
                event_add_box(evt, a);
                a->scrolled = 1;
            } event_end(evt);
            in->mouse.wheelx = 0;
            in->mouse.wheely = 0;
        }
        for (i = 0; i < cntof(in->keys); ++i) {
            /* key - up, down */
            union event *evt = 0;
            struct box *a = ctx->active;
            if (!in->keys[i].transitions)
                continue;

            evt = event_begin(p, EVT_KEY, a);
            evt->key.pressed = in->keys[i].down && in->keys[i].transitions;
            evt->key.released = !in->keys[i].down && in->keys[i].transitions;
            evt->key.shift = in->shift;
            evt->key.super = in->super;
            evt->key.ctrl = in->ctrl;
            evt->key.alt = in->alt;
            evt->key.code = i;
            while ((a = a->parent))
                event_add_box(evt, a);
            event_end(evt);
            in->keys[i].transitions = 0;
        }
        for (i = 0; i < cntof(in->shortcuts); ++i) {
            /* shortcuts */
            struct key *key = in->shortcuts + i;
            union event *evt = 0;
            struct box *a = ctx->active;
            if (!key->transitions)
                continue;

            evt = event_begin(p, EVT_SHORTCUT, a);
            evt->key.pressed = key->down && key->transitions;
            evt->key.released = !key->down && key->transitions;
            evt->key.shift = in->shift;
            evt->key.super = in->super;
            evt->key.ctrl = in->ctrl;
            evt->key.alt = in->alt;
            evt->key.code = i;
            while ((a = a->parent))
                event_add_box(evt, a);
            event_end(evt);
            in->shortcuts[i].transitions = 0;
        }
        if (in->text_len) {
            /* text */
            struct box *a = ctx->active;
            union event *evt = event_begin(p, EVT_TEXT, a);
            evt->text.buf = in->text;
            evt->text.len = in->text_len;
            evt->text.shift = in->shift;
            evt->text.super = in->super;
            evt->text.ctrl = in->ctrl;
            evt->text.alt = in->alt;
            while ((a = a->parent))
                event_add_box(evt, a);
            event_end(evt);
        }
        /* hovered */
        {struct box *h = ctx->hot;
        int mx = in->mouse.x, my = in->mouse.y;
        do if (inbox(mx, my, h->x, h->y, h->w, h->h))
            h->hovered = 1;
        while ((h = h->parent));}

        /* state transition table */
        if (ctx->unbalanced && (flags & PROCESS_BLUEPRINT))
            ctx->state = STATE_BLUEPRINT;
        else if (flags & flag(PROC_PAINT)) {
            if (list_empty(&p->input.evts)) {
                operation_end(p);
                jmpto(ctx, STATE_PAINT);
            } ctx->state = STATE_PAINT;
        } else if (flags & flag(PROC_SERIALIZE))
            ctx->state = STATE_SERIALIZE;
        else if (flags & flag(PROC_CLEAR))
            ctx->state = STATE_CLEAR;
        else ctx->state = STATE_DONE;
        return p;
    }
    case STATE_PAINT: {
        int depth = 0;
        struct box **stk = 0;
        struct list_hook *pi = 0;
        union process *p = &ctx->proc;
        struct temp_memory tmp;

        operation_begin(p, PROC_PAINT, ctx, &ctx->arena);
        p->paint.boxes = 0;
        p->paint.cnt = 0;

        /* calculate tree depth and number of boxes */
        p->paint.cnt = depth = 0;
        list_foreach(pi, &ctx->mod) {
            struct module *m = list_entry(pi, struct module, hook);
            struct repository *r = m->repo[m->repoid];
            depth = max(depth, r->tree_depth + r->depth);
            p->paint.cnt += r->boxcnt;
        }
        /* generate list of boxes in DFS-order */
        p->paint.boxes = arena_push_array(&ctx->arena, p->paint.cnt+1, struct box*);
        tmp = temp_memory_begin(&ctx->arena);
        stk = arena_push_array(&ctx->arena, depth + 1, struct box*);
        p->paint.cnt = dfs(p->paint.boxes, stk, ctx->tree);
        temp_memory_end(tmp);

        /* state transition table */
        if (flags & flag(PROC_SERIALIZE))
            ctx->state = STATE_SERIALIZE;
        else if (flags & flag(PROC_CLEAR))
            ctx->state = STATE_CLEAR;
        else ctx->state = STATE_DONE;
        return p;
    }
    case STATE_TRACE_DISPATCH: {
        union process *p = &ctx->proc;
        operation_begin(p, PROC_FILL_TRACE_CONFIG, ctx, &ctx->arena);
        ctx->state = STATE_TRACE;
        return p;
    }
    case STATE_TRACE: {
        struct list_hook *si = 0;
        union process *p = &ctx->proc;
        FILE *fp = p->trace.file;
        if (!fp) return p;

        list_foreach(si, &ctx->states) {
            /* iterate all states */
            struct state *s = list_entry(si, struct state, hook);
            union param *op = &s->param_list->ops[s->op_begin];
            fprintf(fp, "State: " MIDFMT "\n", s->id);
            while (1) {
                const struct opdef *def = opdefs + op->type;
                switch (op->type) {
                case OP_NEXT_BUF:
                    op = (union param*)op[1].p; break;
                default: {
                    /* print out each argument from string format */
                    union param *param = op;
                    const char *str = def->str;
                    while (*str) {
                        if (*str != '%') {
                            fputc(*str, fp);
                            str++; continue;
                        } str++;

                        param++;
                        assert(param - op <= def->argc);
                        switch (*str++) {default: break;
                        case 'f': fprintf(fp, "%g", param[0].f); break;
                        case 'd': fprintf(fp, "%d", param[0].i); break;
                        case 'u': fprintf(fp, "%u", param[0].u); break;
                        case 'p': fprintf(fp, "%p", param[0].p); break;}
                    } fputc('\n', fp);
                    if (op[0].op == OP_BUF_END && op[1].mid == s->id)
                        goto eot;
                }} op += def->argc + 1;
            } eot:break;
        } jmpto(ctx, STATE_DONE);
    }
    case STATE_SERIALIZE: {
        union process *p = &ctx->proc;
        operation_begin(p, PROC_FILL_SERIAL_CONFIG, ctx, &ctx->arena);
        ctx->state = STATE_SERIALIZE_DISPATCH;
        return p;
    }
    case STATE_SERIALIZE_DISPATCH: {
        union process *p = &ctx->proc;
        if (!p->serial.file)
            jmpto(ctx, STATE_SERIALIZE);
        switch (p->serial.type) {
        case SERIALIZE_BINARY:
            jmpto(ctx, STATE_SERIALIZE_BINARY);
        case SERIALIZE_TABLES:
            jmpto(ctx, STATE_SERIALIZE_TABLE);}
    }
    case STATE_SERIALIZE_BINARY: {
        union process *p = &ctx->proc;
        FILE *fp = p->serial.file;
        struct list_hook *si = 0;
        assert(fp);

        list_foreach(si, &ctx->states) {
            struct state *s = list_entry(si, struct state, hook);
            union param *op = &s->param_list->ops[s->op_begin];
            while (1) {
                const struct opdef *def = opdefs + op->type;
                switch (op->type) {
                case OP_NEXT_BUF:
                    op = (union param*)op[1].p; break;
                default: {
                    for (i = 0; i < def->argc; ++i)
                        fwrite(&op[i], sizeof(op[i]), 1, fp);
                    if (op[0].op == OP_BUF_END && op[1].mid == s->id)
                        goto eob;
                }} op += def->argc;
            } eob: break;
        }
        if (flags & flag(PROCESS_CLEAR))
            jmpto(ctx, STATE_CLEAR);
        else jmpto(ctx, STATE_DONE);
    }
    case STATE_SERIALIZE_TABLE: {
        /* generate box flags */
        static const struct property_def {
            const char *name; int len;
        } property_info[] = {
            #define PROP(p) {"BOX_" #p, (cntof("BOX_" #p)-1)},
                PROPERTY_MAP(PROP)
            #undef PROP
        }; const union process *p = &ctx->proc;
        const struct process_serialize *ps = &p->serial;
        FILE *fp = ps->file;

        /* I.) Dump each repository into C compile time tables */
        struct list_hook *it = 0;
        list_foreach(it, &ctx->mod) {
            struct module *m = list_entry(it, struct module, hook);
            const struct repository *r = m->repo[m->repoid];
            if (!m->id) continue; /* skip root */

            assert(fp);
            fprintf(fp, "static const struct element g_%u_elements[] = {\n", m->id);
            for (i = 0; i < r->boxcnt; ++i) {
                const struct box *b = r->boxes + i;
                const struct box *pb = b->parent;
                uiid pid = (pb && i) ? pb->id: 0;
                char buf[256]; int j, n = 0; buf[n++] = '0';
                for (j = 0; j < PROPERTY_INDEX_MAX; ++j) {
                    if (b->flags & flag(j)) {
                        const struct property_def *pi = 0;
                        pi = property_info + j;
                        buf[n++] = '|';
                        copy(buf+n, pi->name, pi->len);
                        n += pi->len;
                    } buf[n] = 0;
                } fprintf(fp, "    {%d, " IDFMT "lu, " IDFMT "lu, " IDFMT "lu, %d, %d, %s, %u, %u},\n",
                    b->type, b->id, pid, b->wid, b->depth, b->tree_depth, buf,
                    (unsigned)(b->params - r->params), (unsigned)(b->buf - r->buf));
            } fprintf(fp, "};\n");
            fprintf(fp, "static const uiid g_%u_tbl_keys[%d] = {\n%*s", m->id, r->tbl.cnt, ps->indent, "");
            for (i = 0; i < r->tbl.cnt; ++i) {
                if (i && !(i & 0x07)) fprintf(fp, "\n%*s", ps->indent, "");
                fprintf(fp, IDFMT"lu", r->tbl.keys[i]);
                if (i < r->tbl.cnt-1) fputc(',', fp);
            } fprintf(fp, "\n};\n");
            fprintf(fp, "static const int g_%u_tbl_vals[%d] = {\n%*s",m->id, r->tbl.cnt, ps->indent, "");
            for (i = 0; i < r->tbl.cnt; ++i) {
                if (i && !(i & 0x0F)) fprintf(fp, "\n%*s", ps->indent, "");
                fprintf(fp, "%d", r->tbl.vals[i]);
                if (i + 1 < r->tbl.cnt) fputc(',', fp);
            } fprintf(fp, "\n};\n");
            fprintf(fp, "static union param g_%u_params[%d] = {\n%*s", m->id, r->argcnt, ps->indent, "");
            for (i = 0; i < r->argcnt; ++i) {
                if (i && !(i & 0x7)) fprintf(fp, "\n%*s", ps->indent, "");
                fprintf(fp, "{"IDFMT"lu}", r->params[i].id);
                if (i + 1 < r->argcnt) fputc(',', fp);
            } fprintf(fp, "\n};\n");
            fprintf(fp, "static const unsigned char g_%u_data[%d] = {\n%*s", m->id, r->bufsiz, ps->indent, "");
            for (i = 0; i < r->bufsiz; ++i) {
                unsigned char c = cast(unsigned char,r->buf[i]);
                if (i && !(i & 0x0F)) fprintf(fp, "\n%*s", ps->indent, "");
                fprintf(fp, "0x%02x", c);
                if (i + 1 < r->bufsiz) fputc(',', fp);
            } fprintf(fp, "\n};\n");
            fprintf(fp, "static struct box *g_%u_bfs[%d];\n", m->id, r->boxcnt+1);
            fprintf(fp, "static struct box g_%u_boxes[%d];\n", m->id, r->boxcnt);
            fprintf(fp, "static struct module g_%u_module;\n", m->id);
            fprintf(fp, "static struct repository g_%u_repo;\n", m->id);
            fprintf(fp, "static const struct component g_%u_component = {\n", m->id);
            fprintf(fp, "%*s%d," MIDFMT ", %d, %d,", ps->indent, "", VERSION, m->id, r->depth, r->tree_depth);
            fprintf(fp, "g_%u_elements, cntof(g_%u_elements),\n", m->id, m->id);
            fprintf(fp, "%*sg_%u_tbl_vals, g_%u_tbl_keys,", ps->indent, "", m->id, m->id);
            fprintf(fp, "cntof(g_%u_tbl_keys),\n", m->id);
            fprintf(fp, "%*sg_%u_data, cntof(g_%u_data),\n", ps->indent, "", m->id, m->id);
            fprintf(fp, "%*sg_%u_params, cntof(g_%u_params),\n", ps->indent, "", m->id, m->id);
            fprintf(fp, "%*s&g_%u_module, &g_%u_repo, ", ps->indent, "", m->id, m->id);
            fprintf(fp, "g_%u_boxes,\n%*sg_%u_bfs, ", m->id, ps->indent, "", m->id);
            fprintf(fp, "cntof(g_%u_boxes)\n", m->id);
            fprintf(fp, "};\n");
        }
        /* II.) Dump each module into C compile time tables */
        fprintf(fp, "static const struct container g_%s_containers[] = {\n", p->serial.name);
        list_foreach(it, &ctx->mod) {
            struct module *m = list_entry(it, struct module, hook);
            if (!m->id || !m->parent || !m->owner) continue; /* skip root */
            assert(m->parent);
            assert(m->owner);
            fprintf(fp, "%*s{%u, %u, "IDFMT", %u, %d, &g_%u_component},\n",
                ps->indent, "", m->id, m->parent->id, m->root->parent->id,
                m->owner->id, m->rel, m->id);
        } fprintf(fp, "};\n");

        /* state transition table */
        if (flags & flag(PROCESS_CLEAR))
            jmpto(ctx, STATE_CLEAR);
        else jmpto(ctx, STATE_DONE);
    }
    case STATE_GC: {
        struct module *m = 0;
        union process *p = &ctx->proc;

        /* free each modules old data */
        do {ctx->iter = it(&ctx->mod, ctx->iter);
            if (!ctx->iter) {
                /* free not updated dependend modules */
                struct list_hook *h = 0;
                d:list_foreach(h, &ctx->mod) {
                    m = list_entry(h, struct module, hook);
                    if (m->rel != RELATIONSHIP_DEPENDENT) continue;

                    {struct module *pm = m->owner;
                    if (pm->seq == m->seq) continue;
                    module_destroy(ctx, m->id);
                    goto d;}
                } jmpto(ctx, STATE_CLEAR);
            } m = list_entry(ctx->iter, struct module, hook);
        } while (!m->repo[!m->repoid]);

        /* free old repository */
        assert(m->repo[!m->repoid]);
        if (m->repo[!m->repoid]->size) {
            operation_begin(p, PROC_FREE_FRAME, ctx, &ctx->arena);
            p->mem.ptr = m->repo[!m->repoid];
            m->repo[!m->repoid] = 0;
            return p;
        } else {
            m->repo[!m->repoid] = 0;
            jmpto(ctx, STATE_GC);
        }
    };
    case STATE_CLEAR: {
        /* free deleted module */
        struct module *m = 0;
        union process *p = &ctx->proc;
        if (list_empty(&ctx->garbage)) {
            /* start new frame */
            list_init(&ctx->garbage);
            arena_clear(&ctx->arena);
            if (flags & flag(PROC_FULL_CLEAR))
                free_blocks(&ctx->blkmem);
            ctx->seq++;
            jmpto(ctx, STATE_DONE);
        }
        /* free temporary frame repository memory */
        ctx->iter = ctx->garbage.next;
        m = list_entry(ctx->iter, struct module, hook);
        if (m->repo[m->repoid] && m->repo[m->repoid]->size) {
            operation_begin(p, PROC_FREE_FRAME, ctx, &ctx->arena);
            p->mem.ptr = m->repo[m->repoid];
            m->repo[m->repoid] = 0;
            return p;
        } m->repo[m->repoid] = 0;

        if (m->repo[!m->repoid] && m->repo[!m->repoid]->size) {
            operation_begin(p, PROC_FREE_FRAME, ctx, &ctx->arena);
            p->mem.ptr = m->repo[!m->repoid];
            m->repo[!m->repoid] = 0;
            return p;
        } m->repo[!m->repoid] = 0;
        list_del(&m->hook);

        /* free persistent module memory */
        if (m->size) {
            operation_begin(p, PROC_FREE, ctx, &ctx->arena);
            p->mem.ptr = m;
            return p;
        } else jmpto(ctx, STATE_CLEAR);
    }
    case STATE_CLEANUP: {
        /* free all memory */
        list_del(&ctx->root.hook);
        list_splice_tail(&ctx->garbage, &ctx->mod);
        jmpto(ctx, STATE_GC);
    }
    case STATE_DONE: {
        ctx->state = STATE_DISPATCH;
        return 0;
    }} return 0;
}
api void
process_end(union process *p)
{
    assert(p);
    assert(p->hdr.ctx);
    if (!p) return;

    switch (p->type) {
    case PROC_INPUT: {
        struct context *ctx = p->hdr.ctx;
        struct input *in = &ctx->input;
        const int blk = popup_is_active(ctx, POPUP_BLOCKING);
        const int nblk = popup_is_active(ctx, POPUP_NON_BLOCKING);
        if (!blk && !nblk)
            ctx->blocking->flags &= ~(unsigned)BOX_IMMUTABLE;
        else ctx->blocking->flags |= BOX_IMMUTABLE;
        in->text_len = 0;
    } break;}
    operation_end(p);
}
api void
blueprint(union process *op, struct box *b)
{
    assert(b);
    assert(op);
    assert(op->hdr.ctx);
    if (!b || !op) return;

    {struct context *ctx = op->hdr.ctx;
    if (b->type & WIDGET_INTERNAL_BEGIN) {
        switch (b->type) {
        case WIDGET_ROOT: {
            b->dw = ctx->input.width;
            b->dh = ctx->input.height;
        } break;
        case WIDGET_TREE_ROOT:
        case WIDGET_SLOT: {
            box_blueprint(b,0,0);
        } break;}
    } else box_blueprint(b,0,0);}
}
intern void
layout_default(struct box *b)
{
    struct list_hook *i = 0;
    list_foreach(i, &b->lnks) {
        struct box *n = 0;
        n = list_entry(i, struct box, node);
        n->x = b->x, n->y = b->y;
        n->w = b->w, n->h = b->h;
    }
}
api void
layout(union process *op, struct box *b)
{
    assert(b);
    assert(op);
    assert(op->hdr.ctx);
    if (!b || !op) return;

    {struct context *ctx = op->hdr.ctx;
    if (!(b->type & WIDGET_INTERNAL_BEGIN))
        {box_layout(b, 0); return;}

    switch (b->type) {
    case WIDGET_TREE_ROOT:
    case WIDGET_SLOT:
        layout_default(b); break;
    case WIDGET_ROOT:
        b->w = ctx->input.width;
        b->h = ctx->input.height;
        layout_default(b); break;
    case WIDGET_OVERLAY:
    case WIDGET_UNBLOCKING:
    case WIDGET_BLOCKING:
        layout_default(b); break;
    case WIDGET_UI:
        layout_default(b); break;
    case WIDGET_POPUP:
    case WIDGET_CONTEXTUAL: {
        struct list_hook *i = 0;
        list_foreach(i, &b->lnks) {
            struct box *n = list_entry(i, struct box, node);
            if (n != ctx->contextual && n != ctx->unblocking) {
                n->w = min(n->dw, (b->x + b->w)-n->x);
                n->h = min(n->dh, (b->y + b->h)-n->y);
            } else n->w = b->w, n->h = b->h;
        }
    } break;}}
}
api void
input(union process *op, union event *evt, struct box *b)
{
    assert(b);
    assert(op);
    assert(evt);
    assert(op->hdr.ctx);
    if (!b || !evt || !op) return;

    if (!(b->type & WIDGET_INTERNAL_BEGIN)) return;
    switch (b->type) {
    case WIDGET_UNBLOCKING: {
        struct context *ctx = 0;
        struct list_hook *i = 0;
        struct box *contextual = 0;

        if (evt->hdr.origin != b) break;
        if (!b->clicked || evt->type != EVT_CLICKED) break;
        ctx = op->hdr.ctx;

        /* hide all contextual menus */
        contextual = ctx->contextual;
        assert(contextual);
        list_foreach(i, &contextual->lnks) {
            struct box *c = list_entry(i,struct box,node);
            if (c == b) continue;
            c->flags |= BOX_HIDDEN;
        }
    } break;}
}
/* ---------------------------------------------------------------------------
 *                                  Context
 * --------------------------------------------------------------------------- */
api struct box*
query(struct context *ctx, unsigned mid, uiid id)
{
    struct module *m = 0;
    struct repository *repo = 0;

    m = module_find(ctx, mid);
    if (!m) return 0;
    repo = m->repo[m->repoid];
    if (!repo) return 0;
    return find(repo, id);
}
api void
load(struct context *ctx, const struct container *cons, int cnt)
{
    int i = 0;
    assert(ctx);
    assert(cons);
    assert(cnt >= 0);
    if (!cons || !ctx || cnt < 0)
        return;

    ctx->unbalanced = 1;
    for (i = 0; i < cnt; ++i) {
        const struct container *con = cons + i;
        const struct component *c = con->comp;
        struct repository *repo = 0;
        struct module *m = 0;

        /* validate */
        assert(c->bfs);
        assert(c->boxes);
        assert(c->tbl_keys);
        assert(c->tbl_vals);
        assert(VERSION == c->version);
        if (!c || VERSION != c->version || !ctx) continue;
        if (!c->bfs || !c->boxes || !c->tbl_keys || !c->tbl_vals)
            continue;

        /* setup module */
        m = c->module;
        zero(m, szof(*m));
        m->id = con->id;
        m->root = c->boxes;
        m->parent = module_find(ctx, con->parent);
        m->owner = module_find(ctx, con->owner);
        m->rel = (enum relationship)con->rel;
        m->repo[m->repoid] = c->repo;
        list_init(&m->hook);
        list_add_tail(&ctx->mod, &m->hook);

        /* setup repository */
        repo = c->repo;
        zero(repo, szof(*repo));
        repo->depth = c->depth;
        repo->tree_depth = c->tree_depth;
        repo->boxes = c->boxes;
        repo->boxcnt = c->boxcnt;
        repo->bfs = c->bfs;
        repo->tbl.cnt = c->tblcnt;
        repo->tbl.keys = cast(uiid*, c->tbl_keys);
        repo->tbl.vals = cast(int*, c->tbl_vals);
        repo->params = cast(union param*, c->params);
        repo->buf = cast(char*, c->buf);
        repo->argcnt = c->paramcnt;
        repo->bufsiz = c->bufsiz;
        repo->size = 0;

        /* setup root node */
        list_init(&m->root->node);
        if (m->parent) {
            struct module *pm = m->parent;
            struct repository *pr = pm->repo[pm->repoid];
            struct box *rb = find(pr, con->parent_box);
            m->root->parent = rb;
            repo->tree_depth = rb->tree_depth;
            list_add_tail(&rb->lnks, &m->root->node);
        }
        /* setup tree */
        for (i = 0; i < c->elmcnt; ++i) {
            const struct element *e = c->elms + i;
            struct box *b = repo->boxes + lookup(&repo->tbl, e->id);
            struct box *p = repo->boxes + lookup(&repo->tbl, e->parent);

            /* setup tree node */
            list_init(&b->lnks);
            if (b != p) {
                b->parent = p;
                list_init(&b->node);
                list_add_tail(&p->lnks, &b->node);
            }
            /* setup box */
            b->id = e->id;
            b->wid = e->wid;
            b->type = e->type;
            b->flags = e->flags;
            b->depth = e->depth;
            b->tree_depth = e->tree_depth;
            b->params = repo->params + e->params;
            b->buf = repo->buf + e->state;
        } repo->bfs = bfs(repo->bfs, repo->boxes);
    }
}
api void
reset(struct context *ctx)
{
    assert(ctx);
    if (!ctx) return;
    ctx->active = ctx->boxes;
    ctx->origin = ctx->boxes;
    ctx->hot = ctx->boxes;
}
api int
init(struct context *ctx, const struct allocator *a, const struct config *cfg)
{
    assert(ctx);
    assert(cfg);
    if (!ctx || !cfg) return 0;

    memset(ctx, 0, sizeof(*ctx));
    a = (a) ? a: &default_allocator;
    zero(ctx, szof(*ctx));
    ctx->cfg = *cfg;

    /* setup allocators */
    ctx->mem = *a;
    ctx->blkmem.mem = &ctx->mem;
    ctx->arena.mem = &ctx->blkmem;
    block_alloc_init(&ctx->blkmem);

    /* setup lists */
    list_init(&ctx->mod);
    list_init(&ctx->states);
    list_init(&ctx->garbage);

    /* setup root module */
    {struct component root = {0};
    struct container cont = {0};
    compiler_assert(cntof(g_root_elements) <= cntof(ctx->boxes));
    memcpy(&root, &g_root, sizeof(root));
    root.boxcnt = cntof(g_root_elements);
    root.boxes = ctx->boxes;
    root.module = &ctx->root;
    root.repo = &ctx->repo;
    root.bfs = ctx->bfs;
    cont.comp = &root;
    load(ctx, &cont, 1);}

    /* setup direct root boxes pointers */
    ctx->tree = ctx->boxes + (WIDGET_ROOT - WIDGET_LAYER_BEGIN);
    ctx->overlay = ctx->boxes + (WIDGET_OVERLAY - WIDGET_LAYER_BEGIN);
    ctx->popup = ctx->boxes + (WIDGET_POPUP - WIDGET_LAYER_BEGIN);
    ctx->contextual = ctx->boxes + (WIDGET_CONTEXTUAL - WIDGET_LAYER_BEGIN);
    ctx->unblocking = ctx->boxes + (WIDGET_UNBLOCKING - WIDGET_LAYER_BEGIN);
    ctx->blocking = ctx->boxes + (WIDGET_BLOCKING - WIDGET_LAYER_BEGIN);
    ctx->ui = ctx->boxes + (WIDGET_UI - WIDGET_LAYER_BEGIN);
    reset(ctx);
    return 1;
}
api struct context*
create(const struct allocator *a, const struct config *cfg)
{
    struct context *ctx = 0;
    a = (a) ? a: &default_allocator;
    ctx = qalloc(a, szof(*ctx));
    assert(ctx);
    if (!ctx) return 0;
    init(ctx, a, cfg);
    return ctx;
}
api void
destroy(struct context *ctx)
{
    assert(ctx);
    if (!ctx) return;
    qdealloc(&ctx->mem, ctx);
}
api void
clear(struct context *ctx)
{
    union process *p = 0;
    assert(ctx);
    if (!ctx) return;
    while ((p = process_begin(ctx, PROCESS_CLEAR))) {
        switch (p->type) {
        case PROC_FREE_FRAME:
        case PROC_FREE: free(p->mem.ptr); break;}
        process_end(p);
    }
}
api void
cleanup(struct context *ctx)
{
    union process *p = 0;
    assert(ctx);
    if (!ctx) return;
    while ((p = process_begin(ctx, PROCESS_CLEANUP))) {
        switch (p->type) {
        case PROC_FREE_FRAME:
        case PROC_FREE: free(p->mem.ptr); break;}
        process_end(p);
    } destroy(ctx);
}
api void
generate(FILE *fp, struct context *ctx, const char *name)
{
    union process *p = 0;
    assert(ctx);
    assert(fp);
    assert(name);
    if (!ctx || !fp || !name)
        return;

    while ((p = process_begin(ctx, PROCESS_SERIALIZE))) {
        struct process_serialize *serial = &p->serial;
        serial->type = SERIALIZE_TABLES;
        serial->file = fp;
        serial->name = name;
        serial->indent = 4;
        process_end(p);
    }
}
api void
commit(struct context *ctx)
{
    union process *p = 0;
    while ((p = process_begin(ctx, PROCESS_COMMIT))) {
        switch (p->type) {default: break;
        case PROC_ALLOC_FRAME:
        case PROC_ALLOC: p->mem.ptr = calloc((size_t)p->mem.size,1); break;}
        process_end(p);
    }
}
api void
trace(struct context *ctx, FILE *fp)
{
    union process *p = 0;
    assert(ctx);
    assert(fp);
    if (!ctx || !fp) return;
    while ((p = process_begin(ctx, PROCESS_TRACE))) {
        struct process_trace *t = &p->trace;
        t->file = fp;
        process_end(p);
    }
}
/* ---------------------------------------------------------------------------
 *                                  Input
 * --------------------------------------------------------------------------- */
api void
input_resize(struct context *ctx, int w, int h)
{
    struct input *in = 0;
    assert(ctx);
    if (!ctx) return;

    in = &ctx->input;
    in->resized = 1;
    in->width = w;
    in->height = h;
}
api void
input_motion(struct context *ctx, int x, int y)
{
    struct input *in = 0;
    assert(ctx);
    if (!ctx) return;

    in = &ctx->input;
    in->mouse.x = x;
    in->mouse.y = y;
}
api void
input_key(struct context *ctx, int key, int down)
{
    struct input *in = 0;
    assert(ctx);
    if (!ctx) return;
    in = &ctx->input;
    if (key < 0 || key >= cntof(in->keys) ||
        (in->keys[key].down == down))
        return;

    in->keys[key].transitions++;
    in->keys[key].down = !!down;
}
api void
input_button(struct context *ctx, enum mouse_button idx, int down)
{
    struct input *in = 0;
    struct key *btn = 0;
    assert(ctx);

    if (!ctx) return;
    in = &ctx->input;
    if (in->mouse.btn[idx].down == down)
        return;

    btn = in->mouse.btn + idx;
    btn->down = !!down;
    btn->transitions++;
}
api void
input_shortcut(struct context *ctx, int shortcut, int down)
{
    struct input *in = 0;
    assert(ctx);
    if (!ctx) return;

    in = &ctx->input;
    assert(shortcut < cntof(in->shortcuts));
    if (in->shortcuts[shortcut].down == down)
        return;

    in->shortcuts[shortcut].transitions++;
    in->shortcuts[shortcut].down = !!down;
}
api void
input_scroll(struct context *ctx, int x, int y)
{
    struct input *in = 0;
    assert(ctx);
    if (!ctx) return;

    in = &ctx->input;
    in->mouse.wheelx += x;
    in->mouse.wheely += y;
}
api void
input_text(struct context *ctx, const char *buf, const char *end)
{
    int len = 0;
    struct input *in = 0;
    assert(ctx);
    if (!ctx || !buf) return;
    in = &ctx->input;

    len = (end) ? cast(int, end-buf): (int)strn(buf);
    if (in->text_len + len + 1 >= cntof(in->text))
        return;

    copy(in->text + in->text_len, buf, len);
    in->text_len += len;
    in->text[in->text_len] = 0;
}
api void
input_char(struct context *ctx, char c)
{
    input_text(ctx, &c, &c+1);
}
api void
input_rune(struct context *ctx, unsigned long r)
{
    int len = 0;
    char buf[UTF_SIZE];
    assert(ctx);
    if (!ctx) return;
    len = utf_encode(buf, UTF_SIZE, r);
    input_text(ctx, buf, buf + len);
}