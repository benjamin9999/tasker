#include "stdafx.h"
#include <algorithm>

#include "guardband.h"
#include "fragment.h"
#include "texture.h"
#include "render.h"
#include "target.h"
#include "canvas.h"
#include "blend.h"
#include "tri.h"

using namespace std;


inline float tri_2area(const vec4& p1, const vec4& p2, const vec4& p3) {
	/*compute 2*area of a triangle
	
	result is positive if p1, p2, p3 have CCW winding*/
	const auto d31 = p3 - p1;
	const auto d21 = p2 - p1;
	const float area = d31.x*d21.y - d31.y*d21.x;
	return area; }


void GlPipe::process_gl_quad(
	const unsigned char cf0, const unsigned char cf1, const unsigned char cf2, const unsigned char cf3,
	const vec4& _p0, const vec4& _p1, const vec4& _p2, const vec4& _p3,
	int i0, int i1, int i2, int i3
) {
	if (cf0 & cf1 & cf2 & cf3) {
		return; }

	const unsigned char required_clipping = cf0 | cf1 | cf2 | cf3;
	if (required_clipping) {
		clip_gl_triangle(required_clipping, _p0, _p1, _p3, i0, i1, i3);
		clip_gl_triangle(required_clipping, _p1, _p2, _p3, i1, i2, i3);
		return; }

	auto v0 = dv[i0];
	auto v1 = dv[i1];
	auto v2 = dv[i2];
	auto v3 = dv[i3];

	const bool backfacing = tri_2area(v0, v1, v2) < 0;

	if (backfacing) {
		if (gl_culling && (gl_cull_face & GL_BACK)) {
			return; }
		swap(i1, i3);
		swap(v1, v3); }
	else {
		if (gl_culling && (gl_cull_face & GL_FRONT)) {
			return; }}
	
	binner.insert(v0, v1, v3, i0, i1, i3, gl_material, backfacing);
	binner.insert(v1, v2, v3, i1, i2, i3, gl_material, backfacing); }


void GlPipe::process_gl_triangle(
	const unsigned char cf0, const unsigned char cf1, const unsigned char cf2,
	const vec4& _p0, const vec4& _p1, const vec4& _p2,
	int i0, int i1, int i2
	) {
	if (cf0 & cf1 & cf2) return;

	const unsigned char required_clipping = cf0 | cf1 | cf2;
	if (required_clipping) {
		clip_gl_triangle(required_clipping, _p0, _p1, _p2, i0, i1, i2);
		return; }

	auto v0 = dv[i0];
	auto v1 = dv[i1];
	auto v2 = dv[i2];

	const bool backfacing = tri_2area(v0, v1, v2) < 0;

	if (backfacing) {
		if (gl_culling && (gl_cull_face & GL_BACK)) {
			return; }
		swap(i0, i2);
		swap(v0, v2); }
	else {
		if (gl_culling && (gl_cull_face & GL_FRONT)) {
			return; }}

	binner.insert(v0, v1, v2, i0, i1, i2, gl_material, backfacing); }


void Binner::retile() {
	tile_dimensions_in_pixels = tile_dimensions_in_blocks * block_dimensions_in_pixels;

	buffer_dimensions_in_tiles = (buffer_dimensions_in_pixels + (tile_dimensions_in_pixels - ivec2{1, 1})) / tile_dimensions_in_pixels;

	bins.clear();
	int i = 0;
	for (int ty=0; ty<buffer_dimensions_in_tiles.y; ty++) {
		for (int tx=0; tx<buffer_dimensions_in_tiles.x; tx++) {

			auto left = tx * tile_dimensions_in_pixels.x;
			auto top = ty * tile_dimensions_in_pixels.y;
			auto right = min(left + tile_dimensions_in_pixels.x, buffer_dimensions_in_pixels.x);
			auto bottom = min(top + tile_dimensions_in_pixels.y, buffer_dimensions_in_pixels.y);
			irect bbox{ {left, top}, {right, bottom} };
			bins.push_back(Tilebin{i++, bbox}); }}}


