#include <math.h> /* sqrt, sin, cos, tan */
#include <assert.h> /* assert macro */
#include <string.h> /* memcpy */
#include <stdio.h> /* memcpy */
#include <stdarg.h> /* memcpy */
#include <stdlib.h>
#include <float.h>

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define clamp(a,v,b) (max(min(b,v),a))
#define sign(a) (((a) < 0) ? -1: 1)
#define cntof(a) ((int)(sizeof(a)/sizeof((a)[0])))

#define PI_CONSTANT (3.141592654f)
#define TO_RAD  ((PI_CONSTANT/180.0f))

/* ============================================================================
 *
 *                                  FLOAT MATH
 *
 * =========================================================================== */
typedef float m3[9];
static const float f3z[3];
static const m3 m3z;
static const m3 m3id = {1,0,0, 0,1,0, 0,0,1};

#define fop(r,e,a,p,b,i,s) (r) e ((a) p (b)) i (s)
#define f3op(r,e,a,p,b,i,s)\
    fop((r)[0],e,(a)[0],p,(b)[0],i,s),\
    fop((r)[1],e,(a)[1],p,(b)[1],i,s),\
    fop((r)[2],e,(a)[2],p,(b)[2],i,s)
#define f3set(v,x,y,z) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define f3zero(v) f3set(v,0,0,0)
#define f3unpack(v) (v)[0],(v)[1],(v)[2]
#define f3cpy(d,s) (d)[0]=(s)[0],(d)[1]=(s)[1],(d)[2]=(s)[2]
#define f3add(d,a,b) f3op(d,=,a,+,b,+,0)
#define f3sub(d,a,b) f3op(d,=,a,-,b,+,0)
#define f3mul(d,a,s) f3op(d,=,a,+,f3z,*,s)
#define f3dot(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define f3lerp(r,a,t,b) f3op(r,=,a,+,f3z,*,(1.0f - (t))); f3op(r,+=,b,+,f3z,*,t)
#define m3cpy(d,s) memcpy(d,s,sizeof(m3))

static inline void
f3cross(float *r, const float *a, const float *b)
{
    r[0] = (a[1]*b[2]) - (a[2]*b[1]);
    r[1] = (a[2]*b[0]) - (a[0]*b[2]);
    r[2] = (a[0]*b[1]) - (a[1]*b[0]);
}
static inline float
f3box(const float *a, const float *b, const float *c)
{
    float n[3];
    f3cross(n, a, b);
    return f3dot(n, c);
}
static inline void
f3norm(float *v)
{
    float il, l = f3dot(v, v);
    if (l == 0.0f) return;
    il = 1 / sqrtf(l);
    f3mul(v,v,il);
}
static void
m3mul(float *r, const float *a, const float *b)
{
    int i = 0;
    #define A(col, row) (a)[(col*3)+row]
    #define B(col, row) (b)[(col*3)+row]
    #define P(col, row) (r)[(col*3)+row]
    for (i = 0; i < 3; ++i) {
        const float ai0 = A(i,0), ai1 = A(i,1), ai2 = A(i,2);
        P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0);
        P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1);
        P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2);
    }
    #undef A
    #undef B
    #undef P
}
static void
m3rot(float *r, float angle, float X, float Y, float Z)
{
    #define M(col, row) m[(col*3)+row]
    float s = (float)sinf(angle);
    float c = (float)cosf(angle);
    float oc = 1.0f - c;
    float m[16];

    M(0,0) = oc * X * X + c;
    M(0,1) = oc * X * Y - Z * s;
    M(0,2) = oc * Z * X + Y * s;

    M(1,0) = oc * X * Y + Z * s;
    M(1,1) = oc * Y * Y + c;
    M(1,2) = oc * Y * Z - X * s;

    M(2,0) = oc * Z * X - Y * s;
    M(2,1) = oc * Y * Z + X * s;
    M(2,2) = oc * Z * Z + c;
    m3mul(r, r, m);
}
static void
m3mulv(float *r, const float *m, const float *v)
{
    #define X(a) (r)[0]
    #define Y(a) (r)[1]
    #define Z(a) (r)[2]
    #define M(col, row) m[(col*3)+row]
    float o[3]; f3cpy(o,v);

    X(r) = M(0,0)*X(o) + M(0,1)*Y(o) + M(0,2)*Z(o);
    Y(r) = M(1,0)*X(o) + M(1,1)*Y(o) + M(1,2)*Z(o);
    Z(r) = M(2,0)*X(o) + M(2,1)*Y(o) + M(2,2)*Z(o);

    #undef X
    #undef Y
    #undef Z
    #undef M
}
/* ============================================================================
 *
 *                                  GJK
 *
 * =========================================================================== */
struct gjk_support {
    int aid, bid;
    float a[3];
    float b[3];
};
struct gjk_vertex {
    float a[3];
    float b[3];
    float p[3];
    float u;
    int aid, bid;
};
struct gjk_simplex {
    int vcnt, scnt;
    int saveA[5], saveB[5];
    struct gjk_vertex v[5];
    int iter, hit;
    float D, div;
};
struct gjk_result {
    int hit;
    float p0[3];
    float p1[3];
    float distance_squared;
    int iterations;
};
static int
gjk(struct gjk_simplex *s, const struct gjk_support *sup, float dv[3])
{
    assert(s);
    assert(dv);
    assert(sup);
    if (!s || !sup || !dv)
        return 0;

    /* I.) Initialize */
    if (!s->vcnt) {
        s->scnt = 0;
        s->div = 1.0f;
        s->D = FLT_MAX;
    }
    /* II.) Check for duplications */
    for (int i = 0; i < s->scnt; ++i) {
        if (sup->aid != s->saveA[i]) continue;
        if (sup->bid != s->saveB[i]) continue;
        return 0;
    }
    /* III.) Add vertex into simplex */
    struct gjk_vertex *vert = &s->v[s->vcnt++];
    f3cpy(vert->a, sup->a);
    f3cpy(vert->b, sup->b);
    f3cpy(vert->p, dv);
    vert->aid = sup->aid;
    vert->bid = sup->bid;
    vert->u = 1.0f;

    /* IV.) Find closest simplex point */
    switch (s->vcnt) {
    case 1: break;
    case 2: {
        /* -------------------- Line ----------------------- */
        float a[3], b[3];
        f3cpy(a, s->v[0].p);
        f3cpy(b, s->v[1].p);

        /* compute barycentric coordinates */
        float ab[3], ba[3];
        f3sub(ab, a, b);
        f3sub(ba, b, a);

        float u = f3dot(b, ba);
        float v = f3dot(a, ab);
        if (v <= 0.0f) {
            /* region A */
            s->v[0].u = 1.0f;
            s->div = 1.0f;
            s->vcnt = 1;
            break;
        }
        if (u <= 0.0f) {
            /* region B */
            s->v[0] = s->v[1];
            s->v[0].u = 1.0f;
            s->div = 1.0f;
            s->vcnt = 1;
            break;
        }
        /* region AB */
        s->v[0].u = u;
        s->v[1].u = v;
        s->div = f3dot(ba, ba);
        s->vcnt = 2;
    } break;
    case 3: {
        /* -------------------- Triangle ----------------------- */
        float a[3], b[3], c[3];
        f3cpy(a, s->v[0].p);
        f3cpy(b, s->v[1].p);
        f3cpy(c, s->v[2].p);

        float ab[3], ba[3], bc[3], cb[3], ca[3], ac[3];
        f3sub(ab, a, b);
        f3sub(ba, b, a);
        f3sub(bc, b, c);
        f3sub(cb, c, b);
        f3sub(ca, c, a);
        f3sub(ac, a, c);

        /* compute barycentric coordinates */
        float u_ab = f3dot(b, ba);
        float v_ab = f3dot(a, ab);

        float u_bc = f3dot(c, cb);
        float v_bc = f3dot(b, bc);

        float u_ca = f3dot(a, ac);
        float v_ca = f3dot(c, ca);

        if (v_ab <= 0.0f && u_ca <= 0.0f) {
            /* region A */
            s->v[0].u = 1.0f;
            s->div = 1.0f;
            s->vcnt = 1;
            break;
        }
        if (u_ab <= 0.0f && v_bc <= 0.0f) {
            /* region B */
            s->v[0] = s->v[1];
            s->v[0].u = 1.0f;
            s->div = 1.0f;
            s->vcnt = 1;
            break;
        }
        if (u_bc <= 0.0f && v_ca <= 0.0f) {
            /* region C */
            s->v[0] = s->v[2];
            s->v[0].u = 1.0f;
            s->div = 1.0f;
            s->vcnt = 1;
            break;
        }
        /* calculate fractional area */
        float n[3], n1[3], n2[3], n3[3];
        f3cross(n, ba, ca);
        f3cross(n1, b, c);
        f3cross(n2, c, a);
        f3cross(n3, a, b);

        float u_abc = f3dot(n1, n);
        float v_abc = f3dot(n2, n);
        float w_abc = f3dot(n3, n);

        if (u_ab > 0.0f && v_ab > 0.0f && w_abc <= 0.0f) {
            /* region AB */
            s->v[0].u = u_ab;
            s->v[1].u = v_ab;
            s->div = u_ab + v_ab;
            s->vcnt = 2;
            break;
        }
        if (u_bc > 0.0f && v_bc > 0.0f && u_abc <= 0.0f) {
            /* region BC */
            s->v[0] = s->v[1];
            s->v[1] = s->v[2];
            s->v[0].u = u_bc;
            s->v[1].u = v_bc;
            s->div = u_bc + v_bc;
            s->vcnt = 2;
            break;
        }
        if (u_ca > 0.0f && v_ca > 0.0f && v_abc <= 0.0f) {
            /* region CA */
            s->v[1] = s->v[0];
            s->v[0] = s->v[2];
            s->v[0].u = u_ca;
            s->v[1].u = v_ca;
            s->div = u_ca + v_ca;
            s->vcnt = 2;
            break;
        }
        /* region ABC */
        assert(u_abc > 0.0f && v_abc > 0.0f && w_abc > 0.0f);
        s->v[0].u = u_abc;
        s->v[1].u = v_abc;
        s->v[2].u = w_abc;
        s->div = u_abc + v_abc + w_abc;
        s->vcnt = 3;
    } break;
    case 4: {
        /* -------------------- Tetrahedron ----------------------- */
        float a[3], b[3], c[3], d[3];
        f3cpy(a, s->v[0].p);
        f3cpy(b, s->v[1].p);
        f3cpy(c, s->v[2].p);
        f3cpy(d, s->v[3].p);

        float ab[3], ba[3], bc[3], cb[3], ca[3], ac[3];
        float db[3], bd[3], dc[3], cd[3], ad[3], da[3];

        f3sub(ab, a, b);
        f3sub(ba, b, a);
        f3sub(bc, b, c);
        f3sub(cb, c, b);
        f3sub(ca, c, a);
        f3sub(ac, a, c);

        f3sub(db, d, b);
        f3sub(bd, b, d);
        f3sub(dc, d, c);
        f3sub(cd, c, d);
        f3sub(da, d, a);
        f3sub(ad, a, d);

        /* compute barycentric coordinates */
        float u_ab = f3dot(b, ba);
        float v_ab = f3dot(a, ab);

        float u_bc = f3dot(c, cb);
        float v_bc = f3dot(b, bc);

        float u_ca = f3dot(a, ac);
        float v_ca = f3dot(c, ca);

        float u_bd = f3dot(d, db);
        float v_bd = f3dot(b, bd);

        float u_dc = f3dot(c, cd);
        float v_dc = f3dot(d, dc);

        float u_ad = f3dot(d, da);
        float v_ad = f3dot(a, ad);

        /* check verticies for closest point */
        if (v_ab <= 0.0f && u_ca <= 0.0f && v_ad <= 0.0f) {
            /* region A */
            s->v[0].u = 1.0f;
            s->div = 1.0f;
            s->vcnt = 1;
            break;
        }
        if (u_ab <= 0.0f && v_bc <= 0.0f && v_bd <= 0.0f) {
            /* region B */
            s->v[0] = s->v[1];
            s->v[0].u = 1.0f;
            s->div = 1.0f;
            s->vcnt = 1;
            break;
        }
        if (u_bc <= 0.0f && v_ca <= 0.0f && u_dc <= 0.0f) {
            /* region C */
            s->v[0] = s->v[2];
            s->v[0].u = 1.0f;
            s->div = 1.0f;
            s->vcnt = 1;
            break;
        }
        if (u_bd <= 0.0f && v_dc <= 0.0f && u_ad <= 0.0f) {
            /* region D */
            s->v[0] = s->v[3];
            s->v[0].u = 1.0f;
            s->div = 1.0f;
            s->vcnt = 1;
            break;
        }
        /* calculate fractional area */
        float n[3], n1[3], n2[3], n3[3];
        f3cross(n, da, ba);
        f3cross(n1, d, b);
        f3cross(n2, b, a);
        f3cross(n3, a, d);

        float u_adb = f3dot(n1, n);
        float v_adb = f3dot(n2, n);
        float w_adb = f3dot(n3, n);

        f3cross(n, ca, da);
        f3cross(n1, c, d);
        f3cross(n2, d, a);
        f3cross(n3, a, c);

        float u_acd = f3dot(n1, n);
        float v_acd = f3dot(n2, n);
        float w_acd = f3dot(n3, n);

        f3cross(n, bc, dc);
        f3cross(n1, b, d);
        f3cross(n2, d, c);
        f3cross(n3, c, b);

        float u_cbd = f3dot(n1, n);
        float v_cbd = f3dot(n2, n);
        float w_cbd = f3dot(n3, n);

        f3cross(n, ba, ca);
        f3cross(n1, b, c);
        f3cross(n2, c, a);
        f3cross(n3, a, b);

        float u_abc = f3dot(n1, n);
        float v_abc = f3dot(n2, n);
        float w_abc = f3dot(n3, n);

        /* check edges for closest point */
        if (w_abc <= 0.0f && v_adb <= 0.0f && u_ab > 0.0f && v_ab > 0.0f) {
            /* region AB */
            s->v[0].u = u_ab;
            s->v[1].u = v_ab;
            s->div = u_ab + v_ab;
            s->vcnt = 2;
            break;
        }
        if (u_abc <= 0.0f && w_cbd <= 0.0f && u_bc > 0.0f && v_bc > 0.0f) {
            /* region BC */
            s->v[0] = s->v[1];
            s->v[1] = s->v[2];
            s->v[0].u = u_bc;
            s->v[1].u = v_bc;
            s->div = u_bc + v_bc;
            s->vcnt = 2;
            break;
        }
        if (v_abc <= 0.0f && w_acd <= 0.0f && u_ca > 0.0f && v_ca > 0.0f) {
            /* region CA */
            s->v[1] = s->v[0];
            s->v[0] = s->v[2];
            s->v[0].u = u_ca;
            s->v[1].u = v_ca;
            s->div = u_ca + v_ca;
            s->vcnt = 2;
            break;
        }
        if (v_cbd <= 0.0f && u_acd <= 0.0f && u_dc > 0.0f && v_dc > 0.0f) {
            /* region DC */
            s->v[0] = s->v[3];
            s->v[1] = s->v[2];
            s->v[0].u = u_dc;
            s->v[1].u = v_dc;
            s->div = u_dc + v_dc;
            s->vcnt = 2;
            break;
        }
        if (v_acd <= 0.0f && w_adb <= 0.0f && u_ad > 0.0f && v_ad > 0.0f) {
            /* region AD */
            s->v[1] = s->v[3];
            s->v[0].u = u_ad;
            s->v[1].u = v_ad;
            s->div = u_ad + v_ad;
            s->vcnt = 2;
            break;
        }
        if (u_cbd <= 0.0f && u_adb <= 0.0f && u_bd > 0.0f && v_bd > 0.0f) {
            /* region BD */
            s->v[0] = s->v[1];
            s->v[1] = s->v[3];
            s->v[0].u = u_bd;
            s->v[1].u = v_bd;
            s->div = u_bd + v_bd;
            s->vcnt = 2;
            break;
        }
        /* calculate fractional volume */
        float volume = f3box(cb, ab, db);
        float u_abcd = f3box(c, d, b);
        float v_abcd = f3box(c, a, d);
        float w_abcd = f3box(d, a, b);
        float x_abcd = f3box(b, a, c);

        /* check faces for closest point */
        if (x_abcd <= 0.0f && u_abc > 0.0f && v_abc > 0.0f && w_abc > 0.0f) {
            /* region ABC */
            s->v[0].u = u_abc;
            s->v[1].u = v_abc;
            s->v[2].u = w_abc;
            s->div = u_abc + v_abc + w_abc;
            s->vcnt = 3;
            break;
        }
        if (u_abcd <= 0.0f && u_cbd > 0.0f && v_cbd > 0.0f && w_cbd > 0.0f) {
            /* region CBD */
            s->v[0] = s->v[2];
            s->v[2] = s->v[3];
            s->v[0].u = u_cbd;
            s->v[1].u = v_cbd;
            s->v[2].u = w_cbd;
            s->div = u_cbd + v_cbd + w_cbd;
            s->vcnt = 3;
            break;
        }
        if (v_abcd <= 0.0f && u_acd > 0.0f && v_acd > 0.0f && w_acd > 0.0f) {
            /* region ACD */
            s->v[1] = s->v[2];
            s->v[2] = s->v[3];
            s->v[0].u = u_acd;
            s->v[1].u = v_acd;
            s->v[2].u = w_acd;
            s->div = u_acd + v_acd + w_acd;
            s->vcnt = 3;
            break;
        }
        if (w_abcd <= 0.0f && u_adb > 0.0f && v_adb > 0.0f && w_adb > 0.0f) {
            /* region ADB */
            s->v[2] = s->v[1];
            s->v[1] = s->v[3];
            s->v[0].u = u_adb;
            s->v[1].u = v_adb;
            s->v[2].u = w_adb;
            s->div = u_adb + v_adb + w_adb;
            s->vcnt = 3;
            break;
        }
        /* region ABCD */
        assert(u_abcd > 0.0f && v_abcd > 0.0f && w_abcd > 0.0f && x_abcd > 0.0f);
        s->v[0].u = u_abcd;
        s->v[1].u = v_abcd;
        s->v[2].u = w_abcd;
        s->v[3].u = x_abcd;
        s->div = volume;
        s->vcnt = 4;
    } break;}

    /* V.) Check if origin is enclosed by tetrahedron */
    if (s->vcnt == 4) {
        s->hit = 1;
        return 0;
    }

    /* VI.) Ensure closing in on origin to prevent multi-step cycling */
    {float pnt[3];
    float denom = 1.0f / s->div;
    switch (s->vcnt) {
    case 1: f3cpy(pnt, s->v[0].p); break;
    case 2: {
        /* --------- Line -------- */
        float a[3], b[3];
        f3mul(a, s->v[0].p, denom * s->v[0].u);
        f3mul(b, s->v[1].p, denom * s->v[1].u);
        f3add(pnt, a, b);
    } break;
    case 3: {
        /* ------- Triangle ------ */
        float a[3], b[3], c[3];
        f3mul(a, s->v[0].p, denom * s->v[0].u);
        f3mul(b, s->v[1].p, denom * s->v[1].u);
        f3mul(c, s->v[2].p, denom * s->v[2].u);

        f3add(pnt, a, b);
        f3add(pnt, pnt, c);
    } break;
    case 4: {
        /* ----- Tetrahedron ----- */
        float a[3], b[3], c[3], d[3];
        f3mul(a, s->v[0].p, denom * s->v[0].u);
        f3mul(b, s->v[1].p, denom * s->v[1].u);
        f3mul(c, s->v[2].p, denom * s->v[2].u);
        f3mul(d, s->v[3].p, denom * s->v[3].u);

        f3add(pnt, a, b);
        f3add(pnt, pnt, c);
        f3add(pnt, pnt, d);
    } break;}

    float d2 = f3dot(pnt, pnt);
    if (d2 > s->D) return 0;
    s->D = d2;}

    /* VII.) New search direction */
    switch (s->vcnt) {
    default: assert(0); break;
    case 1: {
        /* --------- Point -------- */
        f3mul(dv, s->v[0].p, -1);
    } break;
    case 2: {
        /* ------ Line segment ---- */
        float b0[3], ba[3], t[3];
        f3sub(ba, s->v[1].p, s->v[0].p);
        f3mul(b0, s->v[1].p, -1);
        f3cross(t, ba, b0);
        f3cross(dv, t, ba);
    } break;
    case 3: {
        /* ------- Triangle ------- */
        float n[3], ab[3], ac[3];
        f3sub(ab, s->v[1].p, s->v[0].p);
        f3sub(ac, s->v[2].p, s->v[0].p);
        f3cross(n, ab, ac);
        if (f3dot(n, s->v[0].p) <= 0.0f)
            f3cpy(dv, n);
        else f3mul(dv, n, -1);
    }}
    if (f3dot(dv,dv) < FLT_EPSILON * FLT_EPSILON)
        return 0;

    /* VIII.) Save ids for next duplicate check */
    s->scnt = s->vcnt; s->iter++;
    for (int i = 0; i < s->scnt; ++i) {
        s->saveA[i] = s->v[i].aid;
        s->saveB[i] = s->v[i].bid;
    } return 1;
}
static struct gjk_result
gjk_analyze(const struct gjk_simplex *s)
{
    struct gjk_result r = {0};
    float denom = 1.0f / s->div;
    r.iterations = s->iter;
    r.hit = s->hit;

    switch (s->vcnt) {
    default: assert(0); break;
    case 1: {
        /* Point */
        f3cpy(r.p0, s->v[0].a);
        f3cpy(r.p1, s->v[0].b);
    } break;
    case 2: {
        /* Line */
        float as = denom * s->v[0].u;
        float bs = denom * s->v[1].u;

        float a[3], b[3], c[3], d[3];
        f3mul(a, s->v[0].a, as);
        f3mul(b, s->v[1].a, bs);
        f3mul(c, s->v[0].b, as);
        f3mul(d, s->v[1].b, bs);

        f3add(r.p0, a, b);
        f3add(r.p1, c, d);
    } break;
    case 3: {
        /* Triangle */
        float as = denom * s->v[0].u;
        float bs = denom * s->v[1].u;
        float cs = denom * s->v[2].u;

        float a[3], b[3], c[3];
        f3mul(a, s->v[0].a, as);
        f3mul(b, s->v[1].a, bs);
        f3mul(c, s->v[2].a, cs);

        float d[3], e[3], f[3];
        f3mul(d, s->v[0].b, as);
        f3mul(e, s->v[1].b, bs);
        f3mul(f, s->v[2].b, cs);

        f3add(r.p0, a, b);
        f3add(r.p0, r.p0, c);

        f3add(r.p1, d, e);
        f3add(r.p1, r.p1, f);
    } break;
    case 4: {
        /* Tetrahedron */
        float as = denom * s->v[0].u;
        float bs = denom * s->v[1].u;
        float cs = denom * s->v[2].u;
        float ds = denom * s->v[3].u;

        float a[3], b[3], c[3], d[3];
        f3mul(a, s->v[0].a, as);
        f3mul(b, s->v[1].a, bs);
        f3mul(c, s->v[2].a, cs);
        f3mul(d, s->v[3].a, ds);

        f3add(r.p0, a, b);
        f3add(r.p0, r.p0, c);
        f3add(r.p0, r.p0, d);
        f3cpy(r.p1, r.p0);
    } break;}

    if (!r.hit) {
        /* compute distance */
        float d[3]; f3sub(d, r.p1, r.p0);
        r.distance_squared = f3dot(d, d);
    } return r;
}
/* ============================================================================
 *
 *                                  COLLISION
 *
 * =========================================================================== */
