#pragma once
#include <algorithm>
#include <iostream>
#include <array>

constexpr auto M_PI = 3.14159265358979323846;


template<typename T>
T lerp_fast(const T& a, const T& b, const T& f) {
	return a + (f*(b - a)); }


/*template<typename T>
T lerp(const T& a, const T& b, const T& f) {
	return (1 - f)*a + f*b; }*/


inline float lerp(const float& a, const float& b, const float& t) {
	return (1 - t)*a + t*b; }


template<typename T>
T lerp_premul(const T& a, const T& b, const T& f) {
	return (T(1) - f)*a + b; }


inline double fract(const double x) {
	double b;
	return modf(x, &b);
//	auto ux = static_cast<int>(x);
//	return x - ux;
}


inline float fastsqrt(const float x) {
	union {
		int i;
		float x;
	} u;
	u.x = x;
	u.i = (1 << 29) + (u.i >> 1) - (1 << 22);
	return u.x; }


inline double fastpow(double a, double b) {
	union {
		double d;
		int x[2];
	} u = { a };
	u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
	u.x[0] = 0;
	return u.d; }


struct alignas(8) vec2 {

	inline vec2() = default;
	inline constexpr vec2(float a) noexcept :x(a), y(a) {}
	inline constexpr vec2(float a, float b) noexcept :x(a), y(b) {}

	inline constexpr vec2 operator+(const vec2& b) const { return vec2(x + b.x, y + b.y); }
	inline constexpr vec2 operator-(const vec2& b) const { return vec2(x - b.x, y - b.y); }
	inline constexpr vec2 operator*(const vec2& b) const { return vec2(x * b.x, y * b.y); }
	inline constexpr vec2 operator/(const vec2& b) const { return vec2(x / b.x, y / b.y); }

	inline constexpr vec2 operator+(const float b) const { return vec2(x + b, y + b); }
	inline constexpr vec2 operator-(const float b) const { return vec2(x - b, y - b); }
	inline constexpr vec2 operator*(const float b) const { return vec2(x * b, y * b); }
	inline constexpr vec2 operator/(const float b) const { return vec2(x / b, y / b); }

	inline vec2& operator+=(const vec2& b) { x += b.x; y += b.y; return *this; }
	inline vec2& operator-=(const vec2& b) { x -= b.x; y -= b.y; return *this; }
	inline vec2& operator*=(const vec2& b) { x *= b.x; y *= b.y; return *this; }
	inline vec2& operator/=(const vec2& b) { x /= b.x; y /= b.y; return *this; }

	inline vec2& operator+=(const float b) { x += b; y += b; return *this; }
	inline vec2& operator-=(const float b) { x -= b; y -= b; return *this; }
	inline vec2& operator*=(const float b) { x *= b; y *= b; return *this; }
	inline vec2& operator/=(const float b) { x /= b; y /= b; return *this; }

	inline friend vec2 operator-(const vec2& a) { return vec2(-a.x, -a.y); }

	inline friend vec2 operator*(const float b, const vec2& a) { return vec2(a.x*b, a.y*b); }

	inline friend float dot(const vec2& a, const vec2& b) {
		return a.x*b.x + a.y*b.y; }

	inline friend vec2 normalize(const vec2& a) {
		return (1.0f / sqrt(dot(a, a)))*a; }

	inline friend vec2 lerp(const vec2& a, const vec2& b, const float t) {
		return vec2( lerp(a.x,b.x,t), lerp(a.y,b.y,t) ); }

	inline friend vec2 abs(const vec2& a) {
		return vec2(abs(a.x), abs(a.y)); }

	float x, y; };


struct alignas(4) vec3 {

	inline vec3() = default;
	inline constexpr vec3(const float a, const float b, const float c) noexcept : x(a), y(b), z(c) {}
	inline constexpr vec3(const float a) noexcept : x(a), y(a), z(a) {}

