/*SoA 2x2 vector oprations for shaders*/
#pragma once
#include "stdafx.h"
#include "mvec4.h"


/*constants used for preparing 2x2 SoA gangs*/
const mvec4f FQX{_mm_setr_ps(0,1,0,1)};
const mvec4f FQY{_mm_setr_ps(0,0,1,1)};
const mvec4i IQX{_mm_setr_epi32(0,1,0,1)};
const mvec4i IQY{_mm_setr_epi32(0,0,1,1)};


typedef mvec4f qfloat;


struct qfloat2 {
	/*SoA float2*/

	inline qfloat2() = default;
	qfloat2(const vec2& a) noexcept :x(a.x), y(a.y) {}
	qfloat2(const mvec4f& a) noexcept :x(a.xxxx()), y(a.yyyy()) {}
	qfloat2(const qfloat2& a) noexcept :x(a.x), y(a.y) {}
	qfloat2(const float x, const float y) noexcept :x(x), y(y) {}
	qfloat2(const mvec4f& x, const mvec4f& y) noexcept :x(x), y(y) {}

	inline qfloat2 operator+(const qfloat2& b) const { return{ v[0] + b.v[0], v[1] + b.v[1] }; }
	inline qfloat2 operator-(const qfloat2& b) const { return{ v[0] - b.v[0], v[1] - b.v[1] }; }
	inline qfloat2 operator*(const qfloat2& b) const { return{ v[0] * b.v[0], v[1] * b.v[1] }; }
	inline qfloat2 operator/(const qfloat2& b) const { return{ v[0] / b.v[0], v[1] / b.v[1] }; }

	inline qfloat2 operator+(const qfloat& b) const { return{ v[0] + b, v[1] + b }; }
	inline qfloat2 operator-(const qfloat& b) const { return{ v[0] - b, v[1] - b }; }
	inline qfloat2 operator*(const qfloat& b) const { return{ v[0] * b, v[1] * b }; }
	inline qfloat2 operator/(const qfloat& b) const { return{ v[0] / b, v[1] / b }; }
	
	inline qfloat2& operator+=(const qfloat2& rhs) { x += rhs.x;  y += rhs.y;  return *this; }
	inline qfloat2& operator-=(const qfloat2& rhs) { x -= rhs.x;  y -= rhs.y;  return *this; }
	inline qfloat2& operator*=(const qfloat2& rhs) { x *= rhs.x;  y *= rhs.y;  return *this; }
	inline qfloat2& operator/=(const qfloat2& rhs) { x /= rhs.x;  y /= rhs.y;  return *this; }

	union {
		struct { mvec4f x, y; };
		struct { mvec4f s, t; };
		//struct { mvec4f u, v; };
		mvec4f v[2]; }; };


inline qfloat length(const qfloat2 &a) {
	return sqrt(a.x*a.x + a.y*a.y); }


struct qfloat3 {
	/*SoA float3*/

	inline qfloat3() = default;
	qfloat3(const vec3& a) noexcept : x(a.x), y(a.y), z(a.z) {}
	qfloat3(const mvec4f& a) noexcept :x(a.xxxx()), y(a.yyyy()), z(a.zzzz()) {}
	qfloat3(const qfloat3& a) noexcept :x(a.x), y(a.y), z(a.z) {}
	qfloat3(const float x, const float y, const float z) noexcept :x(x), y(y), z(z) {}
	qfloat3(const mvec4f& x, const mvec4f& y, const mvec4f& z) noexcept :x(x), y(y), z(z) {}

	inline qfloat3 operator+(const qfloat3& b) const { return{v[0]+b.v[0], v[1]+b.v[1], v[2]+b.v[2] }; }
	inline qfloat3 operator-(const qfloat3& b) const { return{v[0]-b.v[0], v[1]-b.v[1], v[2]-b.v[2] }; }
	inline qfloat3 operator*(const qfloat3& b) const { return{v[0]*b.v[0], v[1]*b.v[1], v[2]*b.v[2] }; }
	inline qfloat3 operator/(const qfloat3& b) const { return{v[0]/b.v[0], v[1]/b.v[1], v[2]/b.v[2] }; }

	inline qfloat3 operator+(const qfloat& b) const { return{v[0]+b, v[1]+b, v[2]+b }; }
	inline qfloat3 operator-(const qfloat& b) const { return{v[0]-b, v[1]-b, v[2]-b }; }
	inline qfloat3 operator*(const qfloat& b) const { return{v[0]*b, v[1]*b, v[2]*b }; }
	inline qfloat3 operator/(const qfloat& b) const { return{v[0]/b, v[1]/b, v[2]/b }; }

