#ifndef APP_VEC_H
#define APP_VEC_H
typedef struct
{
    double x, y;
} vec2;

vec2 vec2_make(double x, double y);
vec2 vec2_add(vec2 v1, vec2 v2);
vec2 vec2_sub(vec2 v1, vec2 v2);
// vec2 vec2_mulv(vec2 v1, vec2 v2);
// vec2 vec2_mul(vec2 v1, double d);
double vec2_angle(vec2 v);
vec2 vec2_norm_x(vec2 v);
vec2 vec2_norm_y(vec2 v);
vec2 vec2_perp(vec2 v, double scale);
double vec2_dist_sq(vec2 v1, vec2 v2);
double vec2_len_sq(vec2 v);
double vec2_len(vec2 v);
double vec2_dist(vec2 v1, vec2 v2);
vec2 vec2_norm(vec2 v);
vec2 vec2_rotate(vec2 v, double angle);
#endif

#ifdef APP_VEC_IMPLEMENTATION
#undef APP_VEC_IMPLEMENTATION

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* vec2 */

vec2 vec2_make(double x, double y)
{
    vec2 v;
    v.x = x;
    v.y = y;
    return v;
}

vec2 vec2_add(vec2 v1, vec2 v2)
{
    vec2 v;
    v.x = v1.x + v2.x;
    v.y = v1.y + v2.y;
    return v;
}

vec2 vec2_sub(vec2 v1, vec2 v2)
{
    vec2 v;
    v.x = v1.x - v2.x;
    v.y = v1.y - v2.y;
    return v;
}

/*
vec2 vec2_mul(vec2 v1, double d)
{
    vec2 v;
    v.x = v1.x * d;
    v.y = v1.y * d;
    return v;
}

vec2 vec2_mulv(vec2 v1, vec2 v2)
{
    vec2 v;
    v.x = v1.x * v2.x;
    v.y = v1.y * v2.y;
    return v;
}
*/

double vec2_angle(vec2 v)
{
    double a = atan2(v.y, v.x);
    return a < 0 ? a + 2.0 * M_PI : a;
}

vec2 vec2_norm_x(vec2 v)
{
    double scaleFactor = 1.0 / v.x;
    v.x = 1.0;
    v.y = v.y * scaleFactor;
    return v;
}

vec2 vec2_norm_y(vec2 v)
{
    double scaleFactor = 1.0 / v.y;
    v.x = v.x * scaleFactor;
    v.y = 1.0;
    return v;
}

vec2 vec2_perp(vec2 v, double scale)
{
    v = vec2_make(v.y, -v.x);
    v.x *= scale;
    v.y *= scale;
    return v;
}

double vec2_dist_sq(vec2 v1, vec2 v2)
{
    return (v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y);
}

double vec2_len_sq(vec2 v)
{
    double len = 0.0;
    len += v.x * v.x;
    len += v.y * v.y;
    return len;
}

double vec2_len(vec2 v)
{
    return sqrt(vec2_len_sq(v));
}

double vec2_dist(vec2 v1, vec2 v2)
{
    return sqrt(vec2_dist_sq(v1, v2));
}

vec2 vec2_norm(vec2 v)
{
    double len = vec2_len(v);
    if (len > 0) {
	v.x /= len;
	v.y /= len;
    }
    return v;
}

vec2 vec2_rotate(vec2 v, double angle)
{
    vec2 result = {
	v.x * cos(angle) - v.y * sin(angle),
	v.x * sin(angle) + v.y * cos(angle)
    };
    return result;
}

#endif
