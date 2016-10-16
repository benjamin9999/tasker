#pragma once
#include <intrin.h>
#include <cstring>
#include <cmath>

#include "vec_soa.h"
#include "vec.h"


struct alignas(64) mat4 {

	inline mat4() = default;
	inline mat4(
		const float a00, const float a01, const float a02, const float a03,
		const float a10, const float a11, const float a12, const float a13,
		const float a20, const float a21, const float a22, const float a23,
		const float a30, const float a31, const float a32, const float a33) noexcept :ff({ {
				a00, a10, a20, a30, a01, a11, a21, a31, a02, a12, a22, a32, a03, a13, a23, a33} }) {}

	inline mat4(const std::array<float, 16>& src) {
		std::copy(src.begin(), src.end(), ff.begin()); }

	void print();

	static inline mat4 ident() {
		return mat4{
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 }; }

	static inline mat4 scale(const vec3& a) {
		return mat4{
			a.x, 0, 0, 0,
			0, a.y, 0, 0,
			0, 0, a.z, 0,
			0, 0, 0, 1 }; }

	static inline mat4 scale(const vec4& a) {
		return mat4{
			a.x, 0, 0, 0,
			0, a.y, 0, 0,
			0, 0, a.z, 0,
			0, 0, 0, 1 }; }

	static inline mat4 scale(const float x, const float y, const float z) {
		return mat4{
			x, 0, 0, 0,
			0, y, 0, 0,
			0, 0, z, 0,
			0, 0, 0, 1 }; }

	static inline mat4 translate(const vec4& a) {
		_ASSERT(almost_equal(a.w, 1.0f));
		return mat4{
			1, 0, 0, a.x,
			0, 1, 0, a.y,
			0, 0, 1, a.z,
			0, 0, 0, 1 }; }

	static inline mat4 translate(const vec3& a) {
		return mat4{
			1, 0, 0, a.x,
			0, 1, 0, a.y,
			0, 0, 1, a.z,
			0, 0, 0, 1 }; }

	static inline mat4 translate(const float x, const float y, const float z) {
		return mat4{
			1, 0, 0, x,
			0, 1, 0, y,
			0, 0, 1, z,
			0, 0, 0, 1 }; }

	static inline mat4 rotate(const float theta, const float x, const float y, const float z) {
		/*glRotate() matrix*/
		const float s = sin(theta);
		const float c = cos(theta);
		const float t = 1.0f - c;

		const float tx = t * x;
		const float ty = t * y;
		const float tz = t * z;

		const float sz = s * z;
		const float sy = s * y;
		const float sx = s * x;

		return mat4{
			tx*x + c,  tx*y - sz, tx*z + sy, 0,
			tx*y + sz, ty*y + c,  ty*z - sx, 0,
			tx*z - sy, ty*z + sx, tz*z + c,  0,
			0,         0,         0,         1 }; }

	union {
		float cr[4][4]; // col,row
		std::array<float, 16> ff; }; };


inline mat4 transpose(const mat4& m) {
	return mat4{
		m.ff[ 0], m.ff[ 1], m.ff[ 2], m.ff[ 3],
		m.ff[ 4], m.ff[ 5], m.ff[ 6], m.ff[ 7],
		m.ff[ 8], m.ff[ 9], m.ff[10], m.ff[11],
		m.ff[12], m.ff[13], m.ff[14], m.ff[15] }; }


mat4 inverse(const mat4& m);


inline vec4 operator*(const mat4& a, const vec4& b) {
	return vec4{
		a.ff[0] * b.x + a.ff[4] * b.y + a.ff[8] * b.z + a.ff[12] * b.w,
		a.ff[1] * b.x + a.ff[5] * b.y + a.ff[9] * b.z + a.ff[13] * b.w,
		a.ff[2] * b.x + a.ff[6] * b.y + a.ff[10] * b.z + a.ff[14] * b.w,
		a.ff[3] * b.x + a.ff[7] * b.y + a.ff[11] * b.z + a.ff[15] * b.w
	}; }


