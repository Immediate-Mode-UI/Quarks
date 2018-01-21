/* ===========================================================================
 *
 *                                  LIBRARY
 *
 * =========================================================================== */
#include <assert.h> /* assert */
#include <stdlib.h> /* malloc, free */
#include <string.h> /* memcpy, memset */
#include <stdint.h> /* uint64_t */
#include <limits.h> /* INT_MAX */
#include <stdio.h> /* fprintf, fputc */

#undef min
#undef max
#define api static
#define intern static
#define unused(a) ((void)a)
#define cast(t,p) ((t)(p))
#define flag(n) ((1u)<<(n))
#define szof(a) ((int)sizeof(a))
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define clamp(a,v,b) (max(min(b,v),a))
#define zero(d,sz) memset(d,0,(size_t)(sz))
#define copy(d,s,sz) memcpy(d,s,(size_t)(sz))
#define cntof(a) ((int)(sizeof(a)/sizeof((a)[0])))
#define offsof(st,m) ((int)((uintptr_t)&(((st*)0)->m)))
#define containerof(ptr,type,member) (type*)((void*)((char*)(1?(ptr):&((type*)0)->member)-offsof(type, member)))
#define alignof(t) ((int)((char*)(&((struct {char c; t _h;}*)0)->_h) - (char*)0))
#define align(x,mask) ((void*)(((intptr_t)((const char*)(x)+(mask-1))&~(mask-1))))
#define between(x,a,b) ((a)<=(x) && (x)<=(b))
#define inbox(px,py,x,y,w,h) (between(px,x,x+w) && between(py,y,y+h))
#define intersect(x,y,w,h,X,Y,W,H) ((x)<(X)+(W) && (x)+(w)>(X) && (y)<(Y)+(H) && (y)+(h)>(Y))
#define strjoini(a, b) a ## b
#define strjoind(a, b) strjoini(a,b)
#define strjoin(a, b) strjoind(a,b)
#define uniqid(name) strjoin(name, __LINE__)
#define compiler_assert(exp) typedef char uniqid(_compile_assert_array)[(exp)?1:-1]

/* list */
struct list_hook {
    struct list_hook *next;
    struct list_hook *prev;
};
/* memory */
#define DEFAULT_ALLOCATOR 0
#define DEFAULT_MEMORY_BLOCK_SIZE (16*1024)
typedef void(*dealloc_f)(void *usr, void *data, const char *file, int line);
typedef void*(*alloc_f)(void *usr, int size, const char *file, int line);
struct allocator {
    void *usr;
    alloc_f alloc;
    dealloc_f dealloc;
};
struct memory_block {
    struct memory_block *prev;
    struct list_hook hook;
    int size, used;
    unsigned char *base;
};
struct block_allocator {
    const struct allocator *mem;
    struct list_hook freelist;
    struct list_hook blks;
    int blkcnt;
};
struct temp_memory {
    struct memory_arena *arena;
    struct memory_block *blk;
    int used;
};
struct memory_arena {
    struct block_allocator *mem;
    struct memory_block *blk;
    int blkcnt, tmpcnt;
};

/* input */
enum mouse_button {
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_COUNT
};
struct key {
    unsigned char down;
    unsigned char transitions;
};
struct mouse {
    int x, y;
    int lx, ly;
    int dx, dy;
    int wheelx, wheely;
    struct key btn[MOUSE_BUTTON_COUNT];
};
struct input {
    struct mouse mouse;
    char text[32];
    int text_len;
    int width, height;
    unsigned resized:1;
    unsigned ctrl:1;
    unsigned shift:1;
    unsigned alt:1;
    unsigned super:1;
    struct key shortcuts[256];
    struct key keys[512];
};

/* parameter */
union param {
    int op;
    int type;
    int i;
    unsigned u;
    unsigned id;
    unsigned flags;
    float f;
    void *p;
};

/* compile-time */
struct element {
    int type;
    unsigned id;
    unsigned parent;
    unsigned wid;
    unsigned flags;
    unsigned params;
};
struct component {
    unsigned version;
    unsigned id;
    int depth;

    const struct element *elms;
    const int elmcnt;
    const unsigned *tbl_vals;
    const unsigned *tbl_keys;
    const int tblcnt;

    struct module *module;
    struct repository *repo;
    struct box *boxes;
    struct box **bfs;
    int boxcnt;
};

/* box */
struct rect {int x,y,w,h;};
struct transform {int x,y; float sx,sy;};
#define PROPERTY_MAP(PROP)\
    PROP(INTERACTIVE)\
    PROP(MOVABLE_X)\
    PROP(MOVABLE_Y)\
    PROP(BACKGROUND)\
    PROP(UNIFORM)\
    PROP(SELECTABLE)\
    PROP(HIDDEN)
enum property_index {
#define PROP(p) BOX_ ## p ## _INDEX,
    PROPERTY_MAP(PROP)
#undef PROP
    PROPERTY_INDEX_MAX
};
enum properties {
#define PROP(p) BOX_ ## p = flag(BOX_ ## p ## _INDEX),
    PROPERTY_MAP(PROP)
#undef PROP
    PROPERTY_ALL
};
struct box {
    unsigned id;
    unsigned wid;
    int type;
    unsigned flags;
    union param *params;

    /* transform */
    int dw, dh;
    struct transform tloc, tscr;
    struct rect loc, scr;

    /* state */
    unsigned hidden:1;
    unsigned clicked:1;
    unsigned pressed:1;
    unsigned released:1;
    unsigned entered:1;
    unsigned exited:1;
    unsigned drag_begin:1;
    unsigned dragged:1;
    unsigned drag_end:1;
    unsigned moved:1;
    unsigned scrolled:1;

    /* node */
    int depth, cnt;
    struct list_hook node;
    struct list_hook lnks;
    struct box *parent;
};

/* state */
#define MAX_IDSTACK 256
#define MAX_TREE_DEPTH 128
#define MAX_OPS ((int)((DEFAULT_MEMORY_BLOCK_SIZE - sizeof(struct memory_block)) / sizeof(union param)))
struct repository;
struct idrange {
    unsigned base;
    unsigned cnt;
};
struct widget {
    unsigned id;
    int argcnt;
};
struct param_buffer {
    union param ops[MAX_OPS];
};
struct state {
    unsigned id;
    struct module *mod;
    struct context *ctx;
    struct memory_arena arena;
    struct repository *repo;
    struct list_hook hook;

    int argcnt;
    int tblcnt;
    int tree_depth;
    int boxcnt;

    int op_begin, op_idx;
    struct param_buffer *list;
    struct param_buffer *opbuf;
    const struct component *tbl;

    int depth;
    int stkcnt;
    unsigned lastid;
    struct idrange idstk[MAX_IDSTACK];
    struct widget wstk[MAX_TREE_DEPTH];
    int wtop;
};

/* module */
struct table {
    int cnt;
    unsigned *keys;
    int *vals;
};
struct repository {
    struct table tbl;
    struct box *boxes;
    struct box **bfs;
    int boxcnt, depth;
    union param *params;
    int argcnt;
    int size;
};
struct module {
    struct list_hook hook;
    unsigned id;
    struct box *root;
    int repoid;
    struct repository *repo[2];
};

/* event */
enum event_type {
    EVT_CLICKED,
    EVT_PRESSED,
    EVT_RELEASED,
    EVT_ENTERED,
    EVT_EXITED,
    EVT_DRAG_BEGIN,
    EVT_DRAGGED,
    EVT_DRAG_END,
    EVT_MOVED,
    EVT_KEY,
    EVT_TEXT,
    EVT_SCROLLED,
    EVT_SHORTCUT,
    EVT_COUNT
};
struct event_header {
    enum event_type type;
    int cap, cnt;
    struct box *origin;
    struct box **boxes;
    struct list_hook hook;
};
struct event_entered_exited {
    struct event_header hdr;
    struct box *last;
    struct box *cur;
};
struct event_int2 {
    struct event_header hdr;
    int x, y;
};
struct event_key {
    struct event_header hdr;
    int code;
    unsigned pressed:1;
    unsigned released:1;
    unsigned ctrl:1;
    unsigned shift:1;
    unsigned alt:1;
    unsigned super:1;
};
struct event_text {
    struct event_header hdr;
    char *buf;
    int len;
    unsigned ctrl:1;
    unsigned shift:1;
    unsigned alt:1;
    unsigned super:1;
    unsigned resized:1;
};
union event {
    enum event_type type;
    struct event_header hdr;
    struct event_entered_exited entered;
    struct event_entered_exited exited;
    struct event_int2 moved;
    struct event_int2 scroll;
    struct event_int2 clicked;
    struct event_int2 drag_begin;
    struct event_int2 dragged;
    struct event_int2 drag_end;
    struct event_int2 pressed;
    struct event_int2 released;
    struct event_text text;
    struct event_key key;
    struct event_key shortcut;
};

