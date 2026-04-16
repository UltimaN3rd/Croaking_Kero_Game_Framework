#if !defined(VEC2TYPE) || !defined(VEC2NAME)
#error "You must #define VEC2TYPE and VEC2NAME before #including this file"
#endif

#ifdef __cplusplus
// _Generic not supported in C++. Do not try to use this in C++
#else

#define VEC2CONCAT_(a__, b__) a__ ## b__
#define VEC2CONCAT(a__, b__) VEC2CONCAT_(a__, b__)

#define VEC2FLOATTYPE typeof(_Generic((VEC2TYPE)0, f64: (f64)0, default: (f32)0))

typedef struct {
    VEC2TYPE x, y;
} VEC2NAME;

static inline VEC2NAME VEC2CONCAT(VEC2NAME,_Init) (VEC2TYPE x, VEC2TYPE y) { return (VEC2NAME){x, y}; }
static inline VEC2NAME VEC2CONCAT(VEC2NAME,_Add) (VEC2NAME a, VEC2NAME b) { return (VEC2NAME){a.x + b.x, a.y + b.y}; }
static inline VEC2NAME VEC2CONCAT(VEC2NAME,_Sub) (VEC2NAME a, VEC2NAME b) { return (VEC2NAME){a.x - b.x, a.y - b.y}; }
static inline VEC2NAME VEC2CONCAT(VEC2NAME,_Mul) (VEC2NAME a, VEC2NAME b) { return (VEC2NAME){a.x * b.x, a.y * b.y}; }
static inline v2f VEC2CONCAT(VEC2NAME,_Mulf) (VEC2NAME a, VEC2NAME b) { return (v2f){(VEC2FLOATTYPE)a.x * b.x, (VEC2FLOATTYPE)a.y * b.y}; }
static inline VEC2NAME VEC2CONCAT(VEC2NAME,_MulScalar) (VEC2NAME a, VEC2FLOATTYPE b) { return (VEC2NAME){a.x * b, a.y * b}; }
static inline VEC2NAME VEC2CONCAT(VEC2NAME,_Div) (VEC2NAME a, VEC2NAME b) { return (VEC2NAME){(VEC2FLOATTYPE)a.x / b.x, (VEC2FLOATTYPE)a.y / b.y}; }
static inline v2f VEC2CONCAT(VEC2NAME,_Divf) (VEC2NAME a, VEC2NAME b) { return (v2f){(VEC2FLOATTYPE)a.x / b.x, (VEC2FLOATTYPE)a.y / b.y}; }
static inline VEC2NAME VEC2CONCAT(VEC2NAME,_DivScalar) (VEC2NAME a, VEC2FLOATTYPE b) { return (VEC2NAME){a.x / b, a.y / b}; }
static inline VEC2FLOATTYPE VEC2CONCAT(VEC2NAME,_Dot) (VEC2NAME a, VEC2NAME b) { return (VEC2FLOATTYPE)a.x * b.x + (VEC2FLOATTYPE)a.y * b.y; }
static inline VEC2NAME VEC2CONCAT(VEC2NAME,_Invert) (VEC2NAME a) { return (VEC2NAME){-a.x, -a.y}; }
static inline VEC2FLOATTYPE VEC2CONCAT(VEC2NAME,_Len) (VEC2NAME a) { return sqrtf ((VEC2FLOATTYPE)a.x*a.x + (VEC2FLOATTYPE)a.y*a.y); }
static inline VEC2FLOATTYPE VEC2CONCAT(VEC2NAME,_Scalar) (VEC2NAME a, VEC2NAME inb) { return VEC2CONCAT(VEC2NAME,_Dot) (a, inb) / VEC2CONCAT(VEC2NAME,_Len) (inb); }
static inline v2f VEC2CONCAT(VEC2NAME,_Lerp) (VEC2NAME from, VEC2NAME to, VEC2FLOATTYPE t) { return (v2f) { (to.x - from.x) * t, (to.y - from.y) * t }; }
static inline VEC2FLOATTYPE VEC2CONCAT(VEC2NAME,_Distance) (VEC2NAME a, VEC2NAME b) { return VEC2CONCAT(VEC2NAME,_Len)(VEC2CONCAT(VEC2NAME,_Sub)(a, b)); }
static inline VEC2FLOATTYPE VEC2CONCAT(VEC2NAME,_AngleTurns) (VEC2NAME v) { return XYToAngleTurns (v.x, v.y); }

#endif

#undef VEC2TYPE
#undef VEC2NAME
