#pragma once
#include <algorithm>
#include <cmath>

#include "vec_soa.h"
#include "canvas.h"
#include "vec.h"



/*
inline int iround(const float x) {
	int t;
	__asm {
		fld x
		fistp t
	}
	return t; }
*/

inline int iround(const float x) {
	return int(x); }


struct Edge {
	int c;
	mvec4i b, block_left_start;
	mvec4i bdx, bdy;

	Edge(const int x1, const int y1, const int x2, const int y2, const int startx, const int starty) {
		const int dx = x1 - x2;
		const int dy = y2 - y1;
		
		c = dy*((startx << 4) - x1) + dx*((starty << 4) - y1);

		// correct for top/left fill convention
		if (dy > 0 || (dy == 0 && dx > 0)) c++;

		c = (c - 1) >> 4;

		block_left_start = mvec4i(c) + IQX*dy + IQY*dx;
		b = block_left_start;
		bdx = mvec4i(dy * 2);
		bdy = mvec4i(dx * 2); }

	void inc_y() {
		block_left_start += bdy;
		b = block_left_start; }

	void inc_x() {
		b += bdx; }

	const mvec4i val() {
		return b; } };


template <typename FRAGMENT_PROCESSOR>
void draw_triangle(const int target_height, const irect& r, const vec4& s1, const vec4& s2, const vec4& s3, const bool frontfacing, FRAGMENT_PROCESSOR& fp) {

	using std::max;
	using std::min;
	using std::array;

	const int x1 = iround(16.0f * s1.x);
	const int x2 = iround(16.0f * s2.x);
	const int x3 = iround(16.0f * s3.x);

	const int y1 = iround(16.0f * s1.y);
	const int y2 = iround(16.0f * s2.y);
	const int y3 = iround(16.0f * s3.y);

	int minx = max((min(min(x1,x2),x3) + 0xf) >> 4, r.left.x);
	int maxx = min((max(max(x1,x2),x3) + 0xf) >> 4, r.right.x);
	int miny = max((min(min(y1,y2),y3) + 0xf) >> 4, r.top.y);
	int maxy = min((max(max(y1,y2),y3) + 0xf) >> 4, r.bottom.y);

	const int q = 2; // block size is 2x2
	minx &= ~(q - 1); // align to 2x2 block
	miny &= ~(q - 1);

	array<Edge, 3> e{ {
		Edge{x1, y1, x2, y2, minx, miny},
		Edge{x2, y2, x3, y3, minx, miny},
		Edge{x3, y3, x1, y1, minx, miny},
	} };

	const mvec4f scale(1.0f / (e[0].c + e[1].c + e[2].c));

	const int last_row = target_height - 1;

	fp.goto_xy(minx, miny);
	for (int y = miny; y < maxy; y += 2, e[0].inc_y(), e[1].inc_y(), e[2].inc_y(), fp.inc_y()) {
		for (int x = minx; x < maxx; x += 2, e[0].inc_x(), e[1].inc_x(), e[2].inc_x(), fp.inc_x()) {

			const mvec4i edges(e[0].val() | e[1].val() | e[2].val());
			if (movemask(bits2float(edges)) == 0xf) continue;
			const mvec4i trimask(sar<31>(edges));

			// lower-left-origin opengl screen coords
			const qfloat2 frag_coord = { mvec4f(x+0.5f)+FQX, mvec4f(last_row-y+0.5f)+FQY };

			const qfloat b0 = itof(e[1].val()) * scale;
			const qfloat b2 = itof(e[0].val()) * scale;
			const qfloat b1 = mvec4f(1.0f) - (b0 + b2);
			const tri_qfloat bary{ b0, b1, b2 };

			fp.render(frag_coord, trimask, bary, frontfacing); }}}
