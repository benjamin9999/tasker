#include "stdafx.h"
#include <iostream>

#include <fmt/format.h>

#include "mat4.h"

using namespace std;


/*void mat4::print() {
	using fmt::printf;
	const auto& v1 = v[0];
	const auto& v2 = v[1];
	const auto& v3 = v[2];
	const auto& v4 = v[3];
	printf("[% 11.4f,% 11.4f,% 11.4f,% 11.4f]\n", v1._x, v2._x, v3._x, v4._x);
	printf("[% 11.4f,% 11.4f,% 11.4f,% 11.4f]\n", v1._y, v2._y, v3._y, v4._y);
	printf("[% 11.4f,% 11.4f,% 11.4f,% 11.4f]\n", v1._z, v2._z, v3._z, v4._z);
	printf("[% 11.4f,% 11.4f,% 11.4f,% 11.4f]\n", v1._w, v2._w, v3._w, v4._w); }*/


mat4 inverse(const mat4& src) {
	/*compute the inverse of the mat4 src
	
	the last remaining zed3d bits ;)
	*/
	mat4 dst;
	auto& inv = dst.ff;
	auto& m = src.ff;

	inv[0] = (
		m[ 5] * m[10] * m[15] -
		m[ 5] * m[11] * m[14] -
		m[ 9] * m[ 6] * m[15] +
		m[ 9] * m[ 7] * m[14] +
		m[13] * m[ 6] * m[11] -
		m[13] * m[ 7] * m[10]);

	inv[4] = (
		-m[4] * m[10] * m[15] +
		m[ 4] * m[11] * m[14] +
		m[ 8] * m[ 6] * m[15] -
		m[ 8] * m[ 7] * m[14] -
		m[12] * m[ 6] * m[11] +
		m[12] * m[ 7] * m[10]);

	inv[8] = (
		m[ 4] * m[ 9] * m[15] -
		m[ 4] * m[11] * m[13] -
		m[ 8] * m[ 5] * m[15] +
		m[ 8] * m[ 7] * m[13] +
		m[12] * m[ 5] * m[11] -
		m[12] * m[ 7] * m[ 9]);

	inv[12] = (
		-m[4] * m[ 9] * m[14] +
		m[ 4] * m[10] * m[13] +
		m[ 8] * m[ 5] * m[14] -
		m[ 8] * m[ 6] * m[13] -
		m[12] * m[ 5] * m[10] +
		m[12] * m[ 6] * m[ 9]);

	inv[1] = (
		-m[1] * m[10] * m[15] +
		m[ 1] * m[11] * m[14] +
		m[ 9] * m[ 2] * m[15] -
		m[ 9] * m[ 3] * m[14] -
		m[13] * m[ 2] * m[11] +
		m[13] * m[ 3] * m[10]);

	inv[5] = (
		m[ 0] * m[10] * m[15] -
		m[ 0] * m[11] * m[14] -
		m[ 8] * m[ 2] * m[15] +
		m[ 8] * m[ 3] * m[14] +
		m[12] * m[ 2] * m[11] -
		m[12] * m[ 3] * m[10]);

	inv[9] = (
		-m[0] * m[ 9] * m[15] +
		m[ 0] * m[11] * m[13] +
		m[ 8] * m[ 1] * m[15] -
		m[ 8] * m[ 3] * m[13] -
		m[12] * m[ 1] * m[11] +
		m[12] * m[ 3] * m[ 9]);

	inv[13] = (
		m[ 0] * m[ 9] * m[14] -
		m[ 0] * m[10] * m[13] -
		m[ 8] * m[ 1] * m[14] +
		m[ 8] * m[ 2] * m[13] +
		m[12] * m[ 1] * m[10] -
		m[12] * m[ 2] * m[ 9]);

	inv[2] = (
		m[ 1] * m[ 6] * m[15] -
		m[ 1] * m[ 7] * m[14] -
		m[ 5] * m[ 2] * m[15] +
		m[ 5] * m[ 3] * m[14] +
		m[13] * m[ 2] * m[ 7] -
		m[13] * m[ 3] * m[ 6]);

	inv[6] = (
		-m[0] * m[6] * m[15] +
		m[ 0] * m[7] * m[14] +
		m[ 4] * m[2] * m[15] -
		m[ 4] * m[3] * m[14] -
		m[12] * m[2] * m[ 7] +
		m[12] * m[3] * m[ 6]);

	inv[10] = (
		m[ 0] * m[ 5] * m[15] -
		m[ 0] * m[ 7] * m[13] -
		m[ 4] * m[ 1] * m[15] +
		m[ 4] * m[ 3] * m[13] +
		m[12] * m[ 1] * m[ 7] -
		m[12] * m[ 3] * m[ 5]);

	inv[14] = (
		-m[0] * m[ 5] * m[14] +
		m[ 0] * m[ 6] * m[13] +
		m[ 4] * m[ 1] * m[14] -
		m[ 4] * m[ 2] * m[13] -
		m[12] * m[ 1] * m[ 6] +
		m[12] * m[ 2] * m[ 5]);

	inv[3] = (
		-m[1] * m[ 6] * m[11] +
		m[ 1] * m[ 7] * m[10] +
		m[ 5] * m[ 2] * m[11] -
		m[ 5] * m[ 3] * m[10] -
		m[ 9] * m[ 2] * m[ 7] +
		m[ 9] * m[ 3] * m[ 6]);

	inv[7] = (
		m[ 0] * m[ 6] * m[11] -
		m[ 0] * m[ 7] * m[10] -
		m[ 4] * m[ 2] * m[11] +
		m[ 4] * m[ 3] * m[10] +
		m[ 8] * m[ 2] * m[ 7] -
		m[ 8] * m[ 3] * m[ 6]);

	inv[11] = (
		-m[0] * m[ 5] * m[11] +
		m[ 0] * m[ 7] * m[ 9] +
		m[ 4] * m[ 1] * m[11] -
		m[ 4] * m[ 3] * m[ 9] -
		m[ 8] * m[ 1] * m[ 7] +
		m[ 8] * m[ 3] * m[ 5]);

	inv[15] = (
		m[ 0] * m[ 5] * m[10] -
		m[ 0] * m[ 6] * m[ 9] -
		m[ 4] * m[ 1] * m[10] +
		m[ 4] * m[ 2] * m[ 9] +
		m[ 8] * m[ 1] * m[ 6] -
		m[ 8] * m[ 2] * m[ 5]);

	const auto det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
	_ASSERT(det != 0);
	const auto invdet = 1.0f / det;

	for (auto& val : dst.ff) {
		val *= invdet; }

	return dst; }