	inline vec3 operator+(const vec3& b) const { return { x + b.x, y + b.y, z + b.z }; }
	inline vec3 operator-(const vec3& b) const { return { x - b.x, y - b.y, z - b.z }; }
	inline vec3 operator*(const vec3& b) const { return { x * b.x, y * b.y, z * b.z }; }
	inline vec3 operator/(const vec3& b) const { return { x / b.x, y / b.y, z / b.z }; }

	inline vec3 operator+(const float b) const { return { x + b, y + b, z + b }; }
	inline vec3 operator-(const float b) const { return { x - b, y - b, z - b }; }
	inline vec3 operator*(const float b) const { return { x * b, y * b, z * b }; }
	inline vec3 operator/(const float b) const { return { x / b, y / b, z / b }; }

	inline vec3& operator+=(const vec3& b) { x += b.x; y += b.y; z += b.z; return *this; }
	inline vec3& operator-=(const vec3& b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
	inline vec3& operator*=(const vec3& b) { x *= b.x; y *= b.y; z *= b.z; return *this; }
	inline vec3& operator/=(const vec3& b) { x /= b.x; y /= b.y; z /= b.z; return *this; }

	inline vec3& operator+=(const float b) { x += b; y += b; z += b; return *this; }
	inline vec3& operator-=(const float b) { x -= b; y -= b; z -= b; return *this; }
	inline vec3& operator*=(const float b) { x *= b; y *= b; z *= b; return *this; }
	inline vec3& operator/=(const float b) { x /= b; y /= b; z /= b; return *this; }

/*	inline friend vec3 operator+(float a, const vec3& b) { return b + a; }
	inline friend vec3 operator-(float a, const vec3& b) { return vec3(_mm_set1_ps(a)) - b; }*/
	inline friend vec3 operator*(float a, const vec3& b) { return{ a*b.x, a*b.y, a*b.z }; }
/*	inline friend vec3 operator/(float a, const vec3& b) { return vec3(_mm_set1_ps(a)) / b; }*/

	inline friend vec3 operator-(const vec3& a) {
		return{ -a.x, -a.y, -a.z }; }

	inline friend vec3 abs(const vec3& a) {
		return{ abs(a.x), abs(a.y), abs(a.z) }; }

	inline friend float dot(const vec3& a, const vec3& b) {
		return a.x*b.x + a.y*b.y + a.z*b.z; }

	inline friend vec3 normalize(const vec3& a) {
		/*normalize a to length 1.0*/
		return a / sqrt(dot(a, a)); }
		//return a * (1.0f / sqrt(dot(a, a))); }

	inline friend float length(const vec3& a) {
		return sqrt(dot(a, a)); }

	inline friend float hadd(const vec3& a) {
		return a.x + a.y + a.z; }

	inline friend float hmax(const vec3& a) {
		using std::max;
		return max(max(a.x, a.y), a.z); }

	inline friend float hmin(const vec3& a) {
		using std::min;
		return min(min(a.x, a.y), a.z); }

	inline friend vec3 vmin(const vec3& a, const vec3& b) {
		using std::min;
		return{ min(a.x, b.x), min(a.y, b.y), min(a.z, b.z) }; }

	inline friend vec3 vmax(const vec3& a, const vec3& b) {
		using std::max;
		return{ max(a.x, b.x), max(a.y, b.y), max(a.z, b.z) }; }

	inline friend vec3 reflect(const vec3& i, const vec3& n) {
		return i - vec3{2.0f} * dot(n, i) * n; }

	static inline vec3 from_rgb(const int rgb) {
		return vec3{
			(float)((rgb >> 16) & 0xff) / 256.0f,
			(float)((rgb >> 8) & 0xff) / 256.0f,
			(float)((rgb >> 0) & 0xff) / 256.0f}; }

	inline friend vec3 lerp(const vec3 &a, const vec3 &b, const float t) {
		return (1.0f - t)*a + t*b; }

	union {
		struct { float x, y, z; };
		struct { float r, g, b; }; }; };


struct alignas(16) vec4 {

	inline vec4() = default;
	inline constexpr vec4(const float a, const float b, const float c, const float d) noexcept :x(a), y(b), z(c), w(d) {}
	inline constexpr vec4(const float a) noexcept : x(a), y(a), z(a), w(a) {}

