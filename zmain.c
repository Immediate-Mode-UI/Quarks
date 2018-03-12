#include <assert.h> /* assert */
#include <stdlib.h> /* calloc, free */
#include <string.h> /* memcpy, memset */
#include <inttypes.h> /* PRIu64 */
#include <limits.h> /* INT_MAX */
#include <stdio.h> /* fprintf, fputc */

#include "qk.h"

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
    WIDGET_CON_BOX_SLOT,
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
    WIDGET_WINDOW,
    /* Transformation */
    WIDGET_CLIP_BOX,
    WIDGET_SCROLL_REGION,
    WIDGET_SCROLL_BOX,
    WIDGET_ZOOM_BOX,
    WIDGET_MOVABLE_BOX,
    WIDGET_SCALABLE_BOX,
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
 *                                  LABEL
 * --------------------------------------------------------------------------- */
typedef int(*text_measure_f)(void *usr, int font, int fh, const char *txt, int len);
struct label {
    uiid id;
    const char *txt;
    int *font;
    int *height;
};
static struct label
label_ref(struct box *b)
{
    struct label lbl;
    lbl.height = widget_get_int(b, 0);
    lbl.font = widget_get_int(b, 1);
    lbl.txt = widget_get_str(b, 2);
    return lbl;
}
static struct label
label(struct state *s, const char *txt, const char *end)
{
    int len = 0;
    struct label lbl = {0};
    const struct config *cfg = s->cfg;
    widget_begin(s, WIDGET_LABEL);
    lbl.id = widget_box_push(s);
    widget_box_pop(s);

    len = (end) ? cast(int, (end-txt)): (int)strn(txt);
    lbl.height = widget_param_int(s, cfg->font_default_height);
    lbl.font = widget_param_int(s, cfg->font_default_id);
    lbl.txt = widget_param_str(s, txt, len);
    widget_end(s);
    return lbl;
}
static void
label_blueprint(struct box *b, void *usr, text_measure_f measure)
{
    struct label lbl = label_ref(b);
    int len = cast(int, strn(lbl.txt));
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
static struct icon
icon_ref(struct box *b)
{
    struct icon ico = {0};
    ico.sym = widget_get_int(b, 0);
    ico.size = widget_get_int(b, 1);
    return ico;
}
static struct icon
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
static void
icon_blueprint(struct box *b)
{
    struct icon ico = icon_ref(b);
    b->dw = *ico.size;
    b->dh = *ico.size;
}
static void
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
static int
button_poll(struct state *s, struct button_state *st, struct button *btn)
{
    struct box *b = polls(s, btn->id);
    if (!b) {
        zero(st, szof(*st));
        return 0;
    } button_pull(st, b);
    return 1;
}
static int
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
static struct button
button_begin(struct state *s)
{
    struct button btn = {0};
    widget_begin(s, WIDGET_BUTTON);
    btn.style =widget_param_int(s, 0);
    btn.id = widget_box_push(s);
    return btn;
}
static void
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
static struct slider
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
static int
slider_poll(struct state *s, struct slider_state *st, struct slider *sld)
{
    struct box *b = polls(s, sld->id);
    if (!b) return 0;
    *sld = slider_ref(b);
    slider_pull(st, b);
    return 1;
}
static int
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
static struct slider
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
static void
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
static void
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
static void
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
static struct scroll
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
static struct scroll
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
static void
scroll_end(struct state *s)
{
    widget_box_pop(s);
    widget_box_pop(s);
    widget_end(s);
}
static struct scroll
scroll(struct state *s, float *off_x, float *off_y)
{
    struct scroll scrl;
    scrl = scroll_begin(s, off_x, off_y);
    scroll_end(s);
    return scrl;
}
static void
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
static void
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
static struct combo
combo_ref(struct box *b)
{
    struct combo c = {0};
    c.state = widget_get_int(b, 0);
    c.popup = widget_get_mid(b, 1);
    return c;
}
static struct combo
combo_begin(struct state *s, mid id)
{
    struct combo c = {0};
    widget_begin(s, WIDGET_COMBO);
    c.id = widget_box_push(s);
    c.state = widget_state_int(s, COMBO_COLLAPSED);
    c.popup = widget_param_mid(s, id);
    return c;
}
static struct state*
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
static void
combo_popup_end(struct state *s, struct combo *c)
{
    widget_box_pop(s);
    widget_end(s);
    if (*c->state == COMBO_COLLAPSED)
        widget_box_property_set(s, BOX_HIDDEN);
    popup_end(s);
}
static void
combo_end(struct state *s)
{
    widget_box_pop(s);
    widget_end(s);
}
static void
combo_popup_show(struct context *ctx, struct combo *c)
{
    *c->state = COMBO_VISIBLE;
    popup_show(ctx, *c->popup, VISIBLE);
}
static void
combo_popup_close(struct context *ctx, struct combo *c)
{
    *c->state = COMBO_COLLAPSED;
    popup_show(ctx, *c->popup, HIDDEN);
}
static void
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
static void
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
static struct sborder
sborder_ref(struct box *b)
{
    struct sborder sbx;
    sbx.x = widget_get_int(b, 0);
    sbx.y = widget_get_int(b, 1);
    sbx.content = *widget_get_id(b, 2);
    sbx.id = b->id;
    return sbx;
}
static struct sborder
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
static void
sborder_end(struct state *s)
{
    widget_box_pop(s);
    widget_box_pop(s);
    widget_end(s);
}
static void
sborder_blueprint(struct box *b)
{
    struct sborder sbx = sborder_ref(b);
    if (b->id != sbx.content)
        box_blueprint(b, *sbx.x, *sbx.y);
    else box_blueprint(b, 0, 0);
}
static void
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
static struct salign
salign_ref(struct box *b)
{
    struct salign aln;
    aln.vertical = widget_get_int(b, 0);
    aln.horizontal = widget_get_int(b, 1);
    aln.content = *widget_get_id(b, 2);
    aln.id = b->id;
    return aln;
}
static struct salign
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
static void
salign_end(struct state *s)
{
    widget_box_pop(s);
    widget_box_pop(s);
    widget_end(s);
}
static void
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
            n->w = min(n->w, n->dw);
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
static struct sbox
sbox_begin(struct state *s)
{
    struct sbox sbx = {0};
    widget_begin(s, WIDGET_SBOX);
    sbx.id = widget_box_push(s);
    sbx.border = sborder_begin(s);
    sbx.align = salign_begin(s);
    return sbx;
}
static void
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
static struct flex_box
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
static struct flex_box
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
static void
flex_box_slot_dyn(struct state *s, struct flex_box *fbx)
{
    assert(*fbx->flow != FLEX_BOX_WRAP);
    flex_box_slot(s, fbx, FLEX_BOX_SLOT_DYNAMIC, 0);
}
static void
flex_box_slot_static(struct state *s,
    struct flex_box *fbx, int pixel_width)
{
    flex_box_slot(s, fbx, FLEX_BOX_SLOT_STATIC, pixel_width);
}
static void
flex_box_slot_variable(struct state *s,
    struct flex_box *fbx, int min_pixel_width)
{
    assert(*fbx->flow != FLEX_BOX_WRAP);
    flex_box_slot(s, fbx, FLEX_BOX_SLOT_VARIABLE, min_pixel_width);
}
static void
flex_box_slot_fitting(struct state *s, struct flex_box *fbx)
{
    flex_box_slot(s, fbx, FLEX_BOX_SLOT_FITTING, 0);
}
static void
flex_box_end(struct state *s, struct flex_box *fbx)
{
    int idx = *fbx->cnt;
    if (idx) {
        widget_box_pop(s);
        widget_end(s);
    } widget_box_pop(s);
    widget_end(s);
}
static void
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
static void
flex_box_layout(struct box *b)
{
    struct list_hook *it;
    struct flex_box fbx = flex_box_ref(b);

    int space = 0, slot_size = 0;
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
    int slots;
    int *cnt;
};
struct overlap_box_slot {
    uiid *id;
    int *zorder;
};
static struct overlap_box
overlap_box_ref(struct box *b)
{
    struct overlap_box obx = {0};
    obx.cnt = widget_get_int(b, 0);
    obx.slots = 1;
    return obx;
}
intern struct overlap_box_slot
overlap_box_slot_ref(struct box *b)
{
    struct overlap_box_slot slot;
    slot.id = widget_get_id(b, 0);
    slot.zorder = widget_get_int(b, 1);
    return slot;
}
static struct overlap_box
overlap_box_begin(struct state *s)
{
    struct overlap_box obx = {0};
    widget_begin(s, WIDGET_OVERLAP_BOX);
    obx.id = widget_box_push(s);
    widget_box_property_set(s, BOX_UNSELECTABLE);
    obx.cnt = widget_param_int(s, 0);
    return obx;
}
static void
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
    widget_state_int(s, 0);
    *obx->cnt += 1;
}
static void
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
static void
overlap_box_layout(struct box *b, struct memory_arena *arena)
{
    int i = 0;
    int slot_cnt = 0;
    struct box **boxes = 0;
    struct list_hook *it = 0, *sit = 0;
    box_layout(b, 0);

    /* find maximum zorder */
    list_foreach(it, &b->lnks) {
        struct box *n = list_entry(it, struct box, node);
        struct overlap_box_slot s = overlap_box_slot_ref(n);
        slot_cnt = max(*s.zorder, slot_cnt);
    } slot_cnt += 1;

    /* allocate and setup temp sorting array */
    boxes = arena_push_array(arena, slot_cnt, struct box*);
    list_foreach_s(it, sit, &b->lnks) {
        struct box *n = list_entry(it, struct box, node);
        struct overlap_box_slot s = overlap_box_slot_ref(n);
        if (!*s.zorder) continue;
        list_del(&n->node);
        boxes[*s.zorder] = n;
    }

    /* add boxes back in sorted order */
    for (i = slot_cnt-1; i >= 0; --i) {
        if (!boxes[i]) continue;
        list_add_head(&b->lnks, &boxes[i]->node);
    } i = 1;

    /* set correct zorder for each child box */
    list_foreach(it, &b->lnks) {
        struct box *n = list_entry(it, struct box, node);
        struct overlap_box_slot s = overlap_box_slot_ref(n);
        *s.zorder = i++;
    }
}
static void
overlap_box_input(struct box *b, union event *evt, struct memory_arena *arena)
{
    int i = 0;
    struct box *slot = 0;
    struct list_hook *it = 0;
    if (evt->type != EVT_PRESSED)
        return;

    /* find clicked on slot */
    for (i = 0; i < evt->hdr.cnt; ++i) {
        struct box *s = evt->hdr.boxes[i];
        list_foreach(it, &b->lnks) {
            struct box *n = list_entry(it, struct box, node);
            if (n != s) continue;
            slot = n; break;
        }
        if (slot) break;
    } if (!slot) return;

    /* reorder and rebalance overlap stack */
    {int zorder = 0;
    struct overlap_box obx = overlap_box_ref(b);
    list_foreach(it, &b->lnks) {
        struct box *n = list_entry(it, struct box, node);
        struct overlap_box_slot s = overlap_box_slot_ref(n);
        if (*s.id != slot->id) continue;

        zorder = *s.zorder;
        *s.zorder = *obx.cnt+1;
        break;
    }
    /* set each slots zorder */
    list_foreach(it, &b->lnks) {
        struct box *n = list_entry(it, struct box, node);
        struct overlap_box_slot s = overlap_box_slot_ref(n);
        if (*s.zorder > zorder)
            *s.zorder -= 1;
    } overlap_box_layout(b, arena);}
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
struct con_box_slot {int con_cnt, cons;};
struct con_box {
    uiid id;
    int *cond_cnt;
    int *slot_cnt;
    int conds;
};
intern struct con_box
con_box_ref(struct box *b)
{
    struct con_box cbx = {0};
    cbx.cond_cnt = widget_get_int(b, 0);
    cbx.slot_cnt = widget_get_int(b, 1);
    cbx.conds = 2;
    return cbx;
}
intern struct con_box_slot
con_box_slot_ref(struct box *b)
{
    struct con_box_slot cbs = {0};
    cbs.con_cnt = *widget_get_int(b, 0);
    cbs.cons = 1;
    return cbs;
}
intern struct cond
cond_ref(struct box *b, int off, int idx)
{
    struct cond c = {0};
    off += idx * 7;
    c.op = (unsigned char)*widget_get_int(b, off + 0);
    c.a.slot = *widget_get_int(b, off + 1);
    c.a.attr = *widget_get_int(b, off + 2);
    c.b.slot = *widget_get_int(b, off + 3);
    c.b.attr = *widget_get_int(b, off + 4);
    c.cons.mul = *widget_get_float(b, off + 5);
    c.cons.off = *widget_get_int(b, off + 6);
    return c;
}
intern struct con
con_ref(struct box *b, int off, int idx)
{
    struct con c = {0};
    off += idx * 9;
    c.op = *widget_get_int(b, off + 0);
    c.dst.slot = *widget_get_int(b, off + 1);
    c.dst.attr = *widget_get_int(b, off + 2);
    c.src.slot = *widget_get_int(b, off + 3);
    c.src.attr = *widget_get_int(b, off + 4);
    c.cons.mul = *widget_get_float(b, off + 5);
    c.cons.off = *widget_get_int(b, off + 6);
    c.anchor = *widget_get_int(b, off + 7);
    c.cond = *widget_get_int(b, off + 8);
    return c;
}
static struct con_box
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
static void
con_box_slot(struct state *s, struct con_box *cbx,
    const struct con *cons, int con_cnt)
{
    int i = 0;
    if (*cbx->slot_cnt) {
        widget_box_pop(s);
        widget_end(s);
    }
    widget_begin(s, WIDGET_CON_BOX_SLOT);
    widget_box_push(s);
    widget_box_property_set(s, BOX_UNSELECTABLE);
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
static void
con_box_end(struct state *s, struct con_box *cbx)
{
    if (*cbx->slot_cnt) {
        widget_box_pop(s);
        widget_end(s);
    }
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
static void
con_box_layout(struct box *b, struct memory_arena *arena)
{
    int i = 0;
    struct list_hook *it = 0;
    struct con_box cbx = con_box_ref(b);
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
        struct cond c = cond_ref(b, cbx.conds, i);

        /* extract left operand */
        assert(c.a.slot < *cbx.cond_cnt);
        dst = boxes[c.a.slot + 1];
        av = box_attr_get(dst, c.a.attr);

        /* extract right operand */
        assert(c.b.slot < *cbx.cond_cnt);
        src = boxes[c.b.slot + 1];
        bv = box_attr_get(src, c.b.attr);
        bv = roundi(cast(float, bv) * c.cons.mul) + c.cons.off;

        /* eval operation */
        switch (c.op) {
        case COND_EQ: res[i+1] = (av == bv); break;
        case COND_NE: res[i+1] = (av != bv); break;
        case COND_GR: res[i+1] = (av >  bv); break;
        case COND_GE: res[i+1] = (av >= bv); break;
        case COND_LS: res[i+1] = (av <  bv); break;
        case COND_LE: res[i+1] = (av <= bv); break;}
    }
    /* evaluate constraints */
    list_foreach(it, &b->lnks) {
        struct box *sb = list_entry(it, struct box, node);
        struct con_box_slot slot = con_box_slot_ref(sb);
        for (i = 0; i < slot.con_cnt; ++i) {
            struct box *dst, *src;
            struct con c = con_ref(sb, slot.cons, i);
            assert((c.cond < *cbx.cond_cnt) || (!(*cbx.cond_cnt)));
            if (*cbx.cond_cnt && (c.cond >= *cbx.cond_cnt))
                continue;

            /* destination value */
            {int av = 0, bv = 0, v = 0;
            assert(c.dst.slot < *cbx.slot_cnt);
            dst = boxes[c.dst.slot + 1];
            av = box_attr_get(dst, c.dst.attr);

            /* source value */
            assert(c.src.slot < *cbx.slot_cnt);
            src = boxes[c.src.slot + 1];
            bv = box_attr_get(src, c.src.attr);
            v = roundi(cast(float, bv) * c.cons.mul) + c.cons.off;

            /* eval attribute */
            switch (c.op) {
            case CON_NOP: break;
            case CON_SET: box_attr_set(dst, c.dst.attr, c.anchor, v); break;
            case CON_MIN: box_attr_set(dst, c.dst.attr, c.anchor, min(av, v)); break;
            case CON_MAX: box_attr_set(dst, c.dst.attr, c.anchor, max(av, v)); break;}}
        }
    }

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
static uiid
clip_box_begin(struct state *s)
{
    assert(s);
    if (!s) return 0;
    widget_begin(s, WIDGET_CLIP_BOX);
    return widget_box_push(s);
}
static void
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
static struct scroll_region
scroll_region_ref(struct box *b)
{
    struct scroll_region sr = {0};
    sr.dir = widget_get_int(b, 0);
    sr.off_x = widget_get_float(b, 1);
    sr.off_y = widget_get_float(b, 2);
    sr.id = *widget_get_id(b, 3);
    return sr;
}
static struct scroll_region
scroll_region_begin(struct state *s)
{
    struct scroll_region sr = {0};
    assert(s);
    if (!s) return sr;

    widget_begin(s, WIDGET_SCROLL_REGION);
    sr.dir = widget_state_int(s, SCROLL_DEFAULT);
    sr.off_x = widget_state_float(s, 0);
    sr.off_y = widget_state_float(s, 0);
    sr.id = widget_box_push(s);
    widget_param_id(s, sr.id);
    return sr;
}
static void
scroll_region_end(struct state *s)
{
    assert(s);
    if (!s) return;
    clip_box_end(s);
}
static void
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
static void
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
static uiid
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
static void
scroll_box_end(struct state *s)
{
    assert(s);
    if (!s) return;
    scroll_region_end(s);
    widget_box_pop(s);
    widget_end(s);
}
static void
scroll_box_blueprint(struct box *b)
{
    static const int scroll_size = 10;
    struct list_hook *h = b->lnks.prev;
    struct box *bsr = list_entry(h, struct box, node);
    b->dw = max(b->dw, bsr->dw + scroll_size);
    b->dh = max(b->dh, bsr->dh + scroll_size);
}
static void
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
static void
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
 *                              ZOOM BOX
 * --------------------------------------------------------------------------- */
struct zoom_box {
    uiid id;
    float *scale_x;
    float *scale_y;
};
static struct zoom_box
zoom_box_ref(struct box *b)
{
    struct zoom_box sb;
    sb.scale_x = widget_get_float(b, 0);
    sb.scale_y = widget_get_float(b, 1);
    sb.id = *widget_get_id(b, 2);
    return sb;
}
static struct zoom_box
zoom_box_begin(struct state *s)
{
    struct zoom_box sb = {0};
    assert(s);
    if (!s) return sb;

    widget_begin(s, WIDGET_ZOOM_BOX);
    sb.scale_x = widget_state_float(s, 1);
    sb.scale_y = widget_state_float(s, 1);
    sb.id = widget_box_push(s);
    widget_param_id(s, sb.id);
    return sb;
}
static void
zoom_box_end(struct state *s)
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
static void
zoom_box_input(struct box *b, union event *evt)
{
    struct input *in = evt->hdr.input;
    struct zoom_box sb = zoom_box_ref(b);
    if (evt->type != EVT_SCROLLED) return;
    if (in->shortcuts[SHORTCUT_ZOOM_BOX_SCALE_X].down ||
        in->shortcuts[SHORTCUT_ZOOM_BOX_SCALE].down)
        *sb.scale_x = -evt->scroll.x * 0.1f;
    if (in->shortcuts[SHORTCUT_ZOOM_BOX_SCALE_Y].down ||
        in->shortcuts[SHORTCUT_ZOOM_BOX_SCALE].down)
        *sb.scale_y = -evt->scroll.y * 0.1f;
}
/* ---------------------------------------------------------------------------
 *                              BUTTON_ICON
 * --------------------------------------------------------------------------- */
struct button_icon {
    struct button btn;
    struct icon ico;
    struct sbox sbx;
};
static struct button_icon
button_icon(struct state *s, int sym)
{
    struct button_icon bti;
    bti.btn = button_begin(s);
        bti.sbx = sbox_begin(s);
        *bti.sbx.align.horizontal = SALIGN_CENTER;
        *bti.sbx.align.vertical = SALIGN_MIDDLE;
            bti.ico = icon(s, sym);
        sbox_end(s);
    button_end(s);
    return bti;
}
static int
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
    struct button btn;
    struct label lbl;
    struct sbox sbx;
};
intern void
button_label_pull(struct button_state *btn, const struct box *b)
{
    button_pull(btn, b);
}
static int
button_label_poll(struct state *s, struct button_state *st,
    struct button_label *btn)
{
    return button_poll(s, st, &btn->btn);
}
static int
button_label_query(struct context *ctx, struct button_state *st,
    mid mid, struct button_label *btn)
{
    return button_query(ctx, st, mid, &btn->btn);
}
static struct button_label
button_label(struct state *s, const char *txt, const char *end)
{
    struct button_label btn;
    assert(s);
    assert(txt);
    btn.btn = button_begin(s);
        btn.sbx = sbox_begin(s);
        *btn.sbx.align.horizontal = SALIGN_CENTER;
        *btn.sbx.align.vertical = SALIGN_MIDDLE;
            btn.lbl = label(s, txt, end);
        sbox_end(s);
    button_end(s);
    return btn;
}
static int
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
static struct toggle
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
static struct toggle
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
static struct toggle
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
static struct toggle
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
static void
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
static void
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
static struct combo_box
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
    cbx.selid = widget_state_id(cbx.ps, 0);
    cbx.lblid = widget_param_id(cbx.ps, 0);
    button_begin(s);

    /* setup popup layout */
    cbx.fbx = flex_box_begin(cbx.ps);
    *cbx.fbx.orientation = FLEX_BOX_VERTICAL;
    return cbx;
}
static int
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
        selected = 1;
    } return selected;
}
static void
combo_box_end(struct state *s, struct combo_box *cbx)
{
    /* finish combo */
    flex_box_end(cbx->ps, &cbx->fbx);
    widget_box_pop(cbx->ps);
    widget_end(cbx->ps);
    combo_popup_end(cbx->ps, &cbx->combo);
    button_end(s);
    combo_end(s);

    /* finish combo box */
    widget_box_pop(s);
    widget_end(s);
}
static int
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
static struct panel
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
static uiid
panel_header_begin(struct state *s, struct panel *pan)
{
    flex_box_slot_fitting(s, &pan->fbx);
    widget_begin(s, WIDGET_PANEL_HEADER);
    return widget_box_push(s);
}
static void
panel_header_end(struct state *s, struct panel *pan)
{
    widget_box_pop(s);
    widget_end(s);
}
static void
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
static uiid
panel_toolbar_begin(struct state *s, struct panel *pan)
{
    flex_box_slot_fitting(s, &pan->fbx);
    widget_begin(s, WIDGET_PANEL_TOOLBAR);
    return widget_box_push(s);
}
static void
panel_toolbar_end(struct state *s, struct panel *pan)
{
    widget_box_pop(s);
    widget_end(s);
}
static void
panel_content_begin(struct state *s, struct panel *pan)
{
    flex_box_slot_dyn(s, &pan->fbx);
}
static void
panel_content_end(struct state *s, struct panel *pan)
{

}
static uiid
panel_status_begin(struct state *s, struct panel *pan)
{
    flex_box_slot_fitting(s, &pan->fbx);
    widget_begin(s, WIDGET_PANEL_STATUSBAR);
    return widget_box_push(s);
}
static void
panel_status_end(struct state *s, struct panel *pan)
{
    widget_box_pop(s);
    widget_end(s);
}
static void
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
static void
panel_end(struct state *s, struct panel *pan)
{
    flex_box_end(s, &pan->fbx);
    widget_box_pop(s);
    widget_end(s);
}
/* ---------------------------------------------------------------------------
 *                                  PANEL BOX
 * --------------------------------------------------------------------------- */
