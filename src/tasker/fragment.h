#pragma once
#include "vec_soa.h"
#include "canvas.h"
#include "vec.h"

#define FRAGMENT_PARAMETERS    \
	const qfloat2& frag_coord, \
	const qfloat& frag_depth,  \
	const tri_qfloat& BS,      \
	const tri_qfloat& BP,      \
	qfloat4& frag_color,       \
	mvec4i& frag_mask


struct FlatShader {
	const qfloat3 face_color;

	FlatShader(const vec3& color)
		:face_color(qfloat3{color}) {}

	inline void operator()(FRAGMENT_PARAMETERS) const {
		frag_color = qfloat4{ face_color * frag_depth, 0 }; }};


struct GouraudShader {
	const tri_qfloat3 color;

	GouraudShader(const vec3& v1c, const vec3& v2c, const vec3& v3c)
		:color(v1c, v2c, v3c) {}

	inline void operator()(FRAGMENT_PARAMETERS) const {
		frag_color = qfloat4{ tri_blend(BS, color), 0 }; }};


struct WireShader {
	/*wireframe shader, adapted from
	http://codeflow.org/entries/2012/aug/02/easy-wireframe-display-with-barycentric-coordinates/	
	*/
	const qfloat3 face_color;

	WireShader(const vec3& color)
		:face_color(qfloat3{color}) {}

	inline void operator()(FRAGMENT_PARAMETERS) const {
		static const qfloat grey(0.1f);
		const qfloat e = edgefactor(BS);

		auto fd = (-frag_depth + qfloat(1));
		frag_color = qfloat4{
			mix(grey, face_color.v[0], e) * fd * qfloat{4.0f},
			mix(grey, face_color.v[1], e) * fd * qfloat(4.0f),
			mix(grey, face_color.v[2], e) * fd * qfloat(4.0f),
			0.0f}; }

	inline qfloat edgefactor(const tri_qfloat& BS) const {
		static const qfloat thickfactor(1.5f);
		qfloat3 d = fwidth(BS);
		qfloat3 a3 = smoothstep(0, d*thickfactor, BS);
		return vmin(a3.v[0], vmin(a3.v[1], a3.v[2])); }};


template <typename TEXTURE_UNIT>
struct TextureShader {

	const TEXTURE_UNIT& texunit;
	const tri_qfloat2 vert_uv;

	TextureShader(const TEXTURE_UNIT& tu, const vec2& v1uv, const vec2& v2uv, const vec2& v3uv) 
		:texunit(tu),
		vert_uv(v1uv, v2uv, v3uv) {}

	inline void operator()(FRAGMENT_PARAMETERS) const {
		qfloat2 frag_uv = tri_blend(BP, vert_uv);
		frag_color = texunit.sample(frag_uv); }};