struct manifold {
    float depth;
    float contact_point[3];
    float normal[3];
};
/* segment */
static float segment_closest_point_to_point_sqdist(const float a[3], const float b[3], const float p[3]);
static void segment_closest_point_to_point(float res[3], const float a[3], const float b[3], const float p[3]);
static float segment_closest_point_to_segment(float *t1, float *t2, float c1[3], float c2[3], const float p1[3], const float q1[3], const float p2[3], const float q2[3]);
/* plane */
static void planeq(float plane[4], const float n[3], const float pnt[3]);
static void planeqf(float plane[4], float nx, float ny, float nz, float px, float py, float pz);
/* ray */
static float ray_intersects_plane(const float ro[3], const float rd[3], const float p[4]);
static float ray_intersects_triangle(const float ro[3], const float rd[3], const float p1[3], const float p2[3], const float p3[3]);
static int ray_intersects_sphere(float *t0, float *t1, const float ro[3], const float rd[3], const float c[3], float r);
static int ray_intersects_aabb(float *t0, float *t1, const float ro[3], const float rd[3], const float min[3], const float max[3]);
/* sphere */
static void sphere_closest_point_to_point(float res[3], const float c[3], const float r, const float p[3]);
static int sphere_intersects_sphere(const float ca[3], float ra, const float cb[3], float rb);
static int sphere_intersects_sphere_manifold(struct manifold *m, const float ca[3], float ra, const float cb[3], float rb);
static int sphere_intersect_aabb(const float c[3], float r, const float min[3], const float max[3]);
static int sphere_intersect_aabb_manifold(struct manifold *m, const float c[3], float r, const float min[3], const float max[3]);
static int sphere_intersect_capsule(const float sc[3], float sr, const float ca[3], const float cb[3], float cr);
static int sphere_intersect_capsule_manifold(struct manifold *m, const float sc[3], float sr, const float ca[3], const float cb[3], float cr);
/* aabb */
static void aabb_rebalance_transform(float bmin[3], float bmax[3], const float amin[3], const float amax[3], const float m[3][3], const float t[3]);
static void aabb_closest_point_to_point(float res[3], const float min[3], const float max[3], const float p[3]);
static float aabb_sqdist_to_point(const float min[3], const float max[3], const float p[3]);
static int aabb_contains_point(const float amin[3], const float amax[3], const float p[3]);
static int aabb_intersects_aabb(const float amin[3], const float amax[3], const float bmin[3], const float bmax[3]);
static int aabb_intersects_aabb_manifold(struct manifold *m, const float amin[3], const float amax[3], const float bmin[3], const float bmax[3]);
static int aabb_intersect_sphere(const float min[3], const float max[3], const float c[3], float r);
static int aabb_intersect_sphere_manifold(struct manifold *m, const float min[3], const float max[3], const float c[3], float r);
static int aabb_intersect_capsule(const float min[3], const float max[3], const float ca[3], const float cb[3], float cr);
static int aabb_intersect_capsule_manifold(struct manifold *m, const float min[3], const float max[3], const float ca[3], const float cb[3], float cr);
/* capsule */
static float capsule_point_sqdist(const float ca[3], const float cb[3], float cr, const float p[3]);
static void capsule_closest_point_to_point(float res[3], const float ca[3], const float cb[3], float cr, const float p[3]);
static int capsule_intersect_capsule(const float aa[3], const float ab[3], float ar, const float ba[3], const float bb[3], float br);
static int capsule_intersect_capsule_manifold(struct manifold *m, const float aa[3], const float ab[3], float ar, const float ba[3], const float bb[3], float br);
static int capsule_intersect_sphere(const float ca[3], const float cb[3], float cr, const float sc[3], float sr);
static int capsule_intersect_sphere_manifold(struct manifold *m, const float ca[3], const float cb[3], float cr, const float sc[3], float sr);
static int capsule_intersect_aabb(const float ca[3], const float cb[3], float cr, const float min[3], const float max[3]);
static int capsule_intersect_aabb_manifold(struct manifold *m, const float ca[3], const float cb[3], float cr, const float min[3], const float max[3]);
/* polyhedron */
static int polyhedron_intersect_sphere(const float *verts, int cnt, const float c[3], float r);
static int polyhedron_intersect_polygon(const float *averts, int acnt, const float *bverts, int bcnt);
static int polyhedron_intersect_capsule(const float *verts, int cnt, const float ca[3], const float cb[3], float cr);