/* process */
enum process_type {
    PROC_CLEAR,
    PROC_FULL_CLEAR,
    PROC_COMMIT,
    PROC_FILL_SERIAL_CONFIG,
    PROC_SERIALIZE,
    PROC_ALLOC,
    PROC_ALLOC_FRAME,
    PROC_FREE,
    PROC_FREE_FRAME,
    PROC_BLUEPRINT,
    PROC_LAYOUT,
    PROC_TRANSFORM,
    PROC_INPUT,
    PROC_PAINT,
    PROC_CLEANUP,
    PROC_CNT
};
enum processes {
    PROCESS_CLEAR = flag(PROC_CLEAR),
    PROCESS_FULL_CLEAR = PROCESS_CLEAR|flag(PROC_FULL_CLEAR),
    PROCESS_COMMIT = flag(PROC_COMMIT),
    PROCESS_BLUEPRINT = PROCESS_COMMIT|flag(PROC_BLUEPRINT),
    PROCESS_LAYOUT = PROCESS_BLUEPRINT|flag(PROC_LAYOUT),
    PROCESS_TRANSFORM = PROCESS_LAYOUT|flag(PROC_TRANSFORM),
    PROCESS_LAYOUTING = PROCESS_TRANSFORM,
    PROCESS_INPUT = PROCESS_TRANSFORM|flag(PROC_INPUT),
    PROCESS_PAINT = PROCESS_TRANSFORM|flag(PROC_PAINT),
    PROCESS_SERIALIZE = PROCESS_COMMIT|flag(PROC_SERIALIZE),
    PROCESS_CLEANUP = PROCESS_FULL_CLEAR|flag(PROC_CLEANUP)
};
enum serialization_type {
    SERIALIZE_TRACE,
    SERIALIZE_BINARY,
    SERIALIZE_TABLES,
    SERIALIZE_COUNT
};
struct process_header {
    enum process_type type;
    struct context *ctx;
    struct memory_arena *arena;
    struct temp_memory tmp;
};
struct process_memory {
    struct process_header hdr;
    void *ptr;
    int size;
};
struct process_layouting {
    struct process_header hdr;
    struct repository *repo;
    int begin, end, inc;
    struct box **boxes;
};
struct process_input {
    struct process_header hdr;
    struct input *state;
    struct list_hook evts;
};
struct process_paint {
    struct process_header hdr;
    struct box **boxes;
    int cnt;
};
struct process_serialize {
    struct process_header hdr;
    enum serialization_type type;
    unsigned id;
    FILE *file;
};
union process {
    enum process_type type;
    struct process_header hdr;
    struct process_memory mem;
    struct process_layouting layout;
    struct process_input input;
    struct process_paint paint;
    struct process_serialize serial;
};

/* context */
enum widget_internal {
    WIDGET_INTERNAL_BEGIN = 0x100000,
    /* layers */
    WIDGET_LAYER_BEGIN,
    WIDGET_ROOT = WIDGET_LAYER_BEGIN,
    WIDGET_OVERLAY,
    WIDGET_POPUP,
    WIDGET_CONTEXTUAL,
    WIDGET_UNBLOCKING,
    WIDGET_BLOCKING,
    WIDGET_UI,
    WIDGET_LAYER_END = WIDGET_UI,
    /* widgets */
    WIDGET_PANEL,
    WIDGET_POPUP_PANEL,
    WIDGET_CONTEXTUAL_PANEL,
    WIDGET_SLOT
};
struct context {
    int state;
    struct input input;
    union process proc;
    unsigned unbalanced:1;
    struct list_hook *iter;

    /* memory */
    struct allocator mem;
    struct block_allocator blkmem;
    struct memory_arena arena;

    /* modules */
    struct list_hook states;
    struct list_hook mod;
    struct list_hook garbage;
    struct list_hook freelist;

    /* tree */
    struct module root;
    struct box boxes[8];
    struct box *bfs[8];
    struct repository repo;

    struct box *tree;
    struct box *overlay;
    struct box *popup;
    struct box *contextual;
    struct box *unblocking;
    struct box *blocking;
    struct box *ui;

    struct box *active;
    struct box *origin;
    struct box *hot;
};
/* context */
api struct context *create(const struct allocator *a);
api void destroy(struct context *ctx);
api void init(struct context *ctx, const struct allocator *a);

/* process */
api union process *process_begin(struct context *ctx, unsigned flags);
api void blueprint(union process *p, struct box *b);
api void layout(union process *p, struct box *b);
api void transform(union process *p, struct box *b);
api void input(union process *p, union event *evt, struct box *b);
api void process_end(union process *p);

/* input */
api void input_char(struct context *ctx, char c);
api void input_resize(struct context *ctx, int w, int h);
api void input_scroll(struct context *ctx, int x, int y);
api void input_motion(struct context *ctx, int x, int y);
api void input_rune(struct context *ctx, unsigned long r);
api void input_key(struct context *ctx, int key, int down);
api void input_shortcut(struct context *ctx, int id, int down);
api void input_text(struct context *ctx, const char *t, int len);
api void input_button(struct context *ctx, enum mouse_button btn, int down);

/* module */
api struct state* begin(struct context *ctx, unsigned id);
api void end(struct state *s);
api struct state* module_begin(struct context *ctx, unsigned id, unsigned mid, unsigned bid);
api void module_end(struct state *s);
api void load(struct context *ctx, unsigned id, const struct component *c);
api void call(struct context *ctx, unsigned id, const union param *op, int opcnt);
api void section_begin(struct state *s, unsigned id);
api void section_end(struct state *s);
api void link(struct state *s, unsigned id);
api void slot(struct state *s, unsigned id);

/* widget */
api void widget_begin(struct state *s, int type);
api unsigned widget_box_push(struct state *s, unsigned flags);
api void widget_box_pop(struct state *s);
api unsigned widget_box(struct state *s, unsigned flags);
api void widget_end(struct state *s);

api void widget_push_param_float(struct state *s, float f);
api void widget_push_param_int(struct state *s, int i);
api void widget_push_param_uint(struct state *s, unsigned u);
api void widget_push_param_id(struct state *s, unsigned id);

api void widget_push_modifier_float(struct state *s, float *f);
api void widget_push_modifier_int(struct state *s, int *i);
api void widget_push_modifier_uint(struct state *s, unsigned *u);

/* box */
api void box_shrink(struct box *d, const struct box *s, int pad);
api void box_measure(struct box *b, int pad);
api void box_compute(struct box *b, int pad);

/* id */
api void pushid(struct state *s, unsigned id);
api unsigned genid(struct state *s);
api void popid(struct state *s);

/* utf-8 */
api int utf_encode(char *s, int cap, unsigned long u);
api int utf_decode(unsigned long *rune, const char *str, int len);
api int utf_len(const char *s, int len);
api const char* utf_at(unsigned long *rune, int *rune_len, const char *s, int len, int idx);

/* ---------------------------------------------------------------------------
 *                                  IMPLEMENTATION
 * --------------------------------------------------------------------------- */
#define VERSION 1
#define UTF_SIZE 4
#define UTF_INVALID 0xFFFD
static const unsigned char utfbyte[UTF_SIZE+1] = {0x80,0,0xC0,0xE0,0xF0};
static const unsigned char utfmask[UTF_SIZE+1] = {0xC0,0x80,0xE0,0xF0,0xF8};
static const unsigned long utfmin[UTF_SIZE+1] = {0,0,0x80,0x800,0x10000};
static const unsigned long utfmax[UTF_SIZE+1] = {0x10FFFF,0x7F,0x7FF,0xFFFF,0x10FFFF};

#define qalloc(a,sz) (a)->alloc((a)->usr, sz, __FILE__, __LINE__)
#define qdealloc(a,ptr) (a)->dealloc((a)->usr, ptr, __FILE__, __LINE__)
static void *dalloc(void *usr, int s, const char *file, int line){return malloc((size_t)s);}
static void dfree(void *usr, void *d, const char *file, int line){free(d);}
static const struct allocator default_allocator = {0,dalloc,dfree};
#define arena_push_type(a,type) (type*)arena_push(a, 1, szof(type), alignof(type))
#define arena_push_array(a,n,type) (type*)arena_push(a, (n), szof(type), alignof(type))

#define list_empty(l) ((l)->next == (l))
#define list_entry(ptr,type,member) containerof(ptr,type,member)
#define list_foreach(i,l) for ((i)=(l)->next; (i)!=(l); (i)=(i)->next)
#define list_foreach_rev(i,l) for ((i)=(l)->prev; (i)!=(l); (i)=(i)->prev)
#define list_foreach_s(i,n,l) for ((i)=(l)->next,(n)=(i)->next;(i)!=(l);(i)=(n),(n)=(i)->next)
#define list_foreach_rev_s(i,n,l) for ((i)=(l)->prev,(n)=(i)->prev;(i)!=(l);(i)=(n),(n)=(i)->prev)

static const struct element g_root_elements[] = {
    /* type             id, parent, wid,        flags,                     */
    {WIDGET_ROOT,       0,  0,  WIDGET_ROOT,        BOX_INTERACTIVE|BOX_UNIFORM},
    {WIDGET_OVERLAY,    1,  0,  WIDGET_OVERLAY,     BOX_INTERACTIVE|BOX_UNIFORM},
    {WIDGET_POPUP,      2,  1,  WIDGET_POPUP,       BOX_INTERACTIVE|BOX_UNIFORM},
    {WIDGET_CONTEXTUAL, 3,  2,  WIDGET_CONTEXTUAL,  BOX_INTERACTIVE|BOX_UNIFORM},
    {WIDGET_UNBLOCKING, 4,  3,  WIDGET_UNBLOCKING,  BOX_INTERACTIVE|BOX_UNIFORM},
    {WIDGET_BLOCKING,   5,  4,  WIDGET_BLOCKING,    BOX_INTERACTIVE|BOX_UNIFORM},
    {WIDGET_UI,         6,  5,  WIDGET_UI,          BOX_INTERACTIVE|BOX_UNIFORM},
};
static const unsigned g_root_table_keys[] = {0,1,2,3,4,5,6,0};
static const unsigned g_root_table_vals[] = {0,1,2,3,4,5,6,0};
static const struct component g_root = {
    VERSION, 0, 7,
    g_root_elements, cntof(g_root_elements),
    g_root_table_vals, g_root_table_keys, 8,
    NULL, 0, NULL, NULL
};
#define OPCODES(OP)\
    OP(BUF_BEGIN,       1,  "%u")\
    OP(BUF_ULINK,       2,  "%u %u")\
    OP(BUF_DLINK,       1,  "%u")\
    OP(BUF_END,         1,  "%u")\
    OP(WIDGET_BEGIN,    1,  "%d")\
    OP(WIDGET_END,      0,  "")  \
    OP(BOX_PUSH,        3,  "%u %u %u")\
    OP(BOX_POP,         0,  "")\
    OP(PUSH_FLOAT,      1,  "%f")\
    OP(PUSH_INT,        1,  "%d")\
    OP(PUSH_UINT,       1,  "%u")\
    OP(PUSH_ID,         1,  "%u")\
    OP(NEXT_BUF,        0,  "")\
    OP(EOF,             0,  "")
