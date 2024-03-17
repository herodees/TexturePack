#pragma once

#include <cmath>

#ifndef M_PI
#define M_PI  3.14159265358979323846264f  // from CRC
#endif

namespace box
{
    enum dir
    {
        Right,
        TopRight,
        Top,
        TopLeft,
        Left,
        BottomLeft,
        Bottom,
        BottomRight
    };

    struct dir_pack
    {
        void set(dir d)
        {
            _dir |= (1 << d);
        }
        void reset(dir d)
        {
            _dir &= ~(1 << d);
        }
        bool get(dir d)
        {
            return _dir & (1 << d);
        }
        uint32_t _dir{};
    };

    inline float random_float(float min, float max)
    {
        const float random_0_1 = ((float)rand()) / RAND_MAX;
        return min + random_0_1 * (max - min);
    }

    inline int random_int(int min, int max)
    {
        const float random_0_1 = ((float)rand()) / RAND_MAX;
        return min + int(random_0_1 * (max - min));
    }

    struct rand_generator
    { 
        // Compute a pseudorandom integer.
        // Output value in range [0, 32767]
        inline int randi()
        {
            _seed = (214013 * _seed + 2531011);
            return (_seed >> 16) & 0x7FFF;
        }

        inline float randf()
        {
            return randi() / 32767.f;
        }

        inline float randf(float from, float to)
        {
            return from + (randf() * (to - from));
        }

        unsigned int _seed{};
    };
    constexpr float pow2(float fp)
    {
        return fp * fp;
    }
    constexpr float fract(float fp)
    {
        const float    f    = fp - ((int)fp);
        const uint32_t ux_s = 0x7FFFFFFF & (uint32_t&)f;
        return (float&)(ux_s);
    }

    constexpr float copysign(float fp, float fv)
    {
        const uint32_t ux_s = 0x80000000 & (uint32_t&)fp;
        const uint32_t sgn  = ux_s | (uint32_t&)fv;
        return (float&)(sgn);
    }

    inline float distance_to_block(float pos, bool negative)
    {
        float result;
        if (!negative)
            result = 1.0f - fract(pos);
        else
            result = fract(pos);
        if (result == 0.0f)
            result = 1.0f;
        return result;
    }

    constexpr int qfloor(float x)
    {
        const int i = (int)x;
        return i - (i > x);
    }
    constexpr int qceil(float x)
    {
        const int i = (int)x;
        return i + (i < x);
    }
    constexpr int qround(float x)
    {
        return (x >= 0.0f) ? (int)(x + 0.5f) : (int)(x - 0.5f);
    }

