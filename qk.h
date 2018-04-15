#ifndef QK_H
#define QK_H

#include <assert.h> /* assert */
#include <stdlib.h> /* calloc, free */
#include <string.h> /* memcpy, memset */
#include <inttypes.h> /* PRIu64 */
#include <limits.h> /* INT_MAX */
#include <stdio.h> /* fprintf, fputc */

/* macros */
#define api extern
#define intern static
#define unused(a) ((void)a)
#define cast(t,p) ((t)(p))
#define ucast(p) ((uintptr_t)p)
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
    volatile unsigned lock;
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
#define MAX_CMD_BUF ((int)((DEFAULT_MEMORY_BLOCK_SIZE - (sizeof(struct memory_block) + sizeof(void*))) / sizeof(union cmd)))

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
struct param_buf {
    union param ops[MAX_OPS];
};
struct str_buf {
    struct str_buf *next;
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
    struct param_buf *param_list;
    struct param_buf *opbuf;

    /* strings */
    int buf_off, total_buf_size;
    struct str_buf *buf_list;
    struct str_buf *buf;

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

/* commands */
enum cmd_type {
    CMD_LNK,
    CMD_CONCT,
    CMD_CNT
};
struct cmd_lnk {
    enum cmd_type type;
    mid parent_mid;
    uiid parent_id;
    mid child_mid;
    uiid child_id;
};
struct cmd_con {
    enum cmd_type type;
    mid parent, child;
    int rel;
};
union cmd {
    enum cmd_type type;
    struct cmd_lnk lnk;
    struct cmd_con con;
};
struct cmd_blk {
    struct cmd_blk *next;
    union cmd cmds[MAX_CMD_BUF];
};
struct cmd_buf {
    int idx;
    struct cmd_blk *list;
    struct cmd_blk *buf;
    volatile unsigned lock;
};

/* operation */
struct object;
enum operation_type {
    OP_ALLOC_PERSISTENT,
    OP_ALLOC_FRAME,
    OP_COMPILE
};
struct operation_alloc {
    void *ptr;
    int size;
};
struct operation {
    enum operation_type type;
    struct operation_alloc alloc;
    struct object *obj;
};
struct object {
    int state;
    struct operation op;
    struct context *ctx;
    struct state *in;
    struct repository *out;
    struct cmd_buf *cmds;
    struct memory_arena *mem;
};

/* process */
enum process_type {
    PROC_COMMIT,
    PROC_BLUEPRINT,
    PROC_LAYOUT,
    PROC_INPUT,
    PROC_PAINT,
    PROC_CLEAR,
    PROC_CLEAR_FULL,
    PROC_CLEANUP,
    PROC_FREE_PERSISTENT,
    PROC_FREE_FRAME,
    PROC_CNT
};
enum processes {
    PROCESS_COMMIT = flag(PROC_COMMIT),
    PROCESS_BLUEPRINT = PROCESS_COMMIT|flag(PROC_BLUEPRINT),
    PROCESS_LAYOUT = PROCESS_BLUEPRINT|flag(PROC_LAYOUT),
    PROCESS_INPUT = PROCESS_LAYOUT|flag(PROC_INPUT),
    PROCESS_PAINT = PROCESS_LAYOUT|flag(PROC_PAINT),
    PROCESS_CLEAR = flag(PROC_CLEAR),
    PROCESS_CLEAR_FULL = PROCESS_CLEAR|flag(PROC_CLEAR_FULL),
    PROCESS_CLEANUP = PROCESS_CLEAR_FULL|flag(PROC_CLEANUP)
};
struct process_header {
    enum process_type type;
    struct context *ctx;
    struct memory_arena *arena;
    struct temp_memory tmp;
};
struct process_commit {
    struct process_header hdr;
    struct object *objs;
    int cnt;
    struct cmd_buf lnks;
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
struct process_free {
    struct process_header hdr;
    void *ptr;
};
union process {
    /* base */
    enum process_type type;
    struct process_header hdr;
    /* derived */
    struct process_commit commit;
    struct process_layouting layout;
    struct process_layouting blueprint;
    struct process_input input;
    struct process_paint paint;
    struct process_free free;
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
    volatile unsigned mem_lock;

