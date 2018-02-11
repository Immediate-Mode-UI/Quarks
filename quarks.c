/* ===========================================================================
 *
 *                                  LIBRARY
 *
 * =========================================================================== */
#include <assert.h> /* assert */
#include <stdlib.h> /* calloc, free */
#include <string.h> /* memcpy, memset */
#include <stdint.h> /* uintptr_t, uint64_t */
#include <inttypes.h> /* PRIu64 */
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
#define isaligned(x,mask) (!((uintptr_t)(x) & (mask-1)))
#define type_aligned(x,t) isaligned(x, alignof(t))
#define between(x,a,b) ((a)<=(x) && (x)<=(b))
#define inbox(px,py,x,y,w,h) (between(px,x,x+w) && between(py,y,y+h))
#define intersect(x,y,w,h,X,Y,W,H) ((x)<(X)+(W) && (x)+(w)>(X) && (y)<(Y)+(H) && (y)+(h)>(Y))
#define stringify(x) #x
#define stringifyi(x) stringifyi(x)
#define strjoini(a, b) a ## b
#define strjoind(a, b) strjoini(a,b)
#define strjoin(a, b) strjoind(a,b)
#define uniqid(name) strjoin(name, __LINE__)
#define compiler_assert(exp) {typedef char uniqid(_compile_assert_array)[(exp)?1:-1];}
#define uniqstr __FILE__ ":" stringifyi(__LINE__)

/* callbacks */
typedef void(*dealloc_f)(void *usr, void *data, const char *file, int line);
typedef void*(*alloc_f)(void *usr, int size, const char *file, int line);
struct allocator {
    void *usr;
    alloc_f alloc;
    dealloc_f dealloc;
};
/* list */
struct list_hook {
    struct list_hook *next;
    struct list_hook *prev;
};
/* memory */
#define DEFAULT_ALLOCATOR 0
#define DEFAULT_MEMORY_BLOCK_SIZE (16*1024)
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
    MOUSE_BUTTON_MIDDLE,
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

/* id */
typedef uint64_t uiid;
typedef unsigned mid;
#define IDFMT "%"PRIu64
#define MIDFMT "%u"

/* parameter */
union param {
    uiid id;
    mid mid;
    int op;
    int type;
    int i;
    unsigned u;
    unsigned flags;
    float f;
    void *p;
};

/* compile-time */
struct element {
    int type;
    uiid id;
    uiid parent;
    uiid wid;

    unsigned short depth;
    unsigned short tree_depth;
    unsigned flags;
    unsigned params;
    unsigned state;
};
struct component {
    unsigned version;
    mid id;
    int tree_depth, depth;

    const struct element *elms;
    const int elmcnt;
    const int *tbl_vals;
    const uiid *tbl_keys;
    const int tblcnt;
    const unsigned char *buf;
    const int bufsiz;
    const union param *params;
    const int paramcnt;

    struct module *module;
    struct repository *repo;
    struct box *boxes;
    struct box **bfs;
    int boxcnt;
};
struct container {
    mid id;
    mid parent;
    uiid parent_box;
    mid owner;
    int rel;
    const struct component *comp;
};

/* box */
#define PROPERTY_MAP(PROP)\
    PROP(IMMUTABLE)\
    PROP(UNSELECTABLE)\
    PROP(MOVABLE_X)\
    PROP(MOVABLE_Y)\
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
    BOX_MOVABLE = BOX_MOVABLE_X|BOX_MOVABLE_Y,
    PROPERTY_ALL
};
struct rect {int x,y,w,h;};
struct box {
    uiid id, wid;
    int type;
    unsigned flags;
    union param *params;
    char *buf;

    /* bounds */
    int x,y,w,h;
    int dw,dh;

    /* state */
    unsigned hovered:1;
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
    unsigned short depth;
    unsigned short tree_depth;
    struct box *parent;
    struct list_hook node;
    struct list_hook lnks;
};

/* state */
#define MAX_IDSTACK 256
#define MAX_TREE_DEPTH 128
#define MAX_OPS ((int)((DEFAULT_MEMORY_BLOCK_SIZE - sizeof(struct memory_block)) / sizeof(union param)))
#define MAX_STR_BUF ((int)(DEFAULT_MEMORY_BLOCK_SIZE - (sizeof(struct memory_block) + sizeof(void*))))

struct repository;
struct idrange {
    mid base;
    mid cnt;
};
struct widget {
    uiid id;
    int type;
    int *argc;
};
struct param_buffer {
    union param ops[MAX_OPS];
};
struct buffer {
    struct buffer *next;
    char buf[MAX_STR_BUF];
};
enum relationship {
    RELATIONSHIP_INDEPENDENT,
    RELATIONSHIP_DEPENDENT
};
enum id_gen_state {
    ID_GEN_DEFAULT,
    ID_GEN_ONE_TIME
};
struct state {
    mid id;
    struct list_hook hook;
    struct memory_arena arena;

    /* references */
    struct module *mod;
    struct context *ctx;
    const struct config *cfg;
    struct repository *repo;

    /* stats */
    int argcnt;
    int tblcnt;
    int tree_depth;
    int boxcnt;
    int bufsiz;

    /* opcodes */
    int op_begin, op_idx;
    struct param_buffer *param_list;
    struct param_buffer *opbuf;
    const struct component *tbl;

    /* strings */
    int buf_off, total_buf_size;
    struct buffer *buf_list;
    struct buffer *buf;

    /* ID generator */
    uiid lastid, otid;
    enum id_gen_state idstate;
    struct idrange idstk[MAX_IDSTACK];

    /* tree */
    int depth;
    int stkcnt;
    struct widget wstk[MAX_TREE_DEPTH];
    int wtop;
};