	struct point2f : Vector2
	{
		constexpr point2f() : Vector2{} {}
        template <typename C>
        constexpr point2f(C x, C y) : Vector2{static_cast<float>(x), static_cast<float>(y)} {}
		constexpr point2f(float x, float y) : Vector2{x, y} {}
		constexpr point2f(const Vector2& c) : Vector2{c} {};
		constexpr point2f& operator=(const Vector2& c) { *(Vector2*)(this) = c; return *this; };
		constexpr point2f& operator+=(const point2f& c) { x += c.x; y += c.y; return *this; }
		constexpr point2f& operator-=(const point2f& c) { x -= c.x; y -= c.y; return *this; }
		constexpr point2f& operator*=(const point2f& c) { x *= c.x; y *= c.y; return *this; }
		constexpr point2f& operator/=(const point2f& c) { x /= c.x; y /= c.y; return *this; }
		constexpr point2f& operator+=(float c) { x += c; y += c; return *this; }
		constexpr point2f& operator-=(float c) { x -= c; y -= c; return *this; }
		constexpr point2f& operator*=(float c) { x *= c; y *= c; return *this; }
		constexpr point2f& operator/=(float c) { x /= c; y /= c; return *this; }
        constexpr bool operator==(const Vector2& c) const
        {
            return x == c.x && y == c.y;
        };
        constexpr point2f min(const Vector2& c) const
        {
            return {x < c.x ? x : c.x, y < c.y ? y : c.y};
        }
        constexpr point2f max(const Vector2& c) const
        {
            return {x > c.x ? x : c.x, y > c.y ? y : c.y};
        }
        constexpr bool operator!=(const Vector2& c) const
        {
            return x != c.x || y != c.y;
        };
        inline point2f abs()
        {
            return point2f(std::abs(x), std::abs(y));
        }
        inline point2f round()
        {
            return point2f(qround(x), qround(y));
        }
        inline point2f ceil()
        {
            return point2f(qceil(x), qceil(y));
        }
        inline point2f floor()
        {
            return point2f(qfloor(x), qfloor(y));
        }
        inline void normalize()
        {
            const float length = distance();
            if (length != 0.0f)
            {
                x /= length;
                y /= length;
            }
        }
        inline point2f normalized() const
        {
            const float length = distance();
            if (length != 0.0f)
                return {x / length, y / length};
            return {x,y};
        }
        constexpr float angle_diamond() const
        {
            if (y >= 0)
                return (x >= 0 ? y / (x + y) : 1 - x / (-x + y));
            return (x < 0 ? 2 - y / (-x - y) : 3 + x / (x - y));
        }
        constexpr float dot(const Vector2& other) const
        {
            return x * other.x + y * other.y;
        }
        inline float distance(const Vector2& other) const
        {
            const float dx = x - other.x;
            float dy = y - other.y;
            return std::sqrt(dx * dx + dy * dy);
        }
        inline float distance() const
        {
            return std::sqrt(x * x + y * y);
        }
        constexpr float distance_sqr(const Vector2& other) const
        {
            const float dx = x - other.x;
            const float  dy = y - other.y;
            return dx * dx + dy * dy;
        }
        constexpr float distance_sqr() const
		{
            return x * x + y * y;
		}
        point2f rotate(float angleRadians) const
        {
            const float s = std::sin(angleRadians);
            const float c = std::cos(angleRadians);
            return point2f(x * c - y * s, x * s + y * c);
        }
        point2f rotate(const Vector2& rot) const // {sin(ang), cos(ang)}
        {
            return point2f(x * rot.y - y * rot.x, x * rot.x + y * rot.y);
        }
        float angle(const Vector2& other) const
        {
            return std::atan2(y * other.x - x * other.y, x * other.x + y * other.y);
        }
        float angle() const
        {
            return std::atan2(y, x);
        }
        const float& operator[](int n) const
        {
            return (&x)[n];
        }
        float& operator[](int n)
        {
            return (&x)[n];
        }
        void truncate(float maxdist)
        {
            auto lensqr = distance_sqr();
            if (lensqr > (maxdist * maxdist) && lensqr != 0.0f)
            {
                auto scale = maxdist / std::sqrt(lensqr);
                x *= scale;
                y *= scale;
            }
        }
        bool is_zero() const
        {
            return x == 0.f && y == 0.f;
        };
	};

	constexpr point2f operator+(const point2f& a, const point2f& b)
	{
		return { a.x + b.x, a.y + b.y };
	}
    constexpr point2f operator-(const point2f& a, const point2f& b)
	{
		return { a.x - b.x, a.y - b.y };
	}
    constexpr point2f operator*(const point2f& a, const point2f& b)
	{
		return { a.x * b.x, a.y * b.y };
	}
    constexpr point2f operator/(const point2f& a, const point2f& b)
	{
		return { a.x / b.x, a.y / b.y };
	}
    constexpr point2f operator*(const point2f& a, float b)
	{
		return { a.x * b, a.y * b };
	}
    constexpr point2f operator/(const point2f& a, float b)
	{
		return { a.x / b, a.y / b };
	}