void Binner::insert(
	const vec4& p1, const vec4& p2, const vec4& p3,
	const int i0, const int i1, const int i2,
	const int material_id,
	const bool backfacing
	) {
	using std::min;
	using std::max;

	ivec2 ip1{ int(p1.x), int(p1.y) };
	ivec2 ip2{ int(p2.x), int(p2.y) };
	ivec2 ip3{ int(p3.x), int(p3.y) };

	const int vminx = max(min(ip1.x, min(ip2.x, ip3.x)), 0);
	const int vminy = max(min(ip1.y, min(ip2.y, ip3.y)), 0);
	const int vmaxx = min(max(ip1.x, max(ip2.x, ip3.x)), buffer_dimensions_in_pixels.x);
	const int vmaxy = min(max(ip1.y, max(ip2.y, ip3.y)), buffer_dimensions_in_pixels.y);

	auto top_left = ivec2{ vminx, vminy } / tile_dimensions_in_pixels;
	auto bottom_right = ivec2{ vmaxx, vmaxy } / tile_dimensions_in_pixels;

	bottom_right = ivec2::min(bottom_right, buffer_dimensions_in_tiles - ivec2{ 1, 1 });

	//const auto face = GlPipeFace::make(i0, i1, i2, material_id, backfacing);

	int ofs = top_left.y * buffer_dimensions_in_tiles.x + top_left.x;
	for (int ty = top_left.y; ty <= bottom_right.y; ty++) {
		int rofs = ofs;
		for (int tx = top_left.x; tx <= bottom_right.x; tx++) {
			auto& bin = bins[rofs++];
			//bin.gldata.emplace_back(face); }
			bin.gldata.emplace_back(GlPipeFace::make(i0, i1, i2, material_id, backfacing)); }
		ofs += buffer_dimensions_in_tiles.x; }}


void GlPipe::clip_gl_triangle(
	const unsigned char required,
	const vec4& p0, const vec4& p1, const vec4& p2,
	const int i0, const int i1, const int i2) {

	thread_local a64::vector<ClipPair> poly;
	thread_local a64::vector<ClipPair> tmp;

	poly.clear();
	poly.emplace_back(ClipPair{ p0, pv[i0] });
	poly.emplace_back(ClipPair{ p1, pv[i1] });
	poly.emplace_back(ClipPair{ p2, pv[i2] });

	// phase 1: sutherland-hodgman clipping
	for (int clip_plane = 0; clip_plane < 5; clip_plane++) { // XXX set to 6 to enable the far plane

		const int planebit = 1 << clip_plane;
		if (!(required & planebit)) {
			// this plane is fine, skip
			continue; }

		bool we_are_inside = Guardband::is_inside(planebit, poly[0].position);

		tmp.clear();
		for (int this_vi = 0; this_vi < poly.size(); this_vi++) {
			const auto next_vi = (this_vi + 1) % poly.size(); // wrap

			const auto& here = poly[this_vi];
			const auto& next = poly[next_vi];

			const bool next_is_inside = Guardband::is_inside(planebit, next.position);

			if (we_are_inside) {
				tmp.push_back(ClipPair{ here.position, here.data }); }

			if (we_are_inside != next_is_inside) {
				we_are_inside = !we_are_inside;

				const float t = Guardband::clip_line(planebit, here.position, next.position);
				auto new_position = lerp(here.position, next.position, t);
				auto new_data = GlVertexData::clip(here.data, next.data, t);
				tmp.push_back(ClipPair{ new_position, new_data }); }}

		std::swap(poly, tmp);
		if (poly.size() == 0) {
			return; }
		_ASSERT(poly.size() >= 3); }

	// end of phase 1: pos_a & dat_a contain an N-gon clipped to the Guardband

	clip_faces.push_back(poly.size());
	clip_faces.push_back(gl_material);
	for (const auto& item : poly) {
		clip_queue.push_back(item); }}


void GlPipe::clip_finish() {

	// phase 2: triangulate and output to bins
	int poly_base = 0;
	for (int fi = 0; fi < clip_faces.size(); fi += 2) {
		const int psz = clip_faces[fi];
		const int material = clip_faces[fi + 1];

		for (unsigned vi = 1; vi < psz - 1; vi++) {
			const int idx[3] = { 0, vi, vi + 1 };
			const int bp = dv.size();
			for (int i = 0; i < 3; i++) {
				auto& vertex = clip_queue[poly_base + idx[i]];
				dv.push_back(pdiv(device_matrix * vertex.position));
				pv.push_back(vertex.data); }

			int i0 = bp, i1 = bp + 1, i2 = bp + 2;
			const bool backfacing = tri_2area(dv[i0], dv[i1], dv[i2]) < 0;
			if (backfacing) {
				swap(i0, i1); }

			if (gl_culling &&
				(( backfacing && (gl_cull_face & GL_BACK)) ||
				 (!backfacing && (gl_cull_face & GL_FRONT)))) {
				// cull
			}
			else {
				binner.insert(dv[i0], dv[i1], dv[i2], i0, i1, i2, material, backfacing); }}

		poly_base += psz; }

	clip_queue.clear();
	clip_faces.clear(); }


