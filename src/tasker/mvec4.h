#pragma once
#include <intrin.h>
#include <cstdint>

#include "sse_pow.h"
#include "vec.h"

//#define USE_SSE3

const __m128 SIGNMASK = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
const __m128 XYZ0MASK = _mm_cmpeq_ps(_mm_setzero_ps(), _mm_set_ps(1, 0, 0, 0));


/*
_mm_shufd replacement for SSE2 from LiraNuna
https://github.com/LiraNuna/glsl-sse2/blob/master/source/mat4.h
*/
#define _mm_shufd(xmm,mask) _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm),mask))


inline __m128i sse2_mul32(const __m128i& a, const __m128i& b) {
	/*signed 32-bit integer mul with low 32 bit result

	sse2 compatible.  sse4.1 can use _mm_mullo_epi32 instead
	
	http://stackoverflow.com/questions/10500766/sse-multiplication-of-4-32-bit-integers
	*/
	// mul 2, 0
	const __m128i tmp1 = _mm_mul_epu32(a, b);

	// mul 3, 1
	const __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(a, 4), _mm_srli_si128(b, 4));

	// shuffle results to [63...0] and pack
	return _mm_unpacklo_epi32(
		_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0)),
		_mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0))
	); }


struct alignas(16) mvec4i {

	inline mvec4i() = default;
	inline mvec4i(const int a, const int b, const int c, const int d) : v(_mm_set_epi32(d, c, b, a)){}
	inline mvec4i(const int a) : v(_mm_set_epi32(a, a, a, a)){}
	inline mvec4i(__m128i m) : v(m){}
	inline mvec4i(const mvec4i& x) : v(x.v) {}

	static inline mvec4i zero() { return mvec4i(_mm_setzero_si128()); }
	static const mvec4i two;

	inline mvec4i& operator=(const mvec4i& b) { v = b.v;  return *this; }

	inline mvec4i& operator+=(const mvec4i& b) { v = _mm_add_epi32(v, b.v); return *this; }
	inline mvec4i& operator-=(const mvec4i& b) { v = _mm_sub_epi32(v, b.v); return *this; }
	inline mvec4i& operator*=(const mvec4i& b) { v = sse2_mul32(v, b.v); return *this; }

	inline mvec4i& operator|=(const mvec4i& b) { v = _mm_or_si128(v, b.v); return *this; }
	inline mvec4i& operator&=(const mvec4i& b) { v = _mm_and_si128(v, b.v); return *this; }

public:
	union {
		__m128i v;
		__m128 f;
		struct { int x, y, z, w; };
		uint32_t ui[4];
		int32_t si[4];
	};
};


struct alignas(16) mvec4f {

	inline mvec4f() = default;
	//inline mvec4f(const float a, const float b, const float c, const float d) : v(_mm_set_ps(d, c, b, a)) {}
	inline mvec4f(const float a) : v(_mm_set1_ps(a)) {}
	inline mvec4f(__m128 m) : v(m) {}

	static inline mvec4f load(const __m128 *a) {
		return mvec4f(_mm_load_ps(reinterpret_cast<const float*>(a)));
	}

	inline void store(__m128 *out) const {
		_mm_store_ps(reinterpret_cast<float*>(out), this->v); }

	inline void stream(__m128 *out) const {
		_mm_stream_ps(reinterpret_cast<float*>(out), this->v); }

	/*static inline mvec4f load(mvec4f *a) {
	return _mm_load_ps(reinterpret_cast<float*>(&a->v)); }*/

	static inline mvec4f zero() { return mvec4f(_mm_setzero_ps()); }


	inline vec4 xyzw() {
		vec4 a;
		_mm_store_ps(reinterpret_cast<float*>(&a), v);
		return a; }
	inline vec3 xyz() { return vec3{ get_x(), get_y(), get_z() }; }
	inline vec2 xy() { return vec2{ get_x(), get_y() }; }


	inline mvec4f operator+(const mvec4f& b) const { return _mm_add_ps(v, b.v); }
	inline mvec4f operator-(const mvec4f& b) const { return _mm_sub_ps(v, b.v); }
	inline mvec4f operator*(const mvec4f& b) const { return _mm_mul_ps(v, b.v); }
	inline mvec4f operator/(const mvec4f& b) const { return _mm_div_ps(v, b.v); }

	inline mvec4f& operator+=(const mvec4f& b) { v = _mm_add_ps(v, b.v); return *this; }
	inline mvec4f& operator-=(const mvec4f& b) { v = _mm_sub_ps(v, b.v); return *this; }
	inline mvec4f& operator*=(const mvec4f& b) { v = _mm_mul_ps(v, b.v); return *this; }
	inline mvec4f& operator/=(const mvec4f& b) { v = _mm_div_ps(v, b.v); return *this; }

