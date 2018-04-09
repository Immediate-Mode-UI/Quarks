#include "qk.h"
#include "widget.h"

/* ---------------------------------------------------------------------------
 *                                  LABEL
 * --------------------------------------------------------------------------- */
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

    len = (end) ? cast(int, (end-txt)): (int)strn(txt);
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
    int len = cast(int, strn(lbl.txt));
    b->dw = measure(usr, *lbl.font, *lbl.height, lbl.txt, len);
    b->dh = *lbl.height;
}

/* ---------------------------------------------------------------------------
 *                                  ICON
 * --------------------------------------------------------------------------- */
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
    struct box *b = polls(s, btn->id);
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
    struct box *b = polls(s, sld->id);
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
api struct combo
combo_begin(struct state *s, mid id)
{
    struct combo c = {0};
    widget_begin(s, WIDGET_COMBO);
    c.id = widget_box_push(s);
    c.state = widget_state_int(s, COMBO_COLLAPSED);
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
    sr.dir = widget_state_int(s, SCROLL_DEFAULT);
    sr.off_x = widget_state_float(s, 0);
    sr.off_y = widget_state_float(s, 0);
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
scroll_region_input(struct context *ctx, struct box *b, const union event *evt)
{
    struct input *in = evt->hdr.input;
    struct scroll_region sr = scroll_region_ref(b);
    if (evt->type == EVT_DRAGGED && in->shortcuts[SHORTCUT_SCROLL_REGION_SCROLL].down) {
        b->moved = 1;
        switch (*sr.dir) {
        case SCROLL_DEFAULT:
            *sr.off_x -= evt->moved.x;
            *sr.off_y -= evt->moved.y;
            ctx->unbalanced = 1;
            break;
        case SCROLL_REVERSE:
            *sr.off_x += evt->moved.x;
            *sr.off_y += evt->moved.y;
            ctx->unbalanced = 1;
            break;
        }
    } else if (in->shortcuts[SHORTCUT_SCROLL_REGION_RESET].down) {
        *sr.off_x = *sr.off_y = 0;
        ctx->unbalanced = 1;
    }
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
    struct list_hook *i = 0;
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
    scroll_region_layout(bsr);

    /* offset */
    if (bx->moved)
        *sr.off_x = *x.off_x;
    else *x.off_x = *sr.off_x;

    if (by->moved)
        *sr.off_y = *y.off_y;
    else *y.off_y = *sr.off_y;

    /* HACK(micha): handle wrapping flex box */
    list_foreach(i, &bsr->lnks) {
        struct box *n = list_entry(i, struct box, node);

        struct flex_box fbx;
        if (n->type != WIDGET_FLEX_BOX) continue;
        fbx = flex_box_ref(n);
        if (*fbx.flow != FLEX_BOX_WRAP) continue;

        {struct list_hook *j = 0;
        int max_x = b->x, max_y = b->y;
        flex_box_layout(n);
        list_foreach(j, &n->lnks) {
            const struct box *s = list_entry(j, struct box, node);
            max_x = max(max_x, s->x + (int)*sr.off_x + s->w);
            max_y = max(max_y, s->y + (int)*sr.off_y + s->h);
        }
        bsr->dw = max_x - b->x;
        bsr->dh = max_y - b->y;}
    }
    /* scrollbars */
    *x.size_x = bsr->w;
    *y.size_y = bsr->h;

    *x.total_x = bsr->dw;
    *y.total_y = bsr->dh;

    if (bsr->dw <= bsr->w)
        *sr.off_x = *x.off_x = 0;
    if (bsr->dh <= bsr->h)
        *sr.off_y = *y.off_y = 0;
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
        ctx->unbalanced = 1;
    } break;
    case EVT_SCROLLED: {
        /* mouse scrolling */
        *sr.off_y += -evt->scroll.y * floori((cast(float, *y.size_y) * 0.1f));
        *sr.off_y = clamp(0, *sr.off_y, *y.total_y - *y.size_y);
        ctx->unbalanced = 1;
    } break;
    case EVT_SHORTCUT: {
        /* shortcuts */
        if (!evt->key.pressed) break;
        if (evt->key.code == SHORTCUT_SCROLL_BOX_BEGIN)
            *sr.off_y = 0, ctx->unbalanced = 1;
        else if (evt->key.code == SHORTCUT_SCROLL_BOX_END)
            *sr.off_y = *y.total_y - *y.size_y, ctx->unbalanced = 1;
        else if (evt->key.code == SHORTCUT_SCROLL_BOX_PGDN) {
            *sr.off_y = min(*sr.off_y + *y.size_y, *y.total_y - *y.size_y);
            ctx->unbalanced = 1;
        } else if (evt->key.code == SHORTCUT_SCROLL_BOX_PGUP) {
            *sr.off_y = max(*sr.off_y - *y.size_y, 0);
            ctx->unbalanced = 1;
        }
    } break;}
}

/* ---------------------------------------------------------------------------
 *                              ZOOM BOX
 * --------------------------------------------------------------------------- */
api struct zoom_box
zoom_box_ref(struct box *b)
{
    struct zoom_box sb;
    sb.scale_x = widget_get_float(b, 0);
    sb.scale_y = widget_get_float(b, 1);
    sb.id = *widget_get_id(b, 2);
    return sb;
}
api struct zoom_box
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
api void
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
api void
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
 *                              BUTTON_LABEL
 * --------------------------------------------------------------------------- */
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
 *                              BUTTON_ICON
 * --------------------------------------------------------------------------- */
api struct button_icon
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
 *                                  TOGGLE
 * --------------------------------------------------------------------------- */
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