static struct panel
panel_box_begin(struct state *s, const char *title)
{
    struct panel pan;
    pan = panel_begin(s);
    panel_header(s, &pan, title);
    panel_content_begin(s, &pan);
    scroll_box_begin(s);
    return pan;
}
static void
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
static struct sidebar
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
static struct sidebar
sidebar_begin(struct state *s)
{
    /* sidebar */
    struct sidebar sb = {0};
    sb.cbx = con_box_begin(s, 0, 0);
    {static const struct con sb_cons[] = {
        {CON_SET, {0,ATTR_W}, {SPARENT,ATTR_W}, {0.5f,0}},
        {CON_SET, {0,ATTR_H}, {SPARENT,ATTR_H}, {0.6f,0}},
        {CON_SET, {0,ATTR_L}, {SPARENT,ATTR_L}, {1,0}, ATTR_W},
        {CON_SET, {0,ATTR_CY}, {SPARENT,ATTR_CY}, {1,0}, ATTR_H}
    }; con_box_slot(s, &sb.cbx, sb_cons, cntof(sb_cons));}

    widget_begin(s, WIDGET_SIDEBAR);
    sb.id = widget_box_push(s);
    sb.state = widget_state_int(s, SIDEBAR_CLOSED);
    sb.ratio = widget_state_float(s, 0.5f);
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
static void
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
static void
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
    bb->w = 18, bb->h = b->h;
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
static void
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
static void
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
static void
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
/* ---------------------------------------------------------------------------
 *                              WINDOW
 * --------------------------------------------------------------------------- */
struct window {
    uiid id;
    int *x;
    int *y;
    int *w;
    int *h;
};
static struct window
window_ref(struct box *b)
{
    struct window win = {0};
    win.x = widget_get_int(b, 0);
    win.y = widget_get_int(b, 1);
    win.w = widget_get_int(b, 2);
    win.h = widget_get_int(b, 3);
    return win;
}
static struct window
window_begin(struct state *s, int x, int y, int w, int h)
{
    struct window win = {0};
    widget_begin(s, WIDGET_WINDOW);
    win.id = widget_box_push(s);
    win.x = widget_state_int(s, x);
    win.y = widget_state_int(s, y);
    win.w = widget_state_int(s, w);
    win.h = widget_state_int(s, h);
    return win;
}
static void
window_end(struct state *s)
{
    widget_box_pop(s);
    widget_end(s);
}
static void
window_blueprint(struct box *b)
{
    struct window win = window_ref(b);
    b->dw = *win.w;
    b->dh = *win.h;
}
static void
window_layout(struct box *b)
{
    struct window win = window_ref(b);
    b->x = *win.x;
    b->y = *win.y;
    b->w = *win.w;
    b->h = *win.h;
    box_layout(b, 0);
}
static void
window_input(struct context *ctx, struct box *b, const union event *evt)
{
    int i = 0;
    struct window win;
    if (evt->type != EVT_DRAGGED) return;
    if (evt->hdr.origin != b) {
        for (i = 0; i < evt->hdr.cnt; i++) {
            struct box *n = evt->hdr.boxes[i];
            switch (n->type) {
            default: break;
            case WIDGET_PANEL_HEADER: goto move;}
        } return;
    }
move:
    win = window_ref(b);
    b->moved = 1;
    *win.x += evt->dragged.x;
    *win.y += evt->dragged.y;
    ctx->unbalanced = 1;
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
static int
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
static void
nvgLabel(struct NVGcontext *vg, struct box *b)
{
    struct label lbl = label_ref(b);
    nvgFontFaceId(vg, *lbl.font);
    nvgFontSize(vg, *lbl.height);
    nvgTextAlign(vg, NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGBA(220,220,220,255));
    nvgText(vg, b->x, b->y, lbl.txt, NULL);
}
static void
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
static void
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
static void
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
static void
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
static void
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
static void
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
static void
nvgScaleRegion(struct NVGcontext *vg, struct box *b, float *x, float *y)
{
    struct zoom_box sbx = zoom_box_ref(b);
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
static void
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
static void
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
static void
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
static void
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
    case WIDGET_WINDOW: window_blueprint(b); break;
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
    case WIDGET_WINDOW: window_layout(b); break;
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
        case WIDGET_OVERLAP_BOX: overlap_box_input(b, evt, p->hdr.arena); break;
        case WIDGET_ZOOM_BOX: zoom_box_input(b, evt); break;
        case WIDGET_SIDEBAR: sidebar_input(b, evt); break;
        case WIDGET_SIDEBAR_BAR: sidebar_bar_input(b, evt); break;
        case WIDGET_SIDEBAR_SCALER: sidebar_scaler_input(b, evt); break;
        case WIDGET_WINDOW: window_input(ctx, b, evt); break;
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
    case WIDGET_ZOOM_BOX: nvgScaleRegion(vg, b, scale_x, scale_y); break;
    case WIDGET_PANEL: nvgPanel(vg, b); break;
    case WIDGET_PANEL_HEADER: nvgPanelHeader(vg, b); break;
    case WIDGET_PANEL_STATUSBAR: nvgPanelHeader(vg, b); break;
    case WIDGET_SIDEBAR_BAR: nvgSidebarBar(vg, b); break;
    case WIDGET_SIDEBAR_SCALER: nvgSidebarScaler(vg, b); break;
    default: break;}
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
    commit(ctx);
    generate(fp, ctx, "ui");
    cleanup(ctx);
}
static void
ui_load_serialized_tables(struct context *ctx)
{
    static const struct element g_1555159930_elements[] = {
        {1048584, 0lu, 0lu, 0lu, 0, 7, 0, 0, 0},
        {21, 6679361039399649282lu, 0lu, 6679361039399649281lu, 1, 8, 0, 0, 0},
        {11, 6679361039399649284lu, 6679361039399649282lu, 6679361039399649283lu, 2, 9, 0, 0, 0},
        {12, 6679361039399649286lu, 6679361039399649284lu, 6679361039399649285lu, 3, 10, 0, 6, 0},
        {22, 6679361039399649288lu, 6679361039399649286lu, 6679361039399649287lu, 4, 11, 0, 8, 0},
        {10, 6679361039399649290lu, 6679361039399649288lu, 6679361039399649289lu, 5, 12, 0, 8, 0},
        {8, 6679361039399649292lu, 6679361039399649290lu, 6679361039399649291lu, 6, 13, 0, 8, 0},
        {8, 6679361039399649293lu, 6679361039399649292lu, 6679361039399649291lu, 7, 14, 0, 8, 0},
        {9, 6679361039399649295lu, 6679361039399649293lu, 6679361039399649294lu, 8, 15, 0, 11, 0},
        {9, 6679361039399649296lu, 6679361039399649295lu, 6679361039399649294lu, 9, 16, 0, 11, 0},
        {1, 6679361039399649298lu, 6679361039399649296lu, 6679361039399649297lu, 10, 17, 0, 14, 0},
        {12, 6679361039399649300lu, 6679361039399649284lu, 6679361039399649299lu, 3, 10, 0, 17, 0},
        {32, 6679361039399649302lu, 6679361039399649300lu, 6679361039399649301lu, 4, 11, 0, 19, 0},
        {7, 6679361039399649304lu, 6679361039399649302lu, 6679361039399649303lu, 5, 12, 0, 19, 0},
        {7, 6679361039399649305lu, 6679361039399649304lu, 6679361039399649303lu, 6, 13, 0|BOX_MOVABLE_X|BOX_MOVABLE_Y, 19, 0},
        {7, 6679361039399649307lu, 6679361039399649302lu, 6679361039399649306lu, 5, 12, 0, 26, 0},
        {7, 6679361039399649308lu, 6679361039399649307lu, 6679361039399649306lu, 6, 13, 0|BOX_MOVABLE_X|BOX_MOVABLE_Y, 26, 0},
        {31, 6679361039399649310lu, 6679361039399649302lu, 6679361039399649309lu, 5, 12, 0, 33, 0},
        {11, 6679361039399649312lu, 6679361039399649310lu, 6679361039399649311lu, 6, 13, 0, 38, 0},
        {12, 6679361039399649314lu, 6679361039399649312lu, 6679361039399649313lu, 7, 14, 0, 44, 0},
        {2, 998704728lu, 6679361039399649314lu, 6679361039399649315lu, 8, 15, 0, 46, 0},
        {10, 6679361039399649317lu, 998704728lu, 6679361039399649316lu, 9, 16, 0, 47, 0},
        {8, 6679361039399649319lu, 6679361039399649317lu, 6679361039399649318lu, 10, 17, 0, 47, 0},
        {8, 6679361039399649320lu, 6679361039399649319lu, 6679361039399649318lu, 11, 18, 0, 47, 0},
        {9, 6679361039399649322lu, 6679361039399649320lu, 6679361039399649321lu, 12, 19, 0, 50, 0},
        {9, 6679361039399649323lu, 6679361039399649322lu, 6679361039399649321lu, 13, 20, 0, 50, 0},
        {1, 6679361039399649325lu, 6679361039399649323lu, 6679361039399649324lu, 14, 21, 0, 53, 0},
        {12, 6679361039399649327lu, 6679361039399649312lu, 6679361039399649326lu, 7, 14, 0, 56, 0},
        {11, 6679361039399649329lu, 6679361039399649327lu, 6679361039399649328lu, 8, 15, 0, 58, 0},
        {12, 6679361039399649331lu, 6679361039399649329lu, 6679361039399649330lu, 9, 16, 0, 64, 0},
        {2, 3567268847lu, 6679361039399649331lu, 6679361039399649332lu, 10, 17, 0, 66, 0},
        {10, 6679361039399649334lu, 3567268847lu, 6679361039399649333lu, 11, 18, 0, 67, 0},
        {8, 6679361039399649336lu, 6679361039399649334lu, 6679361039399649335lu, 12, 19, 0, 67, 0},
        {8, 6679361039399649337lu, 6679361039399649336lu, 6679361039399649335lu, 13, 20, 0, 67, 0},
        {9, 6679361039399649339lu, 6679361039399649337lu, 6679361039399649338lu, 14, 21, 0, 70, 0},
        {9, 6679361039399649340lu, 6679361039399649339lu, 6679361039399649338lu, 15, 22, 0, 70, 0},
        {0, 6679361039399649342lu, 6679361039399649340lu, 6679361039399649341lu, 16, 23, 0, 73, 0},
        {12, 6679361039399649344lu, 6679361039399649329lu, 6679361039399649343lu, 9, 16, 0, 75, 0},
        {2, 3282852853lu, 6679361039399649344lu, 6679361039399649345lu, 10, 17, 0, 77, 0},
        {10, 6679361039399649347lu, 3282852853lu, 6679361039399649346lu, 11, 18, 0, 78, 0},
        {8, 6679361039399649349lu, 6679361039399649347lu, 6679361039399649348lu, 12, 19, 0, 78, 0},
        {8, 6679361039399649350lu, 6679361039399649349lu, 6679361039399649348lu, 13, 20, 0, 78, 0},
        {9, 6679361039399649352lu, 6679361039399649350lu, 6679361039399649351lu, 14, 21, 0, 81, 0},
        {9, 6679361039399649353lu, 6679361039399649352lu, 6679361039399649351lu, 15, 22, 0, 81, 0},
        {0, 6679361039399649355lu, 6679361039399649353lu, 6679361039399649354lu, 16, 23, 0, 84, 0},
        {12, 6679361039399649357lu, 6679361039399649329lu, 6679361039399649356lu, 9, 16, 0, 86, 0},
        {2, 3042075085lu, 6679361039399649357lu, 6679361039399649358lu, 10, 17, 0, 88, 0},
        {10, 6679361039399649360lu, 3042075085lu, 6679361039399649359lu, 11, 18, 0, 89, 0},
        {8, 6679361039399649362lu, 6679361039399649360lu, 6679361039399649361lu, 12, 19, 0, 89, 0},
        {8, 6679361039399649363lu, 6679361039399649362lu, 6679361039399649361lu, 13, 20, 0, 89, 0},
        {9, 6679361039399649365lu, 6679361039399649363lu, 6679361039399649364lu, 14, 21, 0, 92, 0},
        {9, 6679361039399649366lu, 6679361039399649365lu, 6679361039399649364lu, 15, 22, 0, 92, 0},
        {0, 6679361039399649368lu, 6679361039399649366lu, 6679361039399649367lu, 16, 23, 0, 95, 0},
        {12, 6679361039399649370lu, 6679361039399649329lu, 6679361039399649369lu, 9, 16, 0, 97, 0},
        {2, 79436257lu, 6679361039399649370lu, 6679361039399649371lu, 10, 17, 0, 99, 0},
        {10, 6679361039399649373lu, 79436257lu, 6679361039399649372lu, 11, 18, 0, 100, 0},
        {8, 6679361039399649375lu, 6679361039399649373lu, 6679361039399649374lu, 12, 19, 0, 100, 0},
        {8, 6679361039399649376lu, 6679361039399649375lu, 6679361039399649374lu, 13, 20, 0, 100, 0},
        {9, 6679361039399649378lu, 6679361039399649376lu, 6679361039399649377lu, 14, 21, 0, 103, 0},
        {9, 6679361039399649379lu, 6679361039399649378lu, 6679361039399649377lu, 15, 22, 0, 103, 0},
        {0, 6679361039399649381lu, 6679361039399649379lu, 6679361039399649380lu, 16, 23, 0, 106, 0},
        {31, 6679361039399649382lu, 6679361039399649310lu, 6679361039399649309lu, 6, 13, 0|BOX_IMMUTABLE, 33, 0},
    };
    static const uiid g_1555159930_tbl_keys[128] = {
        0lu,0lu,6679361039399649282lu,0lu,6679361039399649284lu,0lu,6679361039399649286lu,0lu,
        6679361039399649288lu,0lu,6679361039399649290lu,0lu,6679361039399649292lu,6679361039399649293lu,0lu,6679361039399649295lu,
        6679361039399649296lu,0lu,6679361039399649298lu,0lu,6679361039399649300lu,0lu,6679361039399649302lu,0lu,
        6679361039399649304lu,6679361039399649305lu,0lu,6679361039399649307lu,6679361039399649308lu,0lu,6679361039399649310lu,0lu,
        6679361039399649312lu,0lu,6679361039399649314lu,0lu,0lu,6679361039399649317lu,0lu,6679361039399649319lu,
        6679361039399649320lu,0lu,6679361039399649322lu,6679361039399649323lu,0lu,6679361039399649325lu,0lu,6679361039399649327lu,
        0lu,6679361039399649329lu,0lu,6679361039399649331lu,0lu,0lu,6679361039399649334lu,0lu,
        6679361039399649336lu,6679361039399649337lu,0lu,6679361039399649339lu,6679361039399649340lu,0lu,6679361039399649342lu,0lu,
        6679361039399649344lu,0lu,0lu,6679361039399649347lu,0lu,6679361039399649349lu,6679361039399649350lu,0lu,
        6679361039399649352lu,6679361039399649353lu,0lu,6679361039399649355lu,0lu,6679361039399649357lu,3042075085lu,0lu,
        6679361039399649360lu,0lu,6679361039399649362lu,6679361039399649363lu,0lu,6679361039399649365lu,6679361039399649366lu,0lu,
        998704728lu,6679361039399649368lu,6679361039399649370lu,0lu,0lu,6679361039399649373lu,0lu,6679361039399649375lu,
        6679361039399649376lu,79436257lu,6679361039399649378lu,6679361039399649379lu,0lu,6679361039399649381lu,6679361039399649382lu,0lu,
        0lu,0lu,0lu,0lu,0lu,0lu,0lu,3567268847lu,
        0lu,0lu,0lu,0lu,0lu,3282852853lu,0lu,0lu,
        0lu,0lu,0lu,0lu,0lu,0lu,0lu,0lu
    };
    static const int g_1555159930_tbl_vals[128] = {
        0,0,1,0,2,0,3,0,4,0,5,0,6,7,0,8,
        9,0,10,0,11,0,12,0,13,14,0,15,16,0,17,0,
        18,0,19,0,0,21,0,22,23,0,24,25,0,26,0,27,
        0,28,0,29,0,0,31,0,32,33,0,34,35,0,36,0,
        37,0,0,39,0,40,41,0,42,43,0,44,0,45,46,0,
        47,0,48,49,0,50,51,0,20,52,53,0,0,55,0,56,
        57,54,58,59,0,60,61,0,0,0,0,0,0,0,0,30,
        0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0
    };
    static union param g_1555159930_params[108] = {
        {6679361039399649284lu},{1lu},{0lu},{4lu},{0lu},{2lu},{3lu},{0lu},
        {4lu},{5lu},{6679361039399649293lu},{1lu},{1lu},{6679361039399649296lu},{16lu},{0lu},
        {0lu},{0lu},{0lu},{1065353216lu},{1065353216lu},{1065353216lu},{1065353216lu},{0lu},
        {0lu},{6679361039399649305lu},{1065353216lu},{1065353216lu},{1065353216lu},{1065353216lu},{0lu},{0lu},
        {6679361039399649308lu},{0lu},{0lu},{0lu},{6679361039399649310lu},{6679361039399649382lu},{6679361039399649312lu},{1lu},
        {0lu},{6lu},{6lu},{2lu},{3lu},{0lu},{0lu},{6lu},
        {6lu},{6679361039399649320lu},{1lu},{1lu},{6679361039399649323lu},{16lu},{0lu},{25lu},
        {3lu},{0lu},{6679361039399649329lu},{0lu},{0lu},{4lu},{0lu},{4lu},
        {0lu},{0lu},{0lu},{6lu},{6lu},{6679361039399649337lu},{1lu},{1lu},
        {6679361039399649340lu},{0lu},{16lu},{0lu},{0lu},{0lu},{6lu},{6lu},
        {6679361039399649350lu},{1lu},{1lu},{6679361039399649353lu},{1lu},{16lu},{0lu},{0lu},
        {0lu},{6lu},{6lu},{6679361039399649363lu},{1lu},{1lu},{6679361039399649366lu},{2lu},
        {16lu},{0lu},{0lu},{0lu},{6lu},{6lu},{6679361039399649376lu},{1lu},
        {1lu},{6679361039399649379lu},{3lu},{16lu}
    };
    static const unsigned char g_1555159930_data[31] = {
        0x53,0x65,0x72,0x69,0x61,0x6c,0x69,0x7a,0x65,0x64,0x3a,0x20,0x43,0x6f,0x6d,0x70,
        0x69,0x6c,0x65,0x20,0x54,0x69,0x6d,0x65,0x00,0x4c,0x61,0x62,0x65,0x6c,0x00
    };
    static struct box *g_1555159930_bfs[63];
    static struct box g_1555159930_boxes[62];
    static struct module g_1555159930_module;
    static struct repository g_1555159930_repo;
    static const struct component g_1555159930_component = {
        1,1555159930, 17, 7,g_1555159930_elements, cntof(g_1555159930_elements),
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
ui_sidebar(struct state *s)
{
    /* Sidebar */
    {struct sidebar sb = sidebar_begin(s);
        static const char *folder[] = {"Documents", "Download", "Desktop",
            "Images", "boot", "dev", "lib", "include", "tmp", "usr", "root"};

        int i = 0;
        sborder_begin(s);
        scroll_region_begin(s); {
            struct flex_box fbx = flex_box_begin(s);
            *fbx.flow = FLEX_BOX_WRAP, *fbx.padding = 0;
            for (i = 0; i < cntof(folder); ++i) {
                flex_box_slot_static(s, &fbx, 60);
                if (icon_label(s, ICON_FOLDER, folder[i]))
                    printf("Button: %s clicked\n", folder[i]);
            } flex_box_end(s, &fbx);
        } scroll_region_end(s);
        sborder_end(s);
    sidebar_end(s, &sb);}
}
static void
ui_immedate_mode(struct state *s)
{
    /* Immediate mode panel */
    struct panel pan = panel_box_begin(s, "Immediate"); {
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
                        fprintf(stdout, "Button: Chart pressed\n");
                flex_box_slot_dyn(s, &gbx);
                    if (button_icon_clicked(s, ICON_DESKTOP))
                        fprintf(stdout, "Button: Desktop pressed\n");
                flex_box_slot_dyn(s, &gbx);
                    if (button_icon_clicked(s, ICON_DOWNLOAD))
                        fprintf(stdout, "Button: Download pressed\n");
                flex_box_end(s, &gbx);
            }
            /* combo */
            flex_box_slot_fitting(s, &fbx); {
                static const char *items[] = {"Pistol","Shotgun","Plasma","BFG"};
                combo_box(s, id("weapons"), items, cntof(items));
            }
            /* slider */
            {static float sld_val = 5.0f;
            flex_box_slot_static(s, &fbx, 30);
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
}
static void
ui_main(struct context *ctx)
{
    struct state *s = 0;
    if ((s = begin(ctx, id("Main")))) {
        ui_sidebar(s);
        /* Panels */
        {struct overlap_box obx = overlap_box_begin(s);
        overlap_box_slot(s, &obx, id("Immdiate Mode")); {
            window_begin(s, 600, 50, 180, 250);
            ui_immedate_mode(s);
            window_end(s);
        }
        overlap_box_slot(s, &obx, id("Retained Mode")); {
            window_begin(s, 320, 50, 200, 130);
            link(s, id("retained"), RELATIONSHIP_INDEPENDENT);
            window_end(s);
        }
        overlap_box_slot(s, &obx, id("Compile Time")); {
            window_begin(s, 320, 250, 200, 130);
            link(s, id("serialized_tables"), RELATIONSHIP_INDEPENDENT);
            window_end(s);
        }
        overlap_box_end(s, &obx);}
    } end(s);
}
static void
ui_update(struct context *ctx, struct NVGcontext *vg, struct ui_retained *ui)
{
    /* Process: Input */
    commit(ctx);
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
                        case WIDGET_BUTTON: {
                            if (b->id == id("SERIALIZED_BUTTON"))
                                fprintf(stdout, "serial button clicked\n");
                            else if (b->id == id("SERIALIZED_BUTTON_CONFIG"))
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
        button_label_query(ctx, &btn, ui->id, &ui->btn);
        if (btn.clicked) fprintf(stdout, "Retained button clicked\n");
        if (btn.pressed) fprintf(stdout, "Retained button pressed\n");
        if (btn.released) fprintf(stdout, "Retained button released\n");
        if (btn.entered) fprintf(stdout, "Retained button entered\n");
        if (btn.exited) fprintf(stdout, "Retained button exited\n");}

        /* slider */
        {struct slider_state sld = {0};
        slider_query(ctx, ui->id, &sld, &ui->sld);
        if (sld.value_changed)
            fprintf(stdout, "Retained slider value changed: %.2f\n", *ui->sld.value);
        if (sld.entered) fprintf(stdout, "Retained slider entered\n");
        if (sld.exited) fprintf(stdout, "Retained slider exited\n");}
    }
}
int main(int argc, char *argv[])
{
    enum fonts {FONT_HL, FONT_ICONS, FONT_CNT};
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
        ui_main(ctx);
        ui_update(ctx, vg, &ui);

        /* Paint */
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
        glClearColor(0.10f, 0.18f, 0.24f, 1);
        nvgBeginFrame(vg, w, h, 1.0f);
        ui_paint(vg, ctx, w, h);
        clear(ctx);
        nvgEndFrame(vg);

        /* Finish frame */
        SDL_GL_SwapWindow(win);
        SDL_Delay(16);
    }
    cleanup(ctx);

    /* Cleanup */
    nvgDeleteGL2(vg);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}