#include "stdafx.h"
#include <algorithm>

#include <PixelToaster.h>

#include "canvas.h"
#include "vec.h"

using namespace PixelToaster;
using namespace std;


TrueColorCanvas make_subcanvas(
	TrueColorCanvas& src,
	const int rectleft, int recttop,
	int rectwidth, int rectheight
) {
	auto left = rectleft < 0 ? src.width + rectleft : rectleft;
	auto top = recttop < 0 ? src.height + recttop : recttop;

	auto width = rectwidth < 0 ? src.width + rectwidth : rectwidth;
	auto height = rectheight < 0 ? src.height + rectheight : rectheight;

	auto clipwidth = std::min(src.width - left, width);
	auto clipheight = std::min(src.height - top, height);

	return TrueColorCanvas(src.ptr + (top * src.stride) + left, clipwidth, clipheight, src.stride); }


void fill_canvas_rect(const irect& rect, const float value, QuadFloatCanvas& dst) {
	const int rect_height_in_quads = (rect.height()) / 2;
	const int rect_width_in_quads = (rect.width()) / 2;

	int row_offset = rect.top.y / 2 * dst.stride2;
	int col_offset = rect.left.x / 2;

	const __m128 r0 = _mm_set1_ps(value);
	for (int yy = 0; yy < rect_height_in_quads; yy++) {
		for (int xx = 0; xx < rect_width_in_quads; xx++) {
			dst.ptr[row_offset + col_offset + xx] = r0; }
		row_offset += dst.stride2; }}


void fill_canvas_rect(const irect& rect, const vec4& value, QuadFloat4Canvas& dst) {
	const int rect_height_in_quads = (rect.height()) / 2;
	const int rect_width_in_quads = (rect.width()) / 2;

	int row_offset = (rect.top.y / 2) * dst.stride2;
	int col_offset = rect.left.x / 2;

	const __m128 red = _mm_set1_ps(value.x);
	const __m128 green = _mm_set1_ps(value.y);
	const __m128 blue = _mm_set1_ps(value.z);
	const __m128 alpha = _mm_set1_ps(value.w);

	auto row_dst = &dst.ptr[row_offset + col_offset];
	for (int yy = 0; yy < rect_height_in_quads; yy++) {
		auto _dst = row_dst;
		for (int xx = 0; xx < rect_width_in_quads; xx++) {
			_mm_store_ps(reinterpret_cast<float*>(&_dst->r), red);
			_mm_store_ps(reinterpret_cast<float*>(&_dst->g), green);
			_mm_store_ps(reinterpret_cast<float*>(&_dst->b), blue);
			_mm_store_ps(reinterpret_cast<float*>(&_dst->a), alpha);
			_dst++; }
		row_dst += dst.stride2; }}


void draw_border(const irect& rect, PixelToaster::TrueColorPixel& color, TrueColorCanvas& dst) {
	// top row
	int yy = rect.top.y;
	for (int xx = rect.left.x; xx < rect.right.x; xx++) {
		dst.ptr[yy * dst.stride + xx] = color; }

	// left & right columns between
	for (yy += 1; yy < rect.bottom.y - 1; yy++) {
		dst.ptr[yy * dst.stride + rect.left.x] = color;
		dst.ptr[yy * dst.stride + rect.right.x - 1] = color; }

	// bottom row
	for (int xx = rect.left.x; xx < rect.right.x; xx++) {
		dst.ptr[yy * dst.stride + xx] = color; }}


void copy_canvas_rect_downsample(const irect& rect, QuadFloat4Canvas& src, FloatingPointCanvas& dst) {
	const __m128 one_quarter = _mm_set1_ps(0.25f);
	for (int y = rect.top.y; y < rect.bottom.y; y += 2) {
		auto dstpx = &dst.ptr[(y >> 1) * dst.stride + (rect.left.x >> 1)];
		auto srcpx = &src.ptr[(y >> 1) * (src.width >> 1) + (rect.left.x >> 1)];

		for (int x = rect.left.x; x < rect.right.x; x += 2) {
			//_mm_prefetch(reinterpret_cast<const char*>(srcpx + (src.width >> 1)), _MM_HINT_T0);
			__m128 r0 = _mm_load_ps(reinterpret_cast<const float*>(&srcpx->r.v));
			__m128 r1 = _mm_load_ps(reinterpret_cast<const float*>(&srcpx->g.v));
			__m128 r2 = _mm_load_ps(reinterpret_cast<const float*>(&srcpx->b.v));
			//__m128 r3 = _mm_load_ps(reinterpret_cast<const float*>(&srcpx->a.v));
			__m128 r3 = _mm_setzero_ps();
			_MM_TRANSPOSE4_PS(r0, r1, r2, r3);
			__m128 sum = _mm_add_ps(_mm_add_ps(_mm_add_ps(r0, r1), r2), r3);
			__m128 final = _mm_mul_ps(sum, one_quarter);
			_mm_stream_ps(reinterpret_cast<float*>(&dstpx->v), final);
			//dstpx->v = _mm_mul_ps(one_quarter, sum);
			srcpx++;
			dstpx++; }}}


void copy_canvas_slow(FloatingPointCanvas& src, TrueColorCanvas& dst) {
	for (int y = 0; y < src.height; y++) {
		for (int x = 0; x < src.width; x++) {
			dst.ptr[y*dst.stride + x] = to_tc_from_fpp(src.ptr[y*src.stride + x]); }}}


PixelToaster::FloatingPointPixel to_fp(const vec4& v) {
	return FloatingPointPixel{ v.x, v.y, v.z }; }