static void
planeq(float r[3], const float n[3], const float p[3])
{
    f3cpy(r,n);
    r[3] = -f3dot(n,p);
}
static void
planeqf(float r[3], float nx, float ny, float nz, float px, float py, float pz)
{
    /* Plane: ax + by + cz + d
     * Equation:
     *      n * (p - p0) = 0
     *      n * p - n * p0 = 0
     *      |a b c| * p - |a b c| * p0
     *
     *      |a b c| * p + d = 0
     *          d = -1 * |a b c| * p0
     *
     *  Plane: |a b c d| d = -|a b c| * p0
     */
    float n[3], p[3];
    f3set(n,nx,ny,nz);
    f3set(p,px,py,pz);
    planeq(r, n, p);
}
static void
segment_closest_point_to_point(float res[3],
    const float a[3], const float b[3], const float p[3])
{
    float ab[3], pa[3];
    f3sub(ab, b,a);
    f3sub(pa, p,a);
    float t = f3dot(pa,ab) / f3dot(ab,ab);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    f3mul(res, ab, t);
    f3add(res, a, res);
}
static float
segment_closest_point_to_point_sqdist(const float a[3], const float b[3], const float p[3])
{
    float ab[3], ap[3], bp[3];
    f3sub(ab,a,b);
    f3sub(ap,a,p);
    f3sub(bp,a,p);
    float e = f3dot(ap,ab);

    /* handle cases p proj outside ab */
    if (e <= 0.0f) return f3dot(ap,ap);
    float f = f3dot(ab,ab);
    if (e >= f) return f3dot(bp,bp);
    return f3dot(ap,ap) - (e*e)/f;
}
static float
segment_closest_point_to_segment(float *t1, float *t2, float c1[3], float c2[3],
    const float p1[3], const float q1[3], const float p2[3], const float q2[3])
{
    #define EPSILON (1e-6)
    float r[3], d1[3], d2[3];
    f3sub(d1, q1, p1); /* direction vector segment s1 */
    f3sub(d2, q2, p2); /* direction vector segment s2 */
    f3sub(r, p1, p2);

    float a = f3dot(d1, d1);
    float e = f3dot(d2, d2);
    float f = f3dot(d2, r);

    if (a <= EPSILON && e <= EPSILON) {
        /* both segments degenerate into points */
        float d12[3];
        *t1 = *t2 = 0.0f;
        f3cpy(c1, p1);
        f3cpy(c2, p2);
        f3sub(d12, c1, c2);
        return f3dot(d12,d12);
    }
    if (a > EPSILON) {
        float c = f3dot(d1,r);
        if (e > EPSILON) {
            /* non-degenerate case */
            float b = f3dot(d1,d2);
            float denom = a*e - b*b;

            /* compute closest point on L1/L2 if not parallel else pick any t2 */
            if (denom != 0.0f)
                *t1 = clamp(0.0f, (b*f - c*e) / denom, 1.0f);
            else *t1 = 0.0f;

            /* cmpute point on L2 closest to S1(s) */
            *t2 = (b*(*t1) + f) / e;
            if (*t2 < 0.0f) {
                *t2 = 0.0f;
                *t1 = clamp(0.0f, -c/a, 1.0f);
            } else if (*t2 > 1.0f) {
                *t2 = 1.0f;
                *t1 = clamp(0.0f, (b-c)/a, 1.0f);
            }
        } else {
            /* second segment degenerates into a point */
            *t1 = clamp(0.0f, -c/a, 1.0f);
            *t2 = 0.0f;
        }
    } else {
        /* first segment degenerates into a point */
        *t2 = clamp(0.0f, f / e, 1.0f);
        *t1 = 0.0f;
    }
    /* calculate closest points */
    float n[3], d12[3];
    f3mul(n, d1, *t1);
    f3add(c1, p1, n);
    f3mul(n, d2, *t2);
    f3add(c2, p2, n);

    /* calculate squared distance */
    f3sub(d12, c1, c2);
    return f3dot(d12,d12);
}
static float
ray_intersects_plane(const float ro[3], const float rd[3],
    const float plane[3])
{
    /* Ray: P = origin + rd * t
     * Plane: plane_normal * P + d = 0
     *
     * Substitute:
     *      normal * (origin + rd*t) + d = 0
     *
     * Solve for t:
     *      plane_normal * origin + plane_normal * rd*t + d = 0
     *      -(plane_normal*rd*t) = plane_normal * origin + d
     *
     *                  plane_normal * origin + d
     *      t = -1 * -------------------------
     *                  plane_normal * rd
     *
     * Result:
     *      Behind: t < 0
     *      Infront: t >= 0
     *      Parallel: t = 0
     *      Intersection point: ro + rd * t
     */
    float n = -(f3dot(plane,ro) + plane[3]);
    if (fabs(n) < 0.0001f) return 0.0f;
    return n/(f3dot(plane,rd));
}
static float
ray_intersects_triangle(const float ro[3], const float rd[3],
    const float p0[3], const float p1[3], const float p2[3])
{
    float p[4];
    float t = 0;
    float di0[3], di1[3], di2[3];
    float d21[3], d02[3], in[3];
    float n[3], d10[3], d20[3];
    float in0[3], in1[3], in2[3];

    /* calculate triangle normal */
    f3sub(d10, p1,p0);
    f3sub(d20, p2,p0);
    f3sub(d21, p2,p1);
    f3sub(d02, p0,p2);
    f3cross(n, d10,d20);

    /* check for plane intersection */
    planeq(p, n, p0);
    t = ray_intersects_plane(ro, rd, p);
    if (t <= 0.0f) return t;

    /* intersection point */
    f3mul(in,rd,t);
    f3add(in,in,ro);

    /* check if point inside triangle in plane */
    f3sub(di0, in, p0);
    f3sub(di1, in, p1);
    f3sub(di2, in, p2);

    f3cross(in0, d10, di0);
    f3cross(in1, d21, di1);
    f3cross(in2, d02, di2);

    if (f3dot(in0,n) < 0.0f)
        return -1;
    if (f3dot(in1,n) < 0.0f)
        return -1;
    if (f3dot(in2,n) < 0.0f)
        return -1;
    return t;
}
static int
ray_intersects_sphere(float *t0, float *t1,
    const float ro[3], const float rd[3],
    const float c[3], float r)
{
    float a[3];
    float tc,td,d2,r2;
    f3sub(a,c,ro);
    tc = f3dot(rd,a);
    if (tc < 0) return 0;

    r2 = r*r;
    d2 = f3dot(a,a) - tc*tc;
    if (d2 > r2) return 0;
    td = sqrtf(r2 - d2);

    *t0 = tc - td;
    *t1 = tc + td;
    return 1;
}
static int
ray_intersects_aabb(float *t0, float *t1,
    const float ro[3], const float rd[3],
    const float min[3], const float max[3])
{
    float t0x = (min[0] - ro[0]) / rd[0];
    float t0y = (min[1] - ro[1]) / rd[1];
    float t0z = (min[2] - ro[2]) / rd[2];
    float t1x = (max[0] - ro[0]) / rd[0];
    float t1y = (max[1] - ro[1]) / rd[1];
    float t1z = (max[2] - ro[2]) / rd[2];

    float tminx = min(t0x, t1x);
    float tminy = min(t0y, t1y);
    float tminz = min(t0z, t1z);
    float tmaxx = max(t0x, t1x);
    float tmaxy = max(t0y, t1y);
    float tmaxz = max(t0z, t1z);
    if (tminx > tmaxy || tminy > tmaxx)
        return 0;

    *t0 = max(tminx, tminy);
    *t1 = min(tmaxy, tmaxx);
    if (*t0 > tmaxz || tminz> *t1)
        return 0;

    *t0 = max(*t0, tminz);
    *t1 = min(*t1, tmaxz);
    return 1;
}
static void
sphere_closest_point_to_point(float res[3],
    const float c[3], const float r,
    const float p[3])
{
    float d[3], n[3];
    f3sub(d, p, c);
    f3norm(d);
    f3mul(res,n,r);
    f3add(res,c,res);
}
static int
sphere_intersects_sphere(const float ca[3], float ra,
    const float cb[3], float rb)
{
    float d[3];
    f3sub(d, cb, ca);
    float r = ra + rb;
    if (f3dot(d,d) > r*r)
        return 0;
    return 1;
}
static int
sphere_intersects_sphere_manifold(struct manifold *m,
    const float ca[3], float ra, const float cb[3], float rb)
{
    float d[3];
    f3sub(d, cb, ca);
    float r = ra + rb;
    float d2 = f3dot(d,d);
    if (d2 <= r*r) {
        float l = sqrtf(d2);
        float linv = 1.0f / ((l != 0) ? l: 1.0f);
        f3mul(m->normal, d, linv);
        m->depth = r - l;
        f3mul(d, m->normal, rb);
        f3sub(m->contact_point, cb, d);
        return 1;
    } return 0;
}
static int
sphere_intersect_aabb(const float c[3], float r,
    const float min[3], const float max[3])
{
    return aabb_intersect_sphere(min, max, c, r);
}
static int
sphere_intersect_aabb_manifold(struct manifold *m,
    const float c[3], float r, const float min[3], const float max[3])
{
    /* find closest aabb point to sphere center point */
    float ap[3], d[3];
    aabb_closest_point_to_point(ap, min, max, c);
    f3sub(d, c, ap);
    float d2 = f3dot(d, d);
    if (d2 > r*r) return 0;

    /* calculate distance vector between sphere and aabb center points */
    float ac[3];
    f3sub(ac, max, min);
    f3mul(ac, ac, 0.5f);
    f3add(ac, min, ac);
    f3sub(d, ac, c);

    /* normalize distance vector */
    float l2 = f3dot(d,d);
    float l = l2 != 0.0f ? sqrtf(l2): 1.0f;
    float linv = 1.0f/l;
    f3mul(d, d, linv);

    f3cpy(m->normal, d);
    f3mul(m->contact_point, m->normal, r);
    f3add(m->contact_point, c, m->contact_point);

    /* calculate penetration depth */
    float sp[3];
    sphere_closest_point_to_point(sp, c, r, ap);
    f3sub(d, sp, ap);
    m->depth = sqrtf(f3dot(d,d)) - l;
    return 1;
}
static int
sphere_intersect_capsule(const float sc[3], float sr,
    const float ca[3], const float cb[3], float cr)
{
    return capsule_intersect_sphere(ca, cb, cr, sc, sr);
}
static int
sphere_intersect_capsule_manifold(struct manifold *m,
    const float sc[3], float sr, const float ca[3], const float cb[3], float cr)
{
    /* find closest capsule point to sphere center point */
    float cp[3];
    capsule_closest_point_to_point(cp, ca, cb, cr, sc);
    f3sub(m->normal, cp, sc);
    float d2 = f3dot(m->normal, m->normal);
    if (d2 > sr*sr) return 0;

    /* normalize manifold normal vector */
    float l = d2 != 0.0f ? sqrtf(d2): 1;
    float linv = 1.0f/l;
    f3mul(m->normal, m->normal, linv);

    /* calculate penetration depth */
    f3mul(m->contact_point, m->normal, sr);
    f3add(m->contact_point, sc, m->contact_point);
    m->depth = d2 - sr*sr;
    m->depth = m->depth != 0.0f ? sqrtf(m->depth): 0.0f;
    return 1;
}
static void
aabb_rebalance_transform(float bmin[3], float bmax[3],
    const float amin[3], const float amax[3],
    const float m[3][3], const float t[3])
{
    for (int i = 0; i < 3; ++i) {
        bmin[i] = bmax[i] = t[i];
        for (int j = 0; j < 3; ++j) {
            float e = m[i][j] * amin[j];
            float f = m[i][j] * amax[j];
            if (e < f) {
                bmin[i] += e;
                bmax[i] += f;
            } else {
                bmin[i] += f;
                bmax[i] += e;
            }
        }
    }
}
static void
aabb_closest_point_to_point(float res[3],
    const float min[3], const float max[3],
    const float p[3])
{
    for (int i = 0; i < 3; ++i) {
        float v = p[i];
        if (v < min[i]) v = min[i];
        if (v > max[i]) v = max[i];
        res[i] = v;
    }
}
static float
aabb_sqdist_to_point(const float min[3], const float max[3], const float p[3])
{
    float r = 0;
    for (int i = 0; i < 3; ++i) {
        float v = p[i];
        if (v < min[i]) r += (min[i]-v) * (min[i]-v);
        if (v > max[i]) r += (v-max[i]) * (v-max[i]);
    } return r;
}
static int
aabb_contains_point(const float amin[3], const float amax[3], const float p[3])
{
    if (p[0] < amin[0] || p[0] > amax[0]) return 0;
    if (p[1] < amin[1] || p[1] > amax[1]) return 0;
    if (p[2] < amin[2] || p[2] > amax[2]) return 0;
    return 1;
}
static int
aabb_intersects_aabb(const float amin[3], const float amax[3],
    const float bmin[3], const float bmax[3])
{
    if (amax[0] < bmin[0] || amin[0] > bmax[0]) return 0;
    if (amax[1] < bmin[1] || amin[1] > bmax[1]) return 0;
    if (amax[2] < bmin[2] || amin[2] > bmax[2]) return 0;
    return 1;
}
static int
aabb_intersects_aabb_manifold(struct manifold *m,
    const float amin[3], const float amax[3],
    const float bmin[3], const float bmax[3])
{
    if (!aabb_intersects_aabb(amin, amax, bmin, bmax))
        return 0;

    /* calculate distance vector between both aabb center points */
    float ac[3], bc[3], d[3];
    f3sub(ac, amax, amin);
    f3sub(bc, bmax, bmin);

    f3mul(ac, ac, 0.5f);
    f3mul(bc, bc, 0.5f);

    f3add(ac, amin, ac);
    f3add(bc, bmin, bc);
    f3sub(d, bc, ac);

    /* normalize distance vector */
    float l2 = f3dot(d,d);
    float l = l2 != 0.0f ? sqrtf(l2): 1.0f;
    float linv = 1.0f/l;
    f3mul(d, d, linv);

    /* calculate contact point */
    f3cpy(m->normal, d);
    aabb_closest_point_to_point(m->contact_point, amin, amax, bc);
    f3sub(d, m->contact_point, ac);

    /* calculate penetration depth */
    float r2 = f3dot(d,d);
    float r = sqrtf(r2);
    m->depth = r - l;
    return 1;
}
static int
aabb_intersect_sphere(const float min[3], const float max[3],
    const float c[3], float r)
{
    /* compute squared distance between sphere center and aabb */
    float d2 = aabb_sqdist_to_point(min, max, c);
    /* intersection if distance is smaller/equal sphere radius*/
    return d2 <= r*r;
}
static int
aabb_intersect_sphere_manifold(struct manifold *m,
    const float min[3], const float max[3],
    const float c[3], float r)
{
    /* find closest aabb point to sphere center point */
    float d[3];
    aabb_closest_point_to_point(m->contact_point, min, max, c);
    f3sub(d, c, m->contact_point);
    float d2 = f3dot(d, d);
    if (d2 > r*r) return 0;

    /* calculate distance vector between aabb and sphere center points */
    float ac[3];
    f3sub(ac, max, min);
    f3mul(ac, ac, 0.5f);
    f3add(ac, min, ac);
    f3sub(d, c, ac);

    /* normalize distance vector */
    float l2 = f3dot(d,d) + 1e-15f;
    float l = l2 != 0.0f ? sqrtf(l2): 1.0f;
    float linv = 1.0f/l;
    f3mul(d, d, linv);

    /* calculate penetration depth */
    f3cpy(m->normal, d);
    f3sub(d, m->contact_point, ac);
    m->depth = sqrtf(f3dot(d,d));
    return 1;
}
static int
aabb_intersect_capsule(const float min[3], const float max[3],
    const float ca[3], const float cb[3], float cr)
{
    return capsule_intersect_aabb(ca, cb, cr, min, max);
}
static int
aabb_intersect_capsule_manifold(struct manifold *m,
    const float min[3], const float max[3],
    const float ca[3], const float cb[3], float cr)
{
    /* calculate aabb center point */
    float ac[3];
    f3sub(ac, max, min);
    f3mul(ac, ac, 0.5f);
    f3add(ac, min, ac);

    /* calculate closest point from aabb to point on capsule and check if inside aabb */
    float cp[3];
    capsule_closest_point_to_point(cp, ca, cb, cr, ac);
    if (!aabb_contains_point(min, max, cp))
        return 0;

    /* vector and distance between both capsule closests point and aabb center*/
    float d[3], d2;
    f3sub(d, cp, ac);
    d2 = f3dot(d,d);

    /* calculate penetration depth from closest aabb point to capsule */
    float ap[3], dt[3];
    aabb_closest_point_to_point(ap, min, max, cp);
    f3sub(dt, ap, cp);
    m->depth = sqrtf(f3dot(dt,dt));

    /* calculate normal */
    float l = sqrtf(d2);
    float linv = 1.0f / ((l != 0.0f) ? l: 1.0f);
    f3mul(m->normal, d, linv);
    f3cpy(m->contact_point, ap);
    return 1;
}
static float
capsule_point_sqdist(const float ca[3], const float cb[3], float cr, const float p[3])
{
    float d2 = segment_closest_point_to_point_sqdist(ca, cb, p);
    return d2 - (cr*cr);
}
static void
capsule_closest_point_to_point(float res[3],
    const float ca[3], const float cb[3], float cr, const float p[3])
{
    /* calculate closest point to internal capsule segment */
    float pp[3], d[3];
    segment_closest_point_to_point(pp, ca, cb, p);

    /* extend point out by radius in normal direction */
    f3sub(d,p,pp);
    f3norm(d);
    f3mul(res, d, cr);
    f3add(res, pp, res);
}
static int
capsule_intersect_capsule(const float aa[3], const float ab[3], float ar,
    const float ba[3], const float bb[3], float br)
{
    float t1, t2;
    float c1[3], c2[3];
    float d2 = segment_closest_point_to_segment(&t1, &t2, c1, c2, aa, ab, ba, bb);
    float r = ar + br;
    return d2 <= r*r;
}
static int
capsule_intersect_capsule_manifold(struct manifold *m,
    const float aa[3], const float ab[3], float ar,
    const float ba[3], const float bb[3], float br)
{
    float t1, t2;
    float c1[3], c2[3];
    float d2 = segment_closest_point_to_segment(&t1, &t2, c1, c2, aa, ab, ba, bb);
    float r = ar + br;
    if (d2 > r*r) return 0;

    /* calculate normal from both closest points for each segement */
    float cp[3], d[3];
    f3sub(m->normal, c2, c1);
    f3norm(m->normal);

    /* calculate contact point from closest point and depth */
    capsule_closest_point_to_point(m->contact_point, aa, ab, ar, c2);
    capsule_closest_point_to_point(cp, ba, bb, br, c1);
    f3sub(d, c1, cp);
    m->depth = sqrtf(f3dot(d,d));
    return 1;
}
static int
capsule_intersect_sphere(const float ca[3], const float cb[3], float cr,
    const float sc[3], float sr)
{
    /* squared distance bwetween sphere center and capsule line segment */
    float d2 = segment_closest_point_to_point_sqdist(ca,cb,sc);
    float r = sr + cr;
    return d2 <= r * r;
}
static int
capsule_intersect_sphere_manifold(struct manifold *m,
    const float ca[3], const float cb[3], float cr,
    const float sc[3], float sr)
{
    /* find closest capsule point to sphere center point */
    capsule_closest_point_to_point(m->contact_point, ca, cb, cr, sc);
    f3sub(m->normal, sc, m->contact_point);
    float d2 = f3dot(m->normal, m->normal);
    if (d2 > sr*sr) return 0;

    /* normalize manifold normal vector */
    float l = d2 != 0.0f ? sqrtf(d2): 1;
    float linv = 1.0f/l;
    f3mul(m->normal, m->normal, linv);

    /* calculate penetration depth */
    m->depth = d2 - sr*sr;
    m->depth = m->depth != 0.0f ? sqrtf(m->depth): 0.0f;
    return 1;
}
static int
capsule_intersect_aabb(const float ca[3], const float cb[3], float cr,
    const float min[3], const float max[3])
{
    /* calculate aabb center point */
    float ac[3];
    f3sub(ac, max, min);
    f3mul(ac, ac, 0.5f);

    /* calculate closest point from aabb to point on capsule and check if inside aabb */
    float p[3];
    capsule_closest_point_to_point(p, ca, cb, cr, ac);
    return aabb_contains_point(min, max, p);
}
static int
capsule_intersect_aabb_manifold(struct manifold *m,
    const float ca[3], const float cb[3], float cr,
    const float min[3], const float max[3])
{
    /* calculate aabb center point */
    float ac[3];
    f3sub(ac, max, min);
    f3mul(ac, ac, 0.5f);
    f3add(ac, min, ac);

    /* calculate closest point from aabb to point on capsule and check if inside aabb */
    float cp[3];
    capsule_closest_point_to_point(cp, ca, cb, cr, ac);
    if (!aabb_contains_point(min, max, cp))
        return 0;

    /* vector and distance between both capsule closests point and aabb center*/
    float d[3], d2;
    f3sub(d, ac, cp);
    d2 = f3dot(d,d);

    /* calculate penetration depth from closest aabb point to capsule */
    float ap[3], dt[3];
    aabb_closest_point_to_point(ap, min, max, cp);
    f3sub(dt, ap, cp);
    m->depth = sqrtf(f3dot(dt,dt));

    /* calculate normal */
    float l = sqrtf(d2);
    float linv = 1.0f / ((l != 0.0f) ? l: 1.0f);
    f3mul(m->normal, d, linv);
    f3cpy(m->contact_point, cp);
    return 1;
}
static int
line_support(float *support, const float d[3],
    const float a[3], const float b[3])
{
    int i = 0;
    float adot = f3dot(a, d);
    float bdot = f3dot(b, d);
    if (adot < bdot) {
        f3cpy(support, b);
        i = 1;
    } else f3cpy(support, a);
    return i;
}
static int
polyhedron_support(float *support, const float d[3],
    const float *verts, int cnt)
{
    int imax = 0;
    float dmax = f3dot(verts, d);
    for (int i = 1; i < cnt; ++i) {
        /* find vertex with max dot product in direction d */
        float dot = f3dot(&verts[i*3], d);
        if (dot < dmax) continue;
        imax = i, dmax = dot;
    } f3cpy(support, &verts[imax*3]);
    return imax;
}
static int
polyhedron_intersect_sphere(const float *verts, int cnt,
    const float sc[3], float sr)
{
    /* initial guess */
    float d[3] = {0};
    struct gjk_support s = {0};
    f3cpy(s.a, verts);
    f3cpy(s.b, sc);
    f3sub(d, s.b, s.a);

    /* run gjk algorithm */
    struct gjk_simplex gsx = {0};
    static const int max_iter = 20;
    while (gjk(&gsx, &s, d) && (gsx.iter < max_iter)) {
        float n[3]; f3mul(n, d, -1);
        s.aid = polyhedron_support(s.a, n, verts, cnt);
        f3sub(d, s.b, s.a);
    }
    /* check distance between closest points */
    struct gjk_result r = gjk_analyze(&gsx);
    return r.distance_squared <= sr*sr;
}
static int
polyhedron_intersect_capsule(const float *verts, int cnt,
    const float ca[3], const float cb[3], float cr)
{
    /* initial guess */
    float d[3] = {0};
    struct gjk_support s = {0};
    f3cpy(s.a, verts);
    f3cpy(s.b, ca);
    f3sub(d, s.b, s.a);

    /* run gjk algorithm */
    struct gjk_simplex gsx = {0};
    static const int max_iter = 20;
    while (gjk(&gsx, &s, d) && (gsx.iter < max_iter)) {
        float n[3]; f3mul(n, d, -1);
        s.aid = polyhedron_support(s.a, n, verts, cnt);
        s.bid = line_support(s.b, d, ca, cb);
        f3sub(d, s.b, s.a);
    }
    /* check distance between clostest points */
    struct gjk_result r = gjk_analyze(&gsx);
    return r.distance_squared <= cr*cr;
}
static int
polyhedron_intersect_polygon(const float *averts, int acnt,
    const float *bverts, int bcnt)
{
    /* initial guess */
    float d[3] = {0};
    struct gjk_support s = {0};
    f3cpy(s.a, averts);
    f3cpy(s.b, bverts);
    f3sub(d, s.b, s.a);

    /* run gjk algorithm */
    struct gjk_simplex gsx = {0};
    static const int max_iter = 20;
    while (gjk(&gsx, &s, d) && (gsx.iter < max_iter)) {
        float n[3]; f3mul(n, d, -1);
        s.aid = polyhedron_support(s.a, n, averts, acnt);
        s.bid = polyhedron_support(s.b, d, bverts, bcnt);
        f3sub(d, s.b, s.a);
    } return gsx.hit;
}

