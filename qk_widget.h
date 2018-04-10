#ifndef WIDGET_H
#define WIDGET_H

enum widget_type {
    /* Widgets */
    WIDGET_GENERIC_FIRST = 0x10000,
    WIDGET_ICON = WIDGET_GENERIC_FIRST,
    WIDGET_LABEL,
    WIDGET_BUTTON,
    WIDGET_CHECKBOX,
    WIDGET_TOGGLE,
    WIDGET_RADIO,
    WIDGET_SLIDER,
    WIDGET_SCROLL,
    WIDGET_GENERIC_LAST = WIDGET_SCROLL,
    /* Layouting */
    WIDGET_LAYOUT_FIRST = 0x20000,
    WIDGET_SBORDER = WIDGET_LAYOUT_FIRST,
    WIDGET_SALIGN,
    WIDGET_SBOX,
    WIDGET_FLEX_BOX,
    WIDGET_FLEX_BOX_SLOT,
    WIDGET_OVERLAP_BOX,
    WIDGET_OVERLAP_BOX_SLOT,
    WIDGET_CON_BOX,
    WIDGET_CON_BOX_SLOT,
    WIDGET_LAYOUT_LAST = WIDGET_CON_BOX_SLOT,
    /* Container */
    WIDGET_CONTAINER_FIRST = 0x30000,
    WIDGET_COMBO = WIDGET_CONTAINER_FIRST,
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
    WIDGET_WINDOW,
    WIDGET_WINDOW_CONTENT,
    WIDGET_CONTAINER_LAST = WIDGET_WINDOW_CONTENT,
    /* Transformation */
    WIDGET_TRANSFORM_FIRST = 0x40000,
    WIDGET_CLIP_BOX,
    WIDGET_SCROLL_REGION,
    WIDGET_SCROLL_BOX,
    WIDGET_ZOOM_BOX,
    WIDGET_MOVABLE_BOX,
    WIDGET_SCALABLE_BOX,
    WIDGET_TRANSFORM_LAST = WIDGET_SCALABLE_BOX
};
enum widget_shortcuts {
    SHORTCUT_INTERNAL_BEGIN = 0x80,
    SHORTCUT_SCROLL_REGION_SCROLL = SHORTCUT_INTERNAL_BEGIN,
    SHORTCUT_SCROLL_REGION_RESET,
    SHORTCUT_SCROLL_BOX_BEGIN,
    SHORTCUT_SCROLL_BOX_PGDN,
    SHORTCUT_SCROLL_BOX_PGUP,
    SHORTCUT_SCROLL_BOX_END,
    SHORTCUT_ZOOM_BOX_SCALE_X,
    SHORTCUT_ZOOM_BOX_SCALE_Y,
    SHORTCUT_ZOOM_BOX_SCALE,
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
 *                                  BASE
 * --------------------------------------------------------------------------- */
/* label */
typedef int(*text_measure_f)(void *usr, int font, int fh, const char *txt, int len);
struct label {
    uiid id;
    const char *txt;
    int *font;
    int *height;
};
api struct label label(struct state *s, const char *txt, const char *end);
api void label_blueprint(struct box *b, void *usr, text_measure_f measure);
api struct label label_ref(struct box *b);

/* icon */
struct icon {
    uiid id;
    int *sym;
    int *size;
};
api struct icon icon(struct state *s, int sym);
api void icon_blueprint(struct box *b);
api void icon_layout(struct box *b);
api struct icon icon_ref(struct box *b);

/* button */
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
api struct button button_begin(struct state *s);
api struct button button_begin(struct state *s);
api int button_query(struct context *ctx, struct button_state *st, mid mid, struct button *btn);
api int button_poll(struct state *s, struct button_state *st, struct button *btn);

/* slider */
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
api struct slider sliderf(struct state *s, float min, float *value, float max);
api void slider_blueprint(struct box *b);
api void slider_layout(struct box *b);
api void slider_input(struct box *b, const union event *evt);
api int slider_poll(struct state *s, struct slider_state *st, struct slider *sld);
api int slider_query(struct context *ctx, mid mid, struct slider_state *st, struct slider *sld);

/* scroll */
struct scroll {
    uiid id, cid;
    float *total_x;
    float *total_y;
    float *size_x;
    float *size_y;
    float *off_x;
    float *off_y;
};
api struct scroll scroll(struct state *s, float *off_x, float *off_y);
api struct scroll scroll_begin(struct state *s, float *off_x, float *off_y);
api void scroll_end(struct state *s);
api void scroll_layout(struct box *b);
api void scroll_input(struct box *b, const union event *evt);
api struct scroll scroll_ref(struct box *b);

/* combo */
enum combo_state {
    COMBO_COLLAPSED,
    COMBO_VISIBLE
};
struct combo {
    int *state;
    uiid id;
    mid *popup;
};
static struct combo
combo_ref(struct box *b)
{
    struct combo c = {0};
    c.state = widget_get_int(b, 0);
    c.popup = widget_get_mid(b, 1);
    return c;
}
api struct combo combo_begin(struct state *s, mid id);
api struct state* combo_popup_begin(struct state *s, struct combo *c);
api void combo_popup_end(struct state *s, struct combo *c);
api void combo_end(struct state *s);
api void combo_popup_show(struct context *ctx, struct combo *c);
api void combo_popup_close(struct context *ctx, struct combo *c);
api void combo_layout(struct context *ctx, struct box *b);
api void combo_input(struct context *ctx, struct box *b, const union event *evt);

/* ---------------------------------------------------------------------------
 *                                  LAYOUT
 * --------------------------------------------------------------------------- */
/* sborder */
struct sborder {
    uiid id;
    uiid content;
    int *x;
    int *y;
};
api struct sborder sborder_begin(struct state *s);
api void sborder_end(struct state *s);
api void sborder_blueprint(struct box *b);
api void sborder_layout(struct box *b);
api struct sborder sborder_ref(struct box *b);

/* salign */
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
api struct salign salign_begin(struct state *s);
api void salign_end(struct state *s);
api void salign_layout(struct box *b);
api struct salign salign_ref(struct box *b);

/* sbox */
struct sbox {
    uiid id;
    struct salign align;
    struct sborder border;
};
api struct sbox sbox_begin(struct state *s);
api void sbox_end(struct state *s);

/* flex box */
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
api struct flex_box flex_box_begin(struct state *s);
api void flex_box_slot_dyn(struct state *s, struct flex_box *fbx);
api void flex_box_slot_static(struct state *s, struct flex_box *fbx, int pixel_width);
api void flex_box_slot_variable(struct state *s, struct flex_box *fbx, int min_pixel_width);
api void flex_box_slot_fitting(struct state *s, struct flex_box *fbx);
api void flex_box_end(struct state *s, struct flex_box *fbx);
api void flex_box_blueprint(struct box *b);
api void flex_box_layout(struct box *b);
api struct flex_box flex_box_ref(struct box *b);

/* overlap box */
struct overlap_box {
    uiid id;
    int slots;
    int *cnt;
};
struct overlap_box_slot {
    uiid *id;
    int *zorder;
};
api struct overlap_box overlap_box_begin(struct state *s);
api void overlap_box_slot(struct state *s, struct overlap_box *obx, mid id);
api void overlap_box_end(struct state *s, struct overlap_box *obx);
api void overlap_box_layout(struct box *b, struct memory_arena *arena);
api void overlap_box_input(struct box *b, union event *evt, struct memory_arena *arena);
api struct overlap_box overlap_box_ref(struct box *b);

/* constraint box */
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
struct con_box_slot {int con_cnt, cons;};
struct con_box {
    uiid id;
    int *cond_cnt;
    int *slot_cnt;
    int conds;
};
api struct con_box con_box_begin(struct state *s, struct cond *conds, int cond_cnt);
api void con_box_slot(struct state *s, struct con_box *cbx, const struct con *cons, int con_cnt);
api void con_box_end(struct state *s, struct con_box *cbx);
api void con_box_layout(struct box *b, struct memory_arena *arena);

/* ---------------------------------------------------------------------------
 *                                  TRANSFORM
 * --------------------------------------------------------------------------- */
/* clip box */
api uiid clip_box_begin(struct state *s);
api void clip_box_end(struct state *s);

/* scroll region */
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
api struct scroll_region scroll_region_begin(struct state *s);
api void scroll_region_end(struct state *s);
api void scroll_region_layout(struct box *b);
api void scroll_region_input(struct context *ctx, struct box *b, const union event *evt);
api struct scroll_region scroll_region_ref(struct box *b);

/* scroll box */
api uiid scroll_box_begin(struct state *s);
api void scroll_box_end(struct state *s);
api void scroll_box_blueprint(struct box *b);
api void scroll_box_layout(struct box *b);
api void scroll_box_input(struct context *ctx, struct box *b, union event *evt);

/* zoom box */
struct zoom_box {
    uiid id;
    float *scale_x;
    float *scale_y;
};
api struct zoom_box zoom_box_begin(struct state *s);
api void zoom_box_end(struct state *s);
api void zoom_box_input(struct box *b, union event *evt);
api struct zoom_box zoom_box_ref(struct box *b);

/* ---------------------------------------------------------------------------
 *                                  CONTAINER
 * --------------------------------------------------------------------------- */
/* combo box */
struct combo_box {
    struct combo combo;
    struct flex_box fbx;
    struct state *ps;
    uiid *popup;
    uiid *selid;
    uiid *lblid;
};
api int combo_box(struct state *s, unsigned id, const char **items, int cnt);
api struct combo_box combo_box_begin(struct state *s, mid id);
api int combo_box_item(struct state *s, struct combo_box *cbx, const char *item, const char *end);
api void combo_box_end(struct state *s, struct combo_box *cbx);
api void combo_box_popup_input(struct context *ctx, struct box *b, union event *evt);

/* panel */
struct panel {
    uiid id;
    struct sborder sbx;
    struct flex_box fbx;
};
api struct panel panel_begin(struct state *s);
api void panel_header(struct state *s, struct panel *pan, const char *title);
api uiid panel_header_begin(struct state *s, struct panel *pan);
api void panel_header_end(struct state *s, struct panel *pan);
api uiid panel_toolbar_begin(struct state *s, struct panel *pan);
api void panel_toolbar_end(struct state *s, struct panel *pan);
api void panel_content_begin(struct state *s, struct panel *pan);
api void panel_content_end(struct state *s, struct panel *pan);
api void panel_status(struct state *s, struct panel *pan, const char *status);
api uiid panel_status_begin(struct state *s, struct panel *pan);
api void panel_status_end(struct state *s, struct panel *pan);

/* panel box */
api struct panel panel_box_begin(struct state *s, const char *title);
api void panel_box_end(struct state *s, struct panel *pan, const char *status);

/* window */
struct window {
    uiid id;
    int *x;
    int *y;
    int *w;
    int *h;
    int *border;
};
api struct window window_begin(struct state *s, int x, int y, int w, int h);
api void window_end(struct state *s);
api void window_blueprint(struct box *b);
api void window_layout(struct box *b);
api void window_input(struct context *ctx, struct box *b, const union event *evt);
api struct window window_ref(struct box *b);

/* sidebar */
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
api struct sidebar sidebar_begin(struct state *s);
api void sidebar_end(struct state *s, struct sidebar *sb);
api void sidebar_layout(struct box *b);
api void sidebar_input(struct box *b, union event *evt);
api void sidebar_bar_input(struct box *b, union event *evt);
api void sidebar_scaler_input(struct box *b, union event *evt);
api struct sidebar sidebar_ref(struct box *b);

/* ---------------------------------------------------------------------------
 *                                  COMPOSIT
 * --------------------------------------------------------------------------- */
/* label button */
struct button_label {
    struct button btn;
    struct label lbl;
    struct sbox sbx;
};
api int button_label_clicked(struct state *s, const char *label, const char *end);
api struct button_label button_label(struct state *s, const char *txt, const char *end);
api int button_label_poll(struct state *s, struct button_state *st, struct button_label *btn);
api int button_label_query(struct context *ctx, struct button_state *st, mid mid, struct button_label *btn);

/* icon button */
struct button_icon {
    struct button btn;
    struct icon ico;
    struct sbox sbx;
};
api struct button_icon button_icon(struct state *s, int sym);
api int button_icon_clicked(struct state *s, int sym);

/* toogle */
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
api struct toggle checkbox(struct state *s, int *checked, const char *txt, const char *end);
api struct toggle toggle(struct state *s, int *active, const char *txt, const char *end);
api struct toggle radio(struct state *s, int *sel, const char *txt, const char *end);
api void toggle_input(struct context *ctx, struct box *b, union event *evt);
api struct toggle toggle_ref(struct box *b);

#endif