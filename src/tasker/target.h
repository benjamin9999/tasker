#pragma once
#include "vec_soa.h"
#include "mvec4.h"
#include "vec.h"


template <typename FRAGMENT_PROGRAM, typename BLEND_PROGRAM>
struct DefaultTargetProgram {

	const FRAGMENT_PROGRAM& fp;
	const BLEND_PROGRAM& bp;

	qfloat4* cb;
	qfloat4* cbx;
	qfloat* db;
	qfloat* dbx;

	const int width;
	const int height;
	const qfloat2 target_dimensions;
	int offs, offs_left_start, offs_inc;

	const tri_qfloat vert_invw;
	const tri_qfloat vert_depth;

	DefaultTargetProgram(const FRAGMENT_PROGRAM& fp, const BLEND_PROGRAM& bp, QuadFloat4Canvas& cc, QuadFloatCanvas& dc, const vec4& v1, const vec4& v2, const vec4& v3)
		:fp(fp),
		bp(bp),
		width(cc.width),
		height(cc.height),
		target_dimensions(qfloat2{float(cc.width), float(cc.height)}),
		cb(cc.ptr),
		db(dc.ptr),
		vert_invw(tri_qfloat{v1.w, v2.w,v3.w}),
		vert_depth(tri_qfloat{v1.z, v2.z, v3.z}) {}

	inline void goto_xy(const int x, const int y) {
		offs = offs_left_start = (y >> 1) * (width >> 1) + (x >> 1); }

	inline void inc_y() {
		offs_left_start += width >> 1;
		offs = offs_left_start; }

	inline void inc_x() {
		offs++; }

	inline void depthwrite(const qfloat& old_depth, const qfloat& new_depth, const mvec4i& mask) {
		// to depthbuffer
		//auto dbx = db + offs;
		//selectbits(old_depth, new_depth, mask).store(dbx);

		// to alpha channel
		auto cbx = cb + offs;
		//selectbits(old_depth, new_depth, mask).store(&cbx->a);
		cbx->a = selectbits(old_depth, new_depth, mask);
	}

	inline void render(const qfloat2& frag_coord, const mvec4i& trimask, const tri_qfloat& BS, const bool frontfacing) {

		qfloat frag_depth = tri_blend(BS, vert_depth);

		// read depth buffer
		//qfloat old_depth(vec4::load(&cb[offs].a));  // from alpha
		qfloat& old_depth = cb[offs].a;
		//qfloat old_depth(vec4::load(&cb[offs].a));  // from alpha
		//qfloat old_depth(vec4::load(&db[offs]));  // from depthbuffer
		mvec4i depthmask = float2bits(cmple(frag_depth, old_depth));
		mvec4i frag_mask = andnot(trimask, depthmask);
		//mvec4i frag_mask = andnot(trimask, mvec4i(-1,-1,-1,-1)); // depthmask);

		if (movemask(bits2float(frag_mask)) == 0) {
			// early out if whole quad fails depth test
			return; }
		
		// restore perspective
		const qfloat frag_w = mvec4f{1.0f} / tri_blend(BS, vert_invw);

		const qfloat bp0 = vert_invw.v0 * BS.v0 * frag_w;
		const qfloat bp1 = vert_invw.v1 * BS.v1 * frag_w;
		const qfloat bp2 = qfloat(1.0f) - (bp0 + bp1);

		const tri_qfloat BP{ bp0, bp1, bp2 };

		qfloat4 frag_color;
		fp(frag_coord, frag_depth, BS, BP, frag_color, frag_mask);
		bp(frag_mask, frag_color, &cb[offs]);
		depthwrite(old_depth, frag_depth, frag_mask);
	}

};
