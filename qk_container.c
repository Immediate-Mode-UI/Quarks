#include "qk.h"
#include "widget.h"

/* ---------------------------------------------------------------------------
 *                              COMBO BOX
 * --------------------------------------------------------------------------- */
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
    cbx.selid = widget_state_id(cbx.ps, 0);
    cbx.lblid = widget_param_id(cbx.ps, 0);
    button_begin(s);

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
api void
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
 *                              WINDOW
 * --------------------------------------------------------------------------- */
api struct window
window_ref(struct box *b)
{
    struct window win = {0};
    win.x = widget_get_int(b, 0);
    win.y = widget_get_int(b, 1);
    win.w = widget_get_int(b, 2);
    win.h = widget_get_int(b, 3);
    win.border = widget_get_int(b, 4);
    return win;
}
api struct window
window_begin(struct state *s, int x, int y, int w, int h)
{
    struct window win = {0};
    widget_begin(s, WIDGET_WINDOW);
    win.id = widget_box_push(s);
    win.x = widget_state_int(s, x);
    win.y = widget_state_int(s, y);
    win.w = widget_state_int(s, w);
    win.h = widget_state_int(s, h);
    win.border = widget_state_int(s, 3);
    widget_begin(s, WIDGET_WINDOW_CONTENT);
    widget_box_push(s);
    return win;
}
api void
window_end(struct state *s)
{
    widget_box_pop(s);
    widget_end(s);
    widget_box_pop(s);
    widget_end(s);
}
api void
window_blueprint(struct box *b)
{
    struct window win = window_ref(b);
    b->dw = *win.w;
    b->dh = *win.h;
}
api void
window_layout(struct box *b)
{
    struct window win = window_ref(b);
    b->x = *win.x;
    b->y = *win.y;
    b->w = *win.w;
    b->h = *win.h;
    box_layout(b, *win.border/2);
}
api void
window_input(struct context *ctx, struct box *b, const union event *evt)
{
    int cx, cy, dx, dy;
    struct window win;
    struct input *in = evt->hdr.input;
    if (evt->type != EVT_DRAGGED) return;
    if (evt->hdr.origin != b) return;

    win = window_ref(b);
    cx = *win.x + (*win.w >> 1);
    cy = *win.y + (*win.h >> 1);
    dx = in->mouse.x - cx;
    dy = in->mouse.y - cy;

    if (abs(dx) > abs(dy)) {
        if (dx < 0) {
            *win.x += evt->dragged.x;
            *win.w -= evt->dragged.x;
        } else *win.w += evt->dragged.x;
    } else {
        if (dy < 0) {
            *win.y += evt->dragged.y;
            *win.h -= evt->dragged.y;
        } else *win.h += evt->dragged.y;
    } ctx->unbalanced = 1;
}
api void
window_content_input(struct context *ctx, struct box *c, const union event *evt)
{
    int i = 0;
    struct window win;
    struct box *b = c->parent;
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
    *win.x += evt->dragged.x;
    *win.y += evt->dragged.y;
    ctx->unbalanced = 1;
    b->moved = 1;
}

/* ---------------------------------------------------------------------------
 *                                  SIDEBAR
 * --------------------------------------------------------------------------- */
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