	struct rectf : Rectangle
	{
		constexpr rectf() : Rectangle{} {}
		constexpr rectf(float x, float y, float w, float h) : Rectangle{ x, y, w, h } {}
        constexpr rectf(const Rectangle& rc) : Rectangle{rc} {};
		constexpr rectf(const rectf&) = default;
		constexpr rectf& operator=(const rectf&) = default;
		constexpr float x1() const { return x; }
		constexpr float y1() const { return y; }
		constexpr float x2() const { return x + width; }
		constexpr float y2() const { return y + height; }
		constexpr void expand(const Vector2& p) { x -= p.x; y -= p.y; width += p.x + p.x; height += p.y + p.y; }
        constexpr bool contains(const Vector2& p) const
        {
            return (p.x >= x && p.x < x + width && p.y >= y && p.y < y + height);
        }
        constexpr bool contains(float xx, float yy) const
        {
            return (xx >= x && xx < x + width && yy >= y && yy < y + height);
        }
        constexpr bool intersects(const Rectangle& other) const
        {
            return (x < other.x + other.width && x + width > other.x && y < other.y + other.height && y + height > other.y);
        }
        constexpr rectf merge(const Rectangle& other) const
        {
            const float x1 = std::min(x, other.x);
            const float y1 = std::min(y, other.y);
            const float x2 = std::max(x + width, other.x + other.width);
            const float y2 = std::max(y + height, other.y + other.height);
            return rectf(x1, y1, x2 - x1, y2 - y1);
        }
        constexpr rectf intersection(const Rectangle& other) const
        {
            const float x1 = std::max(x, other.x);
            const float y1 = std::max(y, other.y);
            const float x2 = std::min(x + width, other.x + other.width);
            const float y2 = std::min(y + height, other.y + other.height);
            if (x1 < x2 && y1 < y2)
                return rectf(x1, y1, x2 - x1, y2 - y1);
            return rectf();
        }
	};

    template <typename T>
    struct region
    {
        T x1{};
        T y1{};
        T x2{};
        T y2{};

        T width() const
        {
            return x2 - x1;
        };

        T height() const
        {
            return y2 - y1;
        };

        T center_x() const
        {
            return (x1 + x2) / 2;
        }

        T center_y() const
        {
            return (y1 + y2) / 2;
        }

        constexpr bool operator==(const region& c) const
        {
            return x1 == c.x1 && y1 == c.y1 && x2 == c.x2 && y2 == c.y2;
        };
        constexpr bool operator!=(const region& c) const
        {
            return x1 != c.x1 || y1 != c.y1 || x2 != c.x2 || y2 != c.y2;
        };
        constexpr bool contains(const Vector2& p) const
        {
            return (p.x >= x1 && p.x < x2  && p.y >= y1 && p.y < y2);
        }
        constexpr bool intersects(const region& other) const
        {
            return (x1 < other.x2 && x2 > other.x1 && y1 < other.y2 && y2 > other.y1);
        }
    };

	template <typename T>
	T pov2(const T& v) { return v * v; }

	inline bool check_collision(const Vector2& start1, const Vector2& end1, float r1, const Vector2& start2, const Vector2& end2, float r2)
	{
		// Calculate relative velocity
		const auto	vx_rel = (end2.x - start2.x) - (end1.x - start1.x);
		const auto	vy_rel = (end2.y - start2.y) - (end1.y - start1.y);

		// Calculate relative position
		const auto	rx_rel = start2.x - start1.x;
		const auto	ry_rel = start2.y - start1.y;

		// Solve for time of closest approach
		const auto t_closest = (rx_rel * vx_rel + ry_rel * vy_rel) / (pov2(vx_rel) + pov2(vy_rel));

		// Calculate positions at time of closest approach
		const auto	x1_closest = start1.x + vx_rel * t_closest;
		const auto	y1_closest = start1.y + vy_rel * t_closest;
		const auto	x2_closest = start2.x + vx_rel * t_closest;
		const auto	y2_closest = start2.y + vy_rel * t_closest;

		// Check for collision at time of closest approach
		const auto	distance_squared = pov2(x2_closest - x1_closest) + pov2(y2_closest - y1_closest);
		return  distance_squared < pov2(r1 + r2);
	}

