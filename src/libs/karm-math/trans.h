#pragma once

#include <karm-io/fmt.h>

#include "edge.h"
#include "vec.h"

namespace Karm::Math {

template <typename T>
union Trans2 {
    struct
    {
        T xx;
        T xy;
        T yx;
        T yy;

        T ox;
        T oy;
    };

    struct
    {
        Vec2<T> x;
        Vec2<T> y;
        Vec2<T> o;
    };

    Array<T, 6> _els{};

    bool rotated() const {
        return xx * yy - xy * yx < 0;
    }

    bool skewed() const {
        return xx * yy - xy * yx != 1;
    }

    bool scaled() const {
        return xx != 1 or yy != 1;
    }

    bool translated() const {
        return ox != 0 or oy != 0;
    }

    bool simple() const {
        return not rotated() and not skewed();
    }

    constexpr Trans2()
        : _els{
              1, 0,
              0, 1,
              0, 0
          } {}

    constexpr Trans2(T xx, T xy, T yx, T yy, T ox, T oy)
        : _els{xx, xy, yx, yy, ox, oy} {}

    static constexpr Trans2 identity() {
        return {
            1, 0,
            0, 1,
            0, 0
        };
    }

    static constexpr Trans2 rotate(T angle) {
        T c = cos(angle);
        T s = sin(angle);
        return {
            c, -s,
            s, c,
            0, 0
        };
    }

    static constexpr Trans2 skew(T x, T y) {
        return {
            1, x,
            y, 1,
            0, 0
        };
    }

    static constexpr Trans2 skew(Vec2<T> v) {
        return skew(v.x, v.y);
    }

    static constexpr Trans2 scale(T x, T y) {
        return {
            x, 0,
            0, y,
            0, 0
        };
    }

    static constexpr Trans2 scale(T s) {
        return scale(s, s);
    }

    static constexpr Trans2 scale(Vec2<T> v) {
        return scale(v.x, v.y);
    }

    static constexpr Trans2 translate(T x, T y) {
        return {
            1, 0,
            0, 1,
            x, y
        };
    }

    static constexpr Trans2 translate(Vec2<T> v) {
        return {
            1, 0,
            0, 1,
            v.x, v.y
        };
    }

    constexpr Vec2<T> applyVector(Vec2<T> v) const {
        return {
            v.x * xx + v.y * yx,
            v.x * xy + v.y * yy,
        };
    }

    constexpr Vec2<T> apply(Vec2<T> v) const {
        return applyVector(v) + o;
    }

    constexpr Edge<T> apply(Edge<T> e) const {
        return {apply(e.start), apply(e.end)};
    }

    constexpr Rect<T> apply(Rect<T> r) const {
        return Rect<T>::fromTwoPoint(
            apply(r.topStart()),
            apply(r.bottomEnd())
        );
    }

    constexpr Trans2 multiply(Trans2 const &other) const {
        Trans2 res = {
            xx * other.xx,
            0.0,
            0.0,
            yy * other.yy,
            ox * other.xx + other.ox,
            oy * other.yy + other.oy,
        };

        if (xy != 0.0 or yx != 0.0 ||
            other.xy != 0.0 or other.yx != 0.0) {
            res.xx += xy * other.yx;
            res.xy += xx * other.xy + xy * other.yy;
            res.yx += yx * other.xx + yy * other.yx;
            res.yy += yx * other.xy;
            res.ox += oy * other.yx;
            res.oy += ox * other.xy;
        }

        return res;
    }

    constexpr Trans2 rotated(T angle) {
        return multiply(rotate(angle));
    }

    constexpr Trans2 skewed(T x, T y) {
        return multiply(skew(x, y));
    }

    constexpr Trans2 scaled(T x, T y) {
        return multiply(scale(x, y));
    }

    constexpr Trans2 scaled(T s) {
        return scaled(s, s);
    }

    constexpr Trans2 translated(T x, T y) {
        return multiply(translate(x, y));
    }

    constexpr Trans2 inverse() const {
        T det = xx * yy - xy * yx;
        return {
            yy / det, -xy / det,
            -yx / det, xx / det,
            (oy * yy - ox * yx) / det,
            (ox * xy - oy * xx) / det
        };
    }

    constexpr bool isIdentity() const {
        return xx == 1 and xy == 0 and
               yx == 0 and yy == 1 and
               ox == 0 and oy == 0;
    }

    constexpr bool hasNan() const {
        return x.hasNan() or y.hasNan() or o.hasNan();
    }

    constexpr Vec2<T> delta() const {
        return {xx, xy};
    }
};

using Trans2i = Trans2<isize>;

using Trans2f = Trans2<f64>;

} // namespace Karm::Math

template <typename T>
ReflectableTemplate$(Math::Trans2<T>, xx, xy, yx, yy, ox, oy);