/* ============================================================================
 *
 *                                  MATH OBJECTS
 *
 * =========================================================================== */
#define vf(v) (&((v).x))
typedef struct v2 {float x,y;} v2;
typedef struct v3 {float x,y,z;} v3;
typedef struct v4 {float x,y,z,w;} v4;
typedef struct quat {float x,y,z,w;} quat;
typedef struct mat4 {v4 x,y,z;} mat3;

#define v3unpack(v) (v).x,(v).y,(v).z
static inline v3 v3mk(float x, float y, float z){v3 r; f3set(&r.x, x,y,z); return r;}
static inline v3 v3mkv(const float *v){v3 r; f3set(&r.x, v[0],v[1],v[2]); return r;}
static inline v3 v3add(v3 a, v3 b){f3add(vf(a),vf(a),vf(b));return a;}
static inline v3 v3sub(v3 a, v3 b){f3sub(vf(a),vf(a),vf(b)); return a;}
static inline v3 v3scale(v3 v, float s){f3mul(vf(v),vf(v),s); return v;}
static inline float v3dot(v3 a, v3 b){return f3dot(vf(a),vf(b));}
static inline v3 v3norm(v3 v) {f3norm(vf(v)); return v;}
static inline v3 v3cross(v3 a, v3 b) {v3 r; f3cross(vf(r),vf(a),vf(b)); return r;}
static inline v3 v3lerp(v3 a, float t, v3 b) {v3 r; f3lerp(vf(r),vf(a),t,vf(b)); return r;}

/* ============================================================================
 *
 *                              COLLISION VOLUME
 *
 * =========================================================================== */
struct sphere {v3 p; float r;};
struct aabb {v3 min, max;};
struct plane {v3 p, n;};
struct capsule {v3 a,b; float r;};
struct ray {v3 p, d;};
struct raycast {v3 p, n; float t0, t1;};

/* plane */
static struct plane plane(v3 p, v3 n);
static struct plane planef(float px, float py, float pz, float nx, float ny, float nz);
static struct plane planefv(float px, float py, float pz, float nx, float ny, float nz);
/* ray */
static struct ray ray(v3 p, v3 d);
static struct ray rayf(float px, float py, float pz, float dx, float dy, float dz);
static struct ray rayfv(const float *p, const float *d);
static int ray_test_plane(struct raycast *o, struct ray r,  struct plane p);
static int ray_test_triangle(struct raycast *o, struct ray r, const struct v3 *tri);
static int ray_test_sphere(struct raycast *o, struct ray r, struct sphere s);
static int ray_test_aabb(struct raycast *o, struct ray r, struct aabb a);
/* sphere */
static struct sphere sphere(v3 p, float r);
static struct sphere spheref(float cx, float cy, float cz, float r);
static struct sphere spherefv(const float *p, float r);
static int sphere_test_sphere(struct sphere a, struct sphere b);
static int sphere_test_sphere_manifold(struct manifold *m, struct sphere a, struct sphere b);
static int sphere_test_aabb(struct sphere s, struct aabb a);
static int sphere_test_aabb_manifold(struct manifold *m, struct sphere s, struct aabb a);
static int sphere_test_capsule(struct sphere s, struct capsule c);
static int sphere_test_capsule_manifold(struct manifold *m, struct sphere s, struct capsule c);
/* aabb */
static struct aabb aabb(v3 min, v3 max);
static struct aabb aabbf(float minx, float miny, float minz, float maxx, float maxy, float maxz);
static struct aabb aabbfv(const float *min, const float *max);
static struct aabb aabb_transform(struct aabb a, const float rot[3][3], const v3 t);
static v3 aabb_closest_point(struct aabb, v3 p);
static int aabb_test_aabb(struct aabb, struct aabb);
static int aabb_test_aabb_manifold(struct manifold *m, struct aabb, struct aabb);
static int aabb_test_sphere(struct aabb a, struct sphere s);
static int aabb_test_sphere_manifold(struct manifold *m,struct aabb a, struct sphere s);
static int aabb_test_capsule(struct aabb a, struct capsule c);
static int aabb_test_capsule_manifold(struct manifold *m, struct aabb a, struct capsule c);
/* capsule */
static struct capsule capsule(v3 from, v3 to, float r);
static struct capsule capsulef(float fx, float fy, float fz, float tx, float ty, float tz, float r);
static struct capsule capsulefv(const float *f, const float *t, float r);
static float capsule_sqdist_point(struct capsule, v3 p);
static v3 capsule_closest_point(struct capsule, v3 pnt);
static int capsule_test_sphere(struct capsule, struct sphere s);
static int capsule_test_sphere_manifold(struct manifold *m,struct capsule, struct sphere s);
static int capsule_test_aabb(struct capsule c, struct aabb a);
static int capsule_test_aabb_manifold(struct manifold *m, struct capsule c, struct aabb a);
static int capsule_test_capsule(struct capsule a, struct capsule b);
static int capsule_test_capsule_manifold(struct manifold *m, struct capsule a, struct capsule b);

static struct plane
plane(v3 p, v3 n)
{
    struct plane r;
    r.p = p;
    r.n = n;
    return r;
}
static struct plane
planef(float px, float py, float pz, float nx, float ny, float nz)
{
    struct plane r;
    r.p = v3mk(px,py,pz);
    r.n = v3mk(nx,ny,nz);
    return r;
}
static struct plane
planefv(float px, float py, float pz, float nx, float ny, float nz)
{
    struct plane r;
    r.p = v3mk(px,py,pz);
    r.n = v3mk(nx,ny,nz);
    return r;
}
static struct ray
ray(v3 p, v3 d)
{
    struct ray r;
    r.p = p;
    r.d = v3norm(d);
    return r;
}
static struct ray
rayf(float px, float py, float pz, float dx, float dy, float dz)
{
    struct ray r;
    r.p = v3mk(px,py,pz);
    r.d = v3norm(v3mk(dx,dy,dz));
    return r;
}
static struct ray
rayfv(const float *p, const float *d)
{
    struct ray r;
    r.p = v3mk(p[0],p[1],p[2]);
    r.d = v3norm(v3mk(d[0],d[1],d[2]));
    return r;
}
static int
ray_test_plane(struct raycast *o, struct ray r,  struct plane p)
{
    float pf[4];
    planeq(pf, &p.n.x, &p.p.x);
    float t = ray_intersects_plane(&r.p.x, &r.d.x, pf);
    if (t <= 0.0f) return 0;
    o->p = v3add(r.p, v3scale(r.d, t));
    o->t0 = o->t1 = t;
    o->n = v3scale(p.n, -1.0f);
    return 1;
}
static int
ray_test_triangle(struct raycast *o, struct ray r, const struct v3 *tri)
{
    float t = ray_intersects_triangle(&r.p.x, &r.d.x, &tri[0].x, &tri[1].x, &tri[2].x);
    if (t <= 0) return 0;
    o->t0 = o->t1 = t;
    o->p = v3add(r.p, v3scale(r.d, t));
    o->n = v3norm(v3cross(v3sub(tri[1],tri[0]),v3sub(tri[2],tri[0])));
    return 1;
}
static int
ray_test_sphere(struct raycast *o, struct ray r, struct sphere s)
{
    if (!ray_intersects_sphere(&o->t0, &o->t1, &r.p.x, &r.d.x, &s.p.x, s.r))
        return 0;
    o->p = v3add(r.p, v3scale(r.d, min(o->t0,o->t1)));
    o->n = v3norm(v3sub(o->p, s.p));
    return 1;
}
static int
ray_test_aabb(struct raycast *o, struct ray r, struct aabb a)
{
    v3 pnt, ext, c;
    float d, min;
    if (!ray_intersects_aabb(&o->t0, &o->t1, &r.p.x, &r.d.x, &a.min.x, &a.max.x))
        return 0;

    o->p = v3add(r.p, v3scale(r.d, min(o->t0,o->t1)));
    ext = v3sub(a.max, a.min);
    c = v3add(a.min, v3scale(ext,0.5f));
    pnt = v3sub(o->p, c);

    min = fabs(ext.x - fabs(pnt.x));
    o->n = v3scale(v3mk(1,0,0), sign(pnt.x));
    d = fabs(ext.y - fabs(pnt.y));
    if (d < min) {
        min = d;
        o->n = v3scale(v3mk(0,1,0), sign(pnt.y));
    }
    d = fabs(ext.z - fabs(pnt.z));
    if (d < min)
        o->n = v3scale(v3mk(0,0,1), sign(pnt.z));
    return 1;
}
static struct sphere
sphere(v3 p, float r)
{
    struct sphere s;
    s.p = p;
    s.r = r;
    return s;
}
static struct sphere
spheref(float cx, float cy, float cz, float r)
{
    struct sphere s;
    s.p = v3mk(cx,cy,cz);
    s.r = r;
    return s;
}
static struct sphere
spherefv(const float *p, float r)
{
    return spheref(p[0], p[1], p[2], r);
}
static int
sphere_test_sphere(struct sphere a, struct sphere b)
{
    return sphere_intersects_sphere(&a.p.x, a.r, &b.p.x, b.r);
}
static int
sphere_test_sphere_manifold(struct manifold *m, struct sphere a, struct sphere b)
{
    return sphere_intersects_sphere_manifold(m, &a.p.x, a.r, &b.p.x, b.r);
}
static int
sphere_test_aabb(struct sphere s, struct aabb a)
{
    return sphere_intersect_aabb(&s.p.x, s.r, &a.min.x, &a.max.x);
}
static int
sphere_test_aabb_manifold(struct manifold *m, struct sphere s, struct aabb a)
{
    return sphere_intersect_aabb_manifold(m, &s.p.x, s.r, &a.min.x, &a.max.x);
}
static int
sphere_test_capsule(struct sphere s, struct capsule c)
{
    return sphere_intersect_capsule(&s.p.x, s.r, &c.a.x, &c.b.x, c.r);
}
static int
sphere_test_capsule_manifold(struct manifold *m,
    struct sphere s, struct capsule c)
{
    return sphere_intersect_capsule_manifold(m, &s.p.x, s.r, &c.a.x, &c.b.x, c.r);
}
static struct aabb
aabb(v3 min, v3 max)
{
    struct aabb a;
    a.min = min;
    a.max = max;
    return a;
}
static struct aabb
aabbf(float minx, float miny, float minz, float maxx, float maxy, float maxz)
{
    struct aabb a;
    a.min = v3mk(minx,miny,minz);
    a.max = v3mk(maxx,maxy,maxz);
    return a;
}
static struct aabb
aabbfv(const float *min, const float *max)
{
    struct aabb a;
    a.min = v3mk(min[0],min[1],min[2]);
    a.max = v3mk(max[0],max[1],max[3]);
    return a;
}
static struct aabb
aabb_transform(struct aabb a, const float rot[3][3], const v3 t)
{
    struct aabb res;
    aabb_rebalance_transform(&res.min.x, &res.max.x, &a.min.x, &a.max.x, rot, &t.x);
    return res;
}
static v3
aabb_closest_point(struct aabb a, v3 p)
{
    v3 res = {0};
    aabb_closest_point_to_point(&res.x, &a.min.x, &a.max.x, &p.x);
    return res;
}
static int
aabb_test_aabb(struct aabb a, struct aabb b)
{
    return aabb_intersects_aabb(&a.min.x, &a.max.x, &b.min.x, &b.max.x);
}
static int
aabb_test_aabb_manifold(struct manifold *m, struct aabb a, struct aabb b)
{
    return aabb_intersects_aabb_manifold(m, &a.min.x, &a.max.x, &b.min.x, &b.max.x);
}
static int
aabb_test_sphere(struct aabb a, struct sphere s)
{
    return aabb_intersect_sphere(&a.min.x, &a.max.x, &s.p.x, s.r);
}
static int
aabb_test_sphere_manifold(struct manifold *m,
    struct aabb a, struct sphere s)
{
    return aabb_intersect_sphere_manifold(m, &a.min.x, &a.max.x, &s.p.x, s.r);
}
static int
aabb_test_capsule(struct aabb a, struct capsule c)
{
    return aabb_intersect_capsule(&a.min.x, &a.max.x, &c.a.x, &c.b.x, c.r);
}
static int
aabb_test_capsule_manifold(struct manifold *m, struct aabb a, struct capsule c)
{
    return aabb_intersect_capsule_manifold(m, &a.min.x, &a.max.x, &c.a.x, &c.b.x, c.r);
}
static struct capsule
capsule(v3 a, v3 b, float r)
{
    struct capsule c;
    c.a = a; c.b = b; c.r = r;
    return c;
}
static struct capsule
capsulef(float ax, float ay, float az, float bx, float by, float bz, float r)
{
    struct capsule c;
    c.a = v3mk(ax,ay,az);
    c.b = v3mk(bx,by,bz);
    c.r = r;
    return c;
}
static struct capsule
capsulefv(const float *a, const float *b, float r)
{
    struct capsule c;
    c.a = v3mkv(a);
    c.b = v3mkv(b);
    c.r = r;
    return c;
}
static float
capsule_sqdist_point(struct capsule c, v3 p)
{
    return capsule_point_sqdist(&c.a.x, &c.b.x, c.r, &p.x);
}
static v3
capsule_closest_point(struct capsule c, v3 pnt)
{
    float p[3];
    capsule_closest_point_to_point(p, &c.a.x, &c.b.x, c.r, &pnt.x);
    return v3mkv(p);
}
static int
capsule_test_sphere(struct capsule c, struct sphere s)
{
    return capsule_intersect_sphere(&c.a.x, &c.b.x, c.r, &s.p.x, s.r);
}
static int
capsule_test_sphere_manifold(struct manifold *m, struct capsule c, struct sphere s)
{
    return capsule_intersect_sphere_manifold(m, &c.a.x, &c.b.x, c.r, &s.p.x, s.r);
}
static int
capsule_test_aabb(struct capsule c, struct aabb a)
{
    return capsule_intersect_aabb(&c.a.x, &c.b.x, c.r, &a.min.x, &a.max.x);
}
static int
capsule_test_aabb_manifold(struct manifold *m, struct capsule c, struct aabb a)
{
    return capsule_intersect_aabb_manifold(m, &c.a.x, &c.b.x, c.r, &a.min.x, &a.max.x);
}
static int
capsule_test_capsule(struct capsule a, struct capsule b)
{
    return capsule_intersect_capsule(&a.a.x, &a.b.x, a.r, &b.a.x, &b.b.x, b.r);
}
static int
capsule_test_capsule_manifold(struct manifold *m, struct capsule a, struct capsule b)
{
    return capsule_intersect_capsule_manifold(m, &a.a.x, &a.b.x, a.r, &b.a.x, &b.b.x, b.r);
}
/* ============================================================================
 *
 *                              CAMERA
 *
 * ============================================================================*/