	inline qfloat3& operator+=(const qfloat& rhs) { x+=rhs; y+=rhs; z+=rhs; return *this; }
	inline qfloat3& operator-=(const qfloat& rhs) { x-=rhs; y-=rhs; z-=rhs; return *this; }
	inline qfloat3& operator*=(const qfloat& rhs) { x*=rhs; y*=rhs; z*=rhs; return *this; }
	inline qfloat3& operator/=(const qfloat& rhs) { x/=rhs; y/=rhs; z/=rhs; return *this; }

	inline qfloat3& operator+=(const qfloat3& rhs) { x+=rhs.x; y+=rhs.y; z+=rhs.z; return *this; }
	inline qfloat3& operator-=(const qfloat3& rhs) { x-=rhs.x; y-=rhs.y; z-=rhs.z; return *this; }
	inline qfloat3& operator*=(const qfloat3& rhs) { x*=rhs.x; y*=rhs.y; z*=rhs.z; return *this; }
	inline qfloat3& operator/=(const qfloat3& rhs) { x/=rhs.x; y/=rhs.y; z/=rhs.z; return *this; }

	union {
		struct { mvec4f x, y, z; };
		struct { mvec4f r, g, b; };
		mvec4f v[3]; }; };


inline qfloat3 sin(const qfloat3& a) {
	return qfloat3{ sin(a.x), sin(a.y), sin(a.z) }; }


inline qfloat3 fract(const qfloat3& a) {
	return qfloat3{ fract(a.x), fract(a.y), fract(a.z) }; }


inline qfloat3 pow(const qfloat3& a, const qfloat3& b) {
	return qfloat3{ pow(a.x, b.x), pow(a.y, b.y), pow(a.z, b.z) }; }


inline qfloat3 operator*(const float& lhs, const qfloat3& rhs) {
	return qfloat3{ lhs * rhs.x, lhs * rhs.y, lhs * rhs.z }; }


inline qfloat3 operator*(const qfloat& lhs, const qfloat3& rhs) {
	return qfloat3{ lhs * rhs.x, lhs * rhs.y, lhs * rhs.z }; }


inline qfloat length(const qfloat3 &a) {
	return sqrt(a.x*a.x + a.y*a.y + a.z*a.z); }


inline qfloat3 sqrt(const qfloat3 &a) {
	return qfloat3{ sqrt(a.x), sqrt(a.y), sqrt(a.z) }; }


inline qfloat dot(const qfloat3& a, const qfloat3& b) {
	return a.x*b.x + a.y*b.y + a.z*b.z; }


inline qfloat3 normalize(const qfloat3& a) {
	mvec4f scale = rsqrt(dot(a, a));
	return qfloat3{
		a.x * scale,
		a.y * scale,
		a.z * scale }; }


inline qfloat3 reflect(const qfloat3& i, const qfloat3& n) {
	return i - qfloat{2.0f} * dot(n, i) * n; }


inline qfloat3 hash3(const qfloat& n) {
	/*utility from iq's shadertoy entry "Repelling"*/
	return fract(sin(qfloat3(n, n + 1.0f, n + 2.0f)) * 43758.5453123f); }


struct qfloat4 {

	inline qfloat4() = default;
	qfloat4(const mvec4f& a) noexcept :x(a.xxxx()), y(a.yyyy()), z(a.zzzz()), w(a.wwww()) {}
	qfloat4(const qfloat4& a) noexcept :x(a.x), y(a.y), z(a.z), w(a.w) {}
	qfloat4(const qfloat3& a, const mvec4f& w) noexcept :x(a.x), y(a.y), z(a.z), w(w) {}
	qfloat4(const float x, const float y, const float z, const float w) noexcept :x(x), y(y), z(z), w(w) {}
	qfloat4(const mvec4f& x, const mvec4f& y, const mvec4f& z, const mvec4f& w) noexcept :x(x), y(y), z(z), w(w) {}

	qfloat3 xyz() {
		return qfloat3{ x, y, z }; }

	union {
		struct { mvec4f x, y, z, w; };
		struct { mvec4f r, g, b, a; };
		mvec4f v[4]; }; };


inline mvec4f dFdx(const mvec4f& a) {
	return a.yyww() - a.xxzz(); }

inline mvec4f dFdy(const mvec4f& a) {
	return a.xyxy() - a.zwzw(); }

inline qfloat fwidth(const qfloat& a) {
	return abs(dFdx(a)) + abs(dFdy(a)); } 

inline qfloat2 fwidth(const qfloat2& a) {
	return qfloat2{
		abs(dFdx(a.x)) + abs(dFdy(a.x)),
		abs(dFdx(a.y)) + abs(dFdy(a.y))
	}; }

