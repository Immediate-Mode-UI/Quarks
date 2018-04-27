#include <stdlib.h>
#include <stdio.h>

#include "qk.h"
#include "qk.c"
#include "widget.h"
#include "widget.c"
#include "layout.c"
#include "container.c"

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
    nvgFillColor(vg, nvgRGBA(200,200,200,255));
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
    NVG_BUTTON_TRANSPARENT
};
static void
nvgButton(struct NVGcontext *vg, struct box *b)
{
    NVGcolor col;
    int style = *widget_get_int(b, 0);

    switch (style) {
    default:
    case NVG_BUTTON_DEFAULT: {
        if (b->hovered)
            col = nvgRGBA(40,40,40,255);
        else if (b->pressed)
            col = nvgRGBA(35,35,35,255);
        else col = nvgRGBA(55,55,55,255);

        nvgBeginPath(vg);
        nvgRect(vg, b->x,b->y, b->w,b->h);
        nvgFillColor(vg, col);
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRect(vg, b->x,b->y, b->w, b->h);
        nvgStrokeColor(vg, nvgRGBA(60,60,60,255));
        nvgStroke(vg);
    } break;
    case NVG_BUTTON_TRANSPARENT:
        col = nvgRGBA(0,0,0,0);
        if (b->hovered || b->pressed) {
            nvgBeginPath(vg);
            nvgRect(vg, b->x,b->y, b->w, b->h);
            nvgStrokeColor(vg, nvgRGBA(60,60,60,255));
            nvgStroke(vg);
        } break;
    }
}
static void
nvgSlider(struct NVGcontext *vg, struct box *b)
{
    NVGpaint bg;
    struct slider sld = slider_ref(b);
    struct box *p = b->parent;
    float cy = p->y+(int)(p->h*0.5f);
    if (sld.cid != b->id) return;

    /* Slot */
    bg = nvgBoxGradient(vg, p->x,cy-2+1, p->w,4, 2,2, nvgRGBA(10,10,10,32), nvgRGBA(10,10,10,128));
    nvgBeginPath(vg);
    nvgRoundedRect(vg, p->x, cy-2, p->w, 4, 2);
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    /* knob */
    nvgBeginPath(vg);
    nvgRect(vg, b->x, b->y, b->w, b->h);
    if (b->hovered)
        nvgFillColor(vg, nvgRGBA(50,50,50,255));
    else nvgFillColor(vg, nvgRGBA(60,60,60,255));
    nvgFill(vg);

    /* knob border */
    nvgBeginPath(vg);
    nvgRect(vg, b->x,b->y, b->w, b->h);
    nvgStrokeColor(vg, nvgRGBA(70,70,70,255));
    nvgStroke(vg);
}
static void
nvgScroll(struct NVGcontext *vg, struct box *b)
{
    NVGcolor col;
    struct box *p = b->parent;
    struct scroll scrl = scroll_ref(b);
    if (scrl.cid != b->id) return;
    if (b->h == p->h && b->w == p->w)
        return; /* skip if fully visible */

    /* draw scroll background  */
    nvgBeginPath(vg);
    nvgRect(vg, p->x,p->y, p->w,p->h);
    nvgFillColor(vg, nvgRGBA(40,40,40,255));
    nvgFill(vg);

    /* draw scroll cursor  */
    if (b->hovered)
        col = nvgRGBA(55,55,55,255);
    else if (b->pressed)
        col = nvgRGBA(50,50,50,255);
    else col = nvgRGBA(60,60,60,255);

    nvgBeginPath(vg);
    nvgRect(vg, b->x,b->y, b->w,b->h);
    nvgFillColor(vg, col);
    nvgFill(vg);
}
static void
nvgComboPopup(struct NVGcontext *vg, struct box *b)
{
    nvgBeginPath(vg);
    nvgRect(vg, b->x,b->y, b->w,b->h);
    nvgFillColor(vg, nvgRGBA(45,45,45,255));
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgRect(vg, b->x,b->y, b->w, b->h);
    nvgStrokeColor(vg, nvgRGBA(60,60,60,255));
    nvgStroke(vg);
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
    nvgBeginPath(vg);
    nvgRect(vg, b->x,b->y, b->w, b->h);
    nvgFillColor(vg, nvgRGBA(45,45,45,255));
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgRect(vg, b->x,b->y, b->w, b->h);
    nvgStrokeColor(vg, nvgRGBA(60,60,60,255));
    nvgStroke(vg);
}
static void
nvgPanelHeader(struct NVGcontext *vg, struct box *b)
{
    nvgBeginPath(vg);
    nvgRect(vg, b->x,b->y, b->w,b->h);
    nvgFillColor(vg, nvgRGBA(50,50,50,255));
    nvgFill(vg);
}
static void
nvgSidebarScaler(struct NVGcontext *vg, struct box *b)
{
    nvgBeginPath(vg);
    nvgMoveTo(vg, b->x, b->y);
    nvgLineTo(vg, b->x + b->w, b->y + 5);
    nvgLineTo(vg, b->x + b->w, b->y + b->h - 5);
    nvgLineTo(vg, b->x, b->y + b->h);
    nvgClosePath(vg);
    nvgFillColor(vg, nvgRGBA(35,35,35,255));
    nvgFill(vg);
    nvgStrokeColor(vg, nvgRGBA(60,60,60,255));
    nvgStroke(vg);
}
static void
nvgSidebarBar(struct NVGcontext *vg, struct box *b)
{
    nvgBeginPath(vg);
    nvgMoveTo(vg, b->x, b->y);
    nvgLineTo(vg, b->x + b->w, b->y + 10);
    nvgLineTo(vg, b->x + b->w, b->y + b->h - 10);
    nvgLineTo(vg, b->x, b->y + b->h);
    nvgClosePath(vg);
    nvgFillColor(vg, nvgRGBA(35,35,35,255));
    nvgFill(vg);
    nvgStrokeColor(vg, nvgRGBA(60,60,60,255));
    nvgStroke(vg);
}
/* ---------------------------------------------------------------------------
 *                                  PLATFORM
 * --------------------------------------------------------------------------- */