	inline mvec4f operator+(const float b) const { return _mm_add_ps(v, _mm_set1_ps(b)); }
	inline mvec4f operator-(const float b) const { return _mm_sub_ps(v, _mm_set1_ps(b)); }
	inline mvec4f operator*(const float b) const { return _mm_mul_ps(v, _mm_set1_ps(b)); }
	inline mvec4f operator/(const float b) const { return _mm_div_ps(v, _mm_set1_ps(b)); }

	inline mvec4f& operator+=(const float b) { v = _mm_add_ps(v, _mm_set1_ps(b)); return *this; }
	inline mvec4f& operator-=(const float b) { v = _mm_sub_ps(v, _mm_set1_ps(b)); return *this; }
	inline mvec4f& operator*=(const float b) { v = _mm_mul_ps(v, _mm_set1_ps(b)); return *this; }
	inline mvec4f& operator/=(const float b) { v = _mm_div_ps(v, _mm_set1_ps(b)); return *this; }

	inline friend mvec4f operator*(const float a, const mvec4f& b) { return _mm_mul_ps(_mm_set1_ps(a), b.v); }

	inline friend mvec4f operator-(const mvec4f& a) { return mvec4f(_mm_xor_ps(a.v, SIGNMASK)); }
	inline friend mvec4f abs(const mvec4f& a) { return mvec4f(_mm_andnot_ps(SIGNMASK, a.v)); }

	inline friend mvec4f sqrt(const mvec4f& a) {
		return _mm_sqrt_ps(a.v); }

	inline friend mvec4f rsqrt(const mvec4f& a) {
		return _mm_rsqrt_ps(a.v); }

	inline mvec4f xxxx() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0)); }
	inline mvec4f yyyy() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)); }
	inline mvec4f zzzz() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)); }
	inline mvec4f wwww() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3)); }

	inline mvec4f yzxw() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 2, 1)); }
	inline mvec4f zxyw() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 1, 0, 2)); }

	inline mvec4f xyxy() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 0, 1, 0)); }
	inline mvec4f zwzw() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 2, 3, 2)); }

	inline mvec4f yyww() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 1, 1)); }
	inline mvec4f xxzz() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 0, 0)); }

	inline __m128 xyz0() const { return _mm_and_ps(v, XYZ0MASK); }

	inline float get_x() const { return _mm_cvtss_f32(v); }
	inline float get_y() const { return _mm_cvtss_f32(yyyy().v); }
	inline float get_z() const { return _mm_cvtss_f32(zzzz().v); }
	inline float get_w() const { return _mm_cvtss_f32(wwww().v); }

	inline vec3 get_xyz() const {
		return vec3{ get_x(), get_y(), get_z() }; }

	inline vec2 get_xy() const {
		return vec2{ get_x(), get_y() }; }

	inline friend mvec4f cross(const mvec4f &a, const mvec4f &b) {
		/*cross product*/
		return a.yzxw()*b.zxyw() - a.zxyw()*b.yzxw(); }

	inline friend float hadd(const mvec4f& a) {
		__m128 r0 = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(0, 3, 2, 1));
		__m128 r1 = _mm_add_ps(a.v, r0);
		r0 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(2, 2, 2, 2));
		r1 = _mm_add_ss(r0, r1);
		return _mm_cvtss_f32(r1); }

	inline friend float hmax(const mvec4f& a) {
		__m128 r0 = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(0, 3, 2, 1));
		__m128 r1 = _mm_max_ps(a.v, r0);
		r0 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(2, 2, 2, 2));
		r1 = _mm_max_ss(r0, r1);
		return _mm_cvtss_f32(r1); }

	inline friend float hmin(const mvec4f& a) {
		__m128 r0 = _mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(0, 3, 2, 1));
		__m128 r1 = _mm_min_ps(a.v, r0);
		r0 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(2, 2, 2, 2));
		r1 = _mm_min_ss(r0, r1);
		return _mm_cvtss_f32(r1); }

	inline friend mvec4f lerp(const mvec4f &a, const mvec4f &b, const float t) {
		return (1.0f - t)*a + t*b; }
		//return a + mvec4f{t} * (b - a); }  // xxx faster, but incorrect?

	inline friend mvec4f lerp(const mvec4f &a, const mvec4f &b, const mvec4f& t) {
		return (mvec4f{ 1.0f }-t)*a + t*b; }
		//return a + t * (b - a); }   // xxx faster, but incorrect?

	union {
		__m128 v;
		struct { float x, y, z, w; };
		struct { float r, g, b, a; }; }; };


inline mvec4f operator&(const mvec4f& a, const mvec4f& b) {
	return mvec4f(_mm_and_ps(a.v, b.v)); }

inline mvec4f operator|(const mvec4f& a, const mvec4f& b) {
	return mvec4f(_mm_or_ps(a.v, b.v)); }