    inline dir get_dir(point2f to)
    {
        auto angle = std::atan2(to.y, to.x) * RAD2DEG;
        if (angle < 0)
            angle += 360;

        const int frag = 23;

        if (angle > 45 - frag && angle < 45 + frag)
            return dir::BottomRight;
        if (angle > 90 - frag && angle < 90 + frag)
            return dir::Bottom;
        if (angle > 135 - frag && angle < 135 + frag)
            return dir::BottomLeft;
        if (angle > 180 - frag && angle < 180 + frag)
            return dir::Left;
        if (angle > 225 - frag && angle < 225 + frag)
            return dir::TopLeft;
        if (angle > 270 - frag && angle < 270 + frag)
            return dir::Top;
        if (angle > 315 - frag && angle < 315 + frag)
            return dir::TopRight;

        return dir::Right;
    }

    inline dir get_dir(point2f from, point2f to)
    {
        return get_dir(to - from);
    }

    template <typename CB>
    inline void plot_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, CB POINT)
    {
        int32_t ystep, xstep;   // the step on y and x axis
        int32_t error;          // the error accumulated during the increment
        int32_t errorprev;      // *vision the previous value of the error variable
        int32_t y = y1, x = x1; // the line points
        int32_t ddy, ddx;       // compulsory variables: the double values of dy and dx
        int32_t dx = x2 - x1;
        int32_t dy = y2 - y1;
        POINT(y1, x1); // first point
        // NB the last point can't be here, because of its previous point (which has to be verified)
        if (dy < 0)
        {
            ystep = -1;
            dy    = -dy;
        }
        else
            ystep = 1;
        if (dx < 0)
        {
            xstep = -1;
            dx    = -dx;
        }
        else
            xstep = 1;
        ddy = 2 * dy; // work with double values for full precision
        ddx = 2 * dx;
        if (ddx >= ddy)
        { // first octant (0 <= slope <= 1)
            // compulsory initialization (even for errorprev, needed when dx==dy)
            errorprev = error = dx; // start in the middle of the square
            for (int32_t i = 0; i < dx; i++)
            { // do not use the first point (already done)
                x += xstep;
                error += ddy;
                if (error > ddx)
                { // increment y if AFTER the middle ( > )
                    y += ystep;
                    error -= ddx;
                    // three cases (octant == right->right-top for directions below):
                    if (error + errorprev < ddx) // bottom square also
                        POINT(y - ystep, x);
                    else if (error + errorprev > ddx) // left square also
                        POINT(y, x - xstep);
                    else
                    { // corner: bottom and left squares also
                        POINT(y - ystep, x);
                        POINT(y, x - xstep);
                    }
                }
                POINT(y, x);
                errorprev = error;
            }
        }
        else
        { // the same as above
            errorprev = error = dy;
            for (int32_t i = 0; i < dy; i++)
            {
                y += ystep;
                error += ddx;
                if (error > ddy)
                {
                    x += xstep;
                    error -= ddy;
                    if (error + errorprev < ddy)
                        POINT(y, x - xstep);
                    else if (error + errorprev > ddy)
                        POINT(y - ystep, x);
                    else
                    {
                        POINT(y, x - xstep);
                        POINT(y - ystep, x);
                    }
                }
                POINT(y, x);
                errorprev = error;
            }
        }
    }

    template <typename CB>
    inline void plot_line2(int32_t x0, int32_t y0, int32_t x1, int32_t y1, CB POINT)
    {
        const int32_t dx    = std::abs(x1 - x0);
        const int32_t sx    = x0 < x1 ? 1 : -1;
        const int32_t dy    = -std::abs(y1 - y0);
        const int32_t sy    = y0 < y1 ? 1 : -1;
        int32_t error = dx + dy;
        while (true)
        {
            if (POINT(x0, y0))
                return;
            if (x0 == x1 && y0 == y1)
                break;
            int32_t e2 = 2 * error;
            if (e2 >= dy)
            {
                if (x0 == x1)
                    break;
                error += dy;
                x0 += sx;
            }
            if (e2 <= dx)
            {
                if (y0 == y1)
                    break;
                error += dx;
                y0 += sy;
            }
        }
    }
}