static void
ui_sdl_event(struct context *ctx, const SDL_Event *evt)
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
static void
ui_commit(struct context *ctx)
{
    union process *p = 0;
    while ((p = process_begin(ctx, PROCESS_COMMIT))) {
        assert(p->type == PROC_COMMIT);
        commit(p);
        process_end(p);
    }
}
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
        case WIDGET_SCROLL_REGION: scroll_region_input(ctx, b, evt); break;
        case WIDGET_SCROLL_BOX: scroll_box_input(ctx, b, evt); break;
        case WIDGET_OVERLAP_BOX: overlap_box_input(b, evt, p->hdr.arena); break;
        case WIDGET_ZOOM_BOX: zoom_box_input(b, evt); break;
        case WIDGET_SIDEBAR: sidebar_input(b, evt); break;
        case WIDGET_SIDEBAR_BAR: sidebar_bar_input(b, evt); break;
        case WIDGET_SIDEBAR_SCALER: sidebar_scaler_input(b, evt); break;
        case WIDGET_WINDOW: window_input(ctx, b, evt); break;
        case WIDGET_WINDOW_CONTENT: window_content_input(ctx, b, evt); break;
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
        case PROC_COMMIT: commit(p); break;
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
ui_run(struct context *ctx, struct NVGcontext *vg)
{
    int i; union process *p = 0;
    while ((p = process_begin(ctx, PROCESS_INPUT))) {
        switch (p->type) {default:break;
        case PROC_COMMIT: commit(p); break;
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
                ui_input(p, evt); /* handle widget events */
           }
        } break;}
        process_end(p);
    }
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
    s = begin(ctx, ui->id); {
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
    } end(s);
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
    /*trace(stdout, ctx);*/
    ui_commit(ctx);
    store_table(fp, ctx, "ui", 4);
    cleanup(ctx);
}
static void
ui_load_serialized_tables(struct context *ctx)
{
    static const struct element g_1555159930_elements[] = {
        {1048584, 0lu, 0lu, 0lu, 0, 7, 0, 0, 0},
        {196612, 6679361039399649282lu, 0lu, 6679361039399649281lu, 1, 8, 0, 0, 0},
        {131075, 6679361039399649284lu, 6679361039399649282lu, 6679361039399649283lu, 2, 9, 0, 0, 0},
        {131076, 6679361039399649286lu, 6679361039399649284lu, 6679361039399649285lu, 3, 10, 0, 6, 0},
        {196613, 6679361039399649288lu, 6679361039399649286lu, 6679361039399649287lu, 4, 11, 0, 8, 0},
        {131074, 6679361039399649290lu, 6679361039399649288lu, 6679361039399649289lu, 5, 12, 0, 8, 0},
        {131072, 6679361039399649292lu, 6679361039399649290lu, 6679361039399649291lu, 6, 13, 0, 8, 0},
        {131072, 6679361039399649293lu, 6679361039399649292lu, 6679361039399649291lu, 7, 14, 0, 8, 0},
        {131073, 6679361039399649295lu, 6679361039399649293lu, 6679361039399649294lu, 8, 15, 0, 11, 0},
        {131073, 6679361039399649296lu, 6679361039399649295lu, 6679361039399649294lu, 9, 16, 0, 11, 0},
        {65537, 6679361039399649298lu, 6679361039399649296lu, 6679361039399649297lu, 10, 17, 0, 14, 0},
        {131076, 6679361039399649300lu, 6679361039399649284lu, 6679361039399649299lu, 3, 10, 0, 17, 0},
        {262147, 6679361039399649302lu, 6679361039399649300lu, 6679361039399649301lu, 4, 11, 0, 19, 0},
        {65543, 6679361039399649304lu, 6679361039399649302lu, 6679361039399649303lu, 5, 12, 0, 19, 0},
        {65543, 6679361039399649305lu, 6679361039399649304lu, 6679361039399649303lu, 6, 13, 0|BOX_MOVABLE_X|BOX_MOVABLE_Y, 19, 0},
        {65543, 6679361039399649307lu, 6679361039399649302lu, 6679361039399649306lu, 5, 12, 0, 26, 0},
        {65543, 6679361039399649308lu, 6679361039399649307lu, 6679361039399649306lu, 6, 13, 0|BOX_MOVABLE_X|BOX_MOVABLE_Y, 26, 0},
        {262146, 6679361039399649310lu, 6679361039399649302lu, 6679361039399649309lu, 5, 12, 0, 33, 0},
        {131075, 6679361039399649312lu, 6679361039399649310lu, 6679361039399649311lu, 6, 13, 0, 38, 0},
        {131076, 6679361039399649314lu, 6679361039399649312lu, 6679361039399649313lu, 7, 14, 0, 44, 0},
        {65538, 998704728lu, 6679361039399649314lu, 6679361039399649315lu, 8, 15, 0, 46, 0},
        {131074, 6679361039399649317lu, 998704728lu, 6679361039399649316lu, 9, 16, 0, 47, 0},
        {131072, 6679361039399649319lu, 6679361039399649317lu, 6679361039399649318lu, 10, 17, 0, 47, 0},
        {131072, 6679361039399649320lu, 6679361039399649319lu, 6679361039399649318lu, 11, 18, 0, 47, 0},
        {131073, 6679361039399649322lu, 6679361039399649320lu, 6679361039399649321lu, 12, 19, 0, 50, 0},
        {131073, 6679361039399649323lu, 6679361039399649322lu, 6679361039399649321lu, 13, 20, 0, 50, 0},
        {65537, 6679361039399649325lu, 6679361039399649323lu, 6679361039399649324lu, 14, 21, 0, 53, 0},
        {131076, 6679361039399649327lu, 6679361039399649312lu, 6679361039399649326lu, 7, 14, 0, 56, 0},
        {131075, 6679361039399649329lu, 6679361039399649327lu, 6679361039399649328lu, 8, 15, 0, 58, 0},
        {131076, 6679361039399649331lu, 6679361039399649329lu, 6679361039399649330lu, 9, 16, 0, 64, 0},
        {65538, 3567268847lu, 6679361039399649331lu, 6679361039399649332lu, 10, 17, 0, 66, 0},
        {131074, 6679361039399649334lu, 3567268847lu, 6679361039399649333lu, 11, 18, 0, 67, 0},
        {131072, 6679361039399649336lu, 6679361039399649334lu, 6679361039399649335lu, 12, 19, 0, 67, 0},
        {131072, 6679361039399649337lu, 6679361039399649336lu, 6679361039399649335lu, 13, 20, 0, 67, 0},
        {131073, 6679361039399649339lu, 6679361039399649337lu, 6679361039399649338lu, 14, 21, 0, 70, 0},
        {131073, 6679361039399649340lu, 6679361039399649339lu, 6679361039399649338lu, 15, 22, 0, 70, 0},
        {65536, 6679361039399649342lu, 6679361039399649340lu, 6679361039399649341lu, 16, 23, 0, 73, 0},
        {131076, 6679361039399649344lu, 6679361039399649329lu, 6679361039399649343lu, 9, 16, 0, 75, 0},
        {65538, 3282852853lu, 6679361039399649344lu, 6679361039399649345lu, 10, 17, 0, 77, 0},
        {131074, 6679361039399649347lu, 3282852853lu, 6679361039399649346lu, 11, 18, 0, 78, 0},
        {131072, 6679361039399649349lu, 6679361039399649347lu, 6679361039399649348lu, 12, 19, 0, 78, 0},
        {131072, 6679361039399649350lu, 6679361039399649349lu, 6679361039399649348lu, 13, 20, 0, 78, 0},
        {131073, 6679361039399649352lu, 6679361039399649350lu, 6679361039399649351lu, 14, 21, 0, 81, 0},
        {131073, 6679361039399649353lu, 6679361039399649352lu, 6679361039399649351lu, 15, 22, 0, 81, 0},
        {65536, 6679361039399649355lu, 6679361039399649353lu, 6679361039399649354lu, 16, 23, 0, 84, 0},
        {131076, 6679361039399649357lu, 6679361039399649329lu, 6679361039399649356lu, 9, 16, 0, 86, 0},
        {65538, 3042075085lu, 6679361039399649357lu, 6679361039399649358lu, 10, 17, 0, 88, 0},
        {131074, 6679361039399649360lu, 3042075085lu, 6679361039399649359lu, 11, 18, 0, 89, 0},
        {131072, 6679361039399649362lu, 6679361039399649360lu, 6679361039399649361lu, 12, 19, 0, 89, 0},
        {131072, 6679361039399649363lu, 6679361039399649362lu, 6679361039399649361lu, 13, 20, 0, 89, 0},
        {131073, 6679361039399649365lu, 6679361039399649363lu, 6679361039399649364lu, 14, 21, 0, 92, 0},
        {131073, 6679361039399649366lu, 6679361039399649365lu, 6679361039399649364lu, 15, 22, 0, 92, 0},
        {65536, 6679361039399649368lu, 6679361039399649366lu, 6679361039399649367lu, 16, 23, 0, 95, 0},
        {131076, 6679361039399649370lu, 6679361039399649329lu, 6679361039399649369lu, 9, 16, 0, 97, 0},
        {65538, 79436257lu, 6679361039399649370lu, 6679361039399649371lu, 10, 17, 0, 99, 0},
        {131074, 6679361039399649373lu, 79436257lu, 6679361039399649372lu, 11, 18, 0, 100, 0},
        {131072, 6679361039399649375lu, 6679361039399649373lu, 6679361039399649374lu, 12, 19, 0, 100, 0},
        {131072, 6679361039399649376lu, 6679361039399649375lu, 6679361039399649374lu, 13, 20, 0, 100, 0},
        {131073, 6679361039399649378lu, 6679361039399649376lu, 6679361039399649377lu, 14, 21, 0, 103, 0},
        {131073, 6679361039399649379lu, 6679361039399649378lu, 6679361039399649377lu, 15, 22, 0, 103, 0},
        {65536, 6679361039399649381lu, 6679361039399649379lu, 6679361039399649380lu, 16, 23, 0, 106, 0},
        {262146, 6679361039399649382lu, 6679361039399649310lu, 6679361039399649309lu, 6, 13, 0|BOX_IMMUTABLE, 33, 0},
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
    static const char *folders[] = {
        "Documents", "Download",
        "Desktop", "Images",
        "boot", "dev", "lib",
        "include", "tmp",
        "usr", "root", "bin", "local",
        "src", "shaders", "srv", "sys",
        "var", "proc", "mnt"
    }; int i = 0;

    struct sidebar sb = sidebar_begin(s); {
        sborder_begin(s);
        scroll_box_begin(s); {
            struct flex_box fbx = flex_box_begin(s);
            *fbx.flow = FLEX_BOX_WRAP, *fbx.padding = 0;
            for (i = 0; i < cntof(folders); ++i) {
                flex_box_slot_static(s, &fbx, 60);
                if (icon_label(s, ICON_FOLDER, folders[i]))
                    printf("Button: %s clicked\n", folders[i]);
            } flex_box_end(s, &fbx);
        } scroll_box_end(s);
        sborder_end(s);
    } sidebar_end(s, &sb);
}
static void
ui_immediate_mode(struct state *s)
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
    cfg.font_default_height = 16;
    cfg.font_default_id = fnts[FONT_HL];
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
            ui_sdl_event(ctx, &evt);
        }}
        /* UI */
        {struct state *s = 0;
        if ((s = begin(ctx, id("Main")))) {
            ui_sidebar(s);
            /* Panels */
            {struct overlap_box obx = overlap_box_begin(s);
            overlap_box_slot(s, &obx, id("Immdiate Mode")); {
                window_begin(s, 600, 50, 180, 250);
                ui_immediate_mode(s);
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
            } overlap_box_end(s, &obx);}
        } end(s);}
        ui_run(ctx, vg);

        /* Paint */
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
        glClearColor(0.10f, 0.18f, 0.24f, 1);
        nvgBeginFrame(vg, w, h, 1.0f);
        ui_paint(vg, ctx, w, h);
        nvgEndFrame(vg);
        clear(ctx);

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