inline mvec4f operator-(const float& lhs, const mvec4f& rhs) { return mvec4f{ lhs } -rhs; }
inline mvec4f operator+(const float& lhs, const mvec4f& rhs) { return mvec4f{ lhs } +rhs; }

inline mvec4f vmin(const mvec4f& a, const mvec4f& b) { return mvec4f(_mm_min_ps(a.v, b.v)); }
inline mvec4f vmax(const mvec4f& a, const mvec4f& b) { return mvec4f(_mm_max_ps(a.v, b.v)); }


inline mvec4i operator+(const mvec4i& a, const mvec4i& b) { return mvec4i(_mm_add_epi32(a.v, b.v)); }
inline mvec4i operator-(const mvec4i& a, const mvec4i& b) { return mvec4i(_mm_sub_epi32(a.v, b.v)); }
inline mvec4i operator*(const mvec4i& a, const mvec4i& b) { return mvec4i(sse2_mul32(a.v, b.v)); }
inline mvec4i operator|(const mvec4i& a, const mvec4i& b) { return mvec4i(_mm_or_si128(a.v, b.v)); }
inline mvec4i operator&(const mvec4i& a, const mvec4i& b) { return mvec4i(_mm_and_si128(a.v, b.v)); }

inline mvec4i andnot(const mvec4i& a, const mvec4i& b) { return mvec4i(_mm_andnot_si128(a.v, b.v)); }

inline mvec4f itof(const mvec4i& a) { return mvec4f(_mm_cvtepi32_ps(a.v)); }
inline mvec4i ftoi_round(const mvec4f& a) { return mvec4i(_mm_cvtps_epi32(a.v)); }
inline mvec4i ftoi(const mvec4f& a) { return mvec4i(_mm_cvttps_epi32(a.v)); }

inline mvec4i cmplt(const mvec4i&a, const mvec4i& b) { return mvec4i(_mm_cmplt_epi32(a.v, b.v)); }
//inline mvec4i cmple(const mvec4i&a, const mvec4i& b) { return mvec4i(_mm_cmple_epi32(a.v,b.v)); }
//inline mvec4i cmpge(const mvec4i&a, const mvec4i& b) { return mvec4i(_mm_cmpge_epi32(a.v,b.v)); }
inline mvec4i cmpeq(const mvec4i&a, const mvec4i& b) { return mvec4i(_mm_cmpeq_epi32(a.v, b.v)); }
inline mvec4i cmpgt(const mvec4i&a, const mvec4i& b) { return mvec4i(_mm_cmpgt_epi32(a.v, b.v)); }

inline mvec4f cmplt(const mvec4f&a, const mvec4f& b) { return mvec4f(_mm_cmplt_ps(a.v, b.v)); }
inline mvec4f cmple(const mvec4f&a, const mvec4f& b) { return mvec4f(_mm_cmple_ps(a.v, b.v)); }
inline mvec4f cmpge(const mvec4f&a, const mvec4f& b) { return mvec4f(_mm_cmpge_ps(a.v, b.v)); }
inline mvec4f cmpgt(const mvec4f&a, const mvec4f& b) { return mvec4f(_mm_cmpgt_ps(a.v, b.v)); }

inline mvec4i float2bits(const  mvec4f &a) { return mvec4i(_mm_castps_si128(a.v)); }
inline mvec4f bits2float(const mvec4i &a) { return  mvec4f(_mm_castsi128_ps(a.v)); }

inline int movemask(const mvec4f& a) { return _mm_movemask_ps(a.v); }


inline mvec4f saturate(const mvec4f& a) {
	/*clamp a to the range [0...1.0]*/
	return vmax(vmin(a, mvec4f(1.0f)), mvec4f::zero()); }


inline mvec4f fract(const mvec4f& a) {
	/*fractional part of a*/
	return  a - itof(ftoi_round(a - mvec4f(0.5f))); }


/*
* select*() based on ryg's helpersse
* sse2 equivalents first found in "sseplus" sourcecode
* http://sseplus.sourceforge.net/group__emulated___s_s_e2.html#g3065fcafc03eed79c9d7539435c3257c
*
* i split them into bits and sign so i can save a step when I know that I have
* a complete bitmask or not
*/
inline mvec4f selectbits(const mvec4f& a, const mvec4f& b, const mvec4i& mask) {
	const mvec4i a2 = andnot(mask, float2bits(a));  // keep bits in a where mask is 0000's
	const mvec4i b2 = float2bits(b) & mask;  // keep bits in b where mask is 1111's
	return bits2float(a2 | b2); }


inline mvec4f selectbits(const mvec4f& a, const mvec4f& b, const mvec4f& mask) {
	return selectbits(a, b, float2bits(mask)); }