/* module */
struct table {
    int cnt;
    uiid *keys;
    int *vals;
};
struct repository {
    struct table tbl;
    struct box *boxes;
    struct box **bfs;
    int boxcnt;
    union param *params;
    int argcnt;
    char *buf;
    int bufsiz;
    int depth, tree_depth;
    int size;
};
struct module {
    struct list_hook hook;
    struct module *parent;
    struct module *owner;
    enum relationship rel;
    mid id;
    unsigned seq;
    int repoid;
    struct box *root;
    struct repository *repo[2];
    int size;
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
    struct input *input;
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
    PROCESS_INPUT = PROCESS_LAYOUT|flag(PROC_INPUT),
    PROCESS_PAINT = PROCESS_LAYOUT|flag(PROC_PAINT),
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
    const char *name;
    int indent;
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
enum visibility {HIDDEN, VISIBLE};
enum popup_type {
    POPUP_BLOCKING,
    POPUP_NON_BLOCKING,
    POPUP_CNT
};
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
    WIDGET_TREE_ROOT,
    WIDGET_LINK,
    WIDGET_SLOT
};
struct config {
    int font_default_id;
    int font_default_height;
};
struct context {
    int state;
    unsigned seq;
    struct config cfg;
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
api struct context *create(const struct allocator *a, const struct config *cfg);
api int init(struct context *ctx, const struct allocator *a, const struct config *cfg);
api void reset(struct context *ctx);
api void destroy(struct context *ctx);

/* process */
api union process* process_begin(struct context *ctx, unsigned flags);
api void blueprint(union process *p, struct box *b);
api void layout(union process *p, struct box *b);
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
api void input_text(struct context *ctx, const char *t, const char *end);
api void input_button(struct context *ctx, enum mouse_button btn, int down);

/* module */
api struct state* begin(struct context *ctx, mid id);
api void end(struct state *s);
api struct state* module_begin(struct context *ctx, mid id, enum relationship, mid parent, mid owner, uiid bid);
api void module_end(struct state *s);
api void load(struct context *ctx, const struct container *c, int cnt);
api struct state *section_begin(struct state *s, mid id);
api void section_end(struct state *s);
api void link(struct state *s, mid id, enum relationship);
api void slot(struct state *s, uiid id);

/* popup */
api struct state *popup_begin(struct state *s, mid id, enum popup_type type);
api void popup_show(struct context *ctx, mid id, enum visibility vis);
api void popup_toggle(struct context *ctx, mid id);
api void popup_end(struct state *s);

/* widget */
api void widget_begin(struct state *s, int type);
api uiid widget_box_push(struct state *s);
api void widget_box_property_set(struct state *s, enum properties);
api void widget_box_property_clear(struct state *s, enum properties);
api void widget_box_pop(struct state *s);
api void widget_end(struct state *s);

/* widget: parameter */
api float *widget_param_float(struct state *s, float f);
api int *widget_param_int(struct state *s, int i);
api unsigned *widget_param_uint(struct state *s, unsigned u);
api uiid *widget_param_id(struct state *s, uiid id);
api mid *widget_param_mid(struct state *s, mid id);
api const char* widget_param_str(struct state *s, const char *str, int len);

api float* widget_modifier_float(struct state *s, float *f);
api int* widget_modifier_int(struct state *s, int *i);
api unsigned* widget_modifier_uint(struct state *s, unsigned *u);

api float* widget_state_float(struct state *s, int type, float f);
api int* widget_state_int(struct state *s, int type, int i);
api unsigned* widget_state_uint(struct state *s, int type, unsigned u);
api uiid* widget_state_id(struct state *s, int type, uiid u);

api float *widget_get_float(struct box *b, int idx);
api int *widget_get_int(struct box *b, int idx);
api unsigned *widget_get_uint(struct box *b, int indx);
api uiid *widget_get_id(struct box *b, int idx);
api mid *widget_get_mid(struct box *b, int idx);
api const char *widget_get_str(struct box *b, int idx);

/* box */
api void box_shrink(struct box *d, const struct box *s, int pad);
api void box_blueprint(struct box *b, int padx, int pady);
api void box_layout(struct box *b, int pad);

/* id */
api void pushid(struct state *s, unsigned id);
api void setid(struct state *s, uiid id);
api uiid genwid(struct state *s);
api uiid genbid(struct state *s);
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
#define list_next(l) (l)->next
#define list_prev(l) (l)->prev
#define list_begin(l) list_next(l)
#define list_last(l) list_prev(l)
#define list_foreach(i,l) for ((i)=(l)->next; (i)!=(l); (i)=(i)->next)
#define list_foreach_rev(i,l) for ((i)=(l)->prev; (i)!=(l); (i)=(i)->prev)
#define list_foreach_s(i,n,l) for ((i)=(l)->next,(n)=(i)->next;(i)!=(l);(i)=(n),(n)=(i)->next)
#define list_foreach_rev_s(i,n,l) for ((i)=(l)->prev,(n)=(i)->prev;(i)!=(l);(i)=(n),(n)=(i)->prev)

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
        assert(blk);

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
intern union param*
op_add(struct state *s, const union param *p, int cnt)
{
    int i = 0;
    assert(s && p);
    assert(cnt > 0);
    assert(cnt < MAX_OPS-2);

    {struct param_buffer *ob = s->opbuf;
    if ((s->op_idx + cnt) >= (MAX_OPS-2)) {
        struct param_buffer *b = 0;
        b = arena_push(&s->arena, 1, szof(*ob), 0);
        ob->ops[s->op_idx + 0].op = OP_NEXT_BUF;
        ob->ops[s->op_idx + 1].p = b;
        s->op_idx = 0;
        s->opbuf = ob = b;
    } assert(s->op_idx + cnt < (MAX_OPS-2));
    for (i = 0; i < cnt; ++i)
        ob->ops[s->op_idx++] = p[i];
    return ob->ops + (s->op_idx - cnt);}
}
intern const char*
store(struct state *s, const char *str, int len)
{
    int off = 0;
    assert(s && str);
    assert(s->buf);
    assert(len < MAX_STR_BUF-1);

    {struct buffer *ob = s->buf;
    if ((s->buf_off + len) > MAX_STR_BUF-1) {
        struct buffer *b = 0;
        b = arena_push(&s->arena, 1, szof(*ob), 0);
        s->buf_off = 0;
        ob->next = b;
        s->buf = ob = b;
    }
    assert((s->buf_off + len) <= MAX_STR_BUF-1);
    copy(ob->buf + s->buf_off, str, len);

    off = s->buf_off;
    ob->buf[s->buf_off + len] = 0;
    s->total_buf_size += len + 1;
    s->buf_off += len + 1;
    return ob->buf + off;}
}
api void
pushid(struct state *s, unsigned id)
{
    struct idrange *idr = 0;
    assert(s);
    assert(s->stkcnt < cntof(s->idstk));
    if (!s || s->stkcnt >= cntof(s->idstk))
        return;

    idr = &s->idstk[s->stkcnt++];
    idr->base = id;
    idr->cnt = 0;
    s->lastid = (idr->base & 0xFFFFFFFFlu) << 32lu;
}
api void
setid(struct state *s, uiid id)
{
    assert(s);
    assert(s->stkcnt < cntof(s->idstk));
    if (!s) return;
    s->idstate = ID_GEN_ONE_TIME;
    s->otid = id;
}
api uiid
genwid(struct state *s)
{
    uiid id = 0;
    assert(s);
    if (!s) return 0;

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
intern struct state*
state_find(struct context *ctx, uiid id)
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
module_find(struct context *ctx, mid id)
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
api struct box*
find(struct repository *repo, uiid id)
{
    int idx = lookup(&repo->tbl, id);
    if (!idx) return 0;
    return repo->boxes + idx;
}
api struct box*
poll(struct state *s, uiid id)
{
    struct repository *repo = 0;
    repo = s->repo;
    if (!repo) return 0;
    return find(repo, id);
}
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
    assert(s);
    assert(s->wtop > 0);
    if (!s || s->wtop <= 0) return;

    {union param p[4];
    widget_begin(s, WIDGET_SLOT);
    p[0].op = OP_BOX_PUSH;
    p[1].id = id;
    p[2].id = s->wstk[s->wtop-1].id;
    p[3].op = OP_BOX_POP;
    op_add(s, p, cntof(p));}
    widget_end(s);
}
api void
widget_begin(struct state *s, int type)
{
    assert(s);
    if (!s) return;
    {union param p[3], *q;
    p[0].op = OP_WIDGET_BEGIN;
    p[1].type = type;
    p[2].i = 0;
    q = op_add(s, p, cntof(p));
    assert(q);

    assert(s->wtop < cntof(s->wstk));
    s->wstk[s->wtop].id = genwid(s);
    s->wstk[s->wtop].argc = &q[2].i;
    s->wstk[s->wtop].type = type;
    s->wtop++;}
}
api uiid
widget_box_push(struct state *s)
{
    assert(s);
    if (!s) return 0;

    {union param p[3];
    p[0].op = OP_BOX_PUSH;
    p[1].id = genbid(s);
    p[2].id = s->wstk[s->wtop-1].id;
    op_add(s, p, cntof(p));

    s->depth++;
    s->tree_depth = max(s->depth, s->tree_depth);
    s->boxcnt++;
    return p[1].id;}
}
api void
widget_box_pop(struct state *s)
{
    assert(s);
    if (!s) return;
    {union param p[1];
    p[0].op = OP_BOX_POP;
    op_add(s, p, cntof(p));
    assert(s->depth);
    s->depth = max(s->depth-1, 0);}
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
        const struct context *ctx = s->ctx;
        const struct box *act = ctx->active;
        struct widget w = s->wstk[s->wtop-1];
        if (act->wid == w.id && act->type == w.type)
            *u = act->params[*w.argc].u;
    } return widget_param_uint(s, *u);}
}
api float*
widget_state_float(struct state *s, int type, float f)
{
    const struct box *b;
    struct widget w;

    assert(s);
    assert(s->ctx);
    assert(s->wtop > 0);
    if (!s || s->wtop < 1) return 0;

    w = s->wstk[s->wtop-1];
    b = poll(s, w.id + 1);
    if (!b || b->type != type)
        return widget_param_float(s, f);
    return widget_param_float(s, b->params[*w.argc].f);
}
api int*
widget_state_int(struct state *s, int type, int i)
{
    const struct box *b = 0;
    struct widget w;

    assert(s);
    assert(s->ctx);
    assert(s->wtop > 0);
    if (!s || s->wtop < 1) return 0;

    w = s->wstk[s->wtop-1];
    b = poll(s, w.id + 1);
    if (!b || b->type != type)
        return widget_param_int(s, i);
    return widget_param_int(s, b->params[*w.argc].i);
}
api unsigned*
widget_state_uint(struct state *s, int type, unsigned u)
{
    const struct box *b;
    struct widget w;

    assert(s);
    assert(s->ctx);
    assert(s->wtop > 0);
    if (!s || s->wtop < 1) return 0;

    w = s->wstk[s->wtop-1];
    b = poll(s, w.id + 1);
    if (!b || b->type != type)
        return widget_param_uint(s, u);
    return widget_param_uint(s, b->params[*w.argc].u);
}
api uiid*
widget_state_id(struct state *s, int type, uiid u)
{
    const struct box *b;
    struct widget w;

    assert(s);
    assert(s->ctx);
    assert(s->wtop > 0);
    if (!s || s->wtop < 1) return 0;

    w = s->wstk[s->wtop-1];
    b = poll(s, w.id + 1);
    if (!b || b->type != type)
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
api void
widget_end(struct state *s)
{
    assert(s);
    assert(s->wtop > 0);
    if (!s || s->wtop <= 0) return;
    {union param p[1];
    p[0].op = OP_WIDGET_END;
    op_add(s, p, cntof(p));}
    s->wtop = max(s->wtop-1, 0);
}
intern int
box_intersect(const struct box *a, const struct box *b)
{
    return intersect(a->x, a->y, a->w, a->h, b->x, b->y, b->w, b->h);
}
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
    r:while (&b->lnks != i) {
        struct box *sub = list_entry(i, struct box, node);
        if ((sub->flags & BOX_IMMUTABLE) || (sub->flags & BOX_HIDDEN)) goto n;
        if (inbox(mx, my, sub->x, sub->y, sub->w, sub->h))
            {b = sub; i = b->lnks.prev; goto r;}
        n:i = i->prev;
    }
    if (b->flags & BOX_UNSELECTABLE)
        {i = b->node.prev; b = b->parent; goto r;}
    return b;
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
    res = arena_push_type(p->hdr.arena, union event);
    assert(res);
    list_init(&res->hdr.hook);
    list_add_tail(&p->input.evts, &res->hdr.hook);

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
        STATE_SERIALIZE, STATE_SERIALIZE_DISPATCH,
        STATE_SERIALIZE_TRACE, STATE_SERIALIZE_TABLE, STATE_SERIALIZE_BINARY,
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
        if (flags & flag(PROC_CLEANUP))
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
                        b->hovered = 0;
                        b->entered = b->exited = 0;
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
        s->mod = module_find(ctx, s->id);
        if (s->mod) {
            s->mod->seq = ctx->seq;
            jmpto(ctx, STATE_CALC_REQ_MEMORY);
        }
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

        /* calculate required memory */
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
            in->resized = 0;
            root->w = root->w = in->width;
            root->h = root->h = in->height;
            if (root->w != in->width || root->h != in->height)
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
        assert(fp);

        list_foreach(si, &ctx->states) {
            /* iterate all module */
            struct state *s = list_entry(si, struct state, hook);
            union param *op = &s->param_list->ops[s->op_begin];
            while (1) {
                const struct opdef *def = opdefs + op->type;
                switch (op->type) {
                case OP_NEXT_BUF: op = (union param*)op[1].p; break;
                default: {
                    /* print out each argument from string format */
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
    if (!ctx) return;
    in = &ctx->input;

    len = (end) ? cast(int, end-buf): (int)strlen(buf);
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

/* ===========================================================================
 *
 *                                  WIDGETS
 *
 * =========================================================================== */
enum widget_type {
    /* Widgets */
    WIDGET_ICON,
    WIDGET_LABEL,
    WIDGET_BUTTON,
    WIDGET_BUTTON_LABEL,
    WIDGET_BUTTON_ICON,
    WIDGET_CHECKBOX,
    WIDGET_TOGGLE,
    WIDGET_RADIO,
    WIDGET_SLIDER,
    WIDGET_SCROLL,
    /* Layouting */
    WIDGET_SBORDER,
    WIDGET_SALIGN,
    WIDGET_SBOX,
    WIDGET_FLEX_BOX,
    WIDGET_FLEX_BOX_SLOT,
    WIDGET_OVERLAP_BOX,
    WIDGET_OVERLAP_BOX_SLOT,
    WIDGET_CON_BOX,
    /* Container */
    WIDGET_COMBO,
    WIDGET_COMBO_POPUP,
    WIDGET_COMBO_BOX,
    WIDGET_COMBO_BOX_POPUP,
    WIDGET_PANEL,
    WIDGET_PANEL_HEADER,
    WIDGET_PANEL_TOOLBAR,
    WIDGET_PANEL_STATUSBAR,
    WIDGET_SIDEBAR,
    WIDGET_SIDEBAR_BAR,
    WIDGET_SIDEBAR_SCALER,
    WIDGET_SIDEBAR_CONTENT,
    /* Transformation */
    WIDGET_CLIP_BOX,
    WIDGET_SCROLL_REGION,
    WIDGET_SCROLL_BOX,
    WIDGET_SCALER_BOX,
    WIDGET_TYPE_COUNT
};
enum widget_shortcuts {
    SHORTCUT_INTERNAL_BEGIN = 0x80,
    SHORTCUT_SCROLL_REGION_SCROLL = SHORTCUT_INTERNAL_BEGIN,
    SHORTCUT_SCROLL_REGION_RESET,
    SHORTCUT_SCROLL_BOX_BEGIN,
    SHORTCUT_SCROLL_BOX_PGDN,
    SHORTCUT_SCROLL_BOX_PGUP,
    SHORTCUT_SCROLL_BOX_END,
    SHORTCUT_SCALER_BOX_SCALE_X,
    SHORTCUT_SCALER_BOX_SCALE_Y,
    SHORTCUT_SCALER_BOX_SCALE,
    SHORTCUT_INTERNAL_END
};
enum icon_symbol {
    ICONS_INTERNAL_BEGIN = 0x100000,
    ICON_COMBO_CARRET_DOWN = ICONS_INTERNAL_BEGIN,
    ICON_UNCHECKED,
    ICON_CHECKED,
    ICON_UNTOGGLED,
    ICON_TOGGLED,
    ICON_UNSELECTED,
    ICON_SELECTED,
    ICON_SIDEBAR_LOCK,
    ICON_SIDEBAR_UNLOCK,
    ICON_SIDEBAR_OPEN,
    ICONS_INTERNAL_END
};

/* ---------------------------------------------------------------------------
 *                                  LABEL
 * --------------------------------------------------------------------------- */
typedef int(*text_measure_f)(void *usr, int font, int fh, const char *txt, int len);
struct label {
    uiid id;
    const char *txt;
    int *font;
    int *height;
};
api struct label
label_ref(struct box *b)
{
    struct label lbl;
    lbl.height = widget_get_int(b, 0);
    lbl.font = widget_get_int(b, 1);
    lbl.txt = widget_get_str(b, 2);
    return lbl;
}
api struct label
label(struct state *s, const char *txt, const char *end)
{
    int len = 0;
    struct label lbl = {0};
    const struct config *cfg = s->cfg;
    widget_begin(s, WIDGET_LABEL);
    lbl.id = widget_box_push(s);
    widget_box_pop(s);

    len = (end) ? cast(int, (end-txt)): (int)strlen(txt);
    lbl.height = widget_param_int(s, cfg->font_default_height);
    lbl.font = widget_param_int(s, cfg->font_default_id);
    lbl.txt = widget_param_str(s, txt, len);
    widget_end(s);
    return lbl;
}
api void
label_blueprint(struct box *b, void *usr, text_measure_f measure)
{
    struct label lbl = label_ref(b);
    int len = cast(int, strlen(lbl.txt));
    b->dw = measure(usr, *lbl.font, *lbl.height, lbl.txt, len);
    b->dh = *lbl.height;
}

/* ---------------------------------------------------------------------------
 *                                  ICON
 * --------------------------------------------------------------------------- */
struct icon {
    uiid id;
    int *sym;
    int *size;
};
api struct icon
icon_ref(struct box *b)
{
    struct icon ico = {0};
    ico.sym = widget_get_int(b, 0);
    ico.size = widget_get_int(b, 1);
    return ico;
}
api struct icon
icon(struct state *s, int sym)
{
    struct icon ico = {0};
    const struct config *cfg = s->cfg;
    widget_begin(s, WIDGET_ICON);
    ico.id = widget_box_push(s);
    ico.sym = widget_param_int(s, sym);
    ico.size = widget_param_int(s, cfg->font_default_height);
    widget_box_pop(s);
    widget_end(s);
    return ico;
}
api void
icon_blueprint(struct box *b)
{
    struct icon ico = icon_ref(b);
    b->dw = *ico.size;
    b->dh = *ico.size;
}
api void
icon_layout(struct box *b)
{
    struct icon ico = icon_ref(b);
    *ico.size = min(b->w, b->h);
}

/* ---------------------------------------------------------------------------
 *                                  BUTTON
 * --------------------------------------------------------------------------- */
struct button_state {
    unsigned clicked:1;
    unsigned pressed:1;
    unsigned released:1;
    unsigned entered:1;
    unsigned exited:1;
};
struct button {
    uiid id;
    int *style;
};
intern void
button_pull(struct button_state *btn, const struct box *b)
{
    btn->clicked = b->clicked;
    btn->pressed = b->pressed;
    btn->released = b->released;
    btn->entered = b->entered;
    btn->exited = b->exited;
}
api int
button_poll(struct state *s, struct button_state *st, struct button *btn)
{
    struct box *b = poll(s, btn->id);
    if (!b) {
        zero(st, szof(*st));
        return 0;
    } button_pull(st, b);
    return 1;
}
api int
button_query(struct context *ctx, struct button_state *st,
    mid mid, struct button *btn)
{
    struct box *b = 0;
    b = query(ctx, mid, btn->id);
    if (!b) return 0;
    btn->style = widget_get_int(b, 0);
    button_pull(st, b);
    return 1;
}
api struct button
button_begin(struct state *s)
{
    struct button btn = {0};
    widget_begin(s, WIDGET_BUTTON);
    btn.style =widget_param_int(s, 0);
    btn.id = widget_box_push(s);
    return btn;
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
struct slider_state {
    unsigned clicked:1;
    unsigned pressed:1;
    unsigned released:1;
    unsigned entered:1;
    unsigned exited:1;
    unsigned value_changed:1;
};
struct slider {
    uiid id;
    uiid cid;
    float *min;
    float *value;
    float *max;
    int *cursor_size;
};
api struct slider
slider_ref(struct box *b)
{
    struct slider sld;
    sld.min = widget_get_float(b, 0);
    sld.value = widget_get_float(b, 1);
    sld.max = widget_get_float(b, 2);
    sld.cursor_size = widget_get_int(b, 3);
    sld.cid = *widget_get_id(b, 4);
    return sld;
}
intern void
slider_pull(struct slider_state *sld, const struct box *b)
{
    sld->clicked = b->clicked;
    sld->pressed = b->pressed;
    sld->released = b->released;
    sld->entered = b->entered;
    sld->exited = b->exited;
    sld->value_changed = b->moved;
}
api int
slider_poll(struct state *s, struct slider_state *st, struct slider *sld)
{
    struct box *b = poll(s, sld->id);
    if (!b) return 0;
    *sld = slider_ref(b);
    slider_pull(st, b);
    return 1;
}
api int
slider_query(struct context *ctx, mid mid,
    struct slider_state *st, struct slider *sld)
{
    uiid id;
    struct box *b = 0;
    b = query(ctx, mid, sld->id);
    if (!b) return 0;
    id = sld->id;
    *sld = slider_ref(b);
    sld->id = id;
    slider_pull(st, b);
    return 1;
}
api struct slider
sliderf(struct state *s, float min, float *value, float max)
{
    struct slider sld = {0};
    const struct config *cfg = s->cfg;
    widget_begin(s, WIDGET_SLIDER);
        sld.min = widget_param_float(s, min);
        sld.value = widget_modifier_float(s, value);
        sld.max = widget_param_float(s, max);
        sld.cursor_size = widget_param_int(s, cfg->font_default_height/2);
        sld.id = widget_box_push(s);
            /* cursor */
            sld.cid = widget_box_push(s);
            widget_box_property_set(s, BOX_MOVABLE_X);
            widget_box_pop(s);
            widget_param_id(s, sld.cid);
        widget_box_pop(s);
    widget_end(s);
    return sld;
}
api void
slider_blueprint(struct box *b)
{
    struct slider sld = slider_ref(b);
    const int cursor_size = *sld.cursor_size;
    if (b->id == sld.cid) {
        b->dw = max(b->dw, cursor_size);
        b->dh = max(b->dh, cursor_size);
        box_blueprint(b, 0, 0);
    } else box_blueprint(b, 0, 0);
}
api void
slider_layout(struct box *b)
{
    struct slider sld = slider_ref(b);
    if (b->id != sld.cid)
        {box_layout(b, 0); return;}

    /* validate slider values */
    {const struct box *p = b->parent;
    float sld_min = min(*sld.min, *sld.max);
    float sld_max = max(*sld.min, *sld.max);
    float sld_val = clamp(sld_min, *sld.value, sld_max);
    float cur_off = (sld_val - sld_min) / sld_max;

    /* remove half of cursor size of each box left and right */
    const int cursor_size = *sld.cursor_size;
    int x = p->x + cursor_size/2;
    int w = p->w - cursor_size;

    /* calculate cursor bounds */
    b->w = cursor_size;
    b->h = cursor_size * 2;
    b->x = x + roundi(w * cur_off) - cursor_size/2;
    b->y = p->y + p->h/2 - b->h/2;}
}
api void
slider_input(struct box *b, const union event *evt)
{
    struct box *p = b->parent;
    struct slider sld = slider_ref(b);
    if (b->id != sld.cid) return;
    if (!b->moved || evt->type != EVT_MOVED)
        return;

    /* validate cursor pos */
    {float sld_min = min(*sld.min, *sld.max);
    float sld_max  = max(*sld.min, *sld.max);
    b->x = min(p->x + p->w - b->w, b->x);
    b->x = max(p->x, b->x);

    /* calculate slider value from cursor position */
    {const int cursor_size = *sld.cursor_size;
    int x = p->x, w = p->w - cursor_size;
    *sld.value = cast(float,(b->x - x)) / cast(float,w);
    *sld.value = (*sld.value * sld_max) + sld_min;}}
    p->moved = 1;
}
/* ---------------------------------------------------------------------------
 *                                  SCROLL
 * --------------------------------------------------------------------------- */
struct scroll {
    uiid id, cid;
    float *total_x;
    float *total_y;
    float *size_x;
    float *size_y;
    float *off_x;
    float *off_y;
};
api struct scroll
scroll_ref(struct box *b)
{
    struct scroll scrl = {0};
    scrl.total_x = widget_get_float(b, 0);
    scrl.total_y = widget_get_float(b, 1);
    scrl.size_x = widget_get_float(b, 2);
    scrl.size_y = widget_get_float(b, 3);
    scrl.off_x = widget_get_float(b, 4);
    scrl.off_y = widget_get_float(b, 5);
    scrl.cid = *widget_get_id(b, 6);
    return scrl;
}
api struct scroll
scroll_begin(struct state *s, float *off_x, float *off_y)
{
    struct scroll scrl = {0};
    widget_begin(s, WIDGET_SCROLL);
    scrl.total_x = widget_param_float(s, 1);
    scrl.total_y = widget_param_float(s, 1);
    scrl.size_x = widget_param_float(s, 1);
    scrl.size_y = widget_param_float(s, 1);
    scrl.off_x = widget_modifier_float(s, off_x);
    scrl.off_y = widget_modifier_float(s, off_y);
    scrl.id = widget_box_push(s);
    scrl.cid = widget_box_push(s);
    widget_box_property_set(s, BOX_MOVABLE);
    widget_param_id(s, scrl.cid);
    return scrl;
}
api void
scroll_end(struct state *s)
{
    widget_box_pop(s);
    widget_box_pop(s);
    widget_end(s);
}
api struct scroll
scroll(struct state *s, float *off_x, float *off_y)
{
    struct scroll scrl;
    scrl = scroll_begin(s, off_x, off_y);
    scroll_end(s);
    return scrl;
}
api void
scroll_layout(struct box *b)
{
    struct box *p = b->parent;
    struct scroll scrl = scroll_ref(b);
    if (b->id != scrl.cid) return;

    *scrl.total_x = max(*scrl.total_x, *scrl.size_x);
    *scrl.total_y = max(*scrl.total_y, *scrl.size_y);
    *scrl.off_x = clamp(0, *scrl.off_x, *scrl.total_x - *scrl.size_x);
    *scrl.off_y = clamp(0, *scrl.off_y, *scrl.total_y - *scrl.size_y);

    b->x = floori((float)p->x + (*scrl.off_x / *scrl.total_x) * (float)p->w);
    b->y = floori((float)p->y + (*scrl.off_y / *scrl.total_y) * (float)p->h);
    b->w = ceili((*scrl.size_x / *scrl.total_x) * (float)p->w);
    b->h = ceili((*scrl.size_y / *scrl.total_y) * (float)p->h);
}
api void
scroll_input(struct box *b, const union event *evt)
{
    struct box *p = b->parent;
    struct scroll scrl = scroll_ref(b);
    switch (evt->type) {
    case EVT_CLICKED: {
        if (b->id == scrl.cid) break;
        {struct list_hook *h = list_begin(&b->lnks);
        struct box *cursor = list_entry(h, struct box, node);
        int off_x = (evt->clicked.x - b->x) - floori(*scrl.size_x)/2;
        int off_y = (evt->clicked.y - b->y) - floori(*scrl.size_y)/2;

        cursor->x = b->x + clamp(0, off_x, ceili(*scrl.total_x - *scrl.size_x));
        cursor->y = b->y + clamp(0, off_y, ceili(*scrl.total_y - *scrl.size_y));
        p = b, b = cursor;}
    } /* ---- fallthrough ---- */
    case EVT_MOVED: {
        if (b->id != scrl.cid) break;
        /* calculate scroll ratio from cursor position */
        {int scrl_x = b->x - p->x, scrl_y = b->y - p->y;
        float scrl_rx = cast(float, scrl_x) / cast(float, p->w);
        float scrl_ry = cast(float, scrl_y) / cast(float, p->h);

        /* validate and set cursor position */
        *scrl.off_x = clamp(0, scrl_rx * *scrl.total_x, *scrl.total_x - *scrl.size_x);
        *scrl.off_y = clamp(0, scrl_ry * *scrl.total_y, *scrl.total_y - *scrl.size_y);}
        p->moved = 1;
    } break;}
}
/* ---------------------------------------------------------------------------
 *                                  COMBO
 * --------------------------------------------------------------------------- */
enum combo_state {
    COMBO_COLLAPSED,
    COMBO_VISIBLE
};
struct combo {
    int *state;
    uiid id;
    mid *popup;
};
api struct combo
combo_ref(struct box *b)
{
    struct combo c = {0};
    c.state = widget_get_int(b, 0);
    c.popup = widget_get_mid(b, 1);
    return c;
}
api struct combo
combo_begin(struct state *s, mid id)
{
    struct combo c = {0};
    widget_begin(s, WIDGET_COMBO);
    c.id = widget_box_push(s);
    c.state = widget_state_int(s, WIDGET_COMBO, COMBO_COLLAPSED);
    c.popup = widget_param_mid(s, id);
    return c;
}
api struct state*
combo_popup_begin(struct state *s, struct combo *c)
{
    struct state *ps = 0;
    ps = popup_begin(s, *c->popup, POPUP_NON_BLOCKING);
    widget_begin(ps, WIDGET_COMBO_POPUP);
    widget_box_push(ps);
    if (ps->mod && (ps->mod->root->flags & BOX_HIDDEN))
        *c->state = COMBO_COLLAPSED;
    return ps;
}
api void
combo_popup_end(struct state *s, struct combo *c)
{
    widget_box_pop(s);
    widget_end(s);
    if (*c->state == COMBO_COLLAPSED)
        widget_box_property_set(s, BOX_HIDDEN);
    popup_end(s);
}
api void
combo_end(struct state *s)
{
    widget_box_pop(s);
    widget_end(s);
}
api void
combo_popup_show(struct context *ctx, struct combo *c)
{
    *c->state = COMBO_VISIBLE;
    popup_show(ctx, *c->popup, VISIBLE);
}
api void
combo_popup_close(struct context *ctx, struct combo *c)
{
    *c->state = COMBO_COLLAPSED;
    popup_show(ctx, *c->popup, HIDDEN);
}
api void
combo_layout(struct context *ctx, struct box *b)
{
    struct combo c = combo_ref(b);
    struct box *p = popup_find(ctx, *c.popup);
    assert(p);
    p->x = b->x;
    p->h = p->dh;
    p->y = b->y + b->h;
    p->w = max(b->w, p->dw);
    box_layout(b, 0);
}
api void
combo_input(struct context *ctx, struct box *b, const union event *evt)
{
    struct combo c = combo_ref(b);
    if (!b->clicked || evt->type != EVT_CLICKED) return;
    if (*c.state == COMBO_COLLAPSED)
        combo_popup_show(ctx, &c);
    else combo_popup_close(ctx, &c);
}
/* ---------------------------------------------------------------------------
 *                                  SBORDER
 * --------------------------------------------------------------------------- */
struct sborder {
    uiid id;
    uiid content;
    int *x;
    int *y;
};
api struct sborder
sborder_ref(struct box *b)
{
    struct sborder sbx;
    sbx.x = widget_get_int(b, 0);
    sbx.y = widget_get_int(b, 1);
    sbx.content = *widget_get_id(b, 2);
    sbx.id = b->id;
    return sbx;
}
api struct sborder
sborder_begin(struct state *s)
{
    struct sborder sbx;
    widget_begin(s, WIDGET_SBORDER);
    sbx.x = widget_param_int(s, 6);
    sbx.y = widget_param_int(s, 6);

    sbx.id = widget_box_push(s);
    sbx.content = widget_box_push(s);
    widget_param_id(s, sbx.content);
    return sbx;
}
api void
sborder_end(struct state *s)
{
    widget_box_pop(s);
    widget_box_pop(s);
    widget_end(s);
}
api void
sborder_blueprint(struct box *b)
{
    struct sborder sbx = sborder_ref(b);
    if (b->id != sbx.content)
        box_blueprint(b, *sbx.x, *sbx.y);
    else box_blueprint(b, 0, 0);
}
api void
sborder_layout(struct box *b)
{
    struct box *p = b->parent;
    struct sborder sbx = sborder_ref(b);
    if (b->id != sbx.content) return;
    box_pad(b, p, *sbx.x, *sbx.y);
    box_layout(b, 0);
}
/* ---------------------------------------------------------------------------
 *                                  SALIGN
 * --------------------------------------------------------------------------- */
enum salign_vertical {
    SALIGN_TOP,
    SALIGN_MIDDLE,
    SALIGN_BOTTOM
};
enum salign_horizontal {
    SALIGN_LEFT,
    SALIGN_CENTER,
    SALIGN_RIGHT
};
struct salign {
    uiid id;
    uiid content;
    int *vertical;
    int *horizontal;
};
api struct salign
salign_ref(struct box *b)
{
    struct salign aln;
    aln.vertical = widget_get_int(b, 0);
    aln.horizontal = widget_get_int(b, 1);
    aln.content = *widget_get_id(b, 2);
    aln.id = b->id;
    return aln;
}
api struct salign
salign_begin(struct state *s)
{
    struct salign aln;
    widget_begin(s, WIDGET_SALIGN);
    aln.vertical = widget_param_int(s, SALIGN_TOP);
    aln.horizontal = widget_param_int(s, SALIGN_LEFT);
    aln.id = widget_box_push(s);
    aln.content = widget_box_push(s);
    widget_param_id(s, aln.content);
    return aln;
}
api void
salign_end(struct state *s)
{
    widget_box_pop(s);
    widget_box_pop(s);
    widget_end(s);
}
api void
salign_layout(struct box *b)
{
    struct box *p = b->parent;
    struct salign aln = salign_ref(b);
    struct list_hook *h = 0;
    if (b->id != aln.content) return;
    box_pad(b, p, 0, 0);

    /* horizontal alignment */
    switch (*aln.horizontal) {
    case SALIGN_LEFT:
        list_foreach(h, &b->lnks) {
            struct box *n = list_entry(h, struct box, node);
            n->x = b->x;
            n->w = min(b->w, n->dw);
        } break;
    case SALIGN_CENTER:
        list_foreach(h, &b->lnks) {
            struct box *n = list_entry(h, struct box, node);
            n->w = max((b->x + b->w) - n->x, 0);
            n->w = min(b->w, n->dw);
            n->x = max(b->x, (b->x + b->w/2) - n->w/2);
        } break;
    case SALIGN_RIGHT:
        list_foreach(h, &b->lnks) {
            struct box *n = list_entry(h, struct box, node);
            n->w = min(n->dw, b->w);
            n->x = max(b->x, b->x + b->w - n->w);
        } break;}

    /* vertical alignment */
    switch (*aln.vertical) {
    case SALIGN_TOP:
        list_foreach(h, &b->lnks) {
            struct box *n = list_entry(h, struct box, node);
            n->h = min(n->dh, b->h);
            n->y = b->y;
        } break;
    case SALIGN_MIDDLE:
        list_foreach(h, &b->lnks) {
            struct box *n = list_entry(h, struct box, node);
            n->y = max(b->y,(b->y + b->h/2) - n->dh/2);
            n->h = max((b->y + b->h) - n->y, 0);
        } break;
    case SALIGN_BOTTOM:
        list_foreach(h, &b->lnks) {
            struct box *n = list_entry(h, struct box, node);
            n->y = max(b->y,b->y + b->h - n->dh);
            n->h = min(n->dh, n->h);
        } break;}

    /* reshape sbox after content */
    {int x = b->x + b->w;
    int y = b->y + b->h;
    list_foreach(h, &b->lnks) {
        struct box *n = list_entry(h, struct box, node);
        b->x = max(b->x, n->x);
        b->y = max(b->y, n->y);
        x = min(x, n->x + n->w);
        y = min(y, n->y + n->h);
    }
    b->w = x - b->x;
    b->h = y - b->y;
    box_pad(p, b, 0, 0);}
    box_layout(b, 0);
}
/* ---------------------------------------------------------------------------
 *                                  SBOX
 * --------------------------------------------------------------------------- */
struct sbox {
    uiid id;
    struct salign align;
    struct sborder border;
};
api struct sbox
sbox_begin(struct state *s)
{
    struct sbox sbx = {0};
    widget_begin(s, WIDGET_SBOX);
    sbx.id = widget_box_push(s);
    sbx.border = sborder_begin(s);
    sbx.align = salign_begin(s);
    return sbx;
}
api void
sbox_end(struct state *s)
{
    salign_end(s);
    sborder_end(s);
    widget_box_pop(s);
    widget_end(s);
}
/* ---------------------------------------------------------------------------
 *                                  FLEX BOX
 * --------------------------------------------------------------------------- */
enum flex_box_orientation {
    FLEX_BOX_HORIZONTAL,
    FLEX_BOX_VERTICAL
};
enum flex_box_flow {
    FLEX_BOX_STRETCH,
    FLEX_BOX_FIT,
    FLEX_BOX_WRAP
};
enum flex_box_slot_type {
    FLEX_BOX_SLOT_DYNAMIC,
    FLEX_BOX_SLOT_STATIC,
    FLEX_BOX_SLOT_VARIABLE,
    FLEX_BOX_SLOT_FITTING
};
struct flex_box {
    uiid id;
    int *orientation;
    int *flow;
    int *spacing;
    int *padding;
    int *cnt;
};
api struct flex_box
flex_box_ref(struct box *b)
{
    struct flex_box fbx;
    fbx.id = *widget_get_id(b, 0);
    fbx.orientation = widget_get_int(b, 1);
    fbx.flow = widget_get_int(b, 2);
    fbx.spacing = widget_get_int(b, 3);
    fbx.padding = widget_get_int(b, 4);
    fbx.cnt = widget_get_int(b, 5);
    return fbx;
}
api struct flex_box
flex_box_begin(struct state *s)
{
    struct flex_box fbx;
    widget_begin(s, WIDGET_FLEX_BOX);
    fbx.id = widget_box_push(s);
    widget_param_id(s, fbx.id);
    fbx.orientation = widget_param_int(s, FLEX_BOX_HORIZONTAL);
    fbx.flow = widget_param_int(s, FLEX_BOX_STRETCH);
    fbx.spacing = widget_param_int(s, 4);
    fbx.padding = widget_param_int(s, 4);
    fbx.cnt = widget_param_int(s, 0);
    return fbx;
}
intern void
flex_box_slot(struct state *s, struct flex_box *fbx,
    enum flex_box_slot_type type, int value)
{
    uiid id = 0;
    int idx = *fbx->cnt;
    if (idx) {
        widget_box_pop(s);
        widget_end(s);
    }
    widget_begin(s, WIDGET_FLEX_BOX_SLOT);
    id = widget_box_push(s);
    widget_param_int(s, type);
    widget_param_int(s, value);
    *fbx->cnt = *fbx->cnt + 1;
}
api void
flex_box_slot_dyn(struct state *s, struct flex_box *fbx)
{
    assert(*fbx->flow != FLEX_BOX_WRAP);
    flex_box_slot(s, fbx, FLEX_BOX_SLOT_DYNAMIC, 0);
}
api void
flex_box_slot_static(struct state *s,
    struct flex_box *fbx, int pixel_width)
{
    flex_box_slot(s, fbx, FLEX_BOX_SLOT_STATIC, pixel_width);
}
api void
flex_box_slot_variable(struct state *s,
    struct flex_box *fbx, int min_pixel_width)
{
    assert(*fbx->flow != FLEX_BOX_WRAP);
    flex_box_slot(s, fbx, FLEX_BOX_SLOT_VARIABLE, min_pixel_width);
}
api void
flex_box_slot_fitting(struct state *s, struct flex_box *fbx)
{
    flex_box_slot(s, fbx, FLEX_BOX_SLOT_FITTING, 0);
}
api void
flex_box_end(struct state *s, struct flex_box *fbx)
{
    int idx = *fbx->cnt;
    if (idx) {
        widget_box_pop(s);
        widget_end(s);
    } widget_box_pop(s);
    widget_end(s);
}
api void
flex_box_blueprint(struct box *b)
{
    struct list_hook *it = 0;
    struct flex_box fbx = flex_box_ref(b);
    b->dw = b->dh = 0;

    list_foreach(it, &b->lnks) {
        struct box *sb = list_entry(it, struct box, node);
        int styp = *widget_get_int(sb, 0);
        int spix = *widget_get_int(sb, 1);

        /* horizontal layout */
        switch (*fbx.orientation) {
        case FLEX_BOX_HORIZONTAL: {
            switch (styp) {
            case FLEX_BOX_SLOT_DYNAMIC:
                b->dw += sb->dw; break;
            case FLEX_BOX_SLOT_STATIC:
                b->dw += spix; break;
            case FLEX_BOX_SLOT_FITTING:
            case FLEX_BOX_SLOT_VARIABLE:
                b->dw += max(sb->dw, spix); break;
            } b->dh = max(sb->dh + *fbx.padding*2, b->dh);
        } break;

        /* vertical layout */
        case FLEX_BOX_VERTICAL: {
            switch (styp) {
            case FLEX_BOX_SLOT_DYNAMIC:
                b->dh += sb->dh; break;
            case FLEX_BOX_SLOT_STATIC:
                b->dh += spix; break;
            case FLEX_BOX_SLOT_FITTING:
            case FLEX_BOX_SLOT_VARIABLE:
                b->dh += max(sb->dh, spix); break;
            } b->dw = max(sb->dw + *fbx.padding*2, b->dw);
        } break;}
    }
    /* padding + spacing  */
    {int pad = *fbx.spacing * (*fbx.cnt-1) + *fbx.padding * 2;
    switch (*fbx.orientation) {
    case FLEX_BOX_HORIZONTAL: b->dw += pad; break;
    case FLEX_BOX_VERTICAL: b->dh += pad; break;}}
}
api void
flex_box_layout(struct box *b)
{
    struct list_hook *it;
    struct flex_box fbx = flex_box_ref(b);

    int space = 0;
    int slot_size;
    int varcnt = 0, staticsz = 0, fixsz = 0, maxvar = 0;
    int dynsz = 0, varsz = 0, var = 0, dyncnt = 0;

    /* calculate space for widgets without padding and spacing */
    if (*fbx.orientation == FLEX_BOX_HORIZONTAL)
        space = max(b->w - (*fbx.cnt-1)**fbx.spacing, 0), slot_size = b->h;
    else space = max(b->h - (*fbx.cnt-1)**fbx.spacing, 0), slot_size = b->w;
    space = max(0, space - *fbx.padding*2);

    if (*fbx.flow == FLEX_BOX_FIT || *fbx.flow == FLEX_BOX_WRAP) {
        /* fit flex box to content */
        if (*fbx.orientation == FLEX_BOX_HORIZONTAL) {
            list_foreach(it, &b->lnks) {
                struct box *n = list_entry(it, struct box, node);
                slot_size = max(slot_size, n->dh + *fbx.padding*2);
            } slot_size = min(b->dh, slot_size);
            if (*fbx.flow == FLEX_BOX_FIT)
                b->h = slot_size;
        } else {
            list_foreach(it, &b->lnks) {
                struct box *n = list_entry(it, struct box, node);
                slot_size = max(slot_size, n->dw + *fbx.padding*2);
            } slot_size = min(b->dw, slot_size);
            if (*fbx.flow == FLEX_BOX_FIT)
                b->w = slot_size;
        }
    }
    /* calculate space requirements and slot metrics */
    list_foreach(it, &b->lnks) {
        struct box *sb = list_entry(it, struct box, node);
        int slot_typ = *widget_get_int(sb, 0);
        int *slot_pix = widget_get_int(sb, 1);

        /* calculate min size and dynamic size  */
        switch (slot_typ) {
        case FLEX_BOX_SLOT_DYNAMIC: {
            varcnt++, dyncnt++;
        } break;
        case FLEX_BOX_SLOT_FITTING:
            if (*fbx.orientation == FLEX_BOX_HORIZONTAL)
                *slot_pix = max(*slot_pix, sb->dw);
            else *slot_pix = max(*slot_pix, sb->dh);
        case FLEX_BOX_SLOT_STATIC: {
            staticsz += *slot_pix;
        } break;
        case FLEX_BOX_SLOT_VARIABLE: {
            fixsz += *slot_pix;
            maxvar = max(*slot_pix, maxvar);
            varcnt++;
        } break;}

        /* setup opposite orientation position and size */
        if (*fbx.orientation == FLEX_BOX_HORIZONTAL)
            sb->h = max(0, slot_size - *fbx.padding*2);
        else sb->w = max(0, slot_size - *fbx.padding*2);
    }
    /* calculate dynamic slot size */
    dynsz = max(space - staticsz, 0);
    varsz = max(dynsz - fixsz, 0);
    if (varsz) {
        if (varcnt) {
            assert(*fbx.flow != FLEX_BOX_WRAP);
            var = dynsz / max(varcnt,1);
            if (maxvar > var) {
                /* not enough space so shrink dynamic space */
                list_foreach(it, &b->lnks) {
                    struct box *sb = list_entry(it, struct box, node);
                    int slot_typ = *widget_get_int(sb, 0);
                    int slot_pix = *widget_get_int(sb, 1);

                    if (slot_pix <= var) continue;
                    switch (slot_typ) {
                    case FLEX_BOX_SLOT_FITTING:
                    case FLEX_BOX_SLOT_VARIABLE: {
                        staticsz += slot_pix;
                        varcnt--;
                    } break;}
                }
                dynsz = max(space - staticsz, 0);
                var = dynsz / max(varcnt,1);
            }
        } else var = dynsz / max(varcnt + dyncnt,1);
    } else var = 0;

    /* set position and size */
    {int total_w = b->w, total_h = b->h;
    int posx = b->x + *fbx.padding, posy = b->y + *fbx.padding, cnt = 0;
    list_foreach(it, &b->lnks) {
        struct box *sb = list_entry(it, struct box, node);
        int slot_typ = *widget_get_int(sb, 0);
        int slot_pix = *widget_get_int(sb, 1);

        /* setup slot size (width/height) */
        switch (*fbx.orientation) {
        case FLEX_BOX_HORIZONTAL: {
            if (*fbx.flow == FLEX_BOX_WRAP && cnt) {
                if (posx + slot_pix > b->x + (b->w - *fbx.padding*2)) {
                    posx = b->x, posy += slot_size + *fbx.spacing;
                    total_h = max(total_h, posy - b->y + *fbx.padding), cnt = 0;
                }
            } sb->x = posx, sb->y = posy;

            switch (slot_typ) {
            case FLEX_BOX_SLOT_DYNAMIC: sb->w = var; break;
            case FLEX_BOX_SLOT_STATIC:  sb->w = slot_pix; break;
            case FLEX_BOX_SLOT_FITTING: sb->w = slot_pix; break;
            case FLEX_BOX_SLOT_VARIABLE:
                sb->w = (slot_pix > var) ? slot_pix: var; break;}

            cnt++;
            total_w = max(total_w, (posx + sb->w + *fbx.padding) - b->x);
            posx += *fbx.spacing + sb->w;
        } break;
        case FLEX_BOX_VERTICAL: {
            if (*fbx.flow == FLEX_BOX_WRAP && cnt) {
                if (posy + slot_pix > b->y + (b->h - *fbx.padding*2)) {
                    posy = b->y, posx += slot_size + *fbx.spacing;
                    total_w = max(total_w, posx - b->x + *fbx.padding), cnt = 0;
                }
            } sb->x = posx, sb->y = posy;

            switch (slot_typ) {
            case FLEX_BOX_SLOT_DYNAMIC: sb->h = var; break;
            case FLEX_BOX_SLOT_STATIC:  sb->h = slot_pix; break;
            case FLEX_BOX_SLOT_FITTING: sb->h = slot_pix; break;
            case FLEX_BOX_SLOT_VARIABLE:
                sb->h = (slot_pix > var) ? slot_pix: var; break;}

            cnt++;
            total_h = max(total_h, (posy + sb->h + *fbx.padding) - b->y);
            posy += sb->h + *fbx.spacing;
        } break;}
    }
    b->w = total_w;
    b->h = total_h;}
}
/* ---------------------------------------------------------------------------
 *                                  OVERLAP BOX
 * --------------------------------------------------------------------------- */
struct overlap_box {
    uiid id;
    int *cnt;
};
api struct overlap_box
overlap_box_ref(struct box *b)
{
    struct overlap_box obx = {0};
    obx.cnt = widget_get_int(b, 0);
    return obx;
}
api struct overlap_box
overlap_box_begin(struct state *s)
{
    struct overlap_box obx = {0};
    widget_begin(s, WIDGET_OVERLAP_BOX);
    obx.id = widget_box_push(s);
    widget_box_property_set(s, BOX_UNSELECTABLE);
    obx.cnt = widget_param_int(s, 0);
    return obx;
}
api void
overlap_box_slot(struct state *s, struct overlap_box *obx, mid id)
{
    uiid slot_id = 0;
    int idx = *obx->cnt;
    if (idx) {
        widget_box_pop(s);
        widget_end(s);
        popid(s);
    } pushid(s, id);

    widget_begin(s, WIDGET_OVERLAP_BOX_SLOT);
    slot_id = widget_box_push(s);
    widget_box_property_set(s, BOX_UNSELECTABLE);
    widget_param_id(s, slot_id);
    widget_state_int(s, WIDGET_OVERLAP_BOX, 0);
    *obx->cnt += 1;
}
api void
overlap_box_end(struct state *s, struct overlap_box *obx)
{
    if (*obx->cnt) {
        widget_box_pop(s);
        widget_end(s);
        popid(s);
    }
    widget_box_pop(s);
    widget_end(s);
}
api void
overlap_box_layout(struct box *b, struct memory_arena *arena)
{
    int i = 0;
    int slot_cnt = 0;
    struct box **boxes = 0;
    struct list_hook *it = 0, *sit = 0;
    struct temp_memory tmp = temp_memory_begin(arena);
    box_layout(b, 0);

    /* find maximum zorder */
    list_foreach(it, &b->lnks) {
        struct box *n = list_entry(it, struct box, node);
        int slot_zorder = *widget_get_int(n, 1);
        slot_cnt = max(slot_zorder, slot_cnt);
    } slot_cnt += 1;
    /* allocate and setup temp sorting array */
    boxes = arena_push_array(arena, slot_cnt, struct box*);
    list_foreach_s(it, sit, &b->lnks) {
        struct box *n = list_entry(it, struct box, node);
        int zorder = *widget_get_int(n, 1);
        if (!zorder) continue;
        list_del(&n->node);
        boxes[zorder] = n;
    }
    /* add boxes back in sorted order */
    for (i = slot_cnt-1; i >= 0; --i) {
        if (!boxes[i]) continue;
        list_add_head(&b->lnks, &boxes[i]->node);
    } i = 1;
    /* set correct zorder for each child box */
    list_foreach(it, &b->lnks) {
        struct box *n = list_entry(it, struct box, node);
        int *zorder = widget_get_int(n, 1);
        *zorder = i++;
    } temp_memory_end(tmp);
}
api void
overlap_box_input(struct box *b, union event *evt, struct memory_arena *arena)
{
    int i = 0;
    struct box *slot = 0;
    if (evt->type != EVT_CLICKED)
        return;

    /* find clicked on slot */
    for (i = 0; i < evt->hdr.cnt; ++i) {
        struct box *s = evt->hdr.boxes[i];
        struct list_hook *it = 0;
        list_foreach(it, &b->lnks) {
            struct box *n = list_entry(it, struct box, node);
            if (n != s) continue;
            slot = n; break;
        }
    } if (!slot) return;

    /* reorder and rebalance overlap stack */
    {int p = 0, zorder = 0;
    struct overlap_box obx;
    obx = overlap_box_ref(b);
    for (i = 0, p = 1; i < *obx.cnt; ++i, p += 2) {
        uiid slot_id = *widget_get_id(b, p);
        int *slot_zorder = widget_get_int(b, p + 1);
        if (slot_id != slot->id) continue;

        zorder = *slot_zorder;
        *slot_zorder = *obx.cnt + 1;
        break;
    }
    /* set each slots zorder */
    for (i = 0, p = 2; i < *obx.cnt; ++i, p += 2) {
        int *slot_zorder = widget_get_int(b, p);
        if (*slot_zorder > zorder)
            *slot_zorder -= 1;
    }} overlap_box_layout(b, arena);
}
/* ---------------------------------------------------------------------------
 *                          CONSTRAINT BOX
 * --------------------------------------------------------------------------- */
#define SPARENT (-1)
enum expr_op {EXPR_AND, EXPR_OR};
enum cond_eq {COND_EQ, COND_NE, COND_GR, COND_GE, COND_LS, COND_LE};
enum con_eq {CON_NOP, CON_SET, CON_MIN, CON_MAX};
enum con_attr {ATTR_NONE, ATTR_L, ATTR_T, ATTR_R, ATTR_B, ATTR_CX,
    ATTR_CY, ATTR_W, ATTR_H, ATTR_DW, ATTR_DH};

struct cons {float mul; int off;};
struct convar {int slot; int attr;};
struct cond {unsigned char op; struct convar a, b; struct cons cons;};
struct con {int op; struct convar dst, src; struct cons cons; int anchor, cond;};

struct con_box {
    uiid id;
    int *con_cnt;
    int *cond_cnt;
    int *slot_cnt;
    union param *conds;
};
api struct con_box
con_box_ref(struct box *b)
{
    struct con_box cbx = {0};
    cbx.cond_cnt = widget_get_int(b, 0);
    cbx.slot_cnt = widget_get_int(b, 1);
    cbx.conds = widget_get_param(b, 2);
    return cbx;
}
api struct con_box
con_box_begin(struct state *s,
    struct cond *conds, int cond_cnt)
{
    int i = 0;
    struct con_box cbx;

    /* setup con box */
    widget_begin(s, WIDGET_CON_BOX);
    cbx.id = widget_box_push(s);
    widget_box_property_set(s, BOX_UNSELECTABLE);
    cbx.cond_cnt = widget_param_int(s, cond_cnt);
    cbx.slot_cnt = widget_param_int(s, 0);

    /* push conditions */
    for (i = 0; i < cond_cnt; ++i) {
        const struct cond *c = conds + i;
        widget_param_int(s, c->op);
        widget_param_int(s, c->a.slot);
        widget_param_int(s, c->a.attr);
        widget_param_int(s, c->b.slot);
        widget_param_int(s, c->b.attr);
        widget_param_float(s, c->cons.mul);
        widget_param_int(s, c->cons.off);
    } return cbx;
}
api void
con_box_slot(struct state *s, struct con_box *cbx,
    const struct con *cons, int con_cnt)
{
    int i = 0;
    widget_param_int(s, con_cnt);
    for (i = 0; i < con_cnt; ++i) {
        const struct con *c = cons + i;
        widget_param_int(s, c->op);
        widget_param_int(s, c->dst.slot);
        widget_param_int(s, c->dst.attr);
        widget_param_int(s, c->src.slot);
        widget_param_int(s, c->src.attr);
        widget_param_float(s, c->cons.mul);
        widget_param_int(s, c->cons.off);
        widget_param_int(s, c->anchor);
        widget_param_int(s, c->cond);
    } *cbx->slot_cnt += 1;
}
api void
con_box_end(struct state *s, struct con_box *cbx)
{
    widget_box_pop(s);
    widget_end(s);
}
intern int
box_attr_get(const struct box *b, int attr)
{
    switch (attr) {
    default: return 0;
    case ATTR_L: return b->x;
    case ATTR_T: return b->y;
    case ATTR_R: return b->x + b->w;
    case ATTR_B: return b->y + b->h;
    case ATTR_CX: return b->x + b->w/2;
    case ATTR_CY: return b->y + b->h/2;
    case ATTR_W: return b->w;
    case ATTR_H: return b->h;
    case ATTR_DW: return b->dw;
    case ATTR_DH: return b->dh;}
}
intern void
box_attr_set(struct box *b, int attr, int anchor, int val)
{
    switch (attr) {
    default: return;
    case ATTR_L: {
        if (anchor == ATTR_R)
            b->w = (b->x + b->w) - val;
        else if (anchor == ATTR_CX)
            b->w = ((b->x + b->w/2) - val) * 2;
        b->x = val;
    } break;
    case ATTR_T: {
        if (anchor == ATTR_B)
            b->h = (b->y + b->h) - val;
        else if (anchor == ATTR_CY)
            b->h = ((b->y + b->h/2) - val) * 2;
        b->y = val;
    } break;
    case ATTR_R: {
        if (anchor == ATTR_L)
            b->w = val - b->x;
        else if (anchor == ATTR_CX)
            b->w = (val - (b->x + b->w/2)) * 2;
        b->x = val - b->w;
    } break;
    case ATTR_B: {
        if (anchor == ATTR_T)
            b->h = val - b->y;
        else if (anchor == ATTR_CY)
            b->h = (val - (b->y + b->h/2)) * 2;
        b->y = val - b->h;
    } break;
    case ATTR_CX: {
        if (anchor == ATTR_L)
            b->w = (val - b->x) * 2;
        else if (anchor == ATTR_R)
            b->w = ((b->x + b->w) - val) * 2;
        b->x = val - b->w/2;
    } break;
    case ATTR_CY: {
        if (anchor == ATTR_T)
            b->h = (val - b->y) * 2;
        else if (anchor == ATTR_B)
            b->h = ((b->y + b->h) - val) * 2;
        b->y = val - b->h/2;
    } break;
    case ATTR_W: {
        if (anchor == ATTR_CX)
            b->y = (b->y + b->w/2) - val;
        else if (anchor == ATTR_R)
            b->y = (b->y + b->w) - val;
        b->w = val;
    } break;
    case ATTR_H: {
        if (anchor == ATTR_CY)
            b->y = (b->y + b->h/2) - val;
        else if (anchor == ATTR_B)
            b->y = (b->y + b->h) - val;
        b->h = val;
    } break;}
}
api void
con_box_layout(struct box *b, struct memory_arena *arena)
{
    int poff, j, i = 0;
    struct list_hook *it = 0;
    struct con_box cbx = con_box_ref(b);
    struct temp_memory tmp = temp_memory_begin(arena);
    int *res = arena_push_array(arena, *cbx.cond_cnt, int);

    /* setup boxes into slots */
    struct box **boxes = arena_push_array(arena, *cbx.slot_cnt, struct box*);
    boxes[i++] = b;
    list_foreach(it, &b->lnks)
        boxes[i++] = list_entry(it, struct box, node);

    box_layout(b, 0);
    /* evaluate conditions */
    for (i = 0; i < *cbx.cond_cnt; ++i) {
        int av = 0, bv = 0;
        const struct box *dst, *src;
        union param *p = cbx.conds + i*7;

        /* condition left operand */
        assert(p[1].i < *cbx.cond_cnt);
        dst = boxes[p[1].i + 1];
        av = box_attr_get(dst, p[2].i);

        /* condition right operand */
        assert(p[3].i < *cbx.cond_cnt);
        src = boxes[p[3].i + 1];
        bv = box_attr_get(src, p[4].i);

        /* eval condition */
        switch (p[0].i) {
        case COND_EQ: res[i+1] = (av == bv); break;
        case COND_NE: res[i+1] = (av != bv); break;
        case COND_GR: res[i+1] = (av >  bv); break;
        case COND_GE: res[i+1] = (av >= bv); break;
        case COND_LS: res[i+1] = (av <  bv); break;
        case COND_LE: res[i+1] = (av <= bv); break;}
    }
    /* evaluate constraints */
    for (i = 0, poff = 2 + (*cbx.cond_cnt * 7); i < *cbx.slot_cnt; ++i) {
        int *con_cnt = widget_get_int(b, poff++);
        for (j = 0; j < *con_cnt; ++j) {
            struct box *dst, *src;
            union param *p = widget_get_param(b, (j * 9) + poff);
            assert((p[8].i < *cbx.cond_cnt) || (!(*cbx.cond_cnt)));
            if (*cbx.cond_cnt && (res[p[8].i] >= *cbx.cond_cnt))
                continue;

            /* left value */
            {int av = 0, bv = 0, v = 0;
            assert(p[1].i < *cbx.slot_cnt);
            dst = boxes[p[1].i + 1];
            av = box_attr_get(dst, p[2].i);

            /* right value */
            assert(p[3].i < *cbx.slot_cnt);
            src = boxes[p[3].i + 1];
            bv = box_attr_get(src, p[4].i);
            v = roundi(cast(float, bv) * p[5].f) + p[6].i;

            /* eval attribute */
            switch (p[0].i) {
            case CON_NOP: break;
            case CON_SET: box_attr_set(dst, p[2].i, p[7].i, v); break;
            case CON_MIN: box_attr_set(dst, p[2].i, p[7].i, min(av, v)); break;
            case CON_MAX: box_attr_set(dst, p[2].i, p[7].i, max(av, v)); break;}}
        } poff += *con_cnt * 9;
    } temp_memory_end(tmp);

    /* reshape after contents */
    {int x0 = INT_MAX, y0 = INT_MAX;
    int x1 = INT_MIN, y1 = INT_MIN;
    list_foreach(it, &b->lnks) {
        struct box *n = list_entry(it, struct box, node);
        x0 = min(x0, n->x);
        y0 = min(y0, n->y);
        x1 = max(x1, n->x + n->w);
        y1 = max(y1, n->y + n->h);
    }
    b->x = x0, b->y = y0;
    b->w = x1 - x0, b->h = y1 - y0;}
}
/* ---------------------------------------------------------------------------
 *                                  CLIP BOX
 * --------------------------------------------------------------------------- */
api uiid
clip_box_begin(struct state *s)
{
    assert(s);
    if (!s) return 0;
    widget_begin(s, WIDGET_CLIP_BOX);
    return widget_box_push(s);
}
api void
clip_box_end(struct state *s)
{
    uiid id;
    assert(s);
    if (!s) return;

    id = widget_box_push(s);
    widget_param_id(s, id);
    widget_box_property_set(s, BOX_IMMUTABLE);

    widget_box_pop(s);
    widget_box_pop(s);
    widget_end(s);
}
/* ---------------------------------------------------------------------------
 *                              SCROLL REGION
 * --------------------------------------------------------------------------- */
enum scroll_direction {
    SCROLL_DEFAULT,
    SCROLL_REVERSE
};
struct scroll_region {
    uiid id;
    int *dir;
    float *off_x;
    float *off_y;
};
api struct scroll_region
scroll_region_ref(struct box *b)
{
    struct scroll_region sr = {0};
    sr.dir = widget_get_int(b, 0);
    sr.off_x = widget_get_float(b, 1);
    sr.off_y = widget_get_float(b, 2);
    sr.id = *widget_get_id(b, 3);
    return sr;
}
api struct scroll_region
scroll_region_begin(struct state *s)
{
    struct scroll_region sr = {0};
    assert(s);
    if (!s) return sr;

    widget_begin(s, WIDGET_SCROLL_REGION);
    sr.dir = widget_state_int(s, WIDGET_SCROLL_REGION, SCROLL_DEFAULT);
    sr.off_x = widget_state_float(s, WIDGET_SCROLL_REGION, 0);
    sr.off_y = widget_state_float(s, WIDGET_SCROLL_REGION, 0);
    sr.id = widget_box_push(s);
    widget_param_id(s, sr.id);
    return sr;
}
api void
scroll_region_end(struct state *s)
{
    assert(s);
    if (!s) return;
    clip_box_end(s);
}
api void
scroll_region_layout(struct box *b)
{
    struct list_hook *i = 0;
    struct scroll_region sr;
    assert(b);

    if (!b) return;
    sr = scroll_region_ref(b);
    if (b->id != sr.id) return;

    list_foreach(i, &b->lnks) {
        struct box *n = list_entry(i, struct box, node);
        n->x = b->x - floori(*sr.off_x);
        n->y = b->y - floori(*sr.off_y);
        n->w = b->w, n->h = b->h;
    }
}
api void
scroll_region_input(struct box *b, const union event *evt)
{
    struct input *in = evt->hdr.input;
    struct scroll_region sr = scroll_region_ref(b);
    if (evt->type == EVT_DRAGGED && in->shortcuts[SHORTCUT_SCROLL_REGION_SCROLL].down) {
        b->moved = 1;
        switch (*sr.dir) {
        case SCROLL_DEFAULT:
            *sr.off_x -= evt->moved.x;
            *sr.off_y -= evt->moved.y;
            break;
        case SCROLL_REVERSE:
            *sr.off_x += evt->moved.x;
            *sr.off_y += evt->moved.y;
            break;
        }
    } else if (in->shortcuts[SHORTCUT_SCROLL_REGION_RESET].down)
        *sr.off_x = *sr.off_y = 0;
}
/* ---------------------------------------------------------------------------
 *                              SCROLL BOX
 * --------------------------------------------------------------------------- */
api uiid
scroll_box_begin(struct state *s)
{
    uiid sid = 0;
    float tmpx = 0, tmpy = 0;
    struct scroll x, y;
    struct scroll_region sr;
    assert(s);
    if (!s) return 0;

    widget_begin(s, WIDGET_SCROLL_BOX);
    sid = widget_box_push(s);
    x = scroll(s, &tmpx, &tmpy);
    y = scroll(s, &tmpx, &tmpy);
    sr = scroll_region_begin(s);

    *x.off_x = *sr.off_x;
    *y.off_y = *sr.off_y;
    return sid;
}
api void
scroll_box_end(struct state *s)
{
    assert(s);
    if (!s) return;
    scroll_region_end(s);
    widget_box_pop(s);
    widget_end(s);
}
api void
scroll_box_blueprint(struct box *b)
{
    static const int scroll_size = 10;
    struct list_hook *h = b->lnks.prev;
    struct box *bsr = list_entry(h, struct box, node);
    b->dw = max(b->dw, bsr->dw + scroll_size);
    b->dh = max(b->dh, bsr->dh + scroll_size);
}
api void
scroll_box_layout(struct box *b)
{
    /* retrieve pointer for each widget */
    static const int scroll_size = 10;
    struct list_hook *hx = b->lnks.next;
    struct list_hook *hy = hx->next;
    struct list_hook *hsr = hy->next;

    struct box *bx = list_entry(hx, struct box, node);
    struct box *by = list_entry(hy, struct box, node);
    struct box *bsr = list_entry(hsr, struct box, node);

    struct scroll x = scroll_ref(bx);
    struct scroll y = scroll_ref(by);
    struct scroll_region sr = scroll_region_ref(bsr);

    /* x-scrollbar */
    bx->x = b->x;
    bx->y = b->y + b->h - scroll_size;
    bx->w = b->w - scroll_size;
    bx->h = scroll_size;

    /* y-scrollbar */
    by->x = b->x + b->w - scroll_size;
    by->y = b->y;
    by->w = scroll_size;
    by->h = b->h - scroll_size;

    /* scroll region */
    bsr->x = b->x;
    bsr->y = b->y;
    bsr->w = b->w - scroll_size;
    bsr->h = b->h - scroll_size;

    if (bx->moved)
        *sr.off_x = *x.off_x;
    else *x.off_x = *sr.off_x;
    *x.size_x = bsr->w;
    *x.total_x = bsr->dw;

    if (by->moved)
        *sr.off_y = *y.off_y;
    else *y.off_y = *sr.off_y;
    *y.size_y = bsr->h;
    *y.total_y = bsr->dh;
}
api void
scroll_box_input(struct context *ctx, struct box *b, union event *evt)
{
    struct list_hook *hsr = list_last(&b->lnks);
    struct list_hook *hy = list_prev(hsr);
    struct list_hook *hx = list_prev(hy);

    struct box *bx = list_entry(hx, struct box, node);
    struct box *by = list_entry(hy, struct box, node);
    struct box *bsr = list_entry(hsr, struct box, node);

    struct scroll x = scroll_ref(bx);
    struct scroll y = scroll_ref(by);
    struct scroll_region sr = scroll_region_ref(bsr);

    switch (evt->type) {
    case EVT_CLICKED:
    case EVT_MOVED: {
        /* scrollbars */
        if (bx->moved) *sr.off_x = *x.off_x;
        if (by->moved) *sr.off_y = *y.off_y;
    } break;
    case EVT_DRAG_END: {
        /* overscrolling */
        *sr.off_x = clamp(0, *sr.off_x, *x.total_x - *x.size_x);
        *sr.off_y = clamp(0, *sr.off_y, *y.total_y - *y.size_y);
    } break;
    case EVT_SCROLLED: {
        /* mouse scrolling */
        *sr.off_y += -evt->scroll.y * floori((cast(float, *y.size_y) * 0.1f));
        *sr.off_y = clamp(0, *sr.off_y, *y.total_y - *y.size_y);
    } break;
    case EVT_SHORTCUT: {
        /* shortcuts */
        if (evt->key.code == SHORTCUT_SCROLL_BOX_BEGIN)
            if (evt->key.pressed) *sr.off_y = 0;
        if (evt->key.code == SHORTCUT_SCROLL_BOX_END)
            if (evt->key.pressed) *sr.off_y = *y.total_y - *y.size_y;
        if (evt->key.code == SHORTCUT_SCROLL_BOX_PGDN)
            if (evt->key.pressed) *sr.off_y = min(*sr.off_y + *y.size_y, *y.total_y - *y.size_y);
        if (evt->key.code == SHORTCUT_SCROLL_BOX_PGUP)
            if (evt->key.pressed) *sr.off_y = max(*sr.off_y - *y.size_y, 0);
    } break;}
}
/* ---------------------------------------------------------------------------
 *                              SCALER BOX
 * --------------------------------------------------------------------------- */
struct scaler_box {
    uiid id;
    float *scale_x;
    float *scale_y;
};
api struct scaler_box
scaler_box_ref(struct box *b)
{
    struct scaler_box sb;
    sb.scale_x = widget_get_float(b, 0);
    sb.scale_y = widget_get_float(b, 1);
    sb.id = *widget_get_id(b, 2);
    return sb;
}
api struct scaler_box
scaler_box_begin(struct state *s)
{
    struct scaler_box sb = {0};
    assert(s);
    if (!s) return sb;

    widget_begin(s, WIDGET_SCALER_BOX);
    sb.scale_x = widget_state_float(s, WIDGET_SCALER_BOX, 1);
    sb.scale_y = widget_state_float(s, WIDGET_SCALER_BOX, 1);
    sb.id = widget_box_push(s);
    widget_param_id(s, sb.id);
    return sb;
}
api void
scaler_box_end(struct state *s)
{
    uiid id = 0;
    assert(s);
    if (!s) return;

    id = widget_box_push(s);
    widget_param_id(s, id);
    widget_box_property_set(s, BOX_IMMUTABLE);

    widget_box_pop(s);
    widget_box_pop(s);
    widget_end(s);
}
api void
scaler_box_input(struct box *b, union event *evt)
{
    struct input *in = evt->hdr.input;
    struct scaler_box sb = scaler_box_ref(b);
    if (evt->type != EVT_SCROLLED) return;
    if (in->shortcuts[SHORTCUT_SCALER_BOX_SCALE_X].down ||
        in->shortcuts[SHORTCUT_SCALER_BOX_SCALE].down)
        *sb.scale_x = -evt->scroll.x * 0.1f;
    if (in->shortcuts[SHORTCUT_SCALER_BOX_SCALE_Y].down ||
        in->shortcuts[SHORTCUT_SCALER_BOX_SCALE].down)
        *sb.scale_y = -evt->scroll.y * 0.1f;
}
/* ---------------------------------------------------------------------------
 *                              BUTTON_ICON
 * --------------------------------------------------------------------------- */
struct button_icon {
    uiid id;
    struct button btn;
    struct icon ico;
    struct sbox sbx;
};
api struct button_icon
button_icon(struct state *s, int sym)
{
    struct button_icon bti = {0};
    widget_begin(s, WIDGET_BUTTON_ICON);
    bti.id = widget_box_push(s);
    bti.btn = button_begin(s);
        bti.sbx = sbox_begin(s);
        *bti.sbx.align.horizontal = SALIGN_CENTER;
        *bti.sbx.align.vertical = SALIGN_MIDDLE;
            bti.ico = icon(s, sym);
        sbox_end(s);
    button_end(s);
    widget_box_pop(s);
    widget_end(s);
    return bti;
}
api int
button_icon_clicked(struct state *s, int sym)
{
    struct button_state st;
    struct button_icon bi;
    bi = button_icon(s, sym);
    button_poll(s, &st, &bi.btn);
    return st.clicked != 0;
}

/* ---------------------------------------------------------------------------
 *                              BUTTON_LABEL
 * --------------------------------------------------------------------------- */
struct button_label {
    uiid id;
    struct button btn;
    struct label lbl;
    struct sbox sbx;
};
intern void
button_label_pull(struct button_state *btn, const struct box *b)
{
    button_pull(btn, b);
}
api int
button_label_poll(struct state *s, struct button_state *st,
    struct button_label *btn)
{
    return button_poll(s, st, &btn->btn);
}
api int
button_label_query(struct context *ctx, struct button_state *st,
    mid mid, struct button_label *btn)
{
    return button_query(ctx, st, mid, &btn->btn);
}
api struct button_label
button_label(struct state *s, const char *txt, const char *end)
{
    struct button_label btn = {0};
    assert(txt);
    assert(s);

    widget_begin(s, WIDGET_BUTTON_LABEL);
    btn.id = widget_box_push(s);
    btn.btn = button_begin(s);
        btn.sbx = sbox_begin(s);
        *btn.sbx.align.horizontal = SALIGN_CENTER;
        *btn.sbx.align.vertical = SALIGN_MIDDLE;
            btn.lbl = label(s, txt, end);
        sbox_end(s);
    button_end(s);
    widget_box_pop(s);
    widget_end(s);
    return btn;
}
api int
button_label_clicked(struct state *s, const char *label, const char *end)
{
    struct button_state st;
    struct button_label bl;
    bl = button_label(s, label, end);
    button_poll(s, &st, &bl.btn);
    return st.clicked != 0;
}
/* ---------------------------------------------------------------------------
 *                                  TOGGLE
 * --------------------------------------------------------------------------- */
struct toggle {
    uiid id;
    struct flex_box fbx;
    struct label lbl;
    struct icon icon;

    int *val;
    int *icon_def;
    int *icon_act;
    mid *mid;
    uiid *iconid;
};
api struct toggle
toggle_ref(struct box *b)
{
    struct toggle tog = {0};
    tog.val = widget_get_int(b, 0);
    tog.icon_def = widget_get_int(b, 1);
    tog.icon_act = widget_get_int(b, 2);
    tog.mid = widget_get_mid(b, 3);
    tog.iconid = widget_get_id(b, 4);
    return tog;
}
intern void
toggle_setup(struct state *s, struct toggle *tog,
    const char *txt, const char *end)
{
    struct sbox sbx = sbox_begin(s);
    *sbx.align.horizontal = SALIGN_LEFT;
    *sbx.align.vertical = SALIGN_MIDDLE;
    {
        /* icon */
        tog->fbx = flex_box_begin(s);
        *tog->fbx.spacing = 4;
        *tog->fbx.padding = 0;
        flex_box_slot_fitting(s, &tog->fbx);
        tog->icon = icon(s, (*tog->val) ? *tog->icon_act: *tog->icon_def);

        /* label */
        flex_box_slot_dyn(s, &tog->fbx);
        tog->lbl = label(s, txt, end);
        flex_box_end(s, &tog->fbx);
    } sbox_end(s);
    widget_param_mid(s, s->id);
    tog->iconid = widget_param_id(s, tog->icon.id);
}
api struct toggle
checkbox(struct state *s, int *checked, const char *txt, const char *end)
{
    struct toggle chk;
    widget_begin(s, WIDGET_CHECKBOX);
    chk.id = widget_box_push(s);
    chk.val = widget_modifier_int(s, checked);
    chk.icon_def = widget_param_int(s, ICON_UNCHECKED);
    chk.icon_act = widget_param_int(s, ICON_CHECKED);
    toggle_setup(s, &chk, txt, end);

    widget_box_pop(s);
    widget_end(s);
    return chk;
}
api struct toggle
toggle(struct state *s, int *active, const char *txt, const char *end)
{
    struct toggle tog;
    widget_begin(s, WIDGET_TOGGLE);
    tog.id = widget_box_push(s);
    tog.val = widget_modifier_int(s, active);
    tog.icon_def = widget_param_int(s, ICON_UNTOGGLED);
    tog.icon_act = widget_param_int(s, ICON_TOGGLED);
    toggle_setup(s, &tog, txt, end);

    widget_box_pop(s);
    widget_end(s);
    return tog;
}
api struct toggle
radio(struct state *s, int *sel, const char *txt, const char *end)
{
    struct toggle tog;
    widget_begin(s, WIDGET_RADIO);
    tog.id = widget_box_push(s);
    tog.val = widget_modifier_int(s, sel);
    tog.icon_def = widget_param_int(s, ICON_UNSELECTED);
    tog.icon_act = widget_param_int(s, ICON_SELECTED);
    toggle_setup(s, &tog, txt, end);

    widget_box_pop(s);
    widget_end(s);
    return tog;
}
api void
toggle_input(struct context *ctx, struct box *b, union event *evt)
{
    struct toggle tog = toggle_ref(b);
    if (evt->type != EVT_CLICKED) return;
    *tog.val = !(*tog.val);

    {struct box *ib = query(ctx, *tog.mid, *tog.iconid);
    if (!ib) return;
    {struct icon ico = icon_ref(ib);
    *ico.sym = (*tog.val) ? *tog.icon_act: *tog.icon_def;}}
    ctx->active = b;
}
/* ---------------------------------------------------------------------------
 *                              COMBO BOX
 * --------------------------------------------------------------------------- */
struct combo_box {
    struct combo combo;
    struct flex_box fbx;
    struct state *ps;
    uiid *popup;
    uiid *selid;
    uiid *lblid;
};
api void
combo_box_popup_input(struct context *ctx, struct box *b, union event *evt)
{
    int i = 0;
    uiid *selid = 0;
    if (evt->type != EVT_CLICKED) return;
    if (evt->hdr.origin == b) return;

    /* find combo box */
    {mid cbmid = *widget_get_mid(b, 0);
    uiid cbid = *widget_get_id(b, 1);
    struct box *cbx = query(ctx, cbmid, cbid);
    if (!cbx) return;
    assert(cbx->type == WIDGET_COMBO_BOX);

    /* check if clicked on item */
    selid = widget_get_id(b, 2);
    for (i = 0; i < evt->hdr.cnt; i++) {
        struct box *l = evt->hdr.boxes[i];
        if (l->type != WIDGET_LABEL) continue;
        *selid = l->id;

        /* change selected label */
        {uiid lblid = *widget_get_id(b, 3);
        struct box *lbx = query(ctx, cbmid, lblid);
        if (!lbx) return;
        lbx->params = l->params;}

        /* close combo */
        {struct list_hook *cbh = list_begin(&cbx->lnks);
        struct box *cb = list_entry(cbh, struct box, node);
        struct combo c = combo_ref(cb);
        combo_popup_close(ctx, &c);}
    }}
}
api struct combo_box
combo_box_begin(struct state *s, mid id)
{
    uiid cid;
    struct combo_box cbx;
    widget_begin(s, WIDGET_COMBO_BOX);
    cid = widget_box_push(s);

    /* combo */
    cbx.combo = combo_begin(s, id);
    cbx.ps = combo_popup_begin(s, &cbx.combo);
    widget_begin(cbx.ps, WIDGET_COMBO_BOX_POPUP);
    widget_box_push(cbx.ps);
    widget_param_mid(cbx.ps, s->id);
    widget_param_id(cbx.ps, cid);
    cbx.selid = widget_state_id(cbx.ps, WIDGET_COMBO_BOX_POPUP, 0);
    cbx.lblid = widget_param_id(cbx.ps, 0);

    /* setup popup layout */
    cbx.fbx = flex_box_begin(cbx.ps);
    *cbx.fbx.orientation = FLEX_BOX_VERTICAL;
    return cbx;
}
api int
combo_box_item(struct state *s, struct combo_box *cbx, const char *item, const char *end)
{
    /* item */
    int selected = 0;
    struct label lbl;
    flex_box_slot_fitting(cbx->ps, &cbx->fbx); {
        struct sbox sbx = sbox_begin(cbx->ps);
        *sbx.align.horizontal = SALIGN_LEFT;
        *sbx.align.vertical = SALIGN_MIDDLE; {
            lbl = label(cbx->ps, item, end);
            if (!*cbx->selid) *cbx->selid = lbl.id;
        } sbox_end(cbx->ps);
    }
    if (lbl.id == *cbx->selid) {
        /* combo header */
        button_begin(s); {
            struct flex_box fb = flex_box_begin(s);
            flex_box_slot_dyn(s, &fb); {
                /* selected item label */
                struct sborder sb = sborder_begin(s);
                *sb.x = *sb.y = 2;
                    lbl = label(s, item, end);
                    *cbx->lblid = lbl.id;
                sborder_end(s);
            }
            flex_box_slot_fitting(s, &fb); {
                /* down arrow icon */
                struct sbox x = sbox_begin(s);
                *x.align.horizontal = SALIGN_CENTER;
                *x.align.vertical = SALIGN_MIDDLE;
                *x.border.x = *x.border.y = 2;
                    icon(s, ICON_COMBO_CARRET_DOWN);
                sbox_end(s);
            } flex_box_end(s, &fb);
        } button_end(s);
        selected = 1;
    } return selected;
}
api void
combo_box_end(struct state *s, struct combo_box *cbx)
{
    /* finish combo */
    flex_box_end(cbx->ps, &cbx->fbx);
    widget_box_pop(cbx->ps);
    widget_end(cbx->ps);
    combo_popup_end(cbx->ps, &cbx->combo);
    combo_end(s);

    /* finish combo box */
    widget_box_pop(s);
    widget_end(s);
}
api int
combo_box(struct state *s, unsigned id, const char **items, int cnt)
{
    int i = 0, sel = 0;
    struct combo_box cbx;
    cbx = combo_box_begin(s, id);
    for (i = 0; i < cnt; ++i) {
        if (combo_box_item(s, &cbx, items[i], 0)) sel = i;
    } combo_box_end(s, &cbx);
    return sel;
}
/* ---------------------------------------------------------------------------
 *                                  PANEL
 * --------------------------------------------------------------------------- */
struct panel {
    uiid id;
    struct sborder sbx;
    struct flex_box fbx;
};
api struct panel
panel_begin(struct state *s)
{
    struct panel pan = {0};
    widget_begin(s, WIDGET_PANEL);
    pan.id = widget_box_push(s);
    pan.fbx = flex_box_begin(s);
    *pan.fbx.orientation = FLEX_BOX_VERTICAL;
    *pan.fbx.padding = 0;
    return pan;
}
api uiid
panel_header_begin(struct state *s, struct panel *pan)
{
    flex_box_slot_fitting(s, &pan->fbx);
    widget_begin(s, WIDGET_PANEL_HEADER);
    return widget_box_push(s);
}
api void
panel_header_end(struct state *s, struct panel *pan)
{
    widget_box_pop(s);
    widget_end(s);
}
api void
panel_header(struct state *s, struct panel *pan, const char *title)
{
    assert(s);
    assert(pan);
    if (!s || !pan || !title) return;
    panel_header_begin(s, pan); {
        struct sbox sbx = sbox_begin(s);
        *sbx.border.y = 5; *sbx.border.x = 4;
        *sbx.align.horizontal = SALIGN_CENTER;
        *sbx.align.vertical = SALIGN_MIDDLE;
            label(s, title, 0);
        sbox_end(s);
    } panel_header_end(s, pan);
}
api uiid
panel_toolbar_begin(struct state *s, struct panel *pan)
{
    flex_box_slot_fitting(s, &pan->fbx);
    widget_begin(s, WIDGET_PANEL_TOOLBAR);
    return widget_box_push(s);
}
api void
panel_toolbar_end(struct state *s, struct panel *pan)
{
    widget_box_pop(s);
    widget_end(s);
}
api void
panel_content_begin(struct state *s, struct panel *pan)
{
    flex_box_slot_dyn(s, &pan->fbx);
}
api void
panel_content_end(struct state *s, struct panel *pan)
{

}
api uiid
panel_status_begin(struct state *s, struct panel *pan)
{
    flex_box_slot_fitting(s, &pan->fbx);
    widget_begin(s, WIDGET_PANEL_STATUSBAR);
    return widget_box_push(s);
}
api void
panel_status_end(struct state *s, struct panel *pan)
{
    widget_box_pop(s);
    widget_end(s);
}
api void
panel_status(struct state *s, struct panel *pan, const char *status)
{
    assert(s);
    assert(pan);
    if (!s || !pan || !status) return;
    panel_status_begin(s, pan); {
        struct sbox sbx = sbox_begin(s);
        *sbx.border.y = 2; *sbx.border.x = 2;
        *sbx.align.horizontal = SALIGN_LEFT;
        *sbx.align.vertical = SALIGN_MIDDLE;
            label(s, status, 0);
        sbox_end(s);
    } panel_status_end(s, pan);
}
api void
panel_end(struct state *s, struct panel *pan)
{
    flex_box_end(s, &pan->fbx);
    widget_box_pop(s);
    widget_end(s);
}
/* ---------------------------------------------------------------------------
 *                                  PANEL BOX
 * --------------------------------------------------------------------------- */
api struct panel
panel_box_begin(struct state *s, const char *title)
{
    struct panel pan;
    pan = panel_begin(s);
    panel_header(s, &pan, title);
    panel_content_begin(s, &pan);
    scroll_box_begin(s);
    return pan;
}
api void
panel_box_end(struct state *s, struct panel *pan, const char *status)
{
    scroll_box_end(s);
    panel_content_end(s, pan);
    panel_status(s, pan, status);
    panel_end(s, pan);
}
/* ---------------------------------------------------------------------------
 *                                  SIDEBAR
 * --------------------------------------------------------------------------- */
enum sidebar_state {
    SIDEBAR_CLOSED,
    SIDEBAR_OPEN,
    SIDEBAR_PINNED,
    SIDEBAR_DRAGGED
};
struct sidebar {
    uiid id;
    int *state;
    float *ratio;
    int *icon;
    int *total;
    struct con_box cbx;
    struct panel pan;
};
api struct sidebar
sidebar_ref(struct box *b)
{
    struct sidebar sb;
    sb.state = widget_get_int(b,0);
    sb.ratio = widget_get_float(b,1);
    sb.icon = widget_get_int(b,2);
    sb.total = widget_get_int(b,3);
    return sb;
}
intern void
sidebar_validate_icon(struct sidebar *sb)
{
    switch (*sb->state) {
    case SIDEBAR_OPEN: *sb->icon = ICON_SIDEBAR_LOCK; break;
    case SIDEBAR_CLOSED: *sb->icon = ICON_SIDEBAR_OPEN; break;
    case SIDEBAR_PINNED: *sb->icon = ICON_SIDEBAR_UNLOCK; break;
    case SIDEBAR_DRAGGED: *sb->icon = ICON_SIDEBAR_UNLOCK; break;}
}
api struct sidebar
sidebar_begin(struct state *s)
{
    /* sidebar */
    struct sidebar sb = {0};
    sb.cbx = con_box_begin(s, 0, 0);
    {static const struct con sb_cons[] = {
        {CON_SET, {0,ATTR_W}, {SPARENT,ATTR_W}, {0.5f,0}},
        {CON_SET, {0,ATTR_H}, {SPARENT,ATTR_H}, {0.8f,0}},
        {CON_SET, {0,ATTR_L}, {SPARENT,ATTR_L}, {1,0}, ATTR_W},
        {CON_SET, {0,ATTR_CY}, {SPARENT,ATTR_CY}, {1,0}, ATTR_H}
    }; con_box_slot(s, &sb.cbx, sb_cons, cntof(sb_cons));}

    widget_begin(s, WIDGET_SIDEBAR);
    sb.id = widget_box_push(s);
    sb.state = widget_state_int(s, WIDGET_SIDEBAR, SIDEBAR_CLOSED);
    sb.ratio = widget_state_float(s, WIDGET_SIDEBAR, 0.5f);
    sb.icon = widget_param_int(s, ICON_SIDEBAR_LOCK);
    sb.total = widget_param_int(s, 0);
    sidebar_validate_icon(&sb);

    /* bar */
    widget_begin(s, WIDGET_SIDEBAR_BAR);
    sb.id = widget_box_push(s); {
        struct salign aln = salign_begin(s);
        *aln.vertical = SALIGN_MIDDLE;
        *aln.horizontal = SALIGN_RIGHT;
            icon(s, *sb.icon);
        salign_end(s);
    } widget_box_pop(s);
    widget_end(s);

    /* scaler */
    widget_begin(s, WIDGET_SIDEBAR_SCALER);
    widget_box_push(s);
    if (*sb.state == SIDEBAR_CLOSED)
        widget_box_property_set(s, BOX_HIDDEN);
    widget_box_property_set(s, BOX_MOVABLE_X);
    widget_box_pop(s);
    widget_end(s);

    /* content */
    widget_begin(s, WIDGET_SIDEBAR_CONTENT);
    widget_box_push(s);
    if (*sb.state == SIDEBAR_CLOSED)
        widget_box_property_set(s, BOX_HIDDEN);
    sb.pan = panel_begin(s);
    panel_content_begin(s, &sb.pan);
    return sb;
}
api void
sidebar_end(struct state *s, struct sidebar *sb)
{
    panel_content_end(s, &sb->pan);
    panel_end(s, &sb->pan);
    widget_box_pop(s);
    widget_end(s);

    widget_box_pop(s);
    widget_end(s);
    con_box_end(s, &sb->cbx);
}
api void
sidebar_layout(struct box *b)
{
    struct sidebar sb = sidebar_ref(b);
    struct list_hook *hb = b->lnks.next;
    struct list_hook *hs = hb->next;
    struct list_hook *hc = hs->next;

    struct box *bb = list_entry(hb, struct box, node);
    struct box *bs = list_entry(hs, struct box, node);
    struct box *bc = list_entry(hc, struct box, node);

    /* bar */
    bb->w = 18, bb->h = ceili(cast(float,b->h)*0.8f);
    bb->x = b->x, bb->y = (b->y + b->h/2) - (bb->h/2);

    /* scaler */
    bs->w = 8, bs->h = bb->h - 20;
    bs->x = floori(cast(float, b->w) * *sb.ratio) - bs->w;
    bs->x = max(bb->x + bb->w, bs->x);
    bs->y = bb->y + 10;

    /* content */
    bc->x = bb->x + bb->w, bc->y = bs->y;
    bc->w = bs->x - (bb->x + bb->w), bc->h = bs->h;

    /* fit sidebar */
    *sb.total = b->w;
    b->y = bb->y, b->h = bb->h;
    b->w = (bs->x + bs->w) - b->x;
}
intern void
sidebar_validate(struct sidebar *sb, struct box *b)
{
    /* retrieve scaler, content */
    struct list_hook *hc = b->node.prev;
    {struct list_hook *hs = hc->prev;
    struct box *bs = list_entry(hs, struct box, node);
    struct box *bc = list_entry(hc, struct box, node);

    /* toggle visiblity */
    switch (*sb->state) {
    case SIDEBAR_CLOSED:
        bs->flags |= BOX_HIDDEN;
        bc->flags |= BOX_HIDDEN;
        break;
    case SIDEBAR_OPEN:
    case SIDEBAR_PINNED:
        bs->flags &= ~(unsigned)BOX_HIDDEN;
        bc->flags &= ~(unsigned)BOX_HIDDEN;
        break;
    }}
}
api void
sidebar_input(struct box *b, union event *evt)
{
    struct sidebar sb = sidebar_ref(b);
    if (!b->exited) return;
    switch (evt->type) {
    default: return;
    case EVT_EXITED: {
        switch (*sb.state) {
        case SIDEBAR_OPEN:
            *sb.state = SIDEBAR_CLOSED;
            sidebar_validate(&sb, b);
            sidebar_validate_icon(&sb); break;}
    } break;}
}
api void
sidebar_bar_input(struct box *b, union event *evt)
{
    struct box *p = b->parent;
    struct sidebar sb = sidebar_ref(p);

    /* state change */
    switch (evt->type) {
    default: return;
    case EVT_ENTERED: {
        if (evt->hdr.origin != b) return;
        switch (*sb.state) {
        default: return;
        case SIDEBAR_CLOSED: *sb.state = SIDEBAR_OPEN; break;}
    } break;
    case EVT_CLICKED: {
        switch (*sb.state) {
        default: return;
        case SIDEBAR_CLOSED: *sb.state = SIDEBAR_OPEN; break;
        case SIDEBAR_OPEN: *sb.state = SIDEBAR_PINNED; break;
        case SIDEBAR_PINNED: *sb.state = SIDEBAR_OPEN; break;}
    } break;}
    sidebar_validate(&sb, p);
    sidebar_validate_icon(&sb);
}
api void
sidebar_scaler_input(struct box *b, union event *evt)
{
    struct box *p = b->parent;
    struct sidebar sb = sidebar_ref(p);
    if (evt->hdr.origin != b) return;
    switch (evt->type){
    case EVT_DRAG_BEGIN: *sb.state = SIDEBAR_DRAGGED; break;
    case EVT_DRAG_END: *sb.state = SIDEBAR_PINNED; break;
    case EVT_MOVED: {
        if (!b->moved) return;
        *sb.ratio = cast(float,(b->x + b->w) - p->x) / cast(float, *sb.total);
        *sb.ratio = clamp(0.2f, *sb.ratio, 1.0f);
    } break;}
    sidebar_validate_icon(&sb);
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
#define h32(s,i,x) h16(s,i,h16(s,i+16,x))
#define hash(s,i) ((unsigned)(h32(s,0,i)^(h32(s,0,i)>>32)))
#define idx(s,i) hash(s,(unsigned)i)
#define id(s) idx(s,0)
#define lit(s) s,len(s)
#define txt(s) s,s+strlen(s)

/* System */
#include <GL/gl.h>
#include <GL/glu.h>
#include "SDL2/SDL.h"

/* ---------------------------------------------------------------------------
 *                                  CANVAS
 * --------------------------------------------------------------------------- */
 #ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverlength-strings"
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wimplicit-function-declaration"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wc99-extensions"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wc11-extensions"
#pragma clang diagnostic ignored "-Wbad-function-cast"
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wcomment"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wtypedef-redefinition"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wcast-align"
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverlength-strings"
#endif

/* NanoVG */
#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg/src/fontstash.h"
#include "nanovg/src/stb_image.h"
#include "nanovg/src/stb_truetype.h"
#include "nanovg/src/nanovg.h"
#include "nanovg/src/nanovg.c"
#include "nanovg/src/nanovg_gl.h"

#include "IconsFontAwesome.h"
#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

/* Hacks:
 * To lazy to write complete gui render context on top of NanoVG with icon and
 * image declaration and font,style tables addressable by index, etc ....
 */
static int default_icon_font;
enum icons {
    ICON_CONFIG,
    ICON_CHART_BAR,
    ICON_DESKTOP,
    ICON_DOWNLOAD,
    ICON_FOLDER,
    ICON_CNT
};
intern int
is_black(NVGcolor col)
{
    return col.r == 0.0f && col.g == 0.0f && col.b == 0.0f && col.a == 0.0f;
}
api int
nvgTextMeasure(void *usr, int font,
    int fh, const char *txt, int len)
{
    float bounds[4];
    struct NVGcontext *vg = usr;
    nvgFontFaceId(vg, font);
    nvgFontSize(vg, fh);
    nvgTextBounds(vg, 0,0, txt, txt + len, bounds);
    return cast(int, bounds[2]);
}
api void
nvgLabel(struct NVGcontext *vg, struct box *b)
{
    struct label lbl = label_ref(b);
    nvgFontFaceId(vg, *lbl.font);
    nvgFontSize(vg, *lbl.height);
    nvgTextAlign(vg, NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGBA(220,220,220,255));
    nvgText(vg, b->x, b->y, lbl.txt, NULL);
}
api void
nvgIcon(struct NVGcontext *vg, struct box *b)
{
    struct icon ico = icon_ref(b);
    const char *icon_rune = 0;
    nvgFontFaceId(vg, default_icon_font);
    nvgFontSize(vg, *ico.size);
    nvgTextAlign(vg, NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGBA(220,220,220,255));

    switch (*ico.sym) {
    default: break;
    case ICON_COMBO_CARRET_DOWN: icon_rune = ICON_FA_CARET_DOWN; break;
    case ICON_UNCHECKED: icon_rune = ICON_FA_TIMES; break;
    case ICON_CHECKED: icon_rune = ICON_FA_CHECK; break;
    case ICON_UNTOGGLED: icon_rune = ICON_FA_TOGGLE_OFF; break;
    case ICON_TOGGLED: icon_rune = ICON_FA_TOGGLE_ON; break;
    case ICON_UNSELECTED: icon_rune = ICON_FA_CIRCLE; break;
    case ICON_SELECTED: icon_rune = ICON_FA_CIRCLE_THIN; break;
    case ICON_SIDEBAR_LOCK: icon_rune = ICON_FA_LOCK; break;
    case ICON_SIDEBAR_UNLOCK: icon_rune = ICON_FA_UNLOCK_ALT; break;
    case ICON_SIDEBAR_OPEN: icon_rune = ICON_FA_CARET_RIGHT; break;
    case ICON_CONFIG: icon_rune = ICON_FA_COGS; break;
    case ICON_CHART_BAR: icon_rune = ICON_FA_BAR_CHART; break;
    case ICON_DESKTOP: icon_rune = ICON_FA_DESKTOP; break;
    case ICON_DOWNLOAD: icon_rune = ICON_FA_DOWNLOAD; break;
    case ICON_FOLDER: icon_rune = ICON_FA_FOLDER_OPEN; break;}
    nvgText(vg, b->x, b->y, icon_rune, NULL);
}
enum nvgButtonStyle {
    NVG_BUTTON_DEFAULT,
    NVG_BUTTON_BLACK,
    NVG_BUTTON_TRANSPARENT
};
api void
nvgButton(struct NVGcontext *vg, struct box *b)
{
    NVGcolor col;
    int style = *widget_get_int(b, 0);

    switch (style) {
    default:
    case NVG_BUTTON_DEFAULT: col = nvgRGBA(128,16,8,255); break;
    case NVG_BUTTON_TRANSPARENT: if (!b->hovered) return;
    case NVG_BUTTON_BLACK: col = nvgRGBA(0,0,0,1); break;}

    {static const float corner_radius = 4.0f;
    NVGcolor src = nvgRGBA(255,255,255, is_black(col) ? 16: 32);
    NVGcolor dst = nvgRGBA(0,0,0, is_black(col) ? 16: 32);
    NVGpaint bg = nvgLinearGradient(vg, b->x, b->y, b->x, b->y + b->h, src, dst);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, b->x+1,b->y+1, b->w-2,b->h-2, corner_radius-1);
    if (!is_black(col)) {
        if (b->hovered)
            col.a = 0.9f;
        nvgFillColor(vg, col);
        nvgFill(vg);
    }
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, b->x+0.5f,b->y+0.5f, b->w-1,b->h-1, corner_radius-0.5f);
    nvgStrokeColor(vg, nvgRGBA(0,0,0,48));
    nvgStroke(vg);}
}
api void
nvgSlider(struct NVGcontext *vg, struct box *b)
{
    NVGpaint bg, knob;
    struct slider sld = slider_ref(b);
    if (sld.cid != b->id) return;

    {struct box *p = b->parent;
    float cy = p->y+(int)(p->h*0.5f);
    nvgSave(vg);

    /* Slot */
    bg = nvgBoxGradient(vg, p->x,cy-2+1, p->w,4, 2,2, nvgRGBA(0,0,0,32), nvgRGBA(0,0,0,128));
    nvgBeginPath(vg);
    nvgRoundedRect(vg, p->x, cy-2, p->w, 4, 2);
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    /* Knob Shadow */
    bg = nvgBoxGradient(vg, b->x, b->y, b->w, b->h, 3,4, nvgRGBA(0,0,0,64), nvgRGBA(0,0,0,0));
    nvgBeginPath(vg);
    nvgRect(vg, b->x-5, b->y-5, b->w+5, b->h+5);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    /* Knob */
    {NVGcolor src = nvgRGBA(255,255,255, 32);
    NVGcolor dst = nvgRGBA(0,0,0, 32);
    knob = nvgLinearGradient(vg, b->x, b->y, b->x, b->y + b->h, src, dst);}

    nvgBeginPath(vg);
    nvgRect(vg, b->x, b->y, b->w, b->h);
    if (b->hovered)
        nvgFillColor(vg, nvgRGBA(128,16,8,230));
    else nvgFillColor(vg, nvgRGBA(128,16,8,255));
    nvgFill(vg);
    nvgFillPaint(vg, knob);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgRect(vg, b->x, b->y, b->w, b->h);
    nvgStrokeColor(vg, nvgRGBA(0,0,0,92));
    nvgStroke(vg);
    nvgRestore(vg);}
}
api void
nvgScroll(struct NVGcontext *vg, struct box *b)
{
    NVGpaint pt;
    struct box *p = b->parent;
    struct scroll scrl = scroll_ref(b);
    if (scrl.cid != b->id) return;
    if (b->h == p->h && b->w == p->w)
        return; /* skip if fully visible */

    /* draw scroll background  */
    pt = nvgBoxGradient(vg, p->x, p->y, p->w, p->h, 3,4, nvgRGBA(0,0,0,32), nvgRGBA(0,0,0,92));
    nvgBeginPath(vg);
    nvgRoundedRect(vg, p->x, p->y, p->w, p->h, 3);
    nvgFillPaint(vg, pt);
    nvgFill(vg);

    /* draw scroll cursor  */
    {NVGcolor src = nvgRGBA(255,255,255, 32);
    NVGcolor dst = nvgRGBA(0,0,0, 32);
    pt = nvgLinearGradient(vg, b->x, b->y, b->x, b->y + b->h, src, dst);}
    nvgBeginPath(vg);
    nvgRoundedRect(vg, b->x, b->y, b->w, b->h, 3);
    if (b->hovered)
        nvgFillColor(vg, nvgRGBA(128,16,8,230));
    else nvgFillColor(vg, nvgRGBA(128,16,8,255));
    nvgFill(vg);
    nvgFillPaint(vg, pt);
    nvgFill(vg);
}
api void
nvgComboPopup(struct NVGcontext *vg, struct box *b)
{
    NVGpaint shadowPaint;
    int x = b->x, y = b->y;
    int w = b->w, h = b->h;

    /* Window */
    nvgBeginPath(vg);
    nvgRect(vg, x,y, w,h);
    nvgFillColor(vg, nvgRGBA(28,30,34,192));
    nvgFill(vg);

    /* Drop shadow */
    shadowPaint = nvgBoxGradient(vg, x,y+2, w,h, 2, 10, nvgRGBA(0,0,0,128), nvgRGBA(0,0,0,0));
    nvgBeginPath(vg);
    nvgRect(vg, x-10,y-10, w+20,h+30);
    nvgRect(vg, x,y, w,h);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, shadowPaint);
    nvgFill(vg);
}
api void
nvgClipRegion(struct NVGcontext *vg, struct box *b, int pidx, struct rect *sis)
{
    uiid reset_id = *widget_get_id(b, pidx);
    if (b->id != reset_id) {
        /* safe old scissor rect and generate new one */
        struct list_hook *t = b->lnks.prev;
        struct box *n = list_entry(t, struct box, node);
        n->x = sis->x, n->y = sis->y;
        n->w = sis->w, n->h = sis->h;

        {int minx = max(sis->x, b->x);
        int miny = max(sis->y, b->y);
        int maxx = min(sis->x + sis->w, b->x + b->w);
        int maxy = min(sis->y + sis->h, b->y + b->h);

        sis->x = minx, sis->y = miny;
        sis->w = max(maxx - minx, 0);
        sis->h = max(maxy - miny, 0);}
        nvgScissor(vg, sis->x, sis->y, sis->w, sis->h);
    } else {
        /* reset to previous scissor rect */
        nvgScissor(vg, b->x, b->y, b->w, b->h);
        sis->x = b->x, sis->y = b->y;
        sis->w = b->w, sis->h = b->h;
    }
}
api void
nvgScaleRegion(struct NVGcontext *vg, struct box *b, float *x, float *y)
{
    struct scaler_box sbx = scaler_box_ref(b);
    if (b->id != sbx.id) {
        float xform[6];
        *x *= *sbx.scale_x;
        *y *= *sbx.scale_y;
        /* scale all content */
        nvgTransformIdentity(xform);
        nvgTransformScale(xform, *x, *y);
        nvgCurrentTransform(vg, xform);
    } else {
        float xform[6];
        *x /= *sbx.scale_x;
        *y /= *sbx.scale_y;
        /* reset scaler */
        nvgTransformIdentity(xform);
        nvgTransformScale(xform, *x, *y);
        nvgCurrentTransform(vg, xform);
    }
}
api void
nvgPanel(struct NVGcontext *vg, struct box *b)
{
    float cornerRadius = 3.0f;
    int x = b->x, y = b->y;
    int w = b->w, h = b->h;
    NVGpaint shadow;

    /* panel */
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x,y, w,h, cornerRadius);
    nvgFillColor(vg, nvgRGBA(28,30,34,192));
    nvgFill(vg);

    /* shadow */
    shadow = nvgBoxGradient(vg, x,y+2, w,h, cornerRadius*2, 10,
        nvgRGBA(0,0,0,128), nvgRGBA(0,0,0,0));
    nvgBeginPath(vg);
    nvgRect(vg, x-10,y-10, w+20,h+30);
    nvgRoundedRect(vg, x,y, w,h, cornerRadius);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, shadow);
    nvgFill(vg);
}
api void
nvgPanelHeader(struct NVGcontext *vg, struct box *b)
{
    float cornerRadius = 3.0f;
    int x = b->x, y = b->y;
    int w = b->w, h = b->h;
    NVGpaint headerPaint;

    headerPaint = nvgLinearGradient(vg, x,y,x,y+15, nvgRGBA(255,255,255,8), nvgRGBA(0,0,0,16));
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x+1,y+1, w-2,h, cornerRadius-1);
    nvgFillPaint(vg, headerPaint);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgMoveTo(vg, x+0.5f, y+0.5f+h);
    nvgLineTo(vg, x+0.5f+w-1, y+0.5f+h);
    nvgStrokeColor(vg, nvgRGBA(25,25,25,255));
    nvgStroke(vg);
}
api void
nvgSidebarScaler(struct NVGcontext *vg, struct box *b)
{
    NVGcolor src = nvgRGBA(255,255,255, 32);
    NVGcolor dst = nvgRGBA(0,0,0, 32);
    NVGpaint bg = nvgLinearGradient(vg, b->x, b->y, b->x, b->y + b->h, src, dst);
    NVGcolor col = nvgRGBA(128,16,8,255);

    nvgBeginPath(vg);
    nvgMoveTo(vg, b->x, b->y);
    nvgLineTo(vg, b->x + b->w, b->y + 5);
    nvgLineTo(vg, b->x + b->w, b->y + b->h - 5);
    nvgLineTo(vg, b->x, b->y + b->h);
    nvgClosePath(vg);
    if (b->hovered)
        col.a = 0.9f;
    nvgFillColor(vg, col);
    nvgFill(vg);
    nvgFillPaint(vg, bg);
    nvgFill(vg);
}
api void
nvgSidebarBar(struct NVGcontext *vg, struct box *b)
{
    NVGcolor src = nvgRGBA(255,255,255, 32);
    NVGcolor dst = nvgRGBA(0,0,0, 32);
    NVGpaint bg = nvgLinearGradient(vg, b->x, b->y, b->x, b->y + b->h, src, dst);
    NVGcolor col = nvgRGBA(128,16,8,255);

    nvgBeginPath(vg);
    nvgMoveTo(vg, b->x, b->y);
    nvgLineTo(vg, b->x + b->w, b->y + 10);
    nvgLineTo(vg, b->x + b->w, b->y + b->h - 10);
    nvgLineTo(vg, b->x, b->y + b->h);
    nvgClosePath(vg);
    if (b->hovered)
        col.a = 0.9f;
    nvgFillColor(vg, col);
    nvgFill(vg);
    nvgFillPaint(vg, bg);
    nvgFill(vg);
}
/* ---------------------------------------------------------------------------
 *                                  PLATFORM
 * --------------------------------------------------------------------------- */
static void
ui_blueprint(struct NVGcontext *vg, union process *p, struct box *b)
{
    switch (b->type) {
    case WIDGET_LABEL: label_blueprint(b, vg, nvgTextMeasure); break;
    case WIDGET_ICON: icon_blueprint(b); break;
    case WIDGET_SLIDER: slider_blueprint(b); break;
    case WIDGET_SBORDER: sborder_blueprint(b); break;
    case WIDGET_FLEX_BOX: flex_box_blueprint(b); break;
    case WIDGET_SCROLL_BOX: scroll_box_blueprint(b); break;
    default: blueprint(p, b); break;}
}
static void
ui_layout(struct NVGcontext *vg, union process *p, struct box *b)
{
    switch (b->type) {
    case WIDGET_ICON: icon_layout(b); break;
    case WIDGET_SLIDER: slider_layout(b); break;
    case WIDGET_SCROLL: scroll_layout(b); break;
    case WIDGET_COMBO: combo_layout(p->hdr.ctx, b); break;
    case WIDGET_SALIGN: salign_layout(b); break;
    case WIDGET_SBORDER: sborder_layout(b); break;
    case WIDGET_FLEX_BOX: flex_box_layout(b); break;
    case WIDGET_OVERLAP_BOX: overlap_box_layout(b, p->hdr.arena); break;
    case WIDGET_CON_BOX: con_box_layout(b, p->hdr.arena); break;
    case WIDGET_SCROLL_REGION: scroll_region_layout(b); break;
    case WIDGET_SCROLL_BOX: scroll_box_layout(b); break;
    case WIDGET_SIDEBAR: sidebar_layout(b); break;
    default: layout(p, b); break;}
}
static void
ui_input(union process *p, union event *evt)
{
    int i = 0;
    struct context *ctx = p->hdr.ctx;
    for (i = 0; i < evt->hdr.cnt; i++) {
        struct box *b = evt->hdr.boxes[i];
        switch (b->type) {
        case WIDGET_CHECKBOX:
        case WIDGET_TOGGLE:
        case WIDGET_RADIO: toggle_input(ctx, b, evt); break;
        case WIDGET_SLIDER: slider_input(b, evt); break;
        case WIDGET_COMBO: combo_input(ctx, b, evt); break;
        case WIDGET_COMBO_BOX_POPUP: combo_box_popup_input(ctx, b, evt); break;
        case WIDGET_SCROLL: scroll_input(b, evt); break;
        case WIDGET_SCROLL_REGION: scroll_region_input(b, evt); break;
        case WIDGET_SCROLL_BOX: scroll_box_input(ctx, b, evt); break;
        case WIDGET_SCALER_BOX: scaler_box_input(b, evt); break;
        case WIDGET_SIDEBAR: sidebar_input(b, evt); break;
        case WIDGET_SIDEBAR_BAR: sidebar_bar_input(b, evt); break;
        case WIDGET_SIDEBAR_SCALER: sidebar_scaler_input(b, evt); break;
        default: input(p, evt, b); break;}
    }
}
static void
ui_draw(struct NVGcontext *vg, struct box *b,
    struct rect *scissor, float *scale_x, float *scale_y)
{
    switch (b->type) {
    case WIDGET_LABEL: nvgLabel(vg, b); break;
    case WIDGET_ICON: nvgIcon(vg, b); break;
    case WIDGET_BUTTON: nvgButton(vg, b); break;
    case WIDGET_SLIDER: nvgSlider(vg, b); break;
    case WIDGET_COMBO_POPUP: nvgComboPopup(vg, b); break;
    case WIDGET_SCROLL: nvgScroll(vg, b); break;
    case WIDGET_SCROLL_REGION: nvgClipRegion(vg, b, 4, scissor); break;
    case WIDGET_SCALER_BOX: nvgScaleRegion(vg, b, scale_x, scale_y); break;
    case WIDGET_PANEL: nvgPanel(vg, b); break;
    case WIDGET_PANEL_HEADER: nvgPanelHeader(vg, b); break;
    case WIDGET_PANEL_STATUSBAR: nvgPanelHeader(vg, b); break;
    case WIDGET_SIDEBAR_BAR: nvgSidebarBar(vg, b); break;
    case WIDGET_SIDEBAR_SCALER: nvgSidebarScaler(vg, b); break;
    default: break;}
}
static void
ui_commit(struct context *ctx)
{
    {union process *p = 0;
    while ((p = process_begin(ctx, PROCESS_COMMIT))) {
        switch (p->type) {default: break;
        case PROC_ALLOC_FRAME:
        case PROC_ALLOC: p->mem.ptr = calloc((size_t)p->mem.size,1); break;}
        process_end(p);
    }}
}
static void
ui_paint(struct NVGcontext *vg, struct context *ctx, int w, int h)
{
    int i = 0;
    union process *p = 0;
    assert(vg);
    assert(ctx);
    if (!ctx || !vg) return;

    /* Process: Paint */
    while ((p = process_begin(ctx, PROCESS_PAINT))) {
        switch (p->type) {default:break;
        case PROC_ALLOC_FRAME:
        case PROC_ALLOC:
            /* not called unless PROC_INPUT generates new UI */
            p->mem.ptr = calloc((size_t)p->mem.size,1); break;
        case PROC_BLUEPRINT: {
            /* not called unless PROC_INPUT generates new UI */
            struct process_layouting *op = &p->layout;
            for (i = op->begin; i != op->end; i += op->inc)
                ui_blueprint(vg, p, op->boxes[i]);
        } break;
        case PROC_LAYOUT: {
            /* not called unless PROC_INPUT generates new UI */
            struct process_layouting *op = &p->layout;
            for (i = op->begin; i != op->end; i += op->inc)
                ui_layout(vg, p, op->boxes[i]);
        } break;
        case PROC_PAINT: {
            struct process_paint *op = &p->paint;
            struct rect scissor = {0,0,0,0};
            float scale_x = 1.0f, scale_y = 1.0f;
            scissor.w = w, scissor.h = h;
            for (i = 0; i < op->cnt; ++i)
                ui_draw(vg, op->boxes[i], &scissor, &scale_x, &scale_y);
            nvgResetScissor(vg);
        } break;}
        process_end(p);
    }
}
static void
ui_clear(struct context *ctx)
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
static void
ui_cleanup(struct context *ctx)
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
static void
ui_dump(struct context *ctx, FILE *fp, const char *name)
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
static void
ui_event(struct context *ctx, const SDL_Event *evt)
{
    assert(ctx);
    assert(evt);
    switch (evt->type) {
    case SDL_MOUSEMOTION: input_motion(ctx, evt->motion.x, evt->motion.y); break;
    case SDL_MOUSEWHEEL: input_scroll(ctx, evt->wheel.x, evt->wheel.y); break;
    case SDL_TEXTINPUT: input_text(ctx, txt(evt->text.text)); break;
    case SDL_WINDOWEVENT: {
         if (evt->window.event == SDL_WINDOWEVENT_RESIZED ||
            evt->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            input_resize(ctx, evt->window.data1, evt->window.data2);
    } break;
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN: {
        int down = (evt->type == SDL_MOUSEBUTTONDOWN);
        if (evt->button.button == SDL_BUTTON_LEFT)
            input_button(ctx, MOUSE_BUTTON_LEFT, down);
        else if (evt->button.button == SDL_BUTTON_RIGHT)
            input_button(ctx, MOUSE_BUTTON_RIGHT, down);
        else if (evt->button.button == SDL_BUTTON_MIDDLE)
            input_button(ctx, MOUSE_BUTTON_MIDDLE, down);
    } break;
    case SDL_KEYUP:
    case SDL_KEYDOWN: {
        int down = (evt->type == SDL_KEYDOWN);
        SDL_Keycode sym = evt->key.keysym.sym;
        if (sym == SDLK_BACKSPACE)
            input_shortcut(ctx, SHORTCUT_SCROLL_REGION_RESET, down);
        else if (sym == SDLK_LALT)
            input_shortcut(ctx, SHORTCUT_SCROLL_REGION_SCROLL, down);
        else if (sym == SDLK_HOME)
            input_shortcut(ctx, SHORTCUT_SCROLL_BOX_BEGIN, down);
        else if (sym == SDLK_END)
            input_shortcut(ctx, SHORTCUT_SCROLL_BOX_END, down);
        else if (sym == SDLK_PAGEUP)
            input_shortcut(ctx, SHORTCUT_SCROLL_BOX_PGUP, down);
        else if (sym == SDLK_PAGEDOWN)
            input_shortcut(ctx, SHORTCUT_SCROLL_BOX_PGDN, down);
        input_key(ctx, sym, down);
    } break;}
}

/* ===========================================================================
 *
 *                                  Main
 *
 * =========================================================================== */
struct ui_retained {
    mid id;
    struct button_label btn;
    struct slider sld;
};
static void
ui_build_retained(struct context *ctx, struct ui_retained *ui, const char *label)
{
    struct state *s = 0;
    ui->id = id("retained");
    if ((s = begin(ctx, ui->id))) {
        struct panel pan = panel_box_begin(s, "Retained"); {
            {struct flex_box fbx = flex_box_begin(s);
            *fbx.orientation = FLEX_BOX_VERTICAL;
            *fbx.spacing = 6;
            {
                /* label button */
                flex_box_slot_fitting(s, &fbx);
                ui->btn = button_label(s, label, 0);

                /* slider */
                {static float sld_val = 5.0f;
                flex_box_slot_static(s, &fbx, 30);
                ui->sld = sliderf(s, 0.0f, &sld_val, 10.0f);}
            } flex_box_end(s, &fbx);}
        } panel_box_end(s, &pan, 0);
        end(s);
    }
}
static void
ui_build_serialized_tables(FILE *fp)
{
    struct context *ctx;
    struct config cfg;
    cfg.font_default_id = 0;
    cfg.font_default_height = 16;
    ctx = create(DEFAULT_ALLOCATOR, &cfg);

    {struct state *s = 0;
    if ((s = begin(ctx, id("serialized_tables")))) {
        struct panel pan = panel_box_begin(s, "Serialized: Compile Time"); {
           struct flex_box fbx = flex_box_begin(s);
            *fbx.orientation = FLEX_BOX_VERTICAL;
            *fbx.spacing = 6, *fbx.padding = 6;
            {
                /* label button */
                flex_box_slot_fitting(s, &fbx);
                setid(s, id("SERIALIZED_BUTTON"));
                button_label(s, txt("Label"));

                /* icon buttons  */
                flex_box_slot_fitting(s, &fbx); {
                    struct flex_box gbx = flex_box_begin(s);
                    *gbx.padding = 0;
                    flex_box_slot_dyn(s, &gbx);
                        setid(s, id("SERIALIZED_BUTTON_CONFIG"));
                        button_icon(s, ICON_CONFIG);
                    flex_box_slot_dyn(s, &gbx);
                        setid(s, id("SERIALIZED_BUTTON_CHART"));
                        button_icon(s, ICON_CHART_BAR);
                    flex_box_slot_dyn(s, &gbx);
                        setid(s, id("SERIALIZED_BUTTON_DESKTOP"));
                        button_icon(s, ICON_DESKTOP);
                    flex_box_slot_dyn(s, &gbx);
                        setid(s, id("SERIALIZED_BUTTON_DOWNLOAD"));
                        button_icon(s, ICON_DOWNLOAD);
                    flex_box_end(s, &gbx);
                }
            } flex_box_end(s, &fbx);
        } panel_box_end(s, &pan, 0);
        end(s);
    }}
    ui_commit(ctx);
    ui_dump(ctx, fp, "ui");
    ui_cleanup(ctx);
}
static void
ui_load_serialized_tables(struct context *ctx)
{
    static const struct element g_1555159930_elements[] = {
        {1048584, 0lu, 0lu, 0lu, 0, 7, 0, 0, 0},
        {22, 6679361039399649282lu, 0lu, 6679361039399649281lu, 1, 8, 0, 0, 0},
        {13, 6679361039399649284lu, 6679361039399649282lu, 6679361039399649283lu, 2, 9, 0, 0, 0},
        {14, 6679361039399649286lu, 6679361039399649284lu, 6679361039399649285lu, 3, 10, 0, 6, 0},
        {23, 6679361039399649288lu, 6679361039399649286lu, 6679361039399649287lu, 4, 11, 0, 8, 0},
        {12, 6679361039399649290lu, 6679361039399649288lu, 6679361039399649289lu, 5, 12, 0, 8, 0},
        {10, 6679361039399649292lu, 6679361039399649290lu, 6679361039399649291lu, 6, 13, 0, 8, 0},
        {10, 6679361039399649293lu, 6679361039399649292lu, 6679361039399649291lu, 7, 14, 0, 8, 0},
        {11, 6679361039399649295lu, 6679361039399649293lu, 6679361039399649294lu, 8, 15, 0, 11, 0},
        {11, 6679361039399649296lu, 6679361039399649295lu, 6679361039399649294lu, 9, 16, 0, 11, 0},
        {1, 6679361039399649298lu, 6679361039399649296lu, 6679361039399649297lu, 10, 17, 0, 14, 0},
        {14, 6679361039399649300lu, 6679361039399649284lu, 6679361039399649299lu, 3, 10, 0, 17, 0},
        {32, 6679361039399649302lu, 6679361039399649300lu, 6679361039399649301lu, 4, 11, 0, 19, 0},
        {9, 6679361039399649304lu, 6679361039399649302lu, 6679361039399649303lu, 5, 12, 0, 19, 0},
        {9, 6679361039399649305lu, 6679361039399649304lu, 6679361039399649303lu, 6, 13, 0|BOX_MOVABLE_X|BOX_MOVABLE_Y, 19, 0},
        {9, 6679361039399649307lu, 6679361039399649302lu, 6679361039399649306lu, 5, 12, 0, 26, 0},
        {9, 6679361039399649308lu, 6679361039399649307lu, 6679361039399649306lu, 6, 13, 0|BOX_MOVABLE_X|BOX_MOVABLE_Y, 26, 0},
        {31, 6679361039399649310lu, 6679361039399649302lu, 6679361039399649309lu, 5, 12, 0, 33, 0},
        {13, 6679361039399649312lu, 6679361039399649310lu, 6679361039399649311lu, 6, 13, 0, 38, 0},
        {14, 6679361039399649314lu, 6679361039399649312lu, 6679361039399649313lu, 7, 14, 0, 44, 0},
        {3, 998704728lu, 6679361039399649314lu, 6679361039399649315lu, 8, 15, 0, 46, 0},
        {2, 6679361039399649317lu, 998704728lu, 6679361039399649316lu, 9, 16, 0, 46, 0},
        {12, 6679361039399649319lu, 6679361039399649317lu, 6679361039399649318lu, 10, 17, 0, 47, 0},
        {10, 6679361039399649321lu, 6679361039399649319lu, 6679361039399649320lu, 11, 18, 0, 47, 0},
        {10, 6679361039399649322lu, 6679361039399649321lu, 6679361039399649320lu, 12, 19, 0, 47, 0},
        {11, 6679361039399649324lu, 6679361039399649322lu, 6679361039399649323lu, 13, 20, 0, 50, 0},
        {11, 6679361039399649325lu, 6679361039399649324lu, 6679361039399649323lu, 14, 21, 0, 50, 0},
        {1, 6679361039399649327lu, 6679361039399649325lu, 6679361039399649326lu, 15, 22, 0, 53, 0},
        {14, 6679361039399649329lu, 6679361039399649312lu, 6679361039399649328lu, 7, 14, 0, 56, 0},
        {13, 6679361039399649331lu, 6679361039399649329lu, 6679361039399649330lu, 8, 15, 0, 58, 0},
        {14, 6679361039399649333lu, 6679361039399649331lu, 6679361039399649332lu, 9, 16, 0, 64, 0},
        {4, 3567268847lu, 6679361039399649333lu, 6679361039399649334lu, 10, 17, 0, 66, 0},
        {2, 6679361039399649336lu, 3567268847lu, 6679361039399649335lu, 11, 18, 0, 66, 0},
        {12, 6679361039399649338lu, 6679361039399649336lu, 6679361039399649337lu, 12, 19, 0, 67, 0},
        {10, 6679361039399649340lu, 6679361039399649338lu, 6679361039399649339lu, 13, 20, 0, 67, 0},
        {10, 6679361039399649341lu, 6679361039399649340lu, 6679361039399649339lu, 14, 21, 0, 67, 0},
        {11, 6679361039399649343lu, 6679361039399649341lu, 6679361039399649342lu, 15, 22, 0, 70, 0},
        {11, 6679361039399649344lu, 6679361039399649343lu, 6679361039399649342lu, 16, 23, 0, 70, 0},
        {0, 6679361039399649346lu, 6679361039399649344lu, 6679361039399649345lu, 17, 24, 0, 73, 0},
        {14, 6679361039399649348lu, 6679361039399649331lu, 6679361039399649347lu, 9, 16, 0, 75, 0},
        {4, 3282852853lu, 6679361039399649348lu, 6679361039399649349lu, 10, 17, 0, 77, 0},
        {2, 6679361039399649351lu, 3282852853lu, 6679361039399649350lu, 11, 18, 0, 77, 0},
        {12, 6679361039399649353lu, 6679361039399649351lu, 6679361039399649352lu, 12, 19, 0, 78, 0},
        {10, 6679361039399649355lu, 6679361039399649353lu, 6679361039399649354lu, 13, 20, 0, 78, 0},
        {10, 6679361039399649356lu, 6679361039399649355lu, 6679361039399649354lu, 14, 21, 0, 78, 0},
        {11, 6679361039399649358lu, 6679361039399649356lu, 6679361039399649357lu, 15, 22, 0, 81, 0},
        {11, 6679361039399649359lu, 6679361039399649358lu, 6679361039399649357lu, 16, 23, 0, 81, 0},
        {0, 6679361039399649361lu, 6679361039399649359lu, 6679361039399649360lu, 17, 24, 0, 84, 0},
        {14, 6679361039399649363lu, 6679361039399649331lu, 6679361039399649362lu, 9, 16, 0, 86, 0},
        {4, 3042075085lu, 6679361039399649363lu, 6679361039399649364lu, 10, 17, 0, 88, 0},
        {2, 6679361039399649366lu, 3042075085lu, 6679361039399649365lu, 11, 18, 0, 88, 0},
        {12, 6679361039399649368lu, 6679361039399649366lu, 6679361039399649367lu, 12, 19, 0, 89, 0},
        {10, 6679361039399649370lu, 6679361039399649368lu, 6679361039399649369lu, 13, 20, 0, 89, 0},
        {10, 6679361039399649371lu, 6679361039399649370lu, 6679361039399649369lu, 14, 21, 0, 89, 0},
        {11, 6679361039399649373lu, 6679361039399649371lu, 6679361039399649372lu, 15, 22, 0, 92, 0},
        {11, 6679361039399649374lu, 6679361039399649373lu, 6679361039399649372lu, 16, 23, 0, 92, 0},
        {0, 6679361039399649376lu, 6679361039399649374lu, 6679361039399649375lu, 17, 24, 0, 95, 0},
        {14, 6679361039399649378lu, 6679361039399649331lu, 6679361039399649377lu, 9, 16, 0, 97, 0},
        {4, 79436257lu, 6679361039399649378lu, 6679361039399649379lu, 10, 17, 0, 99, 0},
        {2, 6679361039399649381lu, 79436257lu, 6679361039399649380lu, 11, 18, 0, 99, 0},
        {12, 6679361039399649383lu, 6679361039399649381lu, 6679361039399649382lu, 12, 19, 0, 100, 0},
        {10, 6679361039399649385lu, 6679361039399649383lu, 6679361039399649384lu, 13, 20, 0, 100, 0},
        {10, 6679361039399649386lu, 6679361039399649385lu, 6679361039399649384lu, 14, 21, 0, 100, 0},
        {11, 6679361039399649388lu, 6679361039399649386lu, 6679361039399649387lu, 15, 22, 0, 103, 0},
        {11, 6679361039399649389lu, 6679361039399649388lu, 6679361039399649387lu, 16, 23, 0, 103, 0},
        {0, 6679361039399649391lu, 6679361039399649389lu, 6679361039399649390lu, 17, 24, 0, 106, 0},
        {31, 6679361039399649392lu, 6679361039399649310lu, 6679361039399649309lu, 6, 13, 0|BOX_IMMUTABLE, 33, 0},
    };
    static const uiid g_1555159930_tbl_keys[128] = {
        0lu,0lu,6679361039399649282lu,0lu,6679361039399649284lu,0lu,6679361039399649286lu,0lu,
        6679361039399649288lu,0lu,6679361039399649290lu,0lu,6679361039399649292lu,6679361039399649293lu,0lu,6679361039399649295lu,
        6679361039399649296lu,0lu,6679361039399649298lu,0lu,6679361039399649300lu,0lu,6679361039399649302lu,0lu,
        6679361039399649304lu,6679361039399649305lu,0lu,6679361039399649307lu,6679361039399649308lu,0lu,6679361039399649310lu,0lu,
        6679361039399649312lu,0lu,6679361039399649314lu,0lu,0lu,6679361039399649317lu,0lu,6679361039399649319lu,
        0lu,6679361039399649321lu,6679361039399649322lu,0lu,6679361039399649324lu,6679361039399649325lu,0lu,6679361039399649327lu,
        0lu,6679361039399649329lu,0lu,6679361039399649331lu,0lu,6679361039399649333lu,0lu,0lu,
        6679361039399649336lu,0lu,6679361039399649338lu,0lu,6679361039399649340lu,6679361039399649341lu,0lu,6679361039399649343lu,
        6679361039399649344lu,0lu,6679361039399649346lu,0lu,6679361039399649348lu,0lu,0lu,6679361039399649351lu,
        0lu,6679361039399649353lu,0lu,6679361039399649355lu,6679361039399649356lu,3042075085lu,6679361039399649358lu,6679361039399649359lu,
        0lu,6679361039399649361lu,0lu,6679361039399649363lu,0lu,0lu,6679361039399649366lu,0lu,
        998704728lu,6679361039399649368lu,6679361039399649370lu,6679361039399649371lu,0lu,6679361039399649373lu,6679361039399649374lu,0lu,
        6679361039399649376lu,79436257lu,6679361039399649378lu,0lu,0lu,6679361039399649381lu,0lu,6679361039399649383lu,
        0lu,6679361039399649385lu,6679361039399649386lu,0lu,6679361039399649388lu,6679361039399649389lu,0lu,3567268847lu,
        6679361039399649391lu,6679361039399649392lu,0lu,0lu,0lu,3282852853lu,0lu,0lu,
        0lu,0lu,0lu,0lu,0lu,0lu,0lu,0lu
    };
    static const int g_1555159930_tbl_vals[128] = {
        0,0,1,0,2,0,3,0,4,0,5,0,6,7,0,8,
        9,0,10,0,11,0,12,0,13,14,0,15,16,0,17,0,
        18,0,19,0,0,21,0,22,0,23,24,0,25,26,0,27,
        0,28,0,29,0,30,0,0,32,0,33,0,34,35,0,36,
        37,0,38,0,39,0,0,41,0,42,0,43,44,49,45,46,
        0,47,0,48,0,0,50,0,20,51,52,53,0,54,55,0,
        56,58,57,0,0,59,0,60,0,61,62,0,63,64,0,31,
        65,66,0,0,0,40,0,0,0,0,0,0,0,0,0,0
    };
    static union param g_1555159930_params[108] = {
        {6679361039399649284lu},{1lu},{0lu},{4lu},{0lu},{2lu},{3lu},{0lu},
        {4lu},{5lu},{6679361039399649293lu},{1lu},{1lu},{6679361039399649296lu},{16lu},{0lu},
        {0lu},{0lu},{0lu},{1065353216lu},{1065353216lu},{1065353216lu},{1065353216lu},{0lu},
        {0lu},{6679361039399649305lu},{1065353216lu},{1065353216lu},{1065353216lu},{1065353216lu},{0lu},{0lu},
        {6679361039399649308lu},{0lu},{0lu},{0lu},{6679361039399649310lu},{6679361039399649392lu},{6679361039399649312lu},{1lu},
        {0lu},{6lu},{6lu},{2lu},{3lu},{0lu},{0lu},{6lu},
        {6lu},{6679361039399649322lu},{1lu},{1lu},{6679361039399649325lu},{16lu},{0lu},{25lu},
        {3lu},{0lu},{6679361039399649331lu},{0lu},{0lu},{4lu},{0lu},{4lu},
        {0lu},{0lu},{0lu},{6lu},{6lu},{6679361039399649341lu},{1lu},{1lu},
        {6679361039399649344lu},{0lu},{16lu},{0lu},{0lu},{0lu},{6lu},{6lu},
        {6679361039399649356lu},{1lu},{1lu},{6679361039399649359lu},{1lu},{16lu},{0lu},{0lu},
        {0lu},{6lu},{6lu},{6679361039399649371lu},{1lu},{1lu},{6679361039399649374lu},{2lu},
        {16lu},{0lu},{0lu},{0lu},{6lu},{6lu},{6679361039399649386lu},{1lu},
        {1lu},{6679361039399649389lu},{3lu},{16lu}
    };
    static const unsigned char g_1555159930_data[31] = {
        0x53,0x65,0x72,0x69,0x61,0x6c,0x69,0x7a,0x65,0x64,0x3a,0x20,0x43,0x6f,0x6d,0x70,
        0x69,0x6c,0x65,0x20,0x54,0x69,0x6d,0x65,0x00,0x4c,0x61,0x62,0x65,0x6c,0x00
    };
    static struct box *g_1555159930_bfs[68];
    static struct box g_1555159930_boxes[67];
    static struct module g_1555159930_module;
    static struct repository g_1555159930_repo;
    static const struct component g_1555159930_component = {
        1,1555159930, 18, 7,g_1555159930_elements, cntof(g_1555159930_elements),
        g_1555159930_tbl_vals, g_1555159930_tbl_keys,cntof(g_1555159930_tbl_keys),
        g_1555159930_data, cntof(g_1555159930_data),
        g_1555159930_params, cntof(g_1555159930_params),
        &g_1555159930_module, &g_1555159930_repo, g_1555159930_boxes,
        g_1555159930_bfs, cntof(g_1555159930_boxes)
    };
    static const struct container g_ui_containers[] = {
        {1555159930, 0, 6, 0, 0, &g_1555159930_component},
    };
    load(ctx, g_ui_containers, cntof(g_ui_containers));
}
intern int
icon_label(struct state *s, int icon_id, const char *lbl)
{
    struct button_state st;
    struct button btn = button_begin(s);
    button_poll(s, &st, &btn);
    *btn.style = NVG_BUTTON_TRANSPARENT;
    {
        struct sborder sbr = sborder_begin(s);
        *sbr.x = 6, *sbr.y = 6;
        {
            /* folder icon */
            struct flex_box gbx = flex_box_begin(s);
            *gbx.spacing = *gbx.padding = 0;
            *gbx.orientation = FLEX_BOX_VERTICAL;
            flex_box_slot_dyn(s, &gbx); {
                struct salign aln = salign_begin(s);
                *aln.horizontal = SALIGN_CENTER;
                *aln.vertical = SALIGN_MIDDLE; {
                    struct icon ico = icon(s, icon_id);
                    *ico.size = 32;
                } salign_end(s);
            }
            /* folder title */
            flex_box_slot_fitting(s, &gbx); {
                struct salign aln = salign_begin(s);
                *aln.horizontal = SALIGN_CENTER;
                *aln.vertical = SALIGN_MIDDLE;
                    label(s, txt(lbl));
                salign_end(s);
            } flex_box_end(s, &gbx);
        } sborder_end(s);
    } button_end(s);
    return st.clicked;
}
static void
ui_im(struct context *ctx)
{
    /* GUI */
    struct state *s = 0;
    if ((s = begin(ctx, id("Main")))) {
        /* Sidebar */
        {struct sidebar sb = sidebar_begin(s);
            static const char *folder[] = {"Documents", "Download", "Desktop",
                "Images", "boot", "dev", "lib", "include", "tmp", "usr", "root"};

            int i = 0;
            sborder_begin(s);
            scroll_region_begin(s);
            {struct flex_box fbx = flex_box_begin(s);
            *fbx.flow = FLEX_BOX_WRAP, *fbx.padding = 0;
            for (i = 0; i < cntof(folder); ++i) {
                flex_box_slot_static(s, &fbx, 60);
                if (icon_label(s, ICON_FOLDER, folder[i]))
                    printf("Button: %s clicked\n", folder[i]);
            } flex_box_end(s, &fbx);}
            scroll_region_end(s);
            sborder_end(s);
        sidebar_end(s, &sb);}

        /* Panels */
        {struct overlap_box obx = overlap_box_begin(s);
        overlap_box_slot(s, &obx, id("Immdiate Mode"));
        {
            /* Immediate mode panel */
            struct con_box cbx = con_box_begin(s, 0, 0);
            static const struct con cons[] = {
                {CON_SET, {0,ATTR_L}, {SPARENT,ATTR_L}, {1,600}},
                {CON_SET, {0,ATTR_T}, {SPARENT,ATTR_T}, {1,50}},
                {CON_SET, {0,ATTR_W}, {SPARENT,ATTR_W}, {0,180}},
                {CON_SET, {0,ATTR_H}, {SPARENT,ATTR_H}, {0,250}},
            }; con_box_slot(s, &cbx, cons, cntof(cons));
            {struct panel pan = panel_box_begin(s, "Immediate"); {
               struct flex_box fbx = flex_box_begin(s);
                *fbx.orientation = FLEX_BOX_VERTICAL;
                *fbx.spacing = 6;
                {
                    /* label button */
                    flex_box_slot_fitting(s, &fbx);
                    if (button_label_clicked(s, txt("Label")))
                        fprintf(stdout, "Button pressed\n");

                    /* icon buttons  */
                    flex_box_slot_fitting(s, &fbx); {
                        struct flex_box gbx = flex_box_begin(s);
                        *gbx.padding = 0;
                        flex_box_slot_dyn(s, &gbx);
                            if (button_icon_clicked(s, ICON_CONFIG))
                                fprintf(stdout, "Button: Config pressed\n");
                        flex_box_slot_dyn(s, &gbx);
                            if (button_icon_clicked(s, ICON_CHART_BAR))
                                fprintf(stdout, "Button: Config pressed\n");
                        flex_box_slot_dyn(s, &gbx);
                            if (button_icon_clicked(s, ICON_DESKTOP))
                                fprintf(stdout, "Button: Config pressed\n");
                        flex_box_slot_dyn(s, &gbx);
                            if (button_icon_clicked(s, ICON_DOWNLOAD))
                                fprintf(stdout, "Button: Config pressed\n");
                        flex_box_end(s, &gbx);
                    }
                    /* combo */
                    flex_box_slot_fitting(s, &fbx); {
                        static const char *items[] = {"Pistol","Shotgun","Plasma","BFG"};
                        combo_box(s, id("weapons"), items, cntof(items));
                    }
                    /* slider */
                    {static float sld_val = 5.0f;
                    flex_box_slot_variable(s, &fbx, 30);
                    sliderf(s, 0.0f, &sld_val, 10.0f);}

                    /* checkbox */
                    flex_box_slot_fitting(s, &fbx); {
                        static int unchecked = 0, checked = 1;
                        struct flex_box gbx = flex_box_begin(s);
                        *gbx.padding = 0;
                        flex_box_slot_dyn(s, &gbx);
                            checkbox(s, &unchecked, "unchecked", 0);
                        flex_box_slot_dyn(s, &gbx);
                            checkbox(s, &checked, "checked", 0);
                        flex_box_end(s, &gbx);
                    }
                    /* toggle */
                    flex_box_slot_fitting(s, &fbx); {
                        static int inactive = 0, active = 1;
                        struct flex_box gbx = flex_box_begin(s);
                        *gbx.padding = 0;
                        flex_box_slot_dyn(s, &gbx);
                            toggle(s, &inactive, "inactive", 0);
                        flex_box_slot_dyn(s, &gbx);
                            toggle(s, &active, "active", 0);
                        flex_box_end(s, &gbx);
                    }
                    /* radio */
                    flex_box_slot_fitting(s, &fbx); {
                        static int unselected = 0, selected = 1;
                        struct flex_box gbx = flex_box_begin(s);
                        *gbx.padding = 0;
                        flex_box_slot_dyn(s, &gbx);
                            radio(s, &unselected, "unselected", 0);
                        flex_box_slot_dyn(s, &gbx);
                            radio(s, &selected, "selected", 0);
                        flex_box_end(s, &gbx);
                    }
                } flex_box_end(s, &fbx);
            } panel_box_end(s, &pan, 0);
            con_box_end(s, &cbx);}
        }
        /* link: retained module */
        overlap_box_slot(s, &obx, id("Retained Mode"));
        {
            struct con_box cbx = con_box_begin(s, 0, 0);
            static const struct con cons[] = {
                {CON_SET, {0,ATTR_L}, {SPARENT,ATTR_L}, {1,320}},
                {CON_SET, {0,ATTR_T}, {SPARENT,ATTR_T}, {1,50}},
                {CON_SET, {0,ATTR_W}, {SPARENT,ATTR_W}, {0,200}},
                {CON_SET, {0,ATTR_H}, {SPARENT,ATTR_H}, {0,130}},
            }; con_box_slot(s, &cbx, cons, cntof(cons));
            link(s, id("retained"), RELATIONSHIP_INDEPENDENT);
            con_box_end(s, &cbx);
        }
        /* link: compile time module */
        overlap_box_slot(s, &obx, id("Compile Time"));
        {
            struct con_box cbx = con_box_begin(s, 0, 0);
            static const struct con cons[] = {
                {CON_SET, {0,ATTR_L}, {SPARENT,ATTR_L}, {1,320}},
                {CON_SET, {0,ATTR_T}, {SPARENT,ATTR_T}, {1,250}},
                {CON_SET, {0,ATTR_W}, {SPARENT,ATTR_W}, {0,200}},
                {CON_SET, {0,ATTR_H}, {SPARENT,ATTR_H}, {0,130}}
            }; con_box_slot(s, &cbx, cons, cntof(cons));
            link(s, id("serialized_tables"), RELATIONSHIP_INDEPENDENT);
            con_box_end(s, &cbx);
        }
        overlap_box_end(s, &obx);}
    } end(s);
}
int main(int argc, char *argv[])
{
    enum fonts {
        FONT_HL,
        FONT_ICONS,
        FONT_CNT
    };
    int quit = 0;
    int fnts[FONT_CNT];
    struct context *ctx;
    struct ui_retained ui = {0};
    struct NVGcontext *vg;
    SDL_GLContext glContext;

    /* SDL */
    SDL_Window *win = 0;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
    win = SDL_CreateWindow("GUI", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 1000, 600, SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
    glContext = SDL_GL_CreateContext(win);

    /* NanoVG */
    vg = nvgCreateGL2(NVG_ANTIALIAS);
    if (!vg) return -1;
    fnts[FONT_HL] = nvgCreateFont(vg, "half_life", "half_life.ttf");
    fnts[FONT_ICONS] = nvgCreateFont(vg, "icons", "icons.ttf");
    default_icon_font = fnts[FONT_ICONS];

    /* GUI */
    {struct config cfg;
    cfg.font_default_id = fnts[FONT_HL];
    cfg.font_default_height = 16;
    ctx = create(DEFAULT_ALLOCATOR, &cfg);}
    input_resize(ctx, 1000, 600);

    if (argc > 1) {
        ui_build_serialized_tables(stdout);
        return 0;
    } else ui_load_serialized_tables(ctx);
    ui_build_retained(ctx, &ui, "Initial");

    while (!quit) {
        /* Input */
        int w, h;
        {SDL_Event evt;
        SDL_GetWindowSize(win, &w, &h);
        input_resize(ctx, w, h);
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
            case SDL_QUIT: quit = 1; break;}
            ui_event(ctx, &evt);
        }}
        ui_im(ctx);

        /* Process: Input */
        ui_commit(ctx);
        {int i; union process *p = 0;
        while ((p = process_begin(ctx, PROCESS_INPUT))) {
            switch (p->type) {default:break;
            case PROC_BLUEPRINT: {
                struct process_layouting *op = &p->layout;
                for (i = op->begin; i != op->end; i += op->inc)
                    ui_blueprint(vg, p, op->boxes[i]);
            } break;
            case PROC_LAYOUT: {
                struct process_layouting *op = &p->layout;
                for (i = op->begin; i != op->end; i += op->inc)
                    ui_layout(vg, p, op->boxes[i]);
            } break;
            case PROC_INPUT: {
                struct list_hook *it = 0;
                struct process_input *op = &p->input;
                list_foreach(it, &op->evts) {
                    union event *evt = list_entry(it, union event, hdr.hook);
                    ui_input(p, evt); /* handle widget internal events */

                    switch (evt->type) {
                    case EVT_CLICKED: {
                        /* event driven input handling */
                        for (i = 0; i < evt->hdr.cnt; i++) {
                            struct box *b = evt->hdr.boxes[i];
                            switch (b->type) {
                            case WIDGET_BUTTON_LABEL: {
                                if (b->id == id("SERIALIZED_BUTTON"))
                                    fprintf(stdout, "serial button clicked\n");
                            } break;
                            case WIDGET_BUTTON_ICON: {
                                if (b->id == id("SERIALIZED_BUTTON_CONFIG"))
                                    fprintf(stdout, "serial button: config clicked\n");
                                else if (b->id == id("SERIALIZED_BUTTON_CHART"))
                                    fprintf(stdout, "serial button: chart clicked\n");
                                else if (b->id == id("SERIALIZED_BUTTON_DESKTOP"))
                                    fprintf(stdout, "serial button: desktop clicked\n");
                                else if (b->id == id("SERIALIZED_BUTTON_DOWNLOAD"))
                                    fprintf(stdout, "serial button: download clicked\n");
                            } break;}
                        }
                    } break;}
                }
            } break;}
            process_end(p);
        }}
        /* event handling by polling */
        {
            /* button */
            {struct button_state btn = {0};
            button_label_query(ctx, &btn, ui.id, &ui.btn);
            if (btn.clicked) fprintf(stdout, "Retained button clicked\n");
            if (btn.pressed) fprintf(stdout, "Retained button pressed\n");
            if (btn.released) fprintf(stdout, "Retained button released\n");
            if (btn.entered) fprintf(stdout, "Retained button entered\n");
            if (btn.exited) fprintf(stdout, "Retained button exited\n");}

            /* slider */
            {struct slider_state sld = {0};
            slider_query(ctx, ui.id, &sld, &ui.sld);
            if (sld.value_changed)
                fprintf(stdout, "Retained slider value changed: %.2f\n", *ui.sld.value);
            if (sld.entered) fprintf(stdout, "Retained slider entered\n");
            if (sld.exited) fprintf(stdout, "Retained slider exited\n");}
        }
        /* Paint */
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
        glClearColor(0.10f, 0.18f, 0.24f, 1);
        nvgBeginFrame(vg, w, h, 1.0f);
        ui_paint(vg, ctx, w, h);
        ui_clear(ctx);
        nvgEndFrame(vg);

        /* Finish frame */
        SDL_GL_SwapWindow(win);
        SDL_Delay(16);
    }
    ui_cleanup(ctx);

    /* Cleanup */
    nvgDeleteGL2(vg);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}