	inline constexpr vec4 operator+(const vec4& b) const { return { x + b.x, y + b.y, z + b.z, w + b.w }; }
	inline constexpr vec4 operator-(const vec4& b) const { return { x - b.x, y - b.y, z - b.z, w - b.w }; }
	inline constexpr vec4 operator*(const vec4& b) const { return { x * b.x, y * b.y, z * b.z, w * b.w }; }
	inline constexpr vec4 operator/(const vec4& b) const { return { x / b.x, y / b.y, z / b.z, w / b.w }; }

	inline constexpr vec4 operator+(const float b) const { return { x + b, y + b, z + b, w + b }; }
	inline constexpr vec4 operator-(const float b) const { return { x - b, y - b, z - b, w - b }; }
	inline constexpr vec4 operator*(const float b) const { return { x * b, y * b, z * b, w * b }; }
	inline constexpr vec4 operator/(const float b) const { return { x / b, y / b, z / b, w / b }; }

	inline vec4& operator+=(const vec4& b) { x += b.x; y += b.y; z += b.z; w += b.w; return *this; }
	inline vec4& operator-=(const vec4& b) { x -= b.x; y -= b.y; z -= b.z; w -= b.w; return *this; }
	inline vec4& operator*=(const vec4& b) { x *= b.x; y *= b.y; z *= b.z; w *= b.w; return *this; }
	inline vec4& operator/=(const vec4& b) { x /= b.x; y /= b.y; z /= b.z; w /= b.w; return *this; }

	inline vec4& operator+=(const float b) { x += b; y += b; z += b; w += b; return *this; }
	inline vec4& operator-=(const float b) { x -= b; y -= b; z -= b; w -= b; return *this; }
	inline vec4& operator*=(const float b) { x *= b; y *= b; z *= b; w *= b; return *this; }
	inline vec4& operator/=(const float b) { x /= b; y /= b; z /= b; w /= b; return *this; }

/*	inline friend vec4 operator+(float a, const vec4& b) { return b + a; }
	inline friend vec4 operator-(float a, const vec4& b) { return vec4(_mm_set1_ps(a)) - b; }*/
	inline friend vec4 operator*(float a, const vec4& b) { return{ a*b.x, a*b.y, a*b.z, a*b.w }; }
/*	inline friend vec4 operator/(float a, const vec4& b) { return vec4(_mm_set1_ps(a)) / b; }*/

	inline vec3 xyz() { return vec3{ x, y, z }; }

	inline friend vec4 operator-(const vec4& a) {
		return{ -a.x, -a.y, -a.z, -a.w }; }

	inline friend vec4 abs(const vec4& a) {
		return{ abs(a.x), abs(a.y), abs(a.z), abs(a.w) }; }

	inline friend float dot(const vec4& a, const vec4& b) {
		return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

	inline friend vec4 normalize(const vec4 &a) {
		/*normalize a to length 1.0*/
		return a / sqrt(dot(a, a)); }

	inline friend float hadd(const vec4& a) {
		return a.x + a.y + a.z + a.w; }

	inline friend float hmax(const vec4& a) {
		using std::max;
		return max(max(max(a.x, a.y), a.z), a.w); }

	inline friend float hmin(const vec4& a) {
		using std::min;
		return min(min(min(a.x, a.y), a.z), a.w); }

	inline friend vec4 vmin(const vec4& a, const vec4& b) {
		using std::min;
		return{ min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w) }; }

	inline friend vec4 vmax(const vec4& a, const vec4& b) {
		using std::max;
		return{ max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w) }; }

	inline friend vec4 cross(const vec4& a, const vec4& b) {
		return{
			a.y*b.z - a.z*b.y,
			a.z*b.x - a.x*b.z,
			a.x*b.y - a.y*b.x,
			a.w*b.w - a.w*b.w }; }

/*	inline vec4 from_rgb(const int rgb) {
		return vec4{
			(float)((rgb >> 16) & 0xff) / 256.0f,
			(float)((rgb >> 8) & 0xff) / 256.0f,
			(float)((rgb >> 0) & 0xff) / 256.0f}; }*/