#define CAM_INF (-1.0f)
enum cam_orient {CAM_ORIENT_QUAT, CAM_ORIENT_MAT};
enum cam_output_z_range {
    CAM_OUT_Z_NEGATIVE_ONE_TO_ONE,
    CAM_OUT_Z_NEGATIVE_ONE_TO_ZERO,
    CAM_OUT_Z_ZERO_TO_ONE
};
struct cam {
    /*------- [input] --------*/
    int zout;
    enum cam_orient orient_struct;
    /* projection */
    float fov;
    float near, far;
    float aspect_ratio;
    float z_range_epsilon;

    /* view */
    float pos[3];
    float off[3];
    float ear[3];
    float q[4];
    float m[3][3];

    /*------- [output] --------*/
    float view[4][4];
    float view_inv[4][4];
    float proj[4][4];
    float proj_inv[4][4];

    float loc[3];
    float forward[3];
    float backward[3];
    float right[3];
    float down[3];
    float left[3];
    float up[3];
    /*-------------------------*/
};
static void
cam_init(struct cam *c)
{
    static const float qid[] = {0,0,0,1};
    static const float m3id[] = {1,0,0,0,1,0,0,0,1};
    static const float m4id[] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};

    memset(c, 0, sizeof(*c));
    c->aspect_ratio = 3.0f/2.0f;
    c->fov = PI_CONSTANT / 4.0f;
    c->near = 0.01f;
    c->far = 10000;

    memcpy(c->q, qid, sizeof(qid));
    memcpy(c->m, m3id, sizeof(m3id));
    memcpy(c->view, m4id, sizeof(m4id));
    memcpy(c->view_inv, m4id, sizeof(m4id));
    memcpy(c->proj, m4id, sizeof(m4id));
    memcpy(c->proj_inv, m4id, sizeof(m4id));
}
static void
cam_build(struct cam *c)
{
    assert(c);
    if (!c) return;

    /* convert orientation matrix into quaternion */
    if (c->orient_struct == CAM_ORIENT_MAT) {
        float s,t,
        trace = c->m[0][0];
        trace += c->m[1][1];
        trace += c->m[2][2];
        if (trace > 0.0f) {
            t = trace + 1.0f;
            s = (float)sqrt((double)(1.0f/t)) * 0.5f;

            c->q[3] = s * t;
            c->q[0] = (c->m[2][1] - c->m[1][2]) * s;
            c->q[1] = (c->m[0][2] - c->m[2][0]) * s;
            c->q[2] = (c->m[1][0] - c->m[0][1]) * s;
        } else {
            int i = 0, j, k;
            static const int next[] = {1,2,0};
            if (c->m[1][1] > c->m[0][0] ) i = 1;
            if (c->m[2][2] > c->m[i][i] ) i = 2;

            j = next[i]; k = next[j];
            t = (c->m[i][i] - (c->m[j][j] - c->m[k][k])) + 1.0f;
            s = (float)sqrt((double)(1.0f/t)) * 0.5f;

            c->q[i] = s*t;
            c->q[3] = (c->m[k][j] - c->m[j][k]) * s;
            c->q[j] = (c->m[j][i] + c->m[i][j]) * s;
            c->q[k] = (c->m[k][i] + c->m[i][k]) * s;
        }

        /* normalize quaternion */
        {float len2 = c->q[0]*c->q[0] + c->q[1]*c->q[1];
        len2 += c->q[2]*c->q[2] + c->q[3]*c->q[3];
        if (len2 != 0.0f) {
            float len = (float)sqrt((double)len2);
            float inv_len = 1.0f/len;
            c->q[0] *= inv_len; c->q[1] *= inv_len;
            c->q[2] *= inv_len; c->q[3] *= inv_len;
        }}
    }
    /* Camera euler orientation
    It is not feasible to multiply euler angles directly together to represent the camera
    orientation because of gimbal lock (Even quaternions do not save you against
    gimbal lock under all circumstances). While it is true that it is not a problem for
    FPS style cameras, it is a problem for free cameras. To fix that issue this camera only
    takes in the relative angle rotation and does not store the absolute angle values [7].*/
    {float sx =(float)sin((double)(c->ear[0]*0.5f));
    float sy = (float)sin((double)(c->ear[1]*0.5f));
    float sz = (float)sin((double)(c->ear[2]*0.5f));

    float cx = (float)cos((double)(c->ear[0]*0.5f));
    float cy = (float)cos((double)(c->ear[1]*0.5f));
    float cz = (float)cos((double)(c->ear[2]*0.5f));

    float a[4], b[4];
    a[0] = cz*sx; a[1] = sz*sx; a[2] = sz*cx; a[3] = cz*cx;
    b[0] = c->q[0]*cy - c->q[2]*sy;
    b[1] = c->q[3]*sy + c->q[1]*cy;
    b[2] = c->q[2]*cy + c->q[0]*sy;
    b[3] = c->q[3]*cy - c->q[1]*sy;

    c->q[0] = a[3]*b[0] + a[0]*b[3] + a[1]*b[2] - a[2]*b[1];
    c->q[1] = a[3]*b[1] + a[1]*b[3] + a[2]*b[0] - a[0]*b[2];
    c->q[2] = a[3]*b[2] + a[2]*b[3] + a[0]*b[1] - a[1]*b[0];
    c->q[3] = a[3]*b[3] - a[0]*b[0] - a[1]*b[1] - a[2]*b[2];
    memset(c->ear, 0, sizeof(c->ear));}

    /* Convert quaternion to matrix
    Next up we want to convert our camera quaternion orientation into a 3x3 matrix
    to generate our view matrix. So to convert from quaternion to rotation
    matrix we first look at how to transform a vector quaternion by and how by matrix.
    To transform a vector by a unit quaternion you turn the vector into a zero w
    quaternion and left multiply by quaternion and right multiply by quaternion inverse:
        p2  = q * q(P1) * pi
            = q(qx,qy,qz,qw) * q(x,y,z,0) * q(-qx,-qy,-qz,qw)

    To get the same result with a rotation matrix you just multiply the vector by matrix:
        p2 = M * p1

    So to get the matrix M you first multiply out the quaternion transformation and
    group each x,y and z term into a column. The end result is: */
    {float x2 = c->q[0] + c->q[0];
    float y2 = c->q[1] + c->q[1];
    float z2 = c->q[2] + c->q[2];

    float xx = c->q[0]*x2;
    float xy = c->q[0]*y2;
    float xz = c->q[0]*z2;

    float yy = c->q[1]*y2;
    float yz = c->q[1]*z2;
    float zz = c->q[2]*z2;

    float wx = c->q[3]*x2;
    float wy = c->q[3]*y2;
    float wz = c->q[3]*z2;

    c->m[0][0] = 1.0f - (yy + zz);
    c->m[0][1] = xy - wz;
    c->m[0][2] = xz + wy;

    c->m[1][0] = xy + wz;
    c->m[1][1] = 1.0f - (xx + zz);
    c->m[1][2] = yz - wx;

    c->m[2][0] = xz - wy;
    c->m[2][1] = yz + wx;
    c->m[2][2] = 1.0f - (xx + yy);}

    /* View matrix
    The general transform pipeline is object space to world space to local camera
    space to screenspace to clipping space. While this particular matrix, the view matrix,
    transforms from world space to local camera space.

    So we start by trying to find the camera world transform a 4x3 matrix which
    can and will be in this particular implementation extended to a 4x4 matrix
    composed of camera rotation, camera translation and camera offset.
    While pure camera position and orientation is usefull for free FPS style cameras,
    allows the camera offset 3rd person cameras or tracking ball behavior.

        T.......camera position
        F.......camera offset
        R.......camera orientation 3x3 matrix
        C.......camera world transform 4x4 matrix

            |1 T|   |R 0|   |1 F|
        C = |0 1| * |0 1| * |0 1|

            |R   Rf+T|
        C = |0      1|
    */
    /* 1.) copy orientation matrix */
    c->view_inv[0][0] = c->m[0][0]; c->view_inv[0][1] = c->m[0][1];
    c->view_inv[0][2] = c->m[0][2]; c->view_inv[1][0] = c->m[1][0];
    c->view_inv[1][1] = c->m[1][1]; c->view_inv[1][2] = c->m[1][2];
    c->view_inv[2][0] = c->m[2][0]; c->view_inv[2][1] = c->m[2][1];
    c->view_inv[2][2] = c->m[2][2];

    /* 2.) transform offset by camera orientation and add translation */
    c->view_inv[3][0] = c->view_inv[0][0]*c->off[0];
    c->view_inv[3][1] = c->view_inv[0][1]*c->off[0];
    c->view_inv[3][2] = c->view_inv[0][2]*c->off[0];

    c->view_inv[3][0] += c->view_inv[1][0]*c->off[1];
    c->view_inv[3][1] += c->view_inv[1][1]*c->off[1];
    c->view_inv[3][2] += c->view_inv[1][2]*c->off[1];

    c->view_inv[3][0] += c->view_inv[2][0]*c->off[2];
    c->view_inv[3][1] += c->view_inv[2][1]*c->off[2];
    c->view_inv[3][2] += c->view_inv[2][2]*c->off[2];

    c->view_inv[3][0] += c->pos[0];
    c->view_inv[3][1] += c->pos[1];
    c->view_inv[3][2] += c->pos[2];

    /* 3.) fill last empty 4x4 matrix row */
    c->view_inv[0][3] = 0;
    c->view_inv[1][3] = 0;
    c->view_inv[2][3] = 0;
    c->view_inv[3][3] = 1.0f;

    /* Now we have a matrix to transform from local camera space into world
    camera space. But remember we are looking for the opposite transform since we
    want to transform the world around the camera. So to get the other way
    around we have to invert our world camera transformation to transform to
    local camera space.

    Usually inverting matricies is quite a complex endeavour both in needed complexity
    as well as number of calculations required. Luckily we can use a nice property
    of orthonormal matrices (matrices with each column being unit length)
    on the more complicated matrix the rotation matrix.
    The inverse of orthonormal matrices is the same as the transpose of the same
    matrix, which is just a matrix column to row swap.

    So with inverse rotation matrix covered the only thing left is the inverted
    camera translation which just needs to be negated plus and this is the more
    important part tranformed by the inverted rotation matrix.
    Why? simply because the inverse of a matrix multiplication M = A * B
    is NOT Mi = Ai * Bi (i for inverse) but rather Mi = Bi * Ai.
    So if we put everything together we get the following view matrix:

        R.......camera orientation matrix3x3
        T.......camera translation
        F.......camera offset
        Ti......camera inverse translation
        Fi......camera inverse offset
        Ri......camera inverse orientation matrix3x3
        V.......view matrix

               (|R   Rf+T|)
        V = inv(|0      1|)

            |Ri -Ri*Ti-Fi|
        V = |0          1|

    Now we finally have our matrix composition and can fill out the view matrix:

    1.) Inverse camera orientation by transpose */
    c->view[0][0] = c->m[0][0];
    c->view[0][1] = c->m[1][0];
    c->view[0][2] = c->m[2][0];

    c->view[1][0] = c->m[0][1];
    c->view[1][1] = c->m[1][1];
    c->view[1][2] = c->m[2][1];

    c->view[2][0] = c->m[0][2];
    c->view[2][1] = c->m[1][2];
    c->view[2][2] = c->m[2][2];

    /* 2.) Transform inverted position vector by transposed orientation and subtract offset */
    {float pos_inv[3];
    pos_inv[0] = -c->pos[0];
    pos_inv[1] = -c->pos[1];
    pos_inv[2] = -c->pos[2];

    c->view[3][0] = c->view[0][0] * pos_inv[0];
    c->view[3][1] = c->view[0][1] * pos_inv[0];
    c->view[3][2] = c->view[0][2] * pos_inv[0];

    c->view[3][0] += c->view[1][0] * pos_inv[1];
    c->view[3][1] += c->view[1][1] * pos_inv[1];
    c->view[3][2] += c->view[1][2] * pos_inv[1];

    c->view[3][0] += c->view[2][0] * pos_inv[2];
    c->view[3][1] += c->view[2][1] * pos_inv[2];
    c->view[3][2] += c->view[2][2] * pos_inv[2];

    c->view[3][0] -= c->off[0];
    c->view[3][1] -= c->off[1];
    c->view[3][2] -= c->off[2];}

    /* 3.) fill last empty 4x4 matrix row */
    c->view[0][3] = 0; c->view[1][3] = 0;
    c->view[2][3] = 0; c->view[3][3] = 1.0f;

    /*  Projection matrix
    While the view matrix transforms from world space to local camera space,
    tranforms the perspective projection matrix from camera space to screen space.

    The actual work for the transformation is from eye coordinates camera
    frustum far plane to a cube with coordinates (-1,1), (0,1), (-1,0) depending on
    argument `out_z_range` in this particual implementation.

    To actually build the projection matrix we need:
        - Vertical field of view angle
        - Screen aspect ratio which controls the horizontal view angle in
            contrast to the field of view.
        - Z coordinate of the frustum near clipping plane
        - Z coordinate of the frustum far clipping plane

    While I will explain how to incooperate the near,far z clipping
    plane I would recommend reading [7] for the other values since it is quite
    hard to do understand without visual aid and he is probably better in
    explaining than I would be.*/
    {float hfov = (float)tan((double)(c->fov*0.5f));
    c->proj[0][0] = 1.0f/(c->aspect_ratio * hfov);
    c->proj[0][1] = 0;
    c->proj[0][2] = 0;
    c->proj[0][3] = 0;

    c->proj[1][0] = 0;
    c->proj[1][1] = 1.0f/hfov;
    c->proj[1][2] = 0;
    c->proj[1][3] = 0;

    c->proj[2][0] = 0;
    c->proj[2][1] = 0;
    c->proj[2][3] = -1.0f;

    c->proj[3][0] = 0;
    c->proj[3][1] = 0;
    c->proj[3][3] = 0;

    if (c->far <= CAM_INF) {
        /* Up to this point we got perspective matrix:

            |1/aspect*hfov  0       0  0|
            |0              1/hfov  0  0|
            |0              0       A  B|
            |0              0       -1 0|

        but we are still missing A and B to map between the frustum near/far
        and the clipping cube near/far value. So we take the lower right part
        of the matrix and multiply by a vector containing the missing
        z and w value, which gives us the resulting clipping cube z;

            |A  B|   |z|    |Az + B |           B
            |-1 0| * |1| =  | -z    | = -A + ------
                                               -z

        So far so good but now we need to map from the frustum near,
        far clipping plane (n,f) to the clipping cube near and far plane (cn,cf).
        So we plugin the frustum near/far values into z and the resulting
        cube near/far plane we want to end up with.

                   B                    B
            -A + ------ = cn and -A + ------ = cf
                   n                    f

        We now have two equations with two unkown A and B since n,f as well
        as cn,cf are provided, so we can solved them by subtitution either by
        hand or, if you are like me prone to easily make small mistakes,
        with WolframAlpha which solved for:*/
        switch (c->zout) {
            default:
            case CAM_OUT_Z_NEGATIVE_ONE_TO_ONE: {
                /* cn = -1 and cf = 1: */
                c->proj[2][2] = -(c->far + c->near) / (c->far - c->near);
                c->proj[3][2] = -(2.0f * c->far * c->near) / (c->far - c->near);
            } break;
            case CAM_OUT_Z_NEGATIVE_ONE_TO_ZERO: {
                /* cn = -1 and cf = 0: */
                c->proj[2][2] = (c->near) / (c->near - c->far);
                c->proj[3][2] = (c->far * c->near) / (c->near - c->far);
            } break;
            case CAM_OUT_Z_ZERO_TO_ONE: {
                /* cn = 0 and cf = 1: */
                c->proj[2][2] = -(c->far) / (c->far - c->near);
                c->proj[3][2] = -(c->far * c->near) / (c->far - c->near);
            } break;
        }
    } else {
    /*  Infinite projection [1]:
        In general infinite projection matrices map direction to points on the
        infinite distant far plane, which is mainly useful for rendering:
            - skyboxes, sun, moon, stars
            - stencil shadow volume caps

        To actually calculate the infinite perspective matrix you let the
        far clip plane go to infinity. Once again I would recommend using and
        checking WolframAlpha to make sure all values are correct, while still
        doing the calculation at least once by hand.

        While it is mathematically correct to go to infinity, floating point errors
        result in a depth smaller or bigger. This in term results in
        fragment culling since the hardware thinks fragments are beyond the
        far clipping plane. The general solution is to introduce an epsilon
        to fix the calculation error and map it to the infinite far plane.

        Important:
        For 32-bit floating point epsilon should be greater than 2.4*10^-7,
        to account for floating point precision problems. */
        switch (c->zout) {
            default:
            case CAM_OUT_Z_NEGATIVE_ONE_TO_ONE: {
                /*lim f->inf -((f+n)/(f-n)) => -((inf+n)/(inf-n)) => -(inf)/(inf) => -1.0*/
                c->proj[2][2] = c->z_range_epsilon - 1.0f;
                /* lim f->inf -(2*f*n)/(f-n) => -(2*inf*n)/(inf-n) => -(2*inf*n)/inf => -2n*/
                c->proj[3][2] = (c->z_range_epsilon - 2.0f) * c->near;
            } break;
            case CAM_OUT_Z_NEGATIVE_ONE_TO_ZERO: {
                /* lim f->inf  n/(n-f) => n/(n-inf) => n/(n-inf) => -0 */
                c->proj[2][2] = c->z_range_epsilon;
                /* lim f->inf (f*n)/(n-f) => (inf*n)/(n-inf) => (inf*n)/-inf = -n */
                c->proj[3][2] = (c->z_range_epsilon - 1.0f) * c->near;
            } break;
            case CAM_OUT_Z_ZERO_TO_ONE: {
                /* lim f->inf (-f)/(f-n) => (-inf)/(inf-n) => -inf/inf = -1 */
                c->proj[2][2] = c->z_range_epsilon - 1.0f;
                /* lim f->inf (-f*n)/(f-n) => (-inf*n)/(inf-n) => (-inf*n)/(inf) => -n */
                c->proj[3][2] = (c->z_range_epsilon - 1.0f) * c->near;
            } break;
        }
    }}
    /* Invert projection [2][3]:
    Since perspective matrices have a fixed layout, it makes sense
    to calculate the specific perspective inverse instead of relying on a default
    matrix inverse function. Actually calculating the matrix for any perspective
    matrix is quite straight forward:

        I.......identity matrix
        p.......perspective matrix
        I(p)....inverse perspective matrix

    1.) Fill a variable inversion matrix and perspective layout matrix into the
        inversion formula: I(p) * p = I

        |x0  x1  x2  x3 |   |a 0 0 0|   |1 0 0 0|
        |x4  x5  x6  x7 | * |0 b 0 0| = |0 1 0 0|
        |x8  x9  x10 x11|   |0 0 c d|   |0 0 1 0|
        |x12 x13 x14 x15|   |0 0 e 0|   |0 0 0 1|

    2.) Multiply inversion matrix times our perspective matrix

        |x0*a x1*b x2*c+x3*e x2*d|      |1 0 0 0|
        |x4*a x5*b x6*c+x7*e x6*d|    = |0 1 0 0|
        |x8*a x9*b x10*c+x11*e x10*d|   |0 0 1 0|
        |x12*a x13*b x14*c+x15*e x14*d| |0 0 0 1|

    3.) Finally substitute each x value: e.g: x0*a = 1 => x0 = 1/a so I(p) at column 0, row 0 is 1/a.

                |1/a 0 0 0|
        I(p) =  |0 1/b 0 0|
                |0 0 0 1/e|
                |0 0 1/d -c/de|

    These steps basically work for any invertable matrices, but I would recommend
    using WolframAlpha for these specific kinds of matrices, since it can
    automatically generate inversion matrices without any fuss or possible
    human calculation errors. */
    memset(c->proj_inv, 0, sizeof(c->proj_inv));
    c->proj_inv[0][0] = 1.0f/c->proj[0][0];
    c->proj_inv[1][1] = 1.0f/c->proj[1][1];
    c->proj_inv[2][3] = 1.0f/c->proj[3][2];
    c->proj_inv[3][2] = 1.0f/c->proj[2][3];
    c->proj_inv[3][3] = -c->proj[2][2]/(c->proj[3][2] * c->proj[2][3]);

    /* fill vectors with data */
    c->loc[0] = c->view_inv[3][0];
    c->loc[1] = c->view_inv[3][1];
    c->loc[2] = c->view_inv[3][2];

    c->forward[0] = c->view_inv[2][0];
    c->forward[1] = c->view_inv[2][1];
    c->forward[2] = c->view_inv[2][2];

    c->backward[0] = -c->view_inv[2][0];
    c->backward[1] = -c->view_inv[2][1];
    c->backward[2] = -c->view_inv[2][2];

    c->right[0] = c->view_inv[0][0];
    c->right[1] = c->view_inv[0][1];
    c->right[2] = c->view_inv[0][2];

    c->left[0] = -c->view_inv[0][0];
    c->left[1] = -c->view_inv[0][1];
    c->left[2] = -c->view_inv[0][2];

    c->up[0] = c->view_inv[1][0];
    c->up[1] = c->view_inv[1][1];
    c->up[2] = c->view_inv[1][2];

    c->down[0] = -c->view_inv[1][0];
    c->down[1] = -c->view_inv[1][1];
    c->down[2] = -c->view_inv[1][2];
}
static void
cam_move(struct cam *c, float x, float y, float z)
{
    c->pos[0] += c->view_inv[0][0]*x;
    c->pos[1] += c->view_inv[0][1]*x;
    c->pos[2] += c->view_inv[0][2]*x;

    c->pos[0] += c->view_inv[1][0]*y;
    c->pos[1] += c->view_inv[1][1]*y;
    c->pos[2] += c->view_inv[1][2]*y;

    c->pos[0] += c->view_inv[2][0]*z;
    c->pos[1] += c->view_inv[2][1]*z;
    c->pos[2] += c->view_inv[2][2]*z;
}
static void
cam_screen_to_world(const struct cam *c, float width, float height,
    float *res, float screen_x, float screen_y, float camera_z)
{
    /* Screen space to world space coordinates
    To convert from screen space coordinates to world coordinates we
    basically have to revert all transformations typically done to
    convert from world space to screen space:
        Viewport => NDC => Clip => View => World

    Viewport => NDC => Clip
    -----------------------
    First up is the transform from viewport to clipping space.
    To get from clipping space to viewport we calculate:

                    |((x+1)/2)*w|
        Vn = v =    |((1-y)/2)*h|
                    |((z+1)/2)  |

    Now we need to the the inverse process by solvinging for n:
                |(2*x)/w - 1|
        n =     |(2*y)/h    |
                |(2*z)-1)   |
                | 1         |
    */
    float x = (screen_x / width * 2.0f) - 1.0f;
    float y = (screen_y / height) * 2.0f - 1.0f;
    float z = 2.0f * camera_z - 1.0f;

    /* Clip => View
    -----------------------
    A vector v or position p in view space is tranform to clip
    coordinates c by being transformed by a projection matrix P:

        c = P * v

    To convert from clipping coordinates c to view coordinates we
    just have to transfrom c by the inverse projection matrix Pi:

        v = Pi * c

    The inverse projection matrix for all common projection matrices
    can be calculated by (see Camera_Build for more information):

                |1/a 0 0 0|
        Pi  =   |0 1/b 0 0|
                |0 0 0 1/e|
                |0 0 1/d -c/de|

    View => World
    -----------------------
    Finally we just need to convert from view coordinates to world
    coordinates w by transforming our view coordinates by the inverse view
    matrix Vi which in this context is just the camera translation and
    rotation.

        w = Vi * v

    Now we reached our goal and have our world coordinates. This implementation
    combines both the inverse projection as well as inverse view transformation
    into one since the projection layout is known we can do some optimization:*/
    float ax = c->proj_inv[0][0]*x;
    float by = c->proj_inv[1][1]*y;
    float dz = c->proj_inv[2][3]*z;
    float w = c->proj_inv[3][3] + dz;

    res[0] = c->proj_inv[3][2] * c->view_inv[2][0];
    res[0] += c->proj_inv[3][3] * c->view_inv[3][0];
    res[0] += ax * c->view_inv[0][0];
    res[0] += by * c->view_inv[1][0];
    res[0] += dz * c->view_inv[3][0];

    res[1] = c->proj_inv[3][2] * c->view_inv[2][1];
    res[1] += c->proj_inv[3][3] * c->view_inv[3][1];
    res[1] += ax * c->view_inv[0][1];
    res[1] += by * c->view_inv[1][1];
    res[1] += dz * c->view_inv[3][1];

    res[2] = c->proj_inv[3][2] * c->view_inv[2][2];
    res[2] += c->proj_inv[3][3] * c->view_inv[3][2];
    res[2] += ax * c->view_inv[0][2];
    res[2] += by * c->view_inv[1][2];
    res[2] += dz * c->view_inv[3][2];
    res[0] /= w; res[1] /= w; res[2] /= w;
}
static void
cam_picking_ray(const struct cam *c, float w, float h,
    float mx, float my, float *ro, float *rd)
{
    float world[3];
    cam_screen_to_world(c, w, h, world, mx, my, 0);

    /* calculate direction
    We generate the ray normal vector by first transforming the mouse cursor position
    from screen coordinates into world coordinates. After that we only have to
    subtract our camera position from our calculated mouse world position and
    normalize the result to make sure we have a unit vector as direction. */
    ro[0] = c->loc[0];
    ro[1] = c->loc[1];
    ro[2] = c->loc[2];

    rd[0] = world[0] - ro[0];
    rd[1] = world[1] - ro[1];
    rd[2] = world[2] - ro[2];

    /* normalize */
    {float dot = rd[0]*rd[0] + rd[1]*rd[1] + rd[2]*rd[2];
    if (dot != 0.0f) {
        float len = (float)sqrt((double)dot);
        rd[0] /= len; rd[1] /= len; rd[2] /= len;
    }}
}

