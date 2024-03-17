#pragma once

#include "math.hpp"

namespace box
{
	using pointf_t = point2f;

	struct transformable_2d
	{
		pointf_t position{ 0, 0 };
		pointf_t origin{ 0, 0 };
		pointf_t scale{ 1.f, 1.f };
		float rotation = 0;
	};



	class matrix2d
	{
	public:
		matrix2d() : sx(1.0), shy(0.0), shx(0.0), sy(1.0), tx(0.0), ty(0.0) {}
		matrix2d(const matrix2d& m) : sx(m.sx), shy(m.shy), shx(m.shx), sy(m.sy), tx(m.tx), ty(m.ty) {}
		matrix2d(float v0, float v1, float v2, float v3, float v4, float v5) : sx(v0), shy(v1), shx(v2), sy(v3), tx(v4), ty(v5) {}
		matrix2d(const pointf_t& position, const pointf_t& scale, const pointf_t& origin, float deg) { setTransform(position, scale, origin, deg); }
		matrix2d(const pointf_t& position, const pointf_t& scale, const pointf_t& origin, const pointf_t& cosSin) { setTransform(position, scale, origin, cosSin); }

		inline void assign(float v0, float v1, float v2, float v3, float v4, float v5) { sx = v0; shy = v1; shx = v2; sy = v3; tx = v4; ty = v5; }

		const matrix2d& operator = (const matrix2d& m);
		const matrix2d& operator *= (const matrix2d& m);
		matrix2d operator ~() const;

		const matrix2d& reset();
		const matrix2d& multiply(const matrix2d& m);
		const matrix2d& premultiply(const matrix2d& m);
		const matrix2d& invert();
		const matrix2d& flipX();
		const matrix2d& flipY();

		void getMatrix4x4(float* mtx);

		matrix2d getInverse() const;

		inline void transform(float& x, float& y) const;
		inline void inverseTransform(float& x, float& y) const;

		template <typename T>
		inline T transformPoint(const T& pt) const
		{
			return T{ sx * pt.x + shx * pt.y + tx, shy * pt.x + sy * pt.y + ty };
		}

		template <typename T>
		inline T inverseTransformPoint(const T& pt) const
		{
			const float det = sx * sy - shy * shx;
			const float a = (pt.x - tx) / det;
			const float b = (pt.y - ty) / det;
			return T{ a * sy - b * shx, b * sx - a * shy };
		}

		inline float determinant() const;
		float scale() const;

		matrix2d& rotate(float a);
		matrix2d& rotate(float a, float x, float y);
		matrix2d& scale(float s);
		matrix2d& scale(float sx, float sy);
		matrix2d& scale(float sx, float sy, float x, float y);
		matrix2d& translate(float x, float y);
		matrix2d& skew(float sx, float sy);
		matrix2d& shear(float sx, float sy);
		matrix2d& setTransform(const pointf_t& position, const pointf_t& scale, const pointf_t& origin, float deg);
		matrix2d& setTransform(const pointf_t& position, const pointf_t& scale, float deg);
		matrix2d& setTransform(const pointf_t& position, const pointf_t& scale, const pointf_t& origin, const pointf_t& cosSin);

		void decompose(pointf_t& position, pointf_t& scale, float& deg);
		void decompose(pointf_t& position, pointf_t& scale, pointf_t& skew, float& deg);

		float sx, shy, shx, sy, tx, ty;
	};



	inline matrix2d operator *(const matrix2d& left, const matrix2d& right) { matrix2d e = left; e *= right; return e; }
	inline bool operator==(const matrix2d& left, const matrix2d& right) { return left.sx == right.sx && left.shy == right.shy && left.shx == right.shx && left.sy == right.sy && left.tx == right.tx && left.ty == right.ty; }
	inline bool operator!=(const matrix2d& left, const matrix2d& right) { return left.sx != right.sx || left.shy != right.shy || left.shx != right.shx || left.sy != right.sy || left.tx != right.tx || left.ty != right.ty; }



	inline const matrix2d& matrix2d::operator=(const matrix2d& m)
	{
		sx = (m.sx); shy = (m.shy); shx = (m.shx); sy = (m.sy); tx = (m.tx); ty = (m.ty);
		return *this;
	}



	inline const matrix2d& matrix2d::operator*=(const matrix2d& m)
	{
		return premultiply(m);
	}



	inline matrix2d matrix2d::operator~() const
	{
		matrix2d ret = *this;
		return ret.invert();
	}