	inline friend vec4 lerp(const vec4 &a, const vec4 &b, const float t) {
		return (1.0f - t)*a + t*b; }

	union {
		struct { float x, y, z, w; };
		struct { float r, g, b, a; }; }; };


struct alignas(8) ivec2 {

	inline ivec2() = default;
	inline constexpr ivec2(const int a, const int b) noexcept : x(a), y(b){}
	inline constexpr ivec2(const int a) noexcept : x(a), y(a) {}

	inline constexpr ivec2 operator+(const ivec2& b) const { return ivec2(x + b.x, y + b.y); }
	inline constexpr ivec2 operator-(const ivec2& b) const { return ivec2(x - b.x, y - b.y); }
	inline constexpr ivec2 operator*(const ivec2& b) const { return ivec2(x * b.x, y * b.y); }
	inline constexpr ivec2 operator/(const ivec2& b) const { return ivec2(x / b.x, y / b.y); }

	inline ivec2& operator+=(const ivec2& b) { x += b.x; y += b.y; return *this; }
	inline ivec2& operator-=(const ivec2& b) { x -= b.x; y -= b.y; return *this; }

	inline constexpr ivec2 operator+(int b) const { return ivec2(x + b, y + b); }
	inline constexpr ivec2 operator-(int b) const { return ivec2(x - b, y - b); }

	static ivec2 min(const ivec2& a, const ivec2& b) {
		return {std::min(a.x, b.x),
		        std::min(a.y, b.y)}; }

	static ivec2 max(const ivec2& a, const ivec2& b) {
		return {std::max(a.x, b.x),
		        std::max(a.y, b.y)}; }

	int x, y; };

inline bool operator==(const ivec2& a, const ivec2& b) {
	return (a.x == b.x && a.y == b.y); }

inline bool operator!=(const ivec2& a, const ivec2& b) {
	return (a.x != b.x || a.y != b.y); }


struct ivec4 {

	inline ivec4() = default;
	inline constexpr ivec4(const int a, const int b, const int c, const int d) noexcept :x(a), y(b), z(c), w(d) {}
	inline constexpr ivec4(const int a) noexcept : x(a), y(a), z(a), w(a) {}

	inline ivec4& operator+=(const ivec4& b) { x += b.x; y += b.y; z += b.z; return *this; }
	inline ivec4& operator-=(const ivec4& b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
	inline ivec4& operator*=(const ivec4& b) { x *= b.x; y *= b.y; z *= b.z; return *this; }

public:
	union {
		struct { int x, y, z, w; };
		int32_t si[4]; }; };


inline vec4 pdiv(const vec4& p) {
	/*perspective divide, scalar version

	float operations _must_ match SoA version !

	Args:
		p: point in homogeneous clip-space

	Returns:
		x/w, y/w, z/w, 1/w
	*/
	float one_over_w;
	_mm_store_ss(&one_over_w, _mm_rcp_ss(_mm_set_ss(p.w)));
	return vec4{ p.x*one_over_w, p.y*one_over_w, p.z*one_over_w, one_over_w }; }


inline bool almost_equal(const float a, const float b) {
	return abs(a - b) < 0.0001; }

inline bool almost_equal(const vec4& a, const vec4& b) {
	return (
		almost_equal(a.x, b.x) &&
		almost_equal(a.y, b.y) &&
		almost_equal(a.z, b.z) &&
		almost_equal(a.w, b.w)
	); }

inline bool almost_equal(const vec3& a, const vec3& b) {
	return (
		almost_equal(a.x, b.x) &&
		almost_equal(a.y, b.y) &&
		almost_equal(a.z, b.z)
	); }


std::ostream& operator<<(std::ostream& os, const vec4& v);
std::ostream& operator<<(std::ostream& os, const vec3& v);
std::ostream& operator<<(std::ostream& os, const ivec2& v);
