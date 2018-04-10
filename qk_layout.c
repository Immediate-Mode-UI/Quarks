#include "qk.h"
#include "widget.h"

/* ---------------------------------------------------------------------------
 *                                  SBORDER
 * --------------------------------------------------------------------------- */
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
    widget_param_int(s, (int)type);
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
api struct overlap_box
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
    widget_state_int(s, 0);
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
api void
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
api void
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
api void
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