	inline const matrix2d& matrix2d::reset()
	{
		sx = sy = 1.0;
		shy = shx = tx = ty = 0.0;
		return *this;
	}



	inline const matrix2d& matrix2d::multiply(const matrix2d& m)
	{
		const float t0 = sx * m.sx + shy * m.shx;
		const float t2 = shx * m.sx + sy * m.shx;
		const float t4 = tx * m.sx + ty * m.shx + m.tx;
		shy = sx * m.shy + shy * m.sy;
		sy = shx * m.shy + sy * m.sy;
		ty = tx * m.shy + ty * m.sy + m.ty;
		sx = t0;
		shx = t2;
		tx = t4;
		return *this;
	}



	inline const matrix2d& matrix2d::premultiply(const matrix2d& m)
	{
		matrix2d t = m;
		return *this = t.multiply(*this);
	}



	inline const matrix2d& matrix2d::invert()
	{
		const float det = sx * sy - shy * shx;

		if (det != 0.f)
		{
			const float tyt = ty;
			ty = -(ty * sx - shy * tx) / det;
			tx = (tyt * shx - sy * tx) / det;

			const float sxt = sx;
			sx = sy / det;
			sy = sxt / det;

			shy = -(shy) / det;
			shx = -(shx) / det;
		}
		else
		{
			reset();
		}

		return *this;
	}



	inline const matrix2d& matrix2d::flipX()
	{
		sx = -sx;
		shy = -shy;
		tx = -tx;
		return *this;
	}



	inline const matrix2d& matrix2d::flipY()
	{
		shx = -shx;
		sy = -sy;
		ty = -ty;
		return *this;
	}



	inline void matrix2d::getMatrix4x4(float* mtx)
	{
		mtx[0] = sx;
		mtx[1] = shy;
		mtx[2] = 0;
		mtx[3] = tx;

		mtx[4] = shx;
		mtx[5] = sy;
		mtx[6] = 0;
		mtx[7] = ty;

		mtx[8] = 0;
		mtx[9] = 0;
		mtx[10] = 1.f;
		mtx[11] = 0;

		mtx[12] = 0;
		mtx[13] = 0;
		mtx[14] = 0;
		mtx[15] = 1.f;
	}



	inline matrix2d matrix2d::getInverse() const
	{
		matrix2d t(*this);
		t.invert();
		return t;
	}



	inline float matrix2d::scale() const
	{
		const float x = 0.707106781f * sx + 0.707106781f * shx;
		const float y = 0.707106781f * shy + 0.707106781f * sy;
		return sqrtf(x * x + y * y);
	}



	inline matrix2d& matrix2d::scale(float s)
	{
		premultiply(matrix2d(s, 0.0, 0.0, s, 0.0, 0.0));
		return *this;
	}



	inline matrix2d& matrix2d::scale(float sx, float sy)
	{
		premultiply(matrix2d(sx, 0.0, 0.0, sy, 0.0, 0.0));
		return *this;
	}



	inline matrix2d& matrix2d::scale(float sx, float sy, float x, float y)
	{
		premultiply(matrix2d(sx, 0, 0, sy, x * (1.f - sx), y * (1.f - sy)));
		return *this;
	}



	inline matrix2d& matrix2d::translate(float x, float y)
	{
		premultiply(matrix2d(1.0f, 0.0, 0.0, 1.0f, x, y));
		return *this;
	}



	inline matrix2d& matrix2d::skew(float sx, float sy)
	{
		premultiply(matrix2d(1.0f, tanf(sy), tanf(sx), 1.0f, 0.0, 0.0));
		return *this;
	}



	inline matrix2d& matrix2d::shear(float sx, float sy)
	{
		premultiply(matrix2d(1.0f, sy, sx, 1.0f, 0.0, 0.0));
		return *this;
	}



	inline matrix2d& matrix2d::rotate(float a)
	{
		const float rad = a * -float(M_PI) / 180.f;
		const float cos = cosf(rad);
		const float sin = sinf(rad);
		premultiply(matrix2d(cos, -sin, sin, cos, 0.0f, 0.0f));
		return *this;
	}



	inline matrix2d& matrix2d::rotate(float a, float x, float y)
	{
		const float rad = a * -float(M_PI) / 180.f;
		const float cos = cosf(rad);
		const float sin = sinf(rad);
		premultiply(matrix2d(cos, -sin, sin, cos, x * (1.f - cos) + y * sin, y * (1.f - cos) - x * sin));
		return *this;
	}