mat4 mat4_look_from_to(const vec4& from, const vec4& to) {
	vec4 up;
	vec4 right;

	// build ->vector from 'from' to 'to'
	vec4 dir = normalize(from - to);

	// build a temporary 'up' vector
	if (fabs(dir.x)<FLT_EPSILON && fabs(dir.z)<FLT_EPSILON) {
		/*
		* special case where dir & up would be degenerate
		* this fix came from <midnight>
		* http://www.flipcode.com/archives/3DS_Camera_Orientation.shtml
		* an alternate up vector is used
		*/
		_ASSERT(0); // i'd like to know if i ever hit this
		up = vec4(-dir.y, 0, 0, 0); }
	else {
		up = vec4(0, 1, 0, 0); }

	// dir & right are already normalized
	// so up should be already.

	// UP cross DIR gives us "right"
	right = normalize(cross(up, dir));
	//	vec3_cross( right, up, dir );  vec3_normalize(right);

	// DIR cross RIGHT gives us a proper UP
	up = cross(dir, right);
	//	vec3_cross( up, dir, right );//vec3_normalize(up);

	mat4 mrot = mat4(
		right.x, right.y, right.z, 0.0f,
		up.x, up.y, up.z, 0.0f,
		dir.x, dir.y, dir.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	mat4 mpos = mat4(
		1, 0, 0, -from.x,
		0, 1, 0, -from.y,
		0, 0, 1, -from.z,
		0, 0, 0, 1
	);

	return mrot * mpos; }