/* ============================================================================
 *
 *                                  3D DRAW
 *
 * =========================================================================== */
#include <GL/gl.h>
#include <GL/glu.h>

#define glLine(a,b) glLinef((a)[0],(a)[1],(a)[2],(b)[0],(b)[1],(b)[2])
#define glTriangle(a,b,c) glTrianglef((a)[0],(a)[1],(a)[2],(b)[0],(b)[1],(b)[2],(c)[0],(c)[1],(c)[2])
#define glSphere(c,r) glSpheref((c)[0],(c)[1],(c)[2],r)
#define glPlane(p,n,scale) glPlanef(p[0],p[1],p[2],n[0],n[1],n[2],scale)
#define glArrow(f,t,size) glArrowf((f)[0],(f)[1],(f)[2],(t)[0],(t)[1],(t)[2],size)
#define glBox(c,w,h,d) glBoxf((c)[0],(c)[1],(c)[2],w,h,d)
#define glCapsule(a,b,r) glCapsulef((a)[0],(a)[1],(a)[2], (b)[0],(b)[1],(b)[2], (r))
#define glPyramid(a,b,s) glPyramidf((a)[0],(a)[1],(a)[2], (b)[0],(b)[1],(b)[2], (s))

#define glError() glerror_(__FILE__, __LINE__)
static void
glerror_(const char *file, int line)
{
    const GLenum code = glGetError();
    if (code == GL_INVALID_ENUM)
        fprintf(stdout, "[GL] Error: (%s:%d) invalid value!\n", file, line);
    else if (code == GL_INVALID_OPERATION)
        fprintf(stdout, "[GL] Error: (%s:%d) invalid operation!\n", file, line);
    else if (code == GL_INVALID_FRAMEBUFFER_OPERATION)
        fprintf(stdout, "[GL] Error: (%s:%d) invalid frame op!\n", file, line);
    else if (code == GL_OUT_OF_MEMORY)
        fprintf(stdout, "[GL] Error: (%s:%d) out of memory!\n", file, line);
    else if (code == GL_STACK_UNDERFLOW)
        fprintf(stdout, "[GL] Error: (%s:%d) stack underflow!\n", file, line);
    else if (code == GL_STACK_OVERFLOW)
        fprintf(stdout, "[GL] Error: (%s:%d) stack overflow!\n", file, line);
}
static void
f3ortho(float *left, float *up, const float *v)
{
    #define inv_sqrt(x) (1.0f/sqrtf(x));
    float lenSqr, invLen;
    if (fabs(v[2]) > 0.7f) {
        lenSqr  = v[1]*v[1]+v[2]*v[2];
        invLen  = inv_sqrt(lenSqr);

        up[0] = 0.0f;
        up[1] =  v[2]*invLen;
        up[2] = -v[1]*invLen;

        left[0] = lenSqr*invLen;
        left[1] = -v[0]*up[2];
        left[2] =  v[0]*up[1];
    } else {
        lenSqr = v[0]*v[0] + v[1]*v[1];
        invLen = inv_sqrt(lenSqr);

        left[0] = -v[1] * invLen;
        left[1] =  v[0] * invLen;
        left[2] = 0.0f;

        up[0] = -v[2] * left[1];
        up[1] =  v[2] * left[0];
        up[2] = lenSqr * invLen;
    }
}
static void
glLinef(float ax, float ay, float az, float bx, float by, float bz)
{
    glBegin(GL_LINES);
    glVertex3f(ax, ay, az);
    glVertex3f(bx, by, bz);
    glEnd();
}
static void
glPlanef(float px, float py, float pz,
    float nx, float ny, float nz, float scale)
{
    float n[3], p[3];
    float v1[3], v2[3], v3[3], v4[3];
    float tangent[3], bitangent[3];

    f3set(n,nx,ny,nz);
    f3set(p,px,py,pz);
    f3ortho(tangent, bitangent, n);
    #define DD_PLANE_V(v, op1, op2) \
        v[0] = (p[0] op1 (tangent[0]*scale) op2 (bitangent[0]*scale)); \
        v[1] = (p[1] op1 (tangent[1]*scale) op2 (bitangent[1]*scale)); \
        v[2] = (p[2] op1 (tangent[2]*scale) op2 (bitangent[2]*scale))

    DD_PLANE_V(v1,-,-);
    DD_PLANE_V(v2,+,-);
    DD_PLANE_V(v3,+,+);
    DD_PLANE_V(v4,-,+);
    #undef DD_PLANE_V

    glLine(v1,v2);
    glLine(v2,v3);
    glLine(v3,v4);
    glLine(v4,v1);
}
static void
glTrianglef(float ax, float ay, float az,
    float bx, float by, float bz,
    float cx, float cy, float cz)
{
    glLinef(ax,ay,az, bx,by,bz);
    glLinef(bx,by,bz, cx,cy,cz);
    glLinef(cx,cy,cz, ax,ay,az);
}
static void
glArrowf(float fx, float fy, float fz, float tx, float ty, float tz, const float size)
{
    int i = 0;
    float degrees = 0.0f;
    static const float arrow_step = 30.0f;
    static const float arrow_sin[45] = {
        0.0f, 0.5f, 0.866025f, 1.0f, 0.866025f, 0.5f, -0.0f, -0.5f, -0.866025f,
        -1.0f, -0.866025f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };
    static const float arrow_cos[45] = {
        1.0f, 0.866025f, 0.5f, -0.0f, -0.5f, -0.866026f, -1.0f, -0.866025f, -0.5f, 0.0f,
        0.5f, 0.866026f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    }; float from[3], to[3];
    float up[3], right[3], forward[3];

    f3set(from,fx,fy,fz);
    f3set(to,tx,ty,tz);
    f3sub(forward, to, from);
    f3norm(forward);
    f3ortho(right, up, forward);
    f3mul(forward, forward, size);

    glLine(from, to);
    for (i = 0; degrees < 360.0f; degrees += arrow_step, ++i) {
        float scale;
        float v1[3], v2[3], temp[3];

        scale = 0.5f * size * arrow_cos[i];
        f3mul(temp, right, scale);
        f3sub(v1, to, forward);
        f3add(v1, v1, temp);

        scale = 0.5f * size * arrow_sin[i];
        f3mul(temp, up, scale);
        f3add(v1, v1, temp);

        scale = 0.5f * size * arrow_cos[i + 1];
        f3mul(temp, right, scale);
        f3sub(v2, to, forward);
        f3add(v2, v2, temp);

        scale = 0.5f * size * arrow_sin[i + 1];
        f3mul(temp, up, scale);
        f3add(v2, v2, temp);

        glLine(v1, to);
        glLine(v1, v2);
    }
}
static void
glSpheref(float cx, float cy, float cz, const float radius)
{
    #define STEP_SIZE 15
    float last[3], tmp[3], radius_vec[3];
    float cache[(360 / STEP_SIZE)*3];
    int j = 0, i = 0, n = 0;

    f3set(radius_vec,0,0,radius);
    f3set(&cache[0], cx,cy,cz+radius_vec[2]);
    for (n = 1; n < (cntof(cache)/3); ++n)
        f3cpy(&cache[n*3], &cache[0]);

    /* first circle iteration */
    for (i = STEP_SIZE; i <= 360; i += STEP_SIZE) {
        const float s = sinf(i*TO_RAD);
        const float c = cosf(i*TO_RAD);

        last[0] = cx;
        last[1] = cy + radius * s;
        last[2] = cz + radius * c;

        /* second circle iteration */
        for (n = 0, j = STEP_SIZE; j <= 360; j += STEP_SIZE, ++n) {
            tmp[0] = cx + sinf(TO_RAD*j)*radius*s;
            tmp[1] = cy + cosf(TO_RAD*j)*radius*s;
            tmp[2] = last[2];

            glLine(last, tmp);
            glLine(last, &cache[n*3]);

            f3cpy(&cache[n*3], last);
            f3cpy(last, tmp);
        }
    }
    #undef STEP_SIZE
}
static void
glCapsulef(float fx, float fy, float fz, float tx, float ty, float tz, float r)
{
    int i = 0;
    static const int step_size = 20;
    float up[3], right[3], forward[3];
    float from[3], to[3];
    float lastf[3], lastt[3];

    /* calculate axis */
    f3set(from,fx,fy,fz);
    f3set(to,tx,ty,tz);
    f3sub(forward, to, from);
    f3norm(forward);
    f3ortho(right, up, forward);

    /* calculate first two cone verts (buttom + top) */
    f3mul(lastf, up,r);
    f3add(lastt, to,lastf);
    f3add(lastf, from,lastf);

    /* step along circle outline and draw lines */
    for (i = step_size; i <= 360; i += step_size) {
        /* calculate current rotation */
        float ax[3], ay[3], pf[3], pt[3], tmp[3];
        f3mul(ax, right, sinf(i*TO_RAD));
        f3mul(ay, up, cosf(i*TO_RAD));

        /* calculate current vertices on cone */
        f3add(tmp, ax, ay);
        f3mul(pf, tmp, r);
        f3mul(pt, tmp, r);

        f3add(pf, pf, from);
        f3add(pt, pt, to);

        /* draw cone vertices */
        glLine(lastf, pf);
        glLine(lastt, pt);
        glLine(pf, pt);

        f3cpy(lastf, pf);
        f3cpy(lastt, pt);

        /* calculate first top sphere vert */
        float prevt[3], prevf[3];
        f3mul(prevt, tmp, r);
        f3add(prevf, prevt, from);
        f3add(prevt, prevt, to);

        /* sphere (two half spheres )*/
        for (int j = 1; j < 180/step_size; j++) {
            /* angles */
            float ta = j*step_size;
            float fa = 360-(j*step_size);
            float t[3];

            /* top half-sphere */
            f3mul(ax, forward, sinf(ta*TO_RAD));
            f3mul(ay, tmp, cosf(ta*TO_RAD));

            f3add(t, ax, ay);
            f3mul(pf, t, r);
            f3add(pf, pf, to);
            glLine(pf, prevt);
            f3cpy(prevt, pf);

            /* bottom half-sphere */
            f3mul(ax, forward, sinf(fa*TO_RAD));
            f3mul(ay, tmp, cosf(fa*TO_RAD));

            f3add(t, ax, ay);
            f3mul(pf, t, r);
            f3add(pf, pf, from);
            glLine(pf, prevf);
            f3cpy(prevf, pf);
        }
    }
}
static void
pyramid(float *dst, float fx, float fy, float fz, float tx, float ty, float tz, float size)
{
    float from[3];
    float *a = dst + 0;
    float *b = dst + 3;
    float *c = dst + 6;
    float *d = dst + 9;
    float *r = dst + 12;

    /* calculate axis */
    float up[3], right[3], forward[3];
    f3set(from,fx,fy,fz);
    f3set(r,tx,ty,tz);
    f3sub(forward, r, from);
    f3norm(forward);
    f3ortho(right, up, forward);

    /* calculate extend */
    float xext[3], yext[3];
    float nxext[3], nyext[3];
    f3mul(xext, right, size);
    f3mul(yext, up, size);
    f3mul(nxext, right, -size);
    f3mul(nyext, up, -size);

    /* calculate base vertexes */
    f3add(a, from, xext);
    f3add(a, a, yext);

    f3add(b, from, xext);
    f3add(b, b, nyext);

    f3add(c, from, nxext);
    f3add(c, c, nyext);

    f3add(d, from, nxext);
    f3add(d, d, yext);
}
static void
glPyramidf(float fx, float fy, float fz, float tx, float ty, float tz, float size)
{
    float pyr[16];
    pyramid(pyr, fx, fy, fz, tx, ty, tz, size);
    float *a = pyr + 0;
    float *b = pyr + 3;
    float *c = pyr + 6;
    float *d = pyr + 9;
    float *r = pyr + 12;

    /* draw vertexes */
    glLine(a, b);
    glLine(b, c);
    glLine(c, d);
    glLine(d, a);

    /* draw root */
    glLine(a, r);
    glLine(b, r);
    glLine(c, r);
    glLine(d, r);
}
static void
glGrid(const float mins, const float maxs, const float y, const float step)
{
    float i;
    float from[3], to[3];
    for (i = mins; i <= maxs; i += step) {
        /* Horizontal line (along the X) */
        f3set(from, mins,y,i);
        f3set(to, maxs,y,i);
        glLine(from,to);
        /* Vertical line (along the Z) */
        f3set(from, i,y,mins);
        f3set(to, i,y,maxs);
        glLine(from,to);
    }
}
static void
glBounds(const float *points)
{
    int i = 0;
    for (i = 0; i < 4; ++i) {
        glLine(&points[i*3], &points[((i + 1) & 3)*3]);
        glLine(&points[(4 + i)*3], &points[(4 + ((i + 1) & 3))*3]);
        glLine(&points[i*3], &points[(4 + i)*3]);
    }
}
static void
glBoxf(float cx, float cy, float cz, float w, float h, float d)
{
    float pnts[8*3];
    w  = w*0.5f;
    h  = h*0.5f;
    d  = d*0.5f;

    #define DD_BOX_V(v, op1, op2, op3)\
        (v)[0] = cx op1 w; (v)[1] = cy op2 h; (v)[2] = cz op3 d
    DD_BOX_V(&pnts[0*3], -, +, +);
    DD_BOX_V(&pnts[1*3], -, +, -);
    DD_BOX_V(&pnts[2*3], +, +, -);
    DD_BOX_V(&pnts[3*3], +, +, +);
    DD_BOX_V(&pnts[4*3], -, -, +);
    DD_BOX_V(&pnts[5*3], -, -, -);
    DD_BOX_V(&pnts[6*3], +, -, -);
    DD_BOX_V(&pnts[7*3], +, -, +);
    #undef DD_BOX_V
    glBounds(pnts);
}
static void
glAabb(const float *bounds)
{
    int i = 0;
    float bb[2*3], pnts[8*3];
    f3cpy(&bb[0], bounds);
    f3cpy(&bb[3], bounds+3);
    for (i = 0; i < cntof(pnts)/3; ++i) {
        pnts[i*3+0] = bb[(((i ^ (i >> 1)) & 1)*3+0)];
        pnts[i*3+1] = bb[(((i >> 1) & 1)*3)+1];
        pnts[i*3+2] = bb[(((i >> 2) & 1)*3)+2];
    } glBounds(pnts);
}