inline qfloat3 fwidth(const qfloat3& a) {
	return qfloat3{
		abs(dFdx(a.x)) + abs(dFdy(a.x)),
		abs(dFdx(a.y)) + abs(dFdy(a.y)),
		abs(dFdx(a.z)) + abs(dFdy(a.z))
	}; }


inline qfloat4 pdiv(const qfloat4& p) {
	/*perspective divide, SoA version

	float operations _must_ match scalar version !

	Args:
		p: point in homogeneous clip-space

	Returns:
		x/w, y/w, z/w, 1/w
	*/
	const __m128 r1 = _mm_rcp_ps(p.w.v);
	return qfloat4{ p.x * r1, p.y * r1, p.z * r1, r1 }; }


struct tri_qfloat {
	/*SoA helper for triangle vertex interpolants, qfloat

	the ctor taking 3x-qfloat is for receiving the
	barycentric coordinates, which are used for the
	blending-weights.

	they should be the only non-uniform values used
	for tri_* initialization.
	*/
	union {
		struct { qfloat v0, v1, v2; };
		qfloat v[3]; };

	tri_qfloat() = default;
	tri_qfloat(const float v0, const float v1, const float v2) noexcept : v0(v0), v1(v1), v2(v2) {}

	// non-uniform
	tri_qfloat(const qfloat& v0, const qfloat& v1, const qfloat& v2) noexcept : v0(v0), v1(v1), v2(v2) {} };


struct tri_qfloat2 {
	/*SoA helper for triangle vertex interpolants, qfloat2*/
	union {
		struct { qfloat2 v0, v1, v2; };
		qfloat2 v[3]; };

	tri_qfloat2() = default;
	tri_qfloat2(const vec2& v0, const vec2& v1, const vec2& v2) noexcept : v0(v0), v1(v1), v2(v2) {} };


struct tri_qfloat3 {
	/*SoA helper for triangle vertex interpolants, qfloat3*/
	union {
		struct { qfloat3 v0, v1, v2; };
		qfloat3 v[3]; };

	tri_qfloat3() = default;
	tri_qfloat3(const vec3& v0, const vec3& v1, const vec3& v2) noexcept : v0(v0), v1(v1), v2(v2) {} };


inline qfloat tri_blend(const tri_qfloat& weight, const tri_qfloat& val) {
	return qfloat{
		weight.v0*val.v0 + weight.v1*val.v1 + weight.v2*val.v2
	}; }

inline qfloat2 tri_blend(const tri_qfloat& weight, const tri_qfloat2& val) {
	return qfloat2{
		weight.v0*val.v0.x + weight.v1*val.v1.x + weight.v2*val.v2.x,
		weight.v0*val.v0.y + weight.v1*val.v1.y + weight.v2*val.v2.y
	}; }

inline qfloat3 tri_blend(const tri_qfloat& weight, const tri_qfloat3& val) {
	return qfloat3{
		weight.v0*val.v0.x + weight.v1*val.v1.x + weight.v2*val.v2.x,
		weight.v0*val.v0.y + weight.v1*val.v1.y + weight.v2*val.v2.y,
		weight.v0*val.v0.z + weight.v1*val.v1.z + weight.v2*val.v2.z
	}; }


inline qfloat3 fwidth(const tri_qfloat& a) {
	return qfloat3{
		fwidth(a.v0),
		fwidth(a.v1),
		fwidth(a.v2)
	}; }


inline qfloat3 smoothstep(const float a, const qfloat3& b, const tri_qfloat& x) {
	const mvec4f _a{ a };
	return qfloat3{
		smoothstep(_a, b.x, x.v0),
		smoothstep(_a, b.y, x.v1),
		smoothstep(_a, b.z, x.v2)
	}; }


inline mvec4f mix(const mvec4f& a, const mvec4f& b, const mvec4f& t) {
	return a*(mvec4f(1.0f) - t) + b*t; }


inline mvec4f ddx(const mvec4f& a) {
	return mvec4f(_mm_sub_ps(_mm_movehdup_ps(a.v), _mm_moveldup_ps(a.v))); }


inline mvec4f ddy(const mvec4f& a) {
	// r0 = a2 - a0
	// r1 = a3 - a1
	// r2 = a2 - a0
	// r3 = a3 - a1

	// SSE3 self-shuffles
	__m128 quadtop = _mm_shufd(a.v, _MM_SHUFFLE(0, 1, 0, 1));
	__m128 quadbot = _mm_shufd(a.v, _MM_SHUFFLE(2, 3, 2, 3));
	//__m128 quadtop = _mm_shuffle_ps( a, a, _MM_SHUFFLE(0,1,0,1) );
	//__m128 quadbot = _mm_shuffle_ps( a, a, _MM_SHUFFLE(2,3,2,3) );

	return mvec4f(_mm_sub_ps(quadbot, quadtop)); }