//inline vec4 blendv(const vec4& a, const vec4& b, const mvec4i& mask) {
	/*sse 4.1 blendv wrapper*/
//	return _mm_blendv_ps( a.v, b.v, bits2float(mask).v ); }


inline mvec4f select_by_sign(const mvec4f& a, const mvec4f& b, const mvec4i& mask) {
	/*this is _mm_blendv_ps() for SSE2*/
	const mvec4i newmask(_mm_srai_epi32(mask.v, 31));
	return selectbits(a, b, newmask); }


inline mvec4f select_by_sign(const mvec4f& a, const mvec4f& b, const mvec4f& mask) {
	const mvec4i newmask(_mm_srai_epi32(float2bits(mask).v, 31));
	return selectbits(a, b, newmask); }


inline mvec4f oneover(const mvec4f& a) {
	return mvec4f(_mm_rcp_ps(a.v)); }


//inline vec4 abs( const vec4& a ) { return andnot(signmask,a); }


inline mvec4f pow(const mvec4f& x, const mvec4f& y) {
	return mvec4f(exp2f4(_mm_mul_ps(log2f4(x.v), y.v))); }


template<int N> inline mvec4i shl(const mvec4i& x) { return mvec4i(_mm_slli_epi32(x.v, N)); }
template<int N> inline mvec4i sar(const mvec4i& x) { return mvec4i(_mm_srai_epi32(x.v, N)); }
template<int N> inline mvec4i shr(const mvec4i& x) { return mvec4i(_mm_srli_epi32(x.v, N)); }


/*
some functions for sin/cos ...
based mostly on Nick's thread here:
http://devmaster.net/forums/topic/4648-fast-and-accurate-sinecosine/page__st__80
also referenced on pouet:
http://www.pouet.scene.org/topic.php?which=9132&page=1
also interesting:
ISPC stdlib sincos, https://github.com/ispc/ispc/blob/master/stdlib.ispc
and another here..
http://gruntthepeon.free.fr/ssemath/

I'll use nick's.
It approximates sin() from a parabola, so it requires the input be in
the range -PI ... +PI.

These functions can be combined or decomposed to optimize a few
instructions if the inputs are known to fit, and also if they are in the range -1 ... +1
*/


inline mvec4f wrap1(const mvec4f& a) {
	/*wrap a float to the range -1 ... +1 */
	const __m128 magic = _mm_set1_ps(25165824.0f); // 0x4bc00000
	const mvec4f z = a + magic;
	return a - (z - magic); }


template<bool high_precision>
inline mvec4f nick_sin(const mvec4f& x) {
	/*implementation of nick's original -pi...pi version*/
	const mvec4f B(4 / M_PI);
	const mvec4f C(-4 / (M_PI * M_PI));

	mvec4f y = B*x + C*x * abs(x);

	if (high_precision) {
		const mvec4f P(0.225);
		y = P * (y*abs(y) - y) + y; }

	return y; }


inline mvec4f sin1hp(const mvec4f& x) {
	/*higher-precision sin(), input must be in the range -1 ... 1 */
	const mvec4f Q(3.1f);
	const mvec4f P(3.6f);
	mvec4f y = x - (x * abs(x));
	return y * (Q + P * abs(y)); }


inline mvec4f sin1lp(const mvec4f& x) {
	/*low-precision sin(), input must be in the range -1 ... 1*/
	return mvec4f(4.0f) * (x - x * abs(x)); }


inline mvec4f sin(const mvec4f& x) {
	/*drop-in-sin() replacement

	Args:
		x: input in radians
	*/
	mvec4f M = x * mvec4f(1.0f / M_PI);  // scale x to -1...1
	M = wrap1(M);  // wrap to -1...1
	return sin1hp(M); }


inline mvec4f cos(const mvec4f& x) {
	/*drop-in-cos() replacement

	cos(x) = sin( pi/2 + x)
	if... -1= -pi,  so...
	bring into range -1 ... 1, then add 0.5, then wrap.
	*/
	mvec4f M = x * mvec4f(1.0f / M_PI);	// scale x to -1...1
	M = wrap1(M + mvec4f(0.5));		// add 0.5 (PI/2) and wrap
	return sin1hp(M); }

inline void sincos(const mvec4f& x, mvec4f &os, mvec4f& oc) {
	/*sin() & cos(), saving one mul.*/
	mvec4f M = x * mvec4f(1.0f / M_PI);	// scale x to -1...1
	os = sin1hp(wrap1(M));
	oc = sin1hp(wrap1(M + mvec4f(0.5))); }

inline mvec4f smoothstep(const mvec4f& a, const mvec4f& b, const mvec4f& t) {
	const mvec4f x = saturate((t - a) / (b - a));
	return x*x * (mvec4f(3) - mvec4f(2) * x); }