    /* modules */
    volatile unsigned module_lock;
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
api struct box *query(struct context *ctx, unsigned mid, uiid id);
api void reset(struct context *ctx);
api void destroy(struct context *ctx);

/* serialize */
api void load(struct context *ctx, const struct container *c, int cnt);
api void store_table(FILE *fp, struct context *ctx, const char *name, int indent);
api void store_binary(FILE *fp, struct context *ctx);
api void trace(FILE *fp, struct context *ctx);

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

/* process */
api union process* process_begin(struct context *ctx, unsigned flags);
api struct operation *commit_begin(struct context *ctx, struct process_commit*, int index);
api int compile(struct object *obj);
api void commit_end(struct operation *op);
api void commit(union process *p);
api void blueprint(union process *p, struct box *b);
api void layout(union process *p, struct box *b);
api void input(union process *p, union event *evt, struct box *b);
api void process_end(union process *p);

/* module */
api struct state* begin(struct context *ctx, mid id);
api void end(struct state *s);
api struct state* module_begin(struct context *ctx, mid id, enum relationship, mid parent, mid owner, uiid bid);
api void module_end(struct state *s);
api struct state *section_begin(struct state *s, mid id);
api void section_end(struct state *s);
api void link(struct state *s, mid id, enum relationship);
api void slot(struct state *s, uiid id);
api struct box *polls(struct state *s, uiid id);

/* popup */
api struct state *popup_begin(struct state *s, mid id, enum popup_type type);
api void popup_show(struct context *ctx, mid id, enum visibility vis);
api void popup_toggle(struct context *ctx, mid id);
api void popup_end(struct state *s);
api struct box *popup_find(struct context *ctx, mid id);

/* widget */
api void widget_begin(struct state *s, int type);
api uiid widget_box(struct state *s);
api uiid widget_box_push(struct state *s);
api void widget_box_property_set(struct state *s, enum properties);
api void widget_box_property_clear(struct state *s, enum properties);
api void widget_box_pop(struct state *s);
api void widget_end(struct state *s);

/* parameter */
api float *widget_param_float(struct state *s, float f);
api int *widget_param_int(struct state *s, int i);
api unsigned *widget_param_uint(struct state *s, unsigned u);
api uiid *widget_param_id(struct state *s, uiid id);
api mid *widget_param_mid(struct state *s, mid id);
api const char* widget_param_str(struct state *s, const char *str, int len);

api float* widget_modifier_float(struct state *s, float *f);
api int* widget_modifier_int(struct state *s, int *i);
api unsigned* widget_modifier_uint(struct state *s, unsigned *u);

api float* widget_state_float(struct state *s, float f);
api int* widget_state_int(struct state *s, int i);
api unsigned* widget_state_uint(struct state *s, unsigned u);
api uiid* widget_state_id(struct state *s, uiid u);

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
api void box_pad(struct box *d, const struct box *s, int padx, int pady);
api int box_intersect(const struct box *a, const struct box *b);

/* id */
api void pushid(struct state *s, unsigned id);
api void setid(struct state *s, uiid id);
api uiid genwid(struct state *s);
api uiid genbid(struct state *s);
api void popid(struct state *s);

/* list */
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

api void list_init(struct list_hook *list);
api void list_add_head(struct list_hook *list, struct list_hook *n);
api void list_add_tail(struct list_hook *list, struct list_hook *n);
api void list_del(struct list_hook *entry);
api void list_move_head(struct list_hook *list, struct list_hook *entry);
api void list_move_tail(struct list_hook *list, struct list_hook *entry);

/* utf-8 */
api int utf_encode(char *s, int cap, unsigned long u);
api int utf_decode(unsigned long *rune, const char *str, int len);
api int utf_len(const char *s, int len);
api const char* utf_at(unsigned long *rune, int *rune_len, const char *s, int len, int idx);

/* memory */
#define arena_push_type(a,type) (type*)arena_push(a, 1, szof(type), alignof(type))
#define arena_push_array(a,n,type) (type*)arena_push(a, (n), szof(type), alignof(type))
api void *arena_push(struct memory_arena *a, int cnt, int size, int align);

/* math */
api int npow2(int n);
api int floori(float x);
api int ceili(float x);
api float roundf(float x);
api int roundi(float x);
api int strn(const char *s);

#endif