/* ============================================================================
 *
 *                                  SYSTEM
 *
 * =========================================================================== */
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

typedef struct sys_int2 {int x,y;} sys_int2;
enum {
    SYS_FALSE = 0,
    SYS_TRUE = 1,
    SYS_MAX_KEYS = 256
};
struct sys_button {
    unsigned int down:1;
    unsigned int pressed:1;
    unsigned int released:1;
};
enum sys_mouse_mode {
    SYSTEM_MOUSE_ABSOLUTE,
    SYSTEM_MOUSE_RELATIVE
};
struct sys_mouse {
    enum sys_mouse_mode mode;
    unsigned visible:1;
    sys_int2 pos;
    sys_int2 pos_last;
    sys_int2 pos_delta;
    float scroll_delta;
    struct sys_button left_button;
    struct sys_button right_button;
};
struct sys_time {
    float refresh_rate;
    float last;
    float delta;
    unsigned int start;
};
enum sys_window_mode {
    SYSTEM_WM_WINDOWED,
    SYSTEM_WM_FULLSCREEN,
    SYSTEM_WM_MAX,
};
struct sys_window {
    const char *title;
    SDL_Window *handle;
    SDL_GLContext glContext;
    enum sys_window_mode mode;
    sys_int2 pos;
    sys_int2 size;
    unsigned resized:1;
    unsigned moved:1;
};
struct sys {
    unsigned quit:1;
    unsigned initialized:1;
    struct sys_time time;
    struct sys_window win;
    struct sys_button key[SYS_MAX_KEYS];
    struct sys_mouse mouse;
};
static void
sys_init(struct sys *s)
{
    int monitor_refresh_rate;
    int win_x, win_y, win_w, win_h;
    const char *title;

    SDL_Init(SDL_INIT_VIDEO);
    title = s->win.title ? s->win.title: "SDL";
    win_x = s->win.pos.x ? s->win.pos.x: SDL_WINDOWPOS_CENTERED;
    win_y = s->win.pos.y ? s->win.pos.y: SDL_WINDOWPOS_CENTERED;
    win_w = s->win.size.x ? s->win.size.x: 800;
    win_h = s->win.size.y ? s->win.size.y: 600;
    s->win.handle = SDL_CreateWindow(title, win_x, win_y, win_w, win_h, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    s->win.glContext = SDL_GL_CreateContext(s->win.handle);
    s->mouse.visible = SYS_TRUE;

    monitor_refresh_rate = 60;
    s->time.refresh_rate = 1.0f/(float)monitor_refresh_rate;
    s->initialized = SYS_TRUE;
}
static void
sys_shutdown(struct sys *s)
{
    SDL_GL_DeleteContext(s->win.glContext);
    SDL_DestroyWindow(s->win.handle);
    SDL_Quit();
}
static void
sys_update_button(struct sys_button *b, unsigned down)
{
    int was_down = b->down;
    b->down = down;
    b->pressed = !was_down && down;
    b->released = was_down && !down;
}
static void
sys_pull(struct sys *s)
{
    int i = 0;
    SDL_PumpEvents();
    s->time.start = SDL_GetTicks();
    s->mouse.left_button.pressed = 0;
    s->mouse.left_button.released = 0;
    s->mouse.right_button.pressed = 0;
    s->mouse.right_button.released = 0;
    s->mouse.scroll_delta = 0;

    /* window-mode */
    if (s->win.mode == SYSTEM_WM_WINDOWED)
        SDL_SetWindowFullscreen(s->win.handle, 0);
    else SDL_SetWindowFullscreen(s->win.handle, SDL_WINDOW_FULLSCREEN_DESKTOP);

    /* poll window */
    s->win.moved = 0;
    s->win.resized = 0;
    SDL_GetWindowSize(s->win.handle, &s->win.size.x, &s->win.size.y);
    SDL_GetWindowPosition(s->win.handle, &s->win.pos.x, &s->win.pos.y);

    /* poll keyboard */
    {const unsigned char* keys = SDL_GetKeyboardState(0);
    for (i = 0; i < SYS_MAX_KEYS; ++i) {
        SDL_Keycode sym = SDL_GetKeyFromScancode((SDL_Scancode)i);
        if (sym < SYS_MAX_KEYS)
            sys_update_button(s->key + sym, keys[i]);
    }}
    /* poll mouse */
    s->mouse.pos_delta.x = 0;
    s->mouse.pos_delta.y = 0;
    SDL_SetRelativeMouseMode(s->mouse.mode == SYSTEM_MOUSE_RELATIVE);
    if (s->mouse.mode == SYSTEM_MOUSE_ABSOLUTE) {
        s->mouse.pos_last.x = s->mouse.pos.x;
        s->mouse.pos_last.y = s->mouse.pos.y;
        SDL_GetMouseState(&s->mouse.pos.x, &s->mouse.pos.y);
        s->mouse.pos_delta.x = s->mouse.pos.x - s->mouse.pos_last.x;
        s->mouse.pos_delta.y = s->mouse.pos.y - s->mouse.pos_last.y;
    } else {
        s->mouse.pos_last.x = s->win.size.x/2;
        s->mouse.pos_last.y = s->win.size.y/2;
        s->mouse.pos.x = s->win.size.x/2;
        s->mouse.pos.y = s->win.size.y/2;
    }
    /* handle events */
    {SDL_Event evt;
    while (SDL_PollEvent(&evt)) {
        if (evt.type == SDL_QUIT)
            s->quit = SYS_TRUE;
        else if (evt.type == SDL_MOUSEBUTTONDOWN ||  evt.type == SDL_MOUSEBUTTONUP) {
            unsigned down = evt.type == SDL_MOUSEBUTTONDOWN;
            if (evt.button.button == SDL_BUTTON_LEFT)
                sys_update_button(&s->mouse.left_button, down);
            else if (evt.button.button == SDL_BUTTON_RIGHT)
                sys_update_button(&s->mouse.right_button, down);
        } else if (evt.type == SDL_WINDOWEVENT) {
            if (evt.window.event == SDL_WINDOWEVENT_MOVED)
                s->win.moved = SYS_TRUE;
            else if (evt.window.event == SDL_WINDOWEVENT_RESIZED)
                s->win.resized = SYS_TRUE;
        } else if (evt.type == SDL_MOUSEMOTION) {
            s->mouse.pos_delta.x += evt.motion.xrel;
            s->mouse.pos_delta.y += evt.motion.yrel;
        } else if (evt.type == SDL_MOUSEWHEEL) {
            s->mouse.scroll_delta = -evt.wheel.y;
        }
    }}
}
static void
sys_push(struct sys *s)
{
    SDL_GL_SwapWindow(s->win.handle);
    s->time.delta = (SDL_GetTicks() - s->time.start) / 1000.0f;
    if (s->time.delta < s->time.refresh_rate)
        SDL_Delay((Uint32)((s->time.refresh_rate - s->time.delta) * 1000.0f));
}

/* ============================================================================
 *
 *                                  APP
 *
 * =========================================================================== */
int main(void)
{
    /* Platform */
    struct sys sys;
    memset(&sys, 0, sizeof(sys));
    sys.win.title = "Demo";
    sys.win.size.x = 1200;
    sys.win.size.y = 800;
    sys_init(&sys);

    /* Camera */
    struct cam cam;
    cam_init(&cam);
    cam.pos[2] = 5;
    cam_build(&cam);

    /* Main */
    while (!sys.quit)
    {
        sys_pull(&sys);

        /* Camera control */
        #define CAMERA_SPEED (10.0f)
        {const float frame_movement = CAMERA_SPEED * sys.time.delta;
        const float forward = (sys.key['w'].down ? -1 : 0.0f) + (sys.key['s'].down ? 1 : 0.0f);
        const float right = (sys.key['a'].down ? -1 : 0.0f) + (sys.key['d'].down ? 1 : 0.0f);
        if (sys.mouse.right_button.down) {
            sys.mouse.mode = SYSTEM_MOUSE_RELATIVE;
            cam.ear[0] = (float)sys.mouse.pos_delta.y * TO_RAD;
            cam.ear[1] = (float)sys.mouse.pos_delta.x * TO_RAD;
        } else sys.mouse.mode = SYSTEM_MOUSE_ABSOLUTE;
        if (sys.key['e'].down)
            cam.ear[2] = 1.0f * TO_RAD;
        if (sys.key['r'].down)
            cam.ear[2] = -1.0f * TO_RAD;
        cam.off[2] += sys.mouse.scroll_delta;
        cam.aspect_ratio = (float)sys.win.size.x/(float)sys.win.size.y;
        cam_move(&cam, right * frame_movement, 0, forward * frame_movement);
        cam_build(&cam);}

        /* Keybindings */
        if (sys.key['f'].pressed)
            sys.win.mode = !sys.win.mode;
        if (sys.key['b'].pressed)
            printf("break!\n");

        /* Rendering */
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glClearColor(0.15f, 0.15f, 0.15f, 1);
        glViewport(0, 0, sys.win.size.x, sys.win.size.y);
        {
            /* 3D */
            glEnable(GL_DEPTH_TEST);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glLoadMatrixf((float*)cam.proj);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glLoadMatrixf((float*)cam.view);

            /* grid */
            glColor3f(0.2f,0.2f,0.2f);
            glGrid(-50.0f, 50.0f, -1.0f, 1.7f);

            /* basis-axis */
            glColor3f(1,0,0);
            glArrowf(0,0,0, 1,0,0, 0.1f);
            glColor3f(0,1,0);
            glArrowf(0,0,0, 0,1,0, 0.1f);
            glColor3f(0,0,1);
            glArrowf(0,0,0, 0,0,1, 0.1f);

            {
                /* Triangle-Ray Intersection*/
                static float dx = 0, dy = 0;
                float ro[3], rd[3];
                int suc;

                v3 tri[3];
                tri[0] = v3mk(-9,1,28);
                tri[1] = v3mk(-10,0,28);
                tri[2] = v3mk(-11,1,28);

                /* ray */
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;
                f3set(ro, -10,-1,20);
                f3set(rd, -10+0.4f*sinf(dx), 2.0f*cosf(dy), 29.81023f);
                f3sub(rd, rd, ro);
                f3norm(rd);

                struct raycast hit;
                struct ray r = rayfv(ro, rd);
                if ((suc = ray_test_triangle(&hit, r, tri))) {
                    /* point of intersection */
                    glColor3f(1,0,0);
                    glBox(&hit.p.x, 0.10f, 0.10f, 0.10f);

                    /* intersection normal */
                    glColor3f(0,0,1);
                    v3 v = v3add(hit.p, hit.n);
                    glArrow(&hit.p.x, &v.x, 0.05f);
                }

                /* line */
                glColor3f(1,0,0);
                f3mul(rd,rd,10);
                f3add(rd,ro,rd);
                glLine(ro, rd);

                /* triangle */
                if (suc) glColor3f(1,0,0);
                else glColor3f(1,1,1);
                glTriangle(&tri[0].x,&tri[1].x,&tri[2].x);
            }
            {
                /* Plane-Ray Intersection*/
                static float d = 0;
                struct ray ray;
                struct plane plane;
                struct raycast hit;
                float ro[3], rd[3];
                int suc = 0;
                m3 rot;

                /* ray */
                d += sys.time.delta * 2.0f;
                f3set(ro, 0,-1,20);
                f3set(rd, 0.1f, 0.5f, 9.81023f);
                f3sub(rd, rd, ro);
                f3norm(rd);

                /* rotation */
                m3cpy(rot, m3id);
                m3rot((float*)rot, d, 0,1,0);
                m3mulv(rd,(float*)rot,rd);

                /* intersection */
                ray = rayfv(ro, rd);
                plane = planefv(0,0,28, 0,0,1);
                if ((suc = ray_test_plane(&hit, ray, plane))) {
                    /* point of intersection */
                    glColor3f(1,0,0);
                    glBox(&hit.p.x, 0.10f, 0.10f, 0.10f);

                    /* intersection normal */
                    glColor3f(0,0,1);
                    v3 v = v3add(hit.p, hit.n);
                    glArrow(&hit.p.x, &v.x, 0.05f);
                    glColor3f(1,0,0);
                }
                /* line */
                glColor3f(1,0,0);
                f3mul(rd,rd,9);
                f3add(rd,ro,rd);
                glLine(ro, rd);

                /* plane */
                if (suc) glColor3f(1,0,0);
                else glColor3f(1,1,1);
                glPlanef(0,0,28, 0,0,1, 3.0f);
            }
            {
                /* Sphere-Ray Intersection*/
                static float dx = 0, dy = 0;
                float ro[3], rd[3];
                struct sphere s;
                struct raycast hit;
                struct ray r;
                int suc;

                /* ray */
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                f3set(ro, 0,-1,0);
                f3set(rd, 0.4f*sinf(dx), 2.0f*cosf(dy), 9.81023f);
                f3sub(rd, rd, ro);
                f3norm(rd);

                r = rayfv(ro, rd);
                s = sphere(v3mk(0,0,8), 1);
                if ((suc = ray_test_sphere(&hit, r, s))) {
                    /* points of intersection */
                    float in[3];
                    f3mul(in,rd,hit.t0);
                    f3add(in,ro,in);

                    glColor3f(1,0,0);
                    glBox(in, 0.05f, 0.05f, 0.05f);

                    f3mul(in,rd,hit.t1);
                    f3add(in,ro,in);

                    glColor3f(1,0,0);
                    glBox(in, 0.05f, 0.05f, 0.05f);

                    /* intersection normal */
                    glColor3f(0,0,1);
                    v3 v = v3add(hit.p, hit.n);
                    glArrow(&hit.p.x, &v.x, 0.05f);
                    glColor3f(1,0,0);
                }
                /* line */
                glColor3f(1,0,0);
                f3mul(rd,rd,10);
                f3add(rd,ro,rd);
                glLine(ro, rd);

                /* sphere */
                if (suc) glColor3f(1,0,0);
                else glColor3f(1,1,1);
                glSpheref(0,0,8, 1);
            }
            {
                /* AABB-Ray Intersection*/
                static float dx = 0, dy = 0;
                struct ray ray;
                struct aabb bounds;
                struct raycast hit;
                int suc = 0;
                float ro[3], rd[3];

                /* ray */
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                f3set(ro, 10,-1,0);
                f3set(rd, 10+0.4f*sinf(dx), 2.0f*cosf(dy), 9.81023f);
                f3sub(rd, rd, ro);
                f3norm(rd);

                ray = rayfv(ro, rd);
                bounds = aabb(v3mk(10-0.5f,-0.5f,7.5f), v3mk(10.5f,0.5f,8.5f));
                if ((suc = ray_test_aabb(&hit, ray, bounds))) {
                    /* points of intersection */
                    float in[3];
                    f3mul(in,rd,hit.t0);
                    f3add(in,ro,in);

                    glColor3f(1,0,0);
                    glBox(in, 0.05f, 0.05f, 0.05f);

                    f3mul(in,rd,hit.t1);
                    f3add(in,ro,in);

                    glColor3f(1,0,0);
                    glBox(in, 0.05f, 0.05f, 0.05f);

                    /* intersection normal */
                    glColor3f(0,0,1);
                    v3 v = v3add(hit.p, hit.n);
                    glArrow(&hit.p.x, &v.x, 0.05f);
                    glColor3f(1,0,0);
                } else glColor3f(1,1,1);
                glBoxf(10,0,8, 1,1,1);

                /* line */
                glColor3f(1,0,0);
                f3mul(rd,rd,10);
                f3add(rd,ro,rd);
                glLine(ro, rd);
            }
            {
                /* Sphere-Sphere intersection*/
                int suc = 0;
                struct manifold m;
                static float dx = 0, dy = 0;
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                struct sphere a = sphere(v3mk(-10,0,8), 1);
                struct sphere b = sphere(v3mk(-10+0.6f*sinf(dx), 3.0f*cosf(dy),8), 1);
                if ((suc = sphere_test_sphere_manifold(&m, a, b))) {
                    float v[3];
                    glColor3f(0,0,1);
                    glBox(m.contact_point, 0.05f, 0.05f, 0.05f);
                    f3add(v, m.contact_point, m.normal);
                    glArrow(m.contact_point, v, 0.05f);
                    glColor3f(1,0,0);
                } else glColor3f(1,1,1);

                glSphere(&a.p.x, 1);
                glSphere(&b.p.x, 1);
            }
            {
                /* AABB-AABB intersection*/
                int suc = 0;
                struct manifold m;
                static float dx = 0, dy = 0;
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                const float x = 10+0.6f*sinf(dx);
                const float y = 3.0f*cosf(dy);
                const float z = 20.0f;

                struct aabb a = aabb(v3mk(10-0.5f,-0.5f,20-0.5f), v3mk(10+0.5f,0.5f,20.5f));
                struct aabb b = aabbf(x-0.5f,y-0.5f,z-0.5f, x+0.5f,y+0.5f,z+0.5f);
                if ((suc = aabb_test_aabb_manifold(&m, a, b))) {
                    float v[3];
                    glColor3f(0,0,1);
                    glBox(m.contact_point, 0.05f, 0.05f, 0.05f);
                    f3add(v, m.contact_point, m.normal);
                    glArrow(m.contact_point, v, 0.05f);
                    glColor3f(1,0,0);
                } else glColor3f(1,1,1);

                glBoxf(10,0,20, 1,1,1);
                glBoxf(x,y,z, 1,1,1);
            }
            {
                /* Capsule-Capsule intersection*/
                int suc = 0;
                struct manifold m;
                static float dx = 0, dy = 0;
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                const float x = 20+0.4f*sinf(dx);
                const float y = 3.0f*cosf(dy);
                const float z = 28.5f;

                struct capsule a = capsulef(20.0f,-1.0f,28.0f, 20.0f,1.0f,28.0f, 0.2f);
                struct capsule b = capsulef(x,y-1.0f,z, x,y+1.0f,z-1.0f, 0.2f);
                if ((suc = capsule_test_capsule_manifold(&m, a, b))) {
                    float v[3];
                    glColor3f(0,0,1);
                    glBox(m.contact_point, 0.05f, 0.05f, 0.05f);
                    f3add(v, m.contact_point, m.normal);
                    glArrow(m.contact_point, v, 0.05f);
                    glColor3f(1,0,0);
                } else glColor3f(1,1,1);
                glCapsulef(x,y-1.0f,z, x,y+1.0f,z-1.0f, 0.2f);
                glCapsulef(20.0f,-1.0f,28.0f, 20.0f,1.0f,28.0f, 0.2f);
            }
            {
                /* AABB-Sphere intersection*/
                int suc = 0;
                struct manifold m;
                static float dx = 0, dy = 0;
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                struct aabb a = aabb(v3mk(20-0.5f,-0.5f,7.5f), v3mk(20.5f,0.5f,8.5f));
                struct sphere s = sphere(v3mk(20+0.6f*sinf(dx), 3.0f*cosf(dy),8), 1);
                if ((suc = aabb_test_sphere_manifold(&m, a, s))) {
                    float v[3];
                    glColor3f(0,0,1);
                    glBox(m.contact_point, 0.05f, 0.05f, 0.05f);
                    f3add(v, m.contact_point, m.normal);
                    glArrow(m.contact_point, v, 0.05f);
                    glColor3f(1,0,0);
                } else glColor3f(1,1,1);

                glBoxf(20,0,8, 1,1,1);
                glSphere(&s.p.x, 1);
            }
            {
                /* Sphere-AABB intersection*/
                int suc = 0;
                struct manifold m;
                static float dx = 0, dy = 0;
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                const float x = 10+0.6f*sinf(dx);
                const float y = 3.0f*cosf(dy);
                const float z = -8.0f;

                struct sphere s = sphere(v3mk(10,0,-8), 1);
                struct aabb a = aabbf(x-0.5f,y-0.5f,z-0.5f, x+0.5f,y+0.5f,z+0.5f);
                if ((suc = sphere_test_aabb_manifold(&m, s, a))) {
                    float v[3];
                    glColor3f(0,0,1);
                    glBox(m.contact_point, 0.05f, 0.05f, 0.05f);
                    f3add(v, m.contact_point, m.normal);
                    glArrow(m.contact_point, v, 0.05f);
                    glColor3f(1,0,0);
                } else glColor3f(1,1,1);

                glBoxf(x,y,z, 1,1,1);
                glSphere(&s.p.x, 1);
            }
            {
                /* Capsule-Sphere intersection*/
                int suc = 0;
                struct manifold m;
                static float dx = 0, dy = 0;
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                struct capsule c = capsulef(-20.5f,-1.0f,7.5f, -20+0.5f,1.0f,8.5f, 0.2f);
                struct sphere b = sphere(v3mk(-20+0.6f*sinf(dx), 3.0f*cosf(dy),8), 1);
                if ((suc = capsule_test_sphere_manifold(&m, c, b))) {
                    float v[3];
                    glColor3f(0,0,1);
                    glBox(m.contact_point, 0.05f, 0.05f, 0.05f);
                    f3add(v, m.contact_point, m.normal);
                    glArrow(m.contact_point, v, 0.05f);
                    glColor3f(1,0,0);
                } else glColor3f(1,1,1);
                glSpheref(b.p.x, b.p.y, b.p.z, 1);
                glCapsulef(-20.5f,-1.0f,7.5f, -20+0.5f,1.0f,8.5f, 0.2f);
            }
            {
                /* Sphere-Capsule intersection*/
                int suc = 0;
                struct manifold m;
                static float dx = 0, dy = 0;
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                const float x = 20+0.4f*sinf(dx);
                const float y = 3.0f*cosf(dy);
                const float z = -8;

                struct sphere s = sphere(v3mk(20,0,-8), 1);
                struct capsule c = capsulef(x,y-1.0f,z, x,y+1.0f,z-1.0f, 0.2f);
                if ((suc = sphere_test_capsule_manifold(&m, s, c))) {
                    float v[3];
                    glColor3f(0,0,1);
                    glBox(m.contact_point, 0.05f, 0.05f, 0.05f);
                    f3add(v, m.contact_point, m.normal);
                    glArrow(m.contact_point, v, 0.05f);
                    glColor3f(1,0,0);
                } else glColor3f(1,1,1);

                glCapsulef(x,y-1.0f,z, x,y+1.0f,z-1.0f, 0.2f);
                glSphere(&s.p.x, 1);
            }
            {
                /* Capsule-AABB intersection*/
                int suc = 0;
                struct manifold m;
                static float dx = 0, dy = 0;
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                const float x = -20+0.6f*sinf(dx);
                const float y = 3.0f*cosf(dy);
                const float z = 28.0f;

                struct capsule c = capsulef(-20.5f,-1.0f,27.5f, -20+0.5f,1.0f,28.5f, 0.2f);
                struct aabb b = aabbf(x-0.5f,y-0.5f,z-0.5f, x+0.5f,y+0.5f,z+0.5f);
                if ((suc = capsule_test_aabb_manifold(&m, c, b))) {
                    float v[3];
                    glColor3f(0,0,1);
                    glBox(m.contact_point, 0.05f, 0.05f, 0.05f);
                    f3add(v, m.contact_point, m.normal);
                    glArrow(m.contact_point, v, 0.05f);
                    glColor3f(1,0,0);
                } else glColor3f(1,1,1);
                glBoxf(x,y,z, 1,1,1);
                glCapsulef(-20.5f,-1.0f,27.5f, -20+0.5f,1.0f,28.5f, 0.2f);
            }
            {
                /* AABB-Capsule intersection*/
                int suc = 0;
                struct manifold m;
                static float dx = 0, dy = 0;
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                const float x = 0.4f*sinf(dx);
                const float y = 3.0f*cosf(dy);
                const float z = -8;

                struct aabb a = aabb(v3mk(-0.5f,-0.5f,-8.5f), v3mk(0.5f,0.5f,-7.5f));
                struct capsule c = capsulef(x,y-1.0f,z, x,y+1.0f,z-1.0f, 0.2f);
                if ((suc = aabb_test_capsule_manifold(&m, a, c))) {
                    float v[3];
                    glColor3f(0,0,1);
                    glBox(m.contact_point, 0.05f, 0.05f, 0.05f);
                    f3add(v, m.contact_point, m.normal);
                    glArrow(m.contact_point, v, 0.05f);
                    glColor3f(1,0,0);
                } else glColor3f(1,1,1);

                glCapsulef(x,y-1.0f,z, x,y+1.0f,z-1.0f, 0.2f);
                glBoxf(0,0,-8.0f, 1,1,1);
            }
            {
                /* Polyhedron-Sphere intersection*/
                static float dx = 0, dy = 0;
                dx = dx + sys.time.delta * 2.0f;
                dy = dy + sys.time.delta * 0.8f;

                float pyr[15];
                struct sphere s = sphere(v3mk(-10+0.6f*sinf(dx), 3.0f*cosf(dy),-8), 1);
                pyramid(pyr, -10.5f,-0.5f,-7.5f, -10.5f,0.5f,-7.5f, 1.0f);
                if (polyhedron_intersect_sphere(pyr, 5, &s.p.x, s.r))
                    glColor3f(1,0,0);
                glColor3f(1,1,1);

                glSphere(&s.p.x, 1);
                glPyramidf(-10.5f,-0.5f,-7.5f, -10.5f,1.0f,-7.5f, 1.0f);
            }
        }
        sys_push(&sys);
    }
    sys_shutdown(&sys);
    return 0;
}