enum opcodes {
    #define OP(a,b,c) OP_ ## a,
    OPCODES(OP)
    #undef OP
    OPCNT
};
intern int
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
intern int
floori(float x)
{
    x = cast(float,(cast(int,x) - ((x < 0.0f) ? 1 : 0)));
    return cast(int,x);
}
intern int
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
intern float
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
intern int
roundi(float x)
{
    return cast(int, roundf(x));
}
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
intern int
utf_validate(unsigned long *u, int i)
{
    assert(u);
    if (!u) return 0;
    if (!between(*u, utfmin[i], utfmax[i]) ||
         between(*u, 0xD800, 0xDFFF))
        *u = UTF_INVALID;
    for (i = 1; *u > utfmax[i]; ++i);
    return i;
}
intern unsigned long
utf_decode_byte(char c, int *i)
{
    assert(i);
    if (!i) return 0;
    for(*i = 0; *i < cntof(utfmask); ++(*i)) {
        if (((unsigned char)c & utfmask[*i]) == utfbyte[*i])
            return (unsigned char)(c & ~utfmask[*i]);
    } return 0;
}
intern int
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
    return (char)((utfbyte[i]) | ((unsigned char)u & ~utfmask[i]));
}
intern int
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
intern int
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
intern const char*
utf_at(unsigned long *rune, int *rune_len,
    const char *s, int len, int idx)
{
    int runes = 0;
    if (!s) return 0;
    while ((*rune_len = utf_decode(rune, s, len))) {
        if (runes++ == idx) return s;
        len = max(0, len - *rune_len);
        s += *rune_len;
    } return 0;
}
intern void
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
intern void
list_add_head(struct list_hook *list, struct list_hook *n)
{
    list__add(n, list, list->next);
}
intern void
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
intern void
list_del(struct list_hook *entry)
{
    list__del(entry->prev, entry->next);
    entry->next = entry;
    entry->prev = entry;
}
intern void
list_move_head(struct list_hook *list, struct list_hook *entry)
{
    list_del(entry);
    list_add_head(list, entry);
}
intern void
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
    blksz = max(blksz, DEFAULT_MEMORY_BLOCK_SIZE);
    if (blksz == DEFAULT_MEMORY_BLOCK_SIZE && !list_empty(&a->freelist)) {
        /* allocate from freelist */
        blk = list_entry(a->freelist.next, struct memory_block, hook);
        list_del(&blk->hook);
    } else blk = qalloc(a->mem, blksz);

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
intern void*
arena_push(struct memory_arena *a, int cnt, int size, int align)
{
    int valid = 0;
    assert(a);
    if (!a) return 0;
    valid = a->blk && size_mul_add2_valid(size, cnt, a->blk->used, align);
    if (!valid || ((cnt*size) + a->blk->used + align) > a->blk->size) {
        if (!size_mul_add2_valid(size, cnt, szof(struct memory_block), align))
            return 0;

        {int minsz = cnt*size + szof(struct memory_block) + align;
        int blksz = max(minsz, DEFAULT_MEMORY_BLOCK_SIZE);
        struct memory_block *blk = block_alloc(a->mem, blksz);

        blk->prev = a->blk;
        a->blk = blk;
        a->blkcnt++;}
    }
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
    if (!a) return;
    while (a->blk)
        arena_free_last_blk(a);
}
intern struct temp_memory
temp_memory_begin(struct memory_arena *a)
{
    struct temp_memory res;
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
    while (a->blk != tmp.blk)
        arena_free_last_blk(a);
    if (a->blk) a->blk->used = tmp.used;
    a->tmpcnt--;
}
intern void
insert(struct table *tbl, unsigned id, int val)
{
    unsigned cnt = cast(unsigned, tbl->cnt);
    unsigned idx = id & (cnt-1), begin = idx;
    do {unsigned key = tbl->keys[idx];
        if (key) continue;
        tbl->keys[idx] = id;
        tbl->vals[idx] = val; return;
    } while ((idx = ((idx+1) & (cnt-1))) != begin);
}
intern int
lookup(struct table *tbl, unsigned id)
{
    unsigned key, cnt = cast(unsigned, tbl->cnt);
    unsigned idx = id & (cnt-1), begin = idx;
    do {if (!(key = tbl->keys[idx])) return 0;
        if (key == id) return tbl->vals[idx];
    } while ((idx = ((idx+1) & (cnt-1))) != begin);
    return 0;
}
api void
box_shrink(struct box *d, const struct box *s, int pad)
{
    assert(d && s);
    if (!d || !s) return;
    d->loc.x = s->loc.x + pad;
    d->loc.y = s->loc.y + pad;
    d->loc.w = max(0, s->loc.w - 2*pad);
    d->loc.h = max(0, s->loc.h - 2*pad);
}
intern void
transform_init(struct transform *t)
{
    assert(t);
    if (!t) return;
    t->x = t->y = 0;
    t->sx = t->sy = 1;
}
intern void
transform_concat(struct transform *d,
    const struct transform *a,
    const struct transform *b)
{
    assert(d && a && b);
    if (!d || !a || !b) return;
    d->x = a->x + b->x;
    d->y = a->y + b->y;
    d->sx = a->sx * b->sx;
    d->sy = a->sy * b->sy;
}
intern void
transform_rect(struct rect *d, const struct rect *s, const struct transform *t)
{
    assert(d && s && t);
    if (!d || !s || !t) return;
    d->x = floori(cast(float,(s->x + t->x))*t->sx);
    d->y = floori(cast(float,(s->y + t->y))*t->sy);
    d->w = ceili(cast(float,s->w) * t->sx);
    d->h = ceili(cast(float,s->h) * t->sy);
}
intern void
op_add(struct state *s, const union param *p, int cnt)
{
    int i = 0;
    struct param_buffer *ob = s->opbuf;
    assert((cnt + 1) < MAX_OPS);
    if ((s->op_idx + cnt) > (MAX_OPS-2)) {
        struct param_buffer *b = 0;
        b = arena_push(&s->arena, 1, szof(*ob), 0);
        ob->ops[s->op_idx + 0].op = OP_NEXT_BUF;
        ob->ops[s->op_idx + 1].p = b;
        s->op_idx = 0;
        s->opbuf = ob = b;
    } assert(s->op_idx + cnt < (MAX_OPS-2));
    for (i = 0; i < cnt; ++i)
        ob->ops[s->op_idx++] = p[i];
}
api void
pushid(struct state *s, unsigned id)
{
    struct idrange *idr = 0;
    idr = &s->idstk[s->stkcnt++];
    idr->base = id;
    idr->cnt = 0;
    s->lastid = (idr->base & 0xFFFF) << 16;
}
api unsigned
genid(struct state *s)
{
    unsigned id = 0;
    int idx = max(0, s->stkcnt-1);
    struct idrange *idr = 0;
    idr = &s->idstk[idx];
    id |= (idr->base & 0xFFFF) << 16;
    id |= (++idr->cnt);
    s->lastid = id;
    return id;
}
api void
popid(struct state *s)
{
    if (!s->stkcnt) return;
    s->stkcnt--;
}
intern struct state*
state_find(struct context *ctx, unsigned id)
{
    struct list_hook *i = 0;
    assert(ctx);
    if (!ctx) return 0;
    list_foreach(i, &ctx->states) {
        struct state *s = 0;
        s = list_entry(i, struct state, hook);
        if (s->id == id) return s;
    } return 0;
}
intern struct module*
module_find(struct context *ctx, unsigned id)
{
    struct list_hook *i = 0;
    assert(ctx);
    if (!ctx) return 0;
    list_foreach(i, &ctx->mod) {
        struct module *m = 0;
        m = list_entry(i, struct module, hook);
        if (m->id == id) return m;
    } return 0;
}
api struct state*
module_begin(struct context *ctx, unsigned id, unsigned mid, unsigned bid)
{
    struct state *s = state_find(ctx, id);
    if (s) {s->op_idx -= 2; return s;}
    s = arena_push_type(&ctx->arena, struct state);

    /* setup state */
    s->id = id;
    s->ctx = ctx;
    s->arena.mem = &ctx->blkmem;
    s->list = s->opbuf = arena_push(&s->arena, 1, szof(struct param_buffer), 0);
    s->mod = module_find(ctx, id);
    s->repo = !s->mod ? 0: s->mod->repo[s->mod->repoid];
    list_init(&s->hook);
    list_add_tail(&ctx->states, &s->hook);
    pushid(s, id);

    /* serialize api call */
    {union param p[5];
    p[0].op = OP_BUF_BEGIN;
    p[1].id = id;
    p[2].op = OP_BUF_ULINK;
    p[3].id = mid;
    p[4].id = bid;
    op_add(s, p, cntof(p));}
    return s;
}
api void
module_end(struct state *s)
{
    union param p[2];
    p[0].op = OP_BUF_END;
    p[1].id = s->id;
    op_add(s, p, cntof(p));
    popid(s);
}
api struct state*
begin(struct context *ctx, unsigned id)
{
    return module_begin(ctx, id, 0, WIDGET_UI-WIDGET_LAYER_BEGIN);
}
api void
end(struct state *s)
{
    module_end(s);
}
api void
section_begin(struct state *s, unsigned id)
{
    union param p[5];
    p[0].op = OP_BUF_BEGIN;
    p[1].id = id;
    p[2].op = OP_BUF_ULINK;
    p[3].id = s->id;
    p[4].id = s->lastid;
    op_add(s, p, cntof(p));
}
api void
section_end(struct state *s)
{
    union param p[2];
    p[0].op = OP_BUF_END;
    p[1].id = s->id;
    op_add(s, p, cntof(p));
}
api void
link(struct state *s, unsigned id)
{
    union param p[2];
    p[0].op = OP_BUF_DLINK;
    p[1].id = id;
    op_add(s, p, cntof(p));
}
api void
widget_begin(struct state *s, int type)
{
    union param p[2];
    p[0].op = OP_WIDGET_BEGIN;
    p[1].type = type;
    op_add(s, p, cntof(p));

    assert(s->wtop < cntof(s->wstk));
    s->wstk[s->wtop].id = genid(s);
    s->wstk[s->wtop].argcnt = 0;
    s->wtop++;
}
api unsigned
widget_box_push(struct state *s, unsigned flags)
{
    union param p[4];
    p[0].op = OP_BOX_PUSH;
    p[1].id = genid(s);
    p[2].id = s->wstk[s->wtop-1].id;
    p[3].flags = flags;
    op_add(s, p, cntof(p));

    s->depth++;
    s->tree_depth = max(s->depth, s->tree_depth);
    s->boxcnt++;
    return p[1].id;
}
api void
widget_box_pop(struct state *s)
{
    union param p[1];
    p[0].op = OP_BOX_POP;
    op_add(s, p, cntof(p));
    assert(s->depth);
    s->depth = max(s->depth-1, 0);
}
api unsigned
widget_box(struct state *s, unsigned flags)
{
    unsigned id = 0;
    id = widget_box_push(s, flags);
    widget_box_pop(s);
    return id;
}
intern void
widget_push_param(struct state *s, union param *p, int cnt)
{
    s->argcnt++;
    op_add(s, p, cnt);
}
api void
widget_push_param_float(struct state *s, float f)
{
    union param p[2];
    p[0].op = OP_PUSH_FLOAT;
    p[1].f = f;
    widget_push_param(s, p, cntof(p));
}
api void
widget_push_param_int(struct state *s, int i)
{
    union param p[2];
    p[0].op = OP_PUSH_INT;
    p[1].i = i;
    widget_push_param(s, p, cntof(p));
}
api void
widget_push_param_uint(struct state *s, unsigned u)
{
    union param p[2];
    p[0].op = OP_PUSH_UINT;
    p[1].u = u;
    widget_push_param(s, p, cntof(p));
}
api void
widget_push_param_id(struct state *s, unsigned id)
{
    union param p[2];
    p[0].op = OP_PUSH_ID;
    p[1].id = id;
    widget_push_param(s, p, cntof(p));
}
api void
widget_push_modifier_float(struct state *s, float *f)
{
    struct repository *repo = s->repo;
    if (repo) {
        const struct context *ctx = s->ctx;
        const struct box *act = ctx->active;
        struct widget w = s->wstk[s->wtop-1];
        if (act->wid == w.id)
            *f = act->params[w.argcnt].f;
    } widget_push_param_float(s, *f);
}
api void
widget_push_modifier_int(struct state *s, int *i)
{
    struct repository *repo = s->repo;
    if (repo) {
        const struct context *ctx = s->ctx;
        const struct box *act = ctx->active;
        struct widget w = s->wstk[s->wtop-1];
        if (act->wid == w.id)
            *i = act->params[w.argcnt].i;
    } widget_push_param_int(s, *i);
}
api void
widget_push_modifier_uint(struct state *s, unsigned *u)
{
    struct repository *repo = s->repo;
    if (repo) {
        const struct context *ctx = s->ctx;
        const struct box *act = ctx->active;
        struct widget w = s->wstk[s->wtop-1];
        if (act->wid == w.id)
            *u = act->params[w.argcnt].u;
    } widget_push_param_uint(s, *u);
}
api void
widget_end(struct state *s)
{
    union param p[1];
    p[0].op = OP_WIDGET_END;
    op_add(s, p, cntof(p));
    s->wtop = max(s->wtop-1, 0);
}
api void
slot(struct state *s, unsigned id)
{
    union param p[5];
    widget_begin(s, WIDGET_SLOT);
    p[0].op = OP_BOX_PUSH;
    p[1].id = id;
    p[2].id = s->wstk[s->wtop-1].id;
    p[3].flags = id;
    p[4].op = OP_BOX_POP;
    op_add(s, p, cntof(p));
    widget_end(s);
}
intern struct box**
bfs(struct box **buf, struct box *root)
{
    struct list_hook *i = 0;
    unsigned long head = 0, tail = 1;
    struct box **que = buf; que[tail] = root;
    while (head < tail) {
        struct box *b = que[++head];
        list_foreach(i, &b->lnks)
            que[++tail] = list_entry(i,struct box,node);
    } return que+1;
}
api void
call(struct context *ctx, unsigned id, const union param *op, int opcnt)
{
    static const int op_cnt[] = {
    #define OP(a,b,c) b,
        OPCODES(OP) 0
    #undef OP
    }; struct state *s = begin(ctx, id);
    s->op_idx = 0;

    {int i = 0;
    for (i = 0; i < opcnt; ++i) {
        const int cnt = op_cnt[op[i].op];
        op_add(s, op + i, cnt + 1);
        i += cnt + 1;
    } end(s);}
}
api void
load(struct context *ctx, unsigned id, const struct component *c)
{
    int i = 0;
    struct repository *repo = c->repo;
    struct module *m = c->module;
    assert(VERSION == c->version);

    zero(m, szof(*m));
    m->id = id;
    m->root = c->boxes;
    m->repo[m->repoid] = c->repo;
    list_init(&m->hook);
    list_add_tail(&ctx->mod, &m->hook);

    zero(repo, szof(*repo));
    repo->depth = c->depth;
    repo->boxes = c->boxes;
    repo->boxcnt = c->boxcnt;
    repo->bfs = c->bfs;
    repo->tbl.cnt = c->tblcnt;
    repo->tbl.keys = cast(unsigned*, c->tbl_keys);
    repo->tbl.vals = cast(int*, c->tbl_vals);

    for (i = 0; i < c->elmcnt; ++i) {
        const struct element *e = c->elms + i;
        struct box *b = c->boxes + lookup(&repo->tbl, e->id);
        struct box *p = c->boxes + lookup(&repo->tbl, e->parent);

        b->id = e->id;
        b->wid = e->wid;
        b->type = e->type;
        b->flags = e->flags;

        list_init(&b->node);
        list_init(&b->lnks);
        if (b != p) list_add_tail(&p->lnks, &b->node);
    } repo->bfs = bfs(repo->bfs, repo->boxes);
}
intern void
operation_begin(union process *p, enum process_type type,
    struct context *ctx, struct memory_arena *arena)
{
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
intern void
reorder(struct box *b)
{
    while (b->parent) {
        struct list_hook *i = 0;
        struct box *p = b->parent;
        if (p->flags & BOX_UNIFORM) goto nxt;
        list_foreach(i, &p->lnks) {
            struct box *s = list_entry(i, struct box, node);
            if (s != b || s->flags & BOX_BACKGROUND) continue;
            list_move_tail(&p->lnks, &s->node);
            break;
        } nxt: b = p;
    } return;
}
intern struct box*
at(struct box *b, int mx, int my)
{
    struct list_hook *i = 0;
    r:list_foreach_rev(i, &b->lnks) {
        struct box *sub = list_entry(i, struct box, node);
        if (!(sub->flags & BOX_INTERACTIVE) || (sub->flags & BOX_HIDDEN)) continue;
        if (inbox(mx, my, sub->scr.x, sub->scr.y, sub->scr.w, sub->scr.h))
            {b = sub; goto r;}
    } return b;
}
intern void
event_add_box(union event *evt, struct box *box)
{
    assert(evt->hdr.cnt < evt->hdr.cap);
    if (evt->hdr.cnt >= evt->hdr.cap) return;
    evt->hdr.boxes[evt->hdr.cnt++] = box;
}
intern union event*
event_begin(union process *p, enum event_type type, struct box *orig)
{
    union event *res = 0;
    res = arena_push_type(p->hdr.arena, union event);
    list_init(&res->hdr.hook);
    list_add_tail(&p->input.evts, &res->hdr.hook);

    res->type = type;
    res->hdr.origin = orig;
    res->hdr.cap = orig->depth + 1;
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
    static const int int_align = alignof(int);
    static const int ptr_align = alignof(void*);
    static const int box_align = alignof(struct box);
    static const int param_align = alignof(union param);
    static const int repo_align = alignof(struct repository);

    /* repository member object size */
    static const int int_size = szof(int);
    static const int ptr_size = szof(void*);
    static const int uint_size = szof(unsigned);
    static const int box_size = szof(struct box);
    static const int param_size = szof(union param);
    static const int repo_size = szof(struct repository);

    #define jmpto(ctx,s) do{(ctx)->state = (s); goto r;}while(0)
    enum processing_states {
        STATE_DISPATCH,
        STATE_COMMIT, STATE_CALC_REQ_MEMORY,
        STATE_ALLOC_PERSISTENT, STATE_ALLOC_TEMPORARY, STATE_COMPILE,
        STATE_BLUEPRINT, STATE_LAYOUTING, STATE_TRANSFORM,
        STATE_INPUT, STATE_PAINT,
        STATE_SERIALIZE, STATE_SERIALIZE_DISPATCH,
        STATE_SERIALIZE_TRACE, STATE_SERIALIZE_TABLE, STATE_SERIALIZE_BINARY,
        STATE_CLEAR, STATE_CLEANUP, STATE_DONE
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
    r:switch (ctx->state) {
    case STATE_DISPATCH: {
        if (flags & flag(PROC_CLEANUP))
            jmpto(ctx, STATE_CLEANUP);
        else if (flags & flag(PROC_COMMIT))
            jmpto(ctx, STATE_COMMIT);
        else jmpto(ctx, STATE_DONE);
    }
    case STATE_COMMIT: {
        struct state *s = 0;
        union process *p = &ctx->proc;
        ctx->iter = it(&ctx->states, ctx->iter);
        if (!ctx->iter) {
            /* free states */
            struct list_hook *si = 0;
            list_foreach(si, &ctx->states) {
                s = list_entry(si, struct state, hook);
                arena_clear(&s->arena);
            } list_init(&ctx->states);

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
        s->mod = module_find(ctx, s->id);
        if (s->mod) jmpto(ctx, STATE_ALLOC_TEMPORARY);

        operation_begin(p, PROC_ALLOC, ctx, &ctx->arena);
        p->mem.size = szof(struct module);
        ctx->state = STATE_ALLOC_PERSISTENT;
        return p;
    }
    case STATE_ALLOC_PERSISTENT: {
        struct state *s = list_entry(ctx->iter, struct state, hook);
        union process *p = &ctx->proc;
        if (!p->mem.ptr) return p;

        {struct module *m = cast(struct module*, p->mem.ptr);
        zero(m, szof(*m));
        list_init(&m->hook);
        m->id = s->id;
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
        s->tblcnt = cast(int, cast(float, s->boxcnt) * 1.5f);
        s->tblcnt = npow2(s->tblcnt);

        /* calculate required memory */
        p->mem.size = box_align + param_align;
        p->mem.size += repo_size + repo_align;
        p->mem.size += int_align + uint_align;
        p->mem.size += ptr_size * (s->boxcnt + 1);
        p->mem.size += box_size * s->boxcnt;
        p->mem.size += uint_size * s->boxcnt;
        p->mem.size += uint_size * s->tblcnt;
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
        repo->tbl.keys = (unsigned*)align(repo->bfs + repo->boxcnt + 1, uint_align);
        repo->tbl.vals = (int*)align(repo->tbl.keys + repo->tbl.cnt, int_align);
        repo->params = (union param*)align(repo->tbl.vals + repo->tbl.cnt, param_align);
        repo->depth = s->depth + 1;
        repo->boxcnt = 1;

        m->root = repo->boxes;
        list_init(&m->root->node);
        list_init(&m->root->lnks);
        transform_init(&m->root->tloc);
        transform_init(&m->root->tscr);
        m->root->type = WIDGET_PANEL;

        /* II.) Setup repository data */
        {int type = WIDGET_ROOT;
        int depth = 1, params = 0;
        struct param_buffer *ob = s->list;
        union param *op = &ob->ops[s->op_begin];

        struct box *boxstk[MAX_TREE_DEPTH];
        boxstk[0] = repo->boxes;
        while (1) {
            switch (op[0].op) {
            case OP_BUF_END:
                goto eol0;
            case OP_WIDGET_BEGIN:
                type = op[1].type;
                params = repo->argcnt; break;
            case OP_PUSH_FLOAT:
                repo->params[repo->argcnt++].f = op[1].f; break;
            case OP_PUSH_INT:
                repo->params[repo->argcnt++].i = op[1].i; break;
            case OP_PUSH_UINT:
                repo->params[repo->argcnt++].u = op[1].u; break;
            case OP_PUSH_ID:
                repo->params[repo->argcnt++].id = op[1].id; break;
            case OP_WIDGET_END:
                type = WIDGET_ROOT; break;
            case OP_BOX_POP:
                assert(depth > 1); depth--; break;
            case OP_NEXT_BUF:
                ob = (struct param_buffer*)op[1].p;
                op = ob->ops; break;
            case OP_BUF_BEGIN: {
                struct state *ss = 0;
                if (op[1].id == s->id) break;

                /* split current op buffer into sub-module */
                ss = arena_push_type(&ctx->arena, struct state);
                ss->id = op[1].id;
                ss->ctx = ctx;
                ss->list = ob;
                ss->arena.mem = &ctx->blkmem;
                ss->op_begin = cast(int, op - ob->ops);
                list_init(&ss->hook);
                list__add(&ss->hook, &s->hook, s->hook.next);

                /* skip sub-module */
                while (1) {
                    switch (op[0].op) {
                    case OP_NEXT_BUF: op = (union param*)op[1].p; break;
                    case OP_BUF_END: if (op[1].id == ss->id) goto eos;}
                    op += opdefs[op[0].op].argc + 1;
                } eos:; break;
            }
            case OP_BUF_ULINK: {
                /* find parent module */
                struct repository *prepo;
                struct module *pm = module_find(ctx, op[1].id);
                if (!pm) break;
                prepo = pm->repo[pm->repoid];
                if (!prepo) break;

                /* link module root box into parent module box */
                {int idx = lookup(&prepo->tbl, op[2].id);
                struct box *pb = prepo->boxes + idx;
                struct box *b = m->root;
                list_del(&b->node);
                list_add_tail(&pb->lnks, &b->node);}
            } break;
            case OP_BUF_DLINK: {
                /* find child module */
                struct module *cm = module_find(ctx, op[1].id);
                if (!cm->root) break;

                /* link referenced module root box into current box in module */
                {struct box *pb = boxstk[depth-1];
                struct box *b = cm->root;
                list_del(&b->node);
                list_add_tail(&pb->lnks, &b->node);}
            } break;
            case OP_BOX_PUSH: {
                unsigned id = op[1].id;
                int idx = repo->boxcnt++;
                struct box *pb = boxstk[depth-1];
                insert(&repo->tbl, id, idx);

                /* setup box tree node */
                {struct box *b = repo->boxes + idx;
                transform_init(&b->tloc);
                transform_init(&b->tscr);

                b->id = id;
                b->cnt = 0;
                b->type = type;
                b->parent = pb;
                b->depth = depth;
                b->wid = op[2].id;
                b->flags = op[3].flags;
                b->params = repo->params + params;

                /* link box into parent */
                list_init(&b->node);
                list_init(&b->lnks);
                list_add_tail(&pb->lnks, &b->node);
                pb->cnt++;

                assert(depth < MAX_TREE_DEPTH);
                boxstk[depth++] = b;}
            } break;}
            op += opdefs[op[0].op].argc + 1;
        } eol0:;}
        repo->bfs = bfs(repo->bfs, repo->boxes);
        ctx->unbalanced = 1;

        /* III.) Free old repository */
        operation_begin(p, PROC_FREE_FRAME, ctx, &ctx->arena);
        p->mem.ptr = old;
        ctx->state = STATE_COMMIT;
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
        r = m->repo[m->repoid];

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
            /* state transition table */
            if (flags & flag(PROC_TRANSFORM))
                jmpto(ctx, STATE_TRANSFORM);
            else jmpto(ctx, STATE_DONE);
        } m = list_entry(ctx->iter, struct module, hook);
        r = m->repo[m->repoid];

        operation_begin(p, PROC_LAYOUT, ctx, &ctx->arena);
        p->layout.end = max(0,r->boxcnt);
        p->layout.begin = 0, p->layout.inc = 1;
        p->layout.boxes = r->bfs;
        p->layout.repo = r;
        return p;
    }
    case STATE_TRANSFORM: {
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
        r = m->repo[m->repoid];

        operation_begin(p, PROC_TRANSFORM, ctx, &ctx->arena);
        p->layout.begin = 0, p->layout.inc = 1;
        p->layout.end = max(0, r->boxcnt);
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
            in->resized = 0;
            root->loc.w = root->scr.w = in->width;
            root->loc.h = root->scr.h = in->height;
            if (root->loc.w != in->width || root->loc.h != in->height)
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
                union event *evt;
                struct box *prev = last;
                struct box *cur = ctx->hot;

                {const struct rect *c = &cur->scr, *l = &prev->scr;
                cur->entered = !inbox(lx, ly, c->x, c->y, c->w, c->h);
                prev->exited = !inbox(mx, my, l->x, l->y, l->w, l->h);}

                /* exited */
                evt = event_begin(p, EVT_EXITED, prev);
                evt->entered.cur = ctx->hot;
                evt->entered.last = last;
                while ((prev = prev->parent)) {
                    const struct rect *l = &prev->scr;
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
                    const struct rect *c = &cur->scr;
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
                        a->loc.x += in->mouse.dx;
                    if (a->flags & BOX_MOVABLE_Y)
                        a->loc.y += in->mouse.dy;
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
            ctx->origin = o = (btn->down && btn->transitions) ? ctx->hot: ctx->tree;
            if (ctx->active != a && (btn->down && btn->transitions))
                reorder(a);

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
            evt = event_begin(p, EVT_KEY, a);
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
            struct key *key = in->shortcuts + i;
            key->transitions = 0;
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
        if (flags & flag(PROC_SERIALIZE))
            jmpto(ctx, STATE_SERIALIZE);
        else if (flags & flag(PROCESS_CLEAR))
            jmpto(ctx, STATE_CLEAR);
        else jmpto(ctx, STATE_DONE);
    } break;
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
        case SERIALIZE_TRACE:
            jmpto(ctx, STATE_SERIALIZE_TRACE);
        case SERIALIZE_BINARY:
            jmpto(ctx, STATE_SERIALIZE_BINARY);
        case SERIALIZE_TABLES:
            jmpto(ctx, STATE_SERIALIZE_TABLE);}
    }
    case STATE_SERIALIZE_TRACE: {
        struct list_hook *si = 0;
        union process *p = &ctx->proc;
        FILE *fp = p->serial.file;
        list_foreach(si, &ctx->states) {
            struct state *s = list_entry(si, struct state, hook);
            union param *op = &s->list->ops[s->op_begin];
            if (s->id != p->serial.id) continue;
            while (1) {
                const struct opdef *def = opdefs + op->type;
                switch (op->type) {
                case OP_NEXT_BUF: op = (union param*)op[1].p; break;
                default: {
                    const char *str = def->str;
                    while (*str) {
                        if (*str != '%') {
                            fputc(*str, fp);
                            str++; continue;
                        } str++; ++op;
                        switch (*str++) {default: break;
                        case 'f': fprintf(fp, "%g", op[0].f); break;
                        case 'd': fprintf(fp, "%d", op[0].i); break;
                        case 'u': fprintf(fp, "%u", op[0].u); break;
                        case 'p': fprintf(fp, "%p", op[0].p); break;}
                    } fputc('\n', fp);
                    if (op[0].op == OP_BUF_END && op[1].id == s->id)
                        goto eot;
                }} op += def->argc;
            } eot:break;
        }
        if (flags & flag(PROCESS_CLEAR))
            jmpto(ctx, STATE_CLEAR);
        else jmpto(ctx, STATE_DONE);
    }
    case STATE_SERIALIZE_BINARY: {
        union process *p = &ctx->proc;
        FILE *fp = p->serial.file;
        struct list_hook *si = 0;
        list_foreach(si, &ctx->states) {
            struct state *s = list_entry(si, struct state, hook);
            union param *op = &s->list->ops[s->op_begin];
            if (s->id != p->serial.id) continue;
            while (1) {
                const struct opdef *def = opdefs + op->type;
                switch (op->type) {
                case OP_NEXT_BUF: op = (union param*)op[1].p; break;
                default: {
                    for (i = 0; i < def->argc; ++i)
                        fwrite(&op[i], sizeof(op[i]), 1, fp);
                    if (op[0].op == OP_BUF_END && op[1].id == s->id)
                        goto eob;
                }} op += def->argc;
            } eob: break;
        }
        if (flags & flag(PROCESS_CLEAR))
            jmpto(ctx, STATE_CLEAR);
        else jmpto(ctx, STATE_DONE);
    }
    case STATE_SERIALIZE_TABLE: {
        static const struct property_def {
            const char *name; int len;
        } property_info[] = {
            #define PROP(p) {"BOX_" #p, (cntof("BOX_" #p)-1)},
                PROPERTY_MAP(PROP)
            #undef PROP
        }; const struct repository *r = 0;
        const union process *p = &ctx->proc;
        struct module *m = module_find(ctx, p->serial.id);
        FILE *fp = p->serial.file;

        if (!m) jmpto(ctx, STATE_DONE);
        r = m->repo[m->repoid];
        if (!r) jmpto(ctx, STATE_DONE);

        fprintf(fp, "const struct element g_elements[] = {\n");
        for (i = 0; i < r->boxcnt; ++i) {
            const struct box *b = r->boxes + i;
            const struct box *pb = b->parent;
            unsigned pid = pb ? pb->id: 0;
            char buf[256]; int j, n = 0; buf[n++] = '0';
            for (j = 0; j < PROPERTY_INDEX_MAX; ++j) {
                if (b->flags & flag(j)) {
                    const struct property_def *pi = 0;
                    pi = property_info + j;
                    copy(buf+n, pi->name, pi->len);
                    n += pi->len;
                } buf[n] = 0;
            } fprintf(fp, "    {%d, %u, %u, %u, %s},\n", b->type,b->id,pid,b->wid,buf);
        } fprintf(fp, "};\n");
        fprintf(fp, "const unsigned g_tbl_keys[%d] = {\n    ", r->tbl.cnt);
        for (i = 0; i < r->tbl.cnt; ++i) {
            if (i && !(i & 0x0F)) fprintf(fp, "\n    ");
            fprintf(fp, "%u", r->tbl.keys[i]);
            if (i < r->tbl.cnt-1) fputc(',', fp);
        } fprintf(fp, "\n};\n");
        fprintf(fp, "const unsigned g_tbl_vals[%d] = {\n    ", r->tbl.cnt);
        for (i = 0; i < r->tbl.cnt; ++i) {
            if (i && !(i & 0x0F)) fprintf(fp, "\n    ");
            fprintf(fp, "%d", r->tbl.vals[i]);
            if (i < r->tbl.cnt-1) fputc(',', fp);
        } fprintf(fp, "\n};\n");
        fprintf(fp, "struct box *g_bfs[%d];\n", r->boxcnt+1);
        fprintf(fp, "struct box g_boxes[%d];\n", r->boxcnt);
        fprintf(fp, "struct repository g_repository;\n");
        fprintf(fp, "struct module g_module;\n");
        fprintf(fp, "const struct component g_component = {\n");
        fprintf(fp, "    %d, g_elements, cntof(g_elements),\n", VERSION);
        fprintf(fp, "    g_tbl_vals, g_tbl_keys, cntof(g_tbl), %d \n", r->depth);
        fprintf(fp, "    g_module, g_repo, g_boxes, g_bfs, cntof(g_boxes)\n");
        fprintf(fp, "};\n");

        if (flags & flag(PROCESS_CLEAR))
            jmpto(ctx, STATE_CLEAR);
        else jmpto(ctx, STATE_DONE);
    }
    case STATE_CLEAR: {
        /* free all temporary frame state */
        struct module *m = 0;
        union process *p = &ctx->proc;
        if (list_empty(&ctx->garbage)) {
            list_init(&ctx->garbage);
            arena_clear(&ctx->arena);
            if (flags & flag(PROC_FULL_CLEAR))
                free_blocks(&ctx->blkmem);
            jmpto(ctx, STATE_DONE);
        }
        ctx->iter = ctx->garbage.next;
        m = list_entry(ctx->iter, struct module, hook);
        list_del(&m->hook);

        operation_begin(p, PROC_FREE, ctx, &ctx->arena);
        p->mem.ptr = m;
        return p;
    }
    case STATE_CLEANUP: {
        /* free all memory */
        list_splice_tail(&ctx->garbage, &ctx->mod);
        jmpto(ctx, STATE_CLEAR);
    }
    case STATE_DONE: {
        ctx->state = STATE_DISPATCH;
        return 0;
    }} return 0;
}
api void
process_end(union process *p)
{
    operation_end(p);
}
api void
box_measure(struct box *b, int pad)
{
    struct list_hook *i = 0;
    list_foreach(i, &b->lnks) {
        const struct box *n = list_entry(i, struct box, node);
        b->dw = max(b->dw, n->dw + 2*pad);
        b->dh = max(b->dh, n->dh + 2*pad);
    }
}
api void
box_compute(struct box *b, int pad)
{
    struct list_hook *i = 0;
    list_foreach(i, &b->lnks) {
        struct box *n = list_entry(i, struct box, node);
        n->loc.x = b->loc.x + pad;
        n->loc.y = b->loc.y + pad;
        n->loc.w = max(b->loc.w - 2*pad, 0);
        n->loc.h = max(b->loc.h - 2*pad, 0);
    }
}
intern void
layout_copy(struct box *b)
{
    struct list_hook *i = 0;
    list_foreach(i, &b->lnks) {
        struct box *n = 0;
        n = list_entry(i, struct box, node);
        n->loc.x = b->loc.x;
        n->loc.y = b->loc.y;
        n->loc.w = b->loc.w;
        n->loc.h = b->loc.h;
    }
}
intern void
layout_default(struct box *b)
{
    struct list_hook *i = 0;
    list_foreach(i, &b->lnks) {
        struct box *n = list_entry(i, struct box, node);
        n->loc.x = max(b->loc.x, n->loc.x);
        n->loc.y = max(b->loc.y, n->loc.y);
        n->loc.w = min(n->dw, (b->loc.x+b->loc.w)-n->loc.x);
        n->loc.h = min(n->dh, (b->loc.y+b->loc.h)-n->loc.y);
    }
}
api void
blueprint(union process *op, struct box *b)
{
    struct context *ctx = op->hdr.ctx;
    if (b->type & WIDGET_INTERNAL_BEGIN) {
        switch (b->type) {
        case WIDGET_ROOT: {
            b->dw = ctx->input.width;
            b->dh = ctx->input.height;
        } break;
        case WIDGET_PANEL:
        case WIDGET_SLOT: {
            box_measure(b,0);
        } break;}
    } else box_measure(b,0);
}
api void
layout(union process *op, struct box *b)
{
    struct context *ctx = op->hdr.ctx;
    if (!(b->type & WIDGET_INTERNAL_BEGIN))
        {box_compute(b, 0); return;}

    switch (b->type) {
    case WIDGET_PANEL:
    case WIDGET_SLOT:
        layout_default(b); break;
    case WIDGET_ROOT:
        b->loc.w = ctx->input.width;
        b->loc.h = ctx->input.height;
        layout_copy(b);
        break;
    case WIDGET_OVERLAY:
    case WIDGET_UNBLOCKING:
    case WIDGET_BLOCKING:
        layout_copy(b); break;
    case WIDGET_UI:
        layout_default(b); break;
    case WIDGET_POPUP:
    case WIDGET_CONTEXTUAL: {
        struct list_hook *i = 0;
        list_foreach(i, &b->lnks) {
            struct box *n = list_entry(i, struct box, node);
            if (n != ctx->contextual && n != ctx->unblocking) {
                n->loc.w = min(n->dw, (b->loc.x+b->loc.w)-n->loc.x);
                n->loc.h = min(n->dh, (b->loc.y+b->loc.h)-n->loc.y);
            } else {
                n->loc.w = b->loc.w;
                n->loc.h = b->loc.h;
            }
        }
    } break;}
}
api void
transform(union process *op, struct box *b)
{
    struct list_hook *bi;
    struct transform *tb = &b->tscr;
    list_foreach(bi, &b->lnks) {
        struct box *sub = list_entry(bi, struct box, node);
        transform_concat(&sub->tscr, tb, &sub->tloc);
        transform_rect(&sub->scr, &sub->loc, &sub->tscr);
    }
}
api void
input(union process *op, union event *evt, struct box *b)
{
    struct context *ctx = op->hdr.ctx;
    if (!(b->type & WIDGET_INTERNAL_BEGIN)) return;
    switch (b->type) {
    case WIDGET_UNBLOCKING: {
        struct list_hook *i = 0;
        struct box *contextual = 0;

        if (evt->hdr.origin != b) break;
        if (!b->clicked || evt->type == EVT_CLICKED) break;
        ctx = op->hdr.ctx;

        /* hide all contextual menus */
        contextual = ctx->contextual;
        list_foreach(i, &contextual->lnks) {
            struct box *c = list_entry(i,struct box,node);
            if (c == b) continue;
            c->hidden = 1;
        }
    } break;}
}
api void
init(struct context *ctx, const struct allocator *a)
{
    assert(ctx);
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    a = (a) ? a: &default_allocator;
    zero(ctx, szof(*ctx));

    /* setup allocators */
    ctx->mem = *a;
    ctx->blkmem.mem = &ctx->mem;
    ctx->arena.mem = &ctx->blkmem;
    block_alloc_init(&ctx->blkmem);

    /* list */
    list_init(&ctx->mod);
    list_init(&ctx->states);
    list_init(&ctx->garbage);
    list_init(&ctx->freelist);

    /* setup root module */
    {struct component root;
    memcpy(&root, &g_root, sizeof(root));
    root.boxes = ctx->boxes;
    root.boxcnt = cntof(g_root_elements);
    root.module = &ctx->root;
    root.repo = &ctx->repo;
    root.bfs = ctx->bfs;
    load(ctx, 0, &root);}

    /* setup fast access root boxes pointers */
    ctx->tree = ctx->boxes + (WIDGET_ROOT - WIDGET_LAYER_BEGIN);
    ctx->overlay = ctx->boxes + (WIDGET_OVERLAY - WIDGET_LAYER_BEGIN);
    ctx->popup = ctx->boxes + (WIDGET_POPUP - WIDGET_LAYER_BEGIN);
    ctx->contextual = ctx->boxes + (WIDGET_CONTEXTUAL - WIDGET_LAYER_BEGIN);
    ctx->unblocking = ctx->boxes + (WIDGET_UNBLOCKING - WIDGET_LAYER_BEGIN);
    ctx->blocking = ctx->boxes + (WIDGET_BLOCKING - WIDGET_LAYER_BEGIN);
    ctx->ui = ctx->boxes + (WIDGET_UI - WIDGET_LAYER_BEGIN);

    /* setup tree module */
    ctx->active = ctx->boxes;
    ctx->origin = ctx->boxes;
    ctx->hot = ctx->boxes;
}
api struct context*
create(const struct allocator *a)
{
    struct context *ctx = 0;
    a = (a) ? a: &default_allocator;
    ctx = qalloc(a, szof(*ctx));
    init(ctx, a);
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
input_text(struct context *ctx, const char *buf, int len)
{
    struct input *in = 0;
    assert(ctx);
    if (!ctx) return;
    in = &ctx->input;
    if (in->text_len + len + 1 >= cntof(in->text))
        return;

    copy(in->text + in->text_len, buf, len);
    in->text_len += len;
    in->text[in->text_len] = 0;
}
api void
input_char(struct context *ctx, char c)
{
    input_text(ctx, &c, 1);
}
api void
input_rune(struct context *ctx, unsigned long r)
{
    int len = 0;
    char buf[UTF_SIZE];
    assert(ctx);
    if (!ctx) return;
    len = utf_encode(buf, UTF_SIZE, r);
    input_text(ctx, buf, len);
}

/* ===========================================================================
 *
 *                                  WIDGETS
 *
 * =========================================================================== */
enum widget_type {
    WIDGET_ICON,
    WIDGET_BUTTON,
    WIDGET_SLIDER,
    WIDGET_TYPE_COUNT
};
/* ---------------------------------------------------------------------------
 *                                  ICON
 * --------------------------------------------------------------------------- */
enum icon_symbol {
    ICON_CIRCLE_SOLID,
    ICON_CIRCLE_OUTLINE,
    ICON_RECT_SOLID,
    ICON_RECT_OUTLINE,
    ICON_TRIANGLE_UP,
    ICON_TRIANGLE_DOWN,
    ICON_TRIANGLE_LEFT,
    ICON_TRIANGLE_RIGHT,
    ICON_COUNT
};
api unsigned
icon(struct state *s, enum icon_symbol sym)
{
    unsigned id = 0;
    widget_begin(s, WIDGET_ICON);
    widget_push_param_int(s, sym);
    id = widget_box(s, BOX_INTERACTIVE);
    widget_end(s);
    return id;
}
api void
icon_blueprint(struct box *b)
{
    static const icon_size = 16;
    b->dw = icon_size;
    b->dh = icon_size;
}
api void
icon_layout(struct box *b)
{
    if (b->loc.w > b->loc.h) {
        b->loc.x += (b->loc.w - b->loc.h) / 2;
        b->loc.w = b->loc.h;
    } else if (b->loc.h > b->loc.w) {
        b->loc.y += (b->loc.h - b->loc.w) / 2;
        b->loc.h = b->loc.w;
    }
}

/* ---------------------------------------------------------------------------
 *                                  BUTTON
 * --------------------------------------------------------------------------- */
api unsigned
button_begin(struct state *s)
{
    widget_begin(s, WIDGET_BUTTON);
    return widget_box_push(s, BOX_INTERACTIVE);
}
api void
button_end(struct state *s)
{
    widget_box_pop(s);
    widget_end(s);
}
/* ---------------------------------------------------------------------------
 *                                  SLIDER
 * --------------------------------------------------------------------------- */
struct slider {
    float min;
    float *value;
    float max;
    unsigned cid;
};
api unsigned
slider(struct state *s, float min, float *value, float max)
{
    unsigned sid = 0, cid = 0;
    widget_begin(s, WIDGET_BUTTON);
        widget_push_param_float(s, min);
        widget_push_modifier_float(s, value);
        widget_push_param_float(s, max);
        sid = widget_box_push(s, BOX_INTERACTIVE);
            cid = widget_box(s, BOX_INTERACTIVE|BOX_MOVABLE_X);
            widget_push_param_id(s, cid);
        widget_box_pop(s);
    widget_end(s);
    return sid;
}
api struct slider
slider_build(struct box *b)
{
    struct slider sld;
    union param *args = b->params;
    sld.min = args[0].f;
    sld.value = &args[1].f;
    sld.max = args[2].f;
    sld.cid = args[3].id;
    return sld;
}
api void
slider_blueprint(struct box *b)
{
    static const cursor_size = 8;
    struct slider sld = slider_build(b);
    if (b->id == sld.cid) {
        box_measure(b, 0);
        b->dw = max(b->dw, cursor_size);
        b->dh = max(b->dh, cursor_size);
    } else box_measure(b, 0);
}
api void
slider_layout(struct box *b)
{
    struct slider sld = slider_build(b);
    if (b->id != sld.cid)
        {box_compute(b, 0); return;}

    {const struct box *p = b->parent;
    float sld_min = min(sld.min, sld.max);
    float sld_max = max(sld.min, sld.max);
    float sld_val = clamp(sld_min, *sld.value, sld_max);
    float cur_off = (sld_val - sld_min) / sld_max;

    static const cursor_size = 8;
    int x = p->loc.x + cursor_size/2;
    int w = p->loc.w - cursor_size;

    b->loc.h = cursor_size * 2;
    b->loc.w = cursor_size;
    b->loc.x = x + roundi(w * cur_off) - cursor_size/2;
    b->loc.y = p->loc.y + p->loc.h/2 - b->loc.h/2;}
}
api void
slider_input(struct box *b, const union event *evt)
{
    struct box *p = b->parent;
    struct slider sld = slider_build(b);
    if (b->id != sld.cid) return;
    if (!b->moved || evt->type != EVT_MOVED)
        return;

    {float sld_min = min(sld.min, sld.max);
    float sld_max = max(sld.min, sld.max);
    b->loc.x = min(p->loc.x + p->loc.w - b->loc.w, b->loc.x);
    b->loc.x = max(p->loc.x, b->loc.x);

    {static const cursor_size = 8;
    int x = p->loc.x, w = p->loc.w - cursor_size;
    *sld.value = cast(float,(b->loc.x - x)) / cast(float,w);
    *sld.value = (*sld.value * sld_max) + sld_min;}}
}
/* ===========================================================================
 *
 *                                  APP
 *
 * =========================================================================== */
#define len(s) (cntof(s)-1)
#define h1(s,i,x) (x*65599lu+(unsigned char)(s)[(i)<len(s)?len(s)-1-(i):len(s)])
#define h4(s,i,x) h1(s,i,h1(s,i+1,h1(s,i+2,h1(s,i+3,x))))
#define h16(s,i,x) h4(s,i,h4(s,i+4,h4(s,i+8,h4(s,i+12,x))))
#define hash(s,i) ((unsigned)(h16(s,0,i)^(h16(s,0,i)>>16)))
#define idx(s,i) hash(s,(unsigned)i)
#define id(s) idx(s,0)
#define lit(s) s,len(s)
#define txt(s) s,((int)strlen(s))

/* System */
#include <GL/gl.h>
#include <GL/glu.h>
#include "SDL2/SDL.h"

/* NanoVG */
#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg/src/fontstash.h"
#include "nanovg/src/stb_image.h"
#include "nanovg/src/stb_truetype.h"
#include "nanovg/src/nanovg.h"
#include "nanovg/src/nanovg.c"
#include "nanovg/src/nanovg_gl.h"

intern int
is_black(NVGcolor col)
{
    return col.r == 0.0f && col.g == 0.0f && col.b == 0.0f && col.a == 0.0f;
}
api void
nvgButton(struct NVGcontext *vg, struct box *b, NVGcolor col)
{
    NVGpaint bg;
    NVGcolor src, dst;
    const float corner_radius = 4.0f;
    const struct rect *r = &b->scr;

    src = nvgRGBA(255,255,255, is_black(col) ? 16: 32);
    dst = nvgRGBA(0,0,0, is_black(col) ? 16: 32);
    bg = nvgLinearGradient(vg, r->x, r->y, r->x, r->y + r->h, src, dst);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, r->x+1,r->y+1, r->w-2,r->h-2, corner_radius-1);
    if (!is_black(col)) {
        nvgFillColor(vg, col);
        nvgFill(vg);
    }
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, r->x+0.5f,r->y+0.5f, r->w-1,r->h-1, corner_radius-0.5f);
    nvgStrokeColor(vg, nvgRGBA(0,0,0,48));
    nvgStroke(vg);
}
api void
nvgSlider(struct NVGcontext *vg, struct box *b)
{
    NVGpaint bg, knob;
    struct slider sld = slider_build(b);
    if (sld.cid == b->id) return;

    {struct box *p = b->parent;
    float cy = p->scr.y+(int)(p->scr.h*0.5f);
    float kr = b->scr.h / 2;
    float sx = p->scr.x;
    float cx = b->scr.x + b->scr.w*0.5f;
    nvgSave(vg);

    /* Slot */
    bg = nvgBoxGradient(vg, sx,cy-2+1, p->scr.w,4, 2,2, nvgRGBA(0,0,0,32), nvgRGBA(0,0,0,128));
    nvgBeginPath(vg);
    nvgRoundedRect(vg, sx,cy-2, p->scr.w,4, 2);
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    /* Knob Shadow */
    bg = nvgRadialGradient(vg, cx,cy+1, kr-3,kr+3, nvgRGBA(0,0,0,64), nvgRGBA(0,0,0,0));
    nvgBeginPath(vg);
    nvgRect(vg, cx-kr-5,cy-kr-5,kr*2+5+5,kr*2+5+5+3);
    nvgCircle(vg, cx,cy, kr);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    /* Knob */
    knob = nvgLinearGradient(vg, cx,cy-kr,cx,cy+kr, nvgRGBA(255,255,255,16), nvgRGBA(0,0,0,16));
    nvgBeginPath(vg);
    nvgCircle(vg, cx,cy, kr-1);
    nvgFillColor(vg, nvgRGBA(40,43,48,255));
    nvgFill(vg);
    nvgFillPaint(vg, knob);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgCircle(vg, cx,cy, kr-0.5f);
    nvgStrokeColor(vg, nvgRGBA(0,0,0,92));
    nvgStroke(vg);
    nvgRestore(vg);}
}
api void
nvgIcon(struct NVGcontext *vg, struct box *b)
{
    int sym = b->params[0].i;
    switch (sym) {
    default: assert(0); break;
    case ICON_CIRCLE_SOLID:
    case ICON_CIRCLE_OUTLINE: {
        float cx = b->scr.x + b->scr.w;
        float cy = b->scr.y +(int)(b->scr.h*0.5f);
        float kr = b->scr.h / 2;

        nvgBeginPath(vg);
        nvgCircle(vg, cx,cy, kr);
        nvgFillColor(vg, nvgRGBA(200,200,200,255));
        if (sym == ICON_CIRCLE_SOLID)
            nvgFill(vg);
        else nvgStroke(vg);
    } break;
    case ICON_RECT_SOLID:
    case ICON_RECT_OUTLINE: {
        nvgBeginPath(vg);
        nvgRect(vg, b->scr.x, b->scr.y, b->scr.w, b->scr.h);
        nvgFillColor(vg, nvgRGBA(200,200,200,255));
        if (sym == ICON_RECT_SOLID)
            nvgFill(vg);
        else nvgStroke(vg);
    } break;
    case ICON_TRIANGLE_UP: {
        nvgBeginPath(vg);
        nvgMoveTo(vg, b->scr.x + b->scr.w*0.5f, b->scr.y);
        nvgLineTo(vg, b->scr.x, b->scr.y + b->scr.h);
        nvgLineTo(vg, b->scr.x + b->scr.w, b->scr.y + b->scr.h);
        nvgClosePath(vg);
        nvgFillColor(vg, nvgRGBA(200,200,200,255));
        nvgFill(vg);
    } break;
    case ICON_TRIANGLE_DOWN: {
        nvgBeginPath(vg);
        nvgMoveTo(vg, b->scr.x, b->scr.y);
        nvgLineTo(vg, b->scr.x + b->scr.w, b->scr.y);
        nvgLineTo(vg, b->scr.x + b->scr.w*0.5f, b->scr.y + b->scr.h);
        nvgClosePath(vg);
        nvgFillColor(vg, nvgRGBA(200,200,200,255));
        nvgFill(vg);
    } break;
    case ICON_TRIANGLE_LEFT: {
        nvgBeginPath(vg);
        nvgMoveTo(vg, b->scr.x, b->scr.y + b->scr.h * 0.5f);
        nvgLineTo(vg, b->scr.x + b->scr.w, b->scr.y);
        nvgLineTo(vg, b->scr.x + b->scr.w, b->scr.y + b->scr.h);
        nvgClosePath(vg);
        nvgFillColor(vg, nvgRGBA(200,200,200,255));
        nvgFill(vg);
    } break;
    case ICON_TRIANGLE_RIGHT: {
        nvgBeginPath(vg);
        nvgMoveTo(vg, b->scr.x, b->scr.y);
        nvgLineTo(vg, b->scr.x + b->scr.w, b->scr.y + b->scr.h * 0.5f);
        nvgLineTo(vg, b->scr.x, b->scr.y + b->scr.h);
        nvgClosePath(vg);
        nvgFillColor(vg, nvgRGBA(200,200,200,255));
        nvgFill(vg);
    } break;};
}
int main(void)
{
    int quit = 0;
    struct context *ctx;
    struct NVGcontext *vg;
    SDL_GLContext glContext;

    SDL_Window *win = 0;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
    win = SDL_CreateWindow("GUI", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 1000, 600, SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
    glContext = SDL_GL_CreateContext(win);

    ctx = create(DEFAULT_ALLOCATOR);
    vg = nvgCreateGL2(0);
    if (!vg) return -1;

    input_resize(ctx, 1000, 600);
    while (!quit) {
        /* System: Input */
        {SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
            case SDL_QUIT: quit = 1; break;
            case SDL_MOUSEMOTION: input_motion(ctx, evt.motion.x, evt.motion.y); break;
            case SDL_MOUSEWHEEL: input_scroll(ctx, evt.wheel.x, evt.wheel.y); break;
            case SDL_TEXTINPUT: input_text(ctx, txt(evt.text.text)); break;
            case SDL_WINDOWEVENT: {
                 if (evt.window.event == SDL_WINDOWEVENT_RESIZED ||
                    evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    input_resize(ctx, evt.window.data1, evt.window.data2);
            } break;
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN: {
                int down = (evt.type == SDL_MOUSEBUTTONDOWN);
                if (evt.button.button == SDL_BUTTON_LEFT)
                    input_button(ctx, MOUSE_BUTTON_LEFT, down);
                else if (evt.button.button == SDL_BUTTON_RIGHT)
                    input_button(ctx, MOUSE_BUTTON_RIGHT, down);
            } break;
            case SDL_KEYUP:
            case SDL_KEYDOWN: {
                int down = (evt.type == SDL_KEYDOWN);
                SDL_Keycode sym = evt.key.keysym.sym;
                input_key(ctx, sym, down);
            } break;}
        }}

        /* GUI */
        {struct state *s = 0;
        if ((s = begin(ctx, id("ui")))) {
            button_begin(s);
            icon(s, ICON_CIRCLE_OUTLINE);
            button_end(s);
        } end(s);}

        /* Process: Commit */
        {union process *p = 0;
        while ((p = process_begin(ctx, PROCESS_COMMIT))) {
            switch (p->type) {default: break;
            case PROC_ALLOC_FRAME:
            case PROC_ALLOC: p->mem.ptr = calloc(p->mem.size,1); break;
            case PROC_FREE_FRAME:
            case PROC_FREE: free(p->mem.ptr); break;}
            process_end(p);
        }}

        /* Process: Layouting */
        {int i; union process *p = 0;
        while ((p = process_begin(ctx, PROCESS_LAYOUTING))) {
            switch (p->type) {default: break;
            case PROC_BLUEPRINT: {
                struct process_layouting *op = &p->layout;
                for (i = op->begin; i != op->end; i += op->inc) {
                    struct box *b = op->boxes[i];
                    switch (b->type) {
                    case WIDGET_ICON: icon_blueprint(b); break;
                    case WIDGET_BUTTON: box_measure(b, 0); break;
                    case WIDGET_SLIDER: slider_blueprint(b); break;
                    default: blueprint(p, b); break;}
                }
            } break;
            case PROC_LAYOUT: {
                struct process_layouting *op = &p->layout;
                for (i = op->begin; i != op->end; i += op->inc) {
                    struct box *b = op->boxes[i];
                    switch (b->type) {
                    case WIDGET_ICON: icon_layout(b); break;
                    case WIDGET_BUTTON: box_compute(b, 0); break;
                    case WIDGET_SLIDER: slider_layout(b); break;
                    default: layout(p, b); break;}
                }
            } break;
           case PROC_TRANSFORM: {
                struct process_layouting *op = &p->layout;
                for (i = op->begin; i != op->end; i += op->inc) {
                    struct box *b = op->boxes[i];
                    switch (b->type) {
                    default: transform(p, b); break;}
                }
            } break;}
            process_end(p);
        }}

        /* Process: Input */
        {int i; union process *p = 0;
        while ((p = process_begin(ctx, PROCESS_INPUT))) {
            struct list_hook *it = 0;
            struct process_input *op = &p->input;
            list_foreach(it, &op->evts) {
                union event *evt = list_entry(it, union event, hdr.hook);
                for (i = 0; i < evt->hdr.cnt; i++) {
                    struct box *b = evt->hdr.boxes[i];
                    switch (b->type) {
                    case WIDGET_SLIDER: slider_input(b, evt); break;
                    default: input(p, evt, b); break;}
                }
            } process_end(p);
        }}

        /* Process: Paint */
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
        glClearColor(0.10f, 0.18f, 0.24f, 1);
        nvgBeginFrame(vg, 1000, 600, 1.0f);
        {int i = 0; union process *p = 0;
        while ((p = process_begin(ctx, PROCESS_PAINT))) {
            struct process_paint *op = &p->paint;
            if (p->type != PROC_PAINT) break;
            for (i = 0; i < op->cnt; ++i) {
                struct box *b = op->boxes[i];
                switch (b->type) {
                case WIDGET_ICON: nvgIcon(vg, b); break;
                case WIDGET_BUTTON: nvgButton(vg, b, nvgRGBA(128,16,8,255)); break;
                case WIDGET_SLIDER: nvgSlider(vg, b); break;
                default: break;}
            } process_end(p);
        }} nvgEndFrame(vg);

        /* Process: Clear */
        {union process *p = 0;
        while ((p = process_begin(ctx, PROCESS_CLEAR))) {
            free(p->mem.ptr);
            process_end(p);
        }}
        /* Finish frame */
        SDL_GL_SwapWindow(win);
        SDL_Delay(16);
    }
    /* Process: Serialize */
    {union process *p = 0;
    while ((p = process_begin(ctx, PROCESS_SERIALIZE))) {
        struct process_serialize *serial = &p->serial;
        if (p->type != PROC_FILL_SERIAL_CONFIG) break;
        serial->type = SERIALIZE_TABLES;
        serial->id = id("ui");
        serial->file = stdout;
        process_end(p);
    }}
    /* Process: Cleanup */
    {union process *p = 0;
    while ((p = process_begin(ctx, PROCESS_CLEANUP))) {
        free(p->mem.ptr);
        process_end(p);
    }} destroy(ctx);

    /* System: Cleanup */
    nvgDeleteGL2(vg);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