inline vec4 mul_w1(const mat4& a, const vec4& b) {
	_ASSERT(almost_equal(b.w, 1.0f));
	return vec4{
		a.ff[0] * b.x + a.ff[4] * b.y + a.ff[ 8] * b.z + a.ff[12],
		a.ff[1] * b.x + a.ff[5] * b.y + a.ff[ 9] * b.z + a.ff[13],
		a.ff[2] * b.x + a.ff[6] * b.y + a.ff[10] * b.z + a.ff[14],
		a.ff[3] * b.x + a.ff[7] * b.y + a.ff[11] * b.z + a.ff[15]
	}; }


inline vec4 mul_w1(const mat4& a, const vec3& b) {
	return vec4{
		a.ff[0] * b.x + a.ff[4] * b.y + a.ff[ 8] * b.z + a.ff[12],
		a.ff[1] * b.x + a.ff[5] * b.y + a.ff[ 9] * b.z + a.ff[13],
		a.ff[2] * b.x + a.ff[6] * b.y + a.ff[10] * b.z + a.ff[14],
		a.ff[3] * b.x + a.ff[7] * b.y + a.ff[11] * b.z + a.ff[15]
	}; }


inline vec4 mul_w0(const mat4& a, const vec4& b) {
	_ASSERT(almost_equal(b.w, 0.0f));
	return vec4{
		a.ff[0] * b.x + a.ff[4] * b.y + a.ff[8] * b.z,
		a.ff[1] * b.x + a.ff[5] * b.y + a.ff[9] * b.z,
		a.ff[2] * b.x + a.ff[6] * b.y + a.ff[10] * b.z,
		a.ff[3] * b.x + a.ff[7] * b.y + a.ff[11] * b.z
	}; }


inline vec4 mul_w0(const mat4& a, const vec3& b) {
	return vec4{
		a.ff[0] * b.x + a.ff[4] * b.y + a.ff[ 8] * b.z,
		a.ff[1] * b.x + a.ff[5] * b.y + a.ff[ 9] * b.z,
		a.ff[2] * b.x + a.ff[6] * b.y + a.ff[10] * b.z,
		a.ff[3] * b.x + a.ff[7] * b.y + a.ff[11] * b.z
	}; }


inline mat4 operator*(const mat4& lhs, const mat4& rhs) {
	mat4 r;
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			float ax = 0;
			for (int n = 0; n < 4; n++) {
				ax += lhs.cr[n][row] * rhs.cr[col][n]; }
			r.cr[col][row] = ax; }}
	return r; }


class Mat4Stack {
public:
	Mat4Stack() :sp(0) {}
	void push() {
		stack[sp + 1] = stack[sp];
		sp++; }
	void pop() {
		sp--; }
	const mat4& top() const {
		return stack[sp]; }
	void clear() {
		sp = 0; }
	void mul(const mat4& m) {
		stack[sp] = stack[sp] * m; }
	void load(const mat4& m) {
		stack[sp] = m; }
private:
	std::array<mat4, 16> stack;
	int sp; };


inline mat4 look_at(const vec4& pos, const vec4& center, const vec4& up) {
	vec4 forward = normalize(center - pos);
	vec4 side = normalize(cross(forward, up));
	vec4 up2 = cross(side, forward);

	mat4 mrot = mat4{
		side.x, side.y, side.z, 0,
		up2.x, up2.y, up2.z, 0,
		-forward.x, -forward.y, -forward.z, 0,
		0, 0, 0, 1};

	mat4 mpos = mat4{
		1, 0, 0, -pos.x,
		0, 1, 0, -pos.y,
		0, 0, 1, -pos.z,
		0, 0, 0, 1};

	return mrot * mpos; }


inline mat4 make_glFrustum(const float l, const float r, const float b, const float t, const float n, const float f) {
	/*standard opengl perspective transform*/
	return mat4{
	    (2*n)/(r-l),      0,         (r+l)/(r-l),        0,
	         0,      (2*n)/(t-b),    (t+b)/(t-b),        0,
	         0,           0,      -((f+n)/(f-n)),  -((2*f*n)/(f-n)),
	         0,           0,            -1,              0};}


inline mat4 make_glOrtho(const float l, const float r, const float b, const float t, const float n, const float f) {
	/*standard opengl orthographic transform*/
	return mat4{
	 	   2/(r-l),   0,        0,      -((r+l)/(r-l)),
	         0,    2/(t-b),     0,      -((t+b)/(t-b)),
	         0,       0,    -2/(f-n),   -((f+n)/(f-n)),
	         0,       0,        0,            1};}