void GlPipe::render_bin(
	const int pass,
	const int bin_idx,
	const MaterialStore& mstore,
	const TextureStore& tstore,
	QuadFloatCanvas& dbc,
	QuadFloat4Canvas& cbc
) {
	_mm_setcsr(_mm_getcsr() | 0x8040);

	//const int target_width = cbc.width;
	const int target_height = cbc.height;

	auto& bin = binner.bins[bin_idx];
	//bin.sort();

	for (const auto& face : bin.gldata) {

		bool backfacing;
		int material_id;
		int i0, i1, i2;
		face.unmake(i0, i1, i2, material_id, backfacing);
		const bool frontfacing = !backfacing;

		const auto& vd0 = pv[i0];
		const auto& vd1 = pv[i1];
		const auto& vd2 = pv[i2];

		const auto& dp0 = dv[i0];
		const auto& dp1 = dv[i1];
		const auto& dp2 = dv[i2];

		if (material_id == 101) {
			const WireShader shader(vd0.c);
			const SetBlendProgram bp;
			DefaultTargetProgram<WireShader, SetBlendProgram> target_program(shader, bp, cbc, dbc, dp0, dp1, dp2);
			draw_triangle(target_height, bin.rect, dp0, dp1, dp2, frontfacing, target_program); }

		else if (material_id == 102) {
			const GouraudShader shader(vd0.c, vd1.c, vd2.c);
			const SetBlendProgram bp;
			DefaultTargetProgram<GouraudShader, SetBlendProgram> target_program(shader, bp, cbc, dbc, dp0, dp1, dp2);
			draw_triangle(target_height, bin.rect, dp0, dp1, dp2, frontfacing, target_program); }

		else if (material_id == 103) {
			auto tex = tstore.find_by_name("matcap2.png");
			typedef ts_pow2_mipmap<8> sampler;
			const sampler texunit(tex->buf.data());
			const TextureShader<sampler> shader(texunit, vd0.tc1, vd1.tc1, vd2.tc1);
			const SetBlendProgram bp;
			DefaultTargetProgram<TextureShader<sampler>, SetBlendProgram> target_program(shader, bp, cbc, dbc, dp0, dp1, dp2);
			draw_triangle(target_height, bin.rect, dp0, dp1, dp2, frontfacing, target_program); }

		else if (material_id == 4) {
			auto tex = tstore.find_by_name("matcap.png");
			typedef ts_pow2_mipmap<8> sampler;
			const sampler texunit(tex->buf.data());
			const TextureShader<sampler> shader(texunit, vd0.tc1, vd1.tc1, vd2.tc1);
			const SetBlendProgram bp;
			DefaultTargetProgram<TextureShader<sampler>, SetBlendProgram> target_program(shader, bp, cbc, dbc, dp0, dp1, dp2);
			draw_triangle(target_height, bin.rect, dp0, dp1, dp2, frontfacing, target_program); }

		else if (material_id >= 0) {
			auto& mat = mstore.get(material_id);
			if (mat.name.substr(0, 4) == string("obj-")) {
				if (mat.imagename != string("")) {
					auto tex = tstore.find_by_name(mat.imagename);
					typedef ts_pow2_mipmap<8> sampler;
					const sampler texunit(tex->buf.data());
					const TextureShader<sampler> shader(texunit, vd0.tc1, vd1.tc1, vd2.tc1);
					const SetBlendProgram bp;
					DefaultTargetProgram<TextureShader<sampler>, SetBlendProgram> target_program(shader, bp, cbc, dbc, dp0, dp1, dp2);
					draw_triangle(target_height, bin.rect, dp0, dp1, dp2, frontfacing, target_program); }
				else {
					auto c = vec3{ mat.kd.x, mat.kd.y, mat.kd.z };
					if (0) {
						const WireShader shader(c);
						const SetBlendProgram bp;
						DefaultTargetProgram<WireShader, SetBlendProgram> target_program(shader, bp, cbc, dbc, dp0, dp1, dp2);
						draw_triangle(target_height, bin.rect, dp0, dp1, dp2, frontfacing, target_program); }
					else {
						const FlatShader shader(c);
						const SetBlendProgram bp;
						DefaultTargetProgram<FlatShader, SetBlendProgram> target_program(shader, bp, cbc, dbc, dp0, dp1, dp2);
						draw_triangle(target_height, bin.rect, dp0, dp1, dp2, frontfacing, target_program); }}}}

		else {
			const WireShader shader(vec3{ 1,0,1 });
			const SetBlendProgram bp;
			DefaultTargetProgram<WireShader, SetBlendProgram> target_program(shader, bp, cbc, dbc, dp0, dp1, dp2);
			draw_triangle(target_height, bin.rect, dp0, dp1, dp2, frontfacing, target_program); }}}