	inline void matrix2d::inverseTransform(float& x, float& y) const
	{
		const float det = sx * sy - shy * shx;
		const float a = (x - tx) / det;
		const float b = (y - ty) / det;
		x = a * sy - b * shx;
		y = b * sx - a * shy;

	}



	inline void matrix2d::transform(float& x, float& y) const
	{
		const float tmp = x;
		x = sx * x + shx * y + tx;
		y = shy * tmp + sy * y + ty;
	}



	inline matrix2d& matrix2d::setTransform(const pointf_t& position, const pointf_t& scale, const pointf_t& origin, float deg)
	{
		const float angle = -deg * float(M_PI) / 180.f;
		const float cosine = cosf(angle);
		const float sine = sinf(angle);

		sx = scale.x * cosine;
		shy = -(scale.x * sine);
		shx = scale.y * sine;
		sy = scale.y * cosine;
		tx = -origin.x * sx - origin.y * shx + position.x;
		ty = -origin.x * shy - origin.y * sy + position.y;
		return *this;
	}



	inline matrix2d& matrix2d::setTransform(const pointf_t& position, const pointf_t& scale, float deg)
	{
		const float angle = -deg * float(M_PI) / 180.f;
		const float cosine = cosf(angle);
		const float sine = sinf(angle);

		sx = scale.x * cosine;
		shy = -(scale.x * sine);
		shx = scale.y * sine;
		sy = scale.y * cosine;
		tx = position.x;
		ty = position.y;
		return *this;
	}



	inline matrix2d& matrix2d::setTransform(const pointf_t& position, const pointf_t& scale, const pointf_t& origin, const pointf_t& cosSin)
	{
		sx = scale.x * cosSin.x;
		shy = -(scale.x * cosSin.y);
		shx = scale.y * cosSin.y;
		sy = scale.y * cosSin.x;
		tx = -origin.x * sx - origin.y * shx + position.x;
		ty = -origin.x * shy - origin.y * sy + position.y;
		return *this;
	}



	inline void matrix2d::decompose(pointf_t& position, pointf_t& scale, float& rotation)
	{
		position.x = tx;
		position.y = ty;

		// Apply the QR-like decomposition.
		if (sx != 0 || shy != 0)
		{
			const double delta = (double)sx * sy - (double)shy * shx;
			const double r = sqrt((double)sx * sx + (double)shy * shy);
			rotation = (float)(shy > 0 ? acos((double)sx / r) : -acos((double)sx / r));
			rotation *= 180.f / float(M_PI);
			scale.x = (float)(r);
			scale.y = (float)(delta / r);
		}
		else if (shx != 0 || sy != 0)
		{
			const float delta = sx * sy - shy * shx;
			double s = sqrt(shx * shx + sy * sy);
			rotation = (float)(float(M_PI) / 2 - (sy > 0 ? acos(-shx / s) : -acos(shx / s)));
			rotation *= 180.f / float(M_PI);
			scale.x = (float)(delta / s);
			scale.y = (float)(s);
		}
		else
		{
			scale.x = scale.y = rotation = 0;
		}
	}



	inline void matrix2d::decompose(pointf_t& position, pointf_t& scale, pointf_t& skew, float& rotation)
	{
		position.x = tx;
		position.y = ty;

		// Apply the QR-like decomposition.
		if (sx != 0 || shy != 0)
		{
			const float delta = sx * sy - shy * shx;
			const double r = sqrt(sx * sx + shy * shy);
			rotation = (float)(shy > 0 ? acos(sx / r) : -acos(sx / r));
			rotation *= 180.f / float(M_PI);
			scale.x = (float)(r);
			scale.y = (float)(delta / r);

			skew.x = (float)(atan((sx * shx + shy * sy) / (r * r)));
			skew.y = 0;
		}
		else if (shx != 0 || sy != 0)
		{
			const float delta = sx * sy - shy * shx;
			double s = sqrt(shx * shx + sy * sy);
			rotation = (float)(float(M_PI) / 2 - (sy > 0 ? acos(-shx / s) : -acos(shx / s)));
			rotation *= 180.f / float(M_PI);
			scale.x = (float)(delta / s);
			scale.y = (float)(s);
			skew.x = 0;
			skew.y = (float)(atan((sx * shx + shy * sy) / (s * s)));
		}
		else
		{
			scale.x = scale.y = rotation = 0;
		}
	}



	inline float matrix2d::determinant() const
	{
		return 1.0f / (sx * sy - shy * shx);
	}

}