inline mat4 create_opengl_pinf(const float left, const float right, const float bottom, const float top, const float near, const float far) {
	/*perspective transform with infinite far clip distance
	from the paper
	"Practical and Robust Stenciled Shadow Volumes for Hardware-Accelerated Rendering" (2002, nvidia)
	*/
	return mat4(
		(2*near)/(right-left)           ,0           , (right+left)/(right-left)            ,0
		         ,0           ,(2*near)/(top-bottom) , (top+bottom)/(top-bottom)            ,0
		         ,0                     ,0                       ,-1                     ,-2*near
		         ,0                     ,0                       ,-1                        ,0
	); }


inline mat4 make_gluPerspective(const float fovy, const float aspect, const float znear, const float zfar) {
	auto h = tan((fovy / 2) / 180 * M_PI) * znear;
	auto w = h * aspect;
	return make_glFrustum(-w, w, -h, h, znear, zfar); }


inline mat4 make_device_matrix(const int width, const int height) {
	const auto x0 = 0;
	const auto y0 = 0;

	auto md_yfix = mat4(
		   1,        0,       0,       0,
		   0,       -1,       0,       0,
		   0,        0,       1,       0,
		   0,        0,       0,       1);

	auto md_scale = mat4(
		float(width/2), 0,          0,       0,
		     0,    float(height/2), 0,       0,
		     0,         0,          1.0f,    0, ///zfar-znear, // 0,
		     0,         0,          0,       1.0f);

	auto md_origin = mat4(
		   1,        0,       0,  float(x0+width /2),
		   0,        1,       0,  float(y0+height/2),
		   0,        0,       1,       0,
		   0,        0,       0,       1);

	//md = mat4_mul(md_origin, mat4_mul(md_scale, md_yfix));
	return md_origin * (md_scale * md_yfix); }


struct qmat4 {
	qmat4(const mat4& m) :f({ {
		/*these are transposed, the indices match openGL*/
		mvec4f{m.ff[0x00]}, mvec4f{m.ff[0x01]}, mvec4f{m.ff[0x02]}, mvec4f{m.ff[0x03]},
		mvec4f{m.ff[0x04]}, mvec4f{m.ff[0x05]}, mvec4f{m.ff[0x06]}, mvec4f{m.ff[0x07]},
		mvec4f{m.ff[0x08]}, mvec4f{m.ff[0x09]}, mvec4f{m.ff[0x0a]}, mvec4f{m.ff[0x0b]},
		mvec4f{m.ff[0x0c]}, mvec4f{m.ff[0x0d]}, mvec4f{m.ff[0x0e]}, mvec4f{m.ff[0x0f]} } }) {}

	std::array<mvec4f, 16> f; };

inline qfloat4 mul_w1(const qmat4& a, const qfloat3& b) {
	return qfloat4{
		a.f[0] * b.x + a.f[4] * b.y + a.f[8] * b.z + a.f[12],
		a.f[1] * b.x + a.f[5] * b.y + a.f[9] * b.z + a.f[13],
		a.f[2] * b.x + a.f[6] * b.y + a.f[10] * b.z + a.f[14],
		a.f[3] * b.x + a.f[7] * b.y + a.f[11] * b.z + a.f[15]
	}; }

inline qfloat4 mul_w1(const qmat4& a, const qfloat4& b) {
	return qfloat4{
		a.f[0]*b.x + a.f[4]*b.y + a.f[8]*b.z + a.f[12]*b.w,
		a.f[1]*b.x + a.f[5]*b.y + a.f[9]*b.z + a.f[13]*b.w,
		a.f[2]*b.x + a.f[6]*b.y + a.f[10]*b.z + a.f[14]*b.w,
		a.f[3]*b.x + a.f[7]*b.y + a.f[11]*b.z + a.f[15]*b.w
	}; }

inline qfloat3 mul_w0(const qmat4& a, const qfloat3& b) {
	return qfloat3{
		a.f[0] * b.x + a.f[4] * b.y + a.f[8] * b.z,
		a.f[1] * b.x + a.f[5] * b.y + a.f[9] * b.z,
		a.f[2] * b.x + a.f[6] * b.y + a.f[10] * b.z
	}; }
