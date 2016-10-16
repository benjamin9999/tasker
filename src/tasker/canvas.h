#pragma once
#include <algorithm>
#include <intrin.h>

#include "../pixeltoaster/PixelToaster.h"

#include "srgb-ryg.h"
#include "vec_soa.h"
#include "irect.h"
#include "vec.h"


inline PixelToaster::TrueColorPixel to_tc_from_fpp(PixelToaster::FloatingPointPixel& fpp) {
	return{
		PixelToaster::integer8(fpp.r * 255.0),
		PixelToaster::integer8(fpp.g * 255.0),
		PixelToaster::integer8(fpp.b * 255.0)
	}; }


struct QuadFloatCanvas {
	QuadFloatCanvas(qfloat * ptr,
		const int width,
		const int height,
		const int stride = 0)
		:ptr(ptr),
		width(width),
		height(height),
		stride2(stride == 0 ? width / 2 : stride),
		aspect(float(width) / float(height)) {}

	qfloat* ptr;
	const int width;
	const int height;
	const int stride2;
	const float aspect; };


struct QuadFloat4Canvas {
	QuadFloat4Canvas(
		qfloat4* ptr,
		const int width,
		const int height,
		const int stride = 0)
		:ptr(ptr),
		width(width),
		height(height),
		stride2(stride == 0 ? width / 2 : stride),
		aspect(float(width) / float(height)) {}

	qfloat4* ptr;
	const int width;
	const int height;
	const int stride2;
	const float aspect; };


struct TrueColorCanvas {
	TrueColorCanvas(PixelToaster::TrueColorPixel * ptr,
					const int width,
					const int height,
					const int stride = 0)
		:ptr(ptr),
		width(width),
		height(height),
		stride(stride == 0 ? width : stride),
		aspect(float(width) / float(height)) {}

	PixelToaster::TrueColorPixel * ptr;
	const int width;
	const int height;
	const int stride;
	const float aspect; };


TrueColorCanvas make_subcanvas(
	TrueColorCanvas& src,
	int rectleft, int recttop,
	int rectwidth, int rectheight);


struct FloatingPointCanvas {
	FloatingPointCanvas(PixelToaster::FloatingPointPixel* ptr,
					const int width,
					const int height,
					const int stride = 0)
		:ptr(ptr),
		width(width),
		height(height),
		stride(stride == 0 ? width : stride),
		aspect(float(width) / float(height)) {}

	PixelToaster::FloatingPointPixel * ptr;
	const int width;
	const int height;
	const int stride;
	const float aspect; };


void fill_canvas_rect(const irect& rect, const float value, QuadFloatCanvas& dst);
void fill_canvas_rect(const irect& rect, const vec4& value, QuadFloat4Canvas& dst);

void draw_border(const irect& rect, PixelToaster::TrueColorPixel& color, TrueColorCanvas& dst);


void copy_canvas_rect_downsample(const irect& rect, QuadFloat4Canvas& src, FloatingPointCanvas& dst);
//void copy_canvas(FloatingPointCanvas& src, TrueColorCanvas& dst);


inline auto to_tc_ryg(const qfloat3& color) {
	using ryg::float_to_srgb8_var2_SSE2;
	return shl<16>(float_to_srgb8_var2_SSE2(color.x.v)) |
	       shl< 8>(float_to_srgb8_var2_SSE2(color.y.v)) |
	               float_to_srgb8_var2_SSE2(color.z.v); };


inline auto to_tc_basic(const qfloat3& color) {
	auto fix = [](__m128 a) {
		__m128 r0 = _mm_min_ps(a, _mm_set1_ps(1.0f));
		r0 = _mm_max_ps(r0, _mm_setzero_ps());
		r0 = _mm_mul_ps(r0, _mm_set1_ps(255.0f));
		return ftoi(r0); };
	return shl<16>(fix(color.x.v)) |
		   shl< 8>(fix(color.y.v)) |
				   fix(color.z.v); };


struct GlowShader {
	inline mvec4i operator()(const qfloat2& q, qfloat4& s1, PixelToaster::FloatingPointPixel& s2) const {
		auto& blur = s2;
		auto& sc = s1;

		const mvec4f b{ 0.8f };

		qfloat3 col{
			sc.r + blur.r*b,
			sc.g + blur.g*b,
			sc.b + blur.b*b,
		};
		col *= 0.5f;
		return to_tc_ryg(col); }};


struct DofShader {
	inline mvec4i operator()(const qfloat2& q, qfloat4& s1, PixelToaster::FloatingPointPixel& s2) const {
		auto& blur = s2;
		auto& sc = s1;

		qfloat dof = vmin(vmax(-0.5f + ((sc.a + 1.0f) * 0.5f) * 1.6f, mvec4f::zero()), 1.0f);
		qfloat3 col{
			lerp(sc.r, qfloat{blur.r}, dof),
			lerp(sc.g, qfloat{blur.g}, dof),
			lerp(sc.b, qfloat{blur.b}, dof)
		};
		return to_tc_ryg(col); }};


struct GammaShader {
	inline mvec4i operator()(const qfloat2& q, qfloat4& s1, PixelToaster::FloatingPointPixel& s2) const {
		qfloat3 c{ s1.r, s1.g, s1.b };
		return to_tc_ryg(c); }
	inline mvec4i operator()(const qfloat2& q, qfloat4& s1) const {
		qfloat3 c{ s1.r, s1.g, s1.b };
		return to_tc_ryg(c); } };


struct NoShader {
	inline mvec4i operator()(const qfloat2& q, qfloat4& s1, PixelToaster::FloatingPointPixel& s2) const {
		qfloat3 c{ s1.r, s1.g, s1.b };
		return to_tc_basic(c); }
	inline mvec4i operator()(const qfloat2& q, qfloat4& s1) const {
		qfloat3 c{ s1.r, s1.g, s1.b };
		return to_tc_basic(c); } };


struct IqShader {
	inline mvec4i operator()(const qfloat2& q, const qfloat4& s1) const {
		auto& sc = s1;

		qfloat3 col{sc.r, sc.g, sc.b};
		col = pow(col, qfloat3(0.45f, 0.5f, 0.55f));
		col *= 0.2f + 0.8f * pow(16.0f * q.x * q.y * (1.0f - q.x) * (1.0f - q.y), 0.2f);
		col += (1.0 / 255.0) * hash3(q.x + 13.0*q.y);

		return to_tc_basic(col); }

	inline mvec4i operator()(const qfloat2& q, const qfloat4& s1, const PixelToaster::FloatingPointPixel& s2) const {
		return operator()(q, s1); }};


template <typename CANVAS_SHADER>
void canvas_shader_rows(
	const int y0, const int y1,
	const CANVAS_SHADER& shader,
	const QuadFloat4Canvas& src1,
	const FloatingPointCanvas& src2,
	TrueColorCanvas& dst
) {
	const float dw = 1.0f / dst.width;
	const float dh = 1.0f / dst.height;

	const qfloat2 dqx{ dw*2, 0 };
	const qfloat2 dqy{ 0, -dh*2 };
	qfloat2 q_row{
		mvec4f{0} +mvec4f{dw}*FQX,
		mvec4f{(dst.height - 1 - y0) / float(dst.height)} - mvec4f{dh}*FQY
	};

	for (int y = y0; y < y1; y += 2, q_row += dqy) {

		auto dst1 = &dst.ptr[y * dst.stride];
		auto dst2 = dst1 + dst.stride;

		auto* px1 = &src1.ptr[(y >> 1) * (dst.width >> 1)];
		auto* px2 = &src2.ptr[(y >> 1) * src2.stride];

		qfloat2 q{ q_row };
		for (int x = 0; x < src1.width; x += 4, dst1+=4, dst2+=4) {
			mvec4i packed[2];
			for (int sub = 0; sub < 2; sub++, px1++, px2++, q+=dqx) {
				packed[sub] = shader(q, *px1, *px2); }
			_mm_stream_si128(reinterpret_cast<__m128i*>(dst1), _mm_unpacklo_epi64(packed[0].v, packed[1].v));
			_mm_stream_si128(reinterpret_cast<__m128i*>(dst2), _mm_unpackhi_epi64(packed[0].v, packed[1].v)); }}}


template <typename CANVAS_SHADER>
void canvas_shader_rect(
	const irect& rect,
	const CANVAS_SHADER& shader,
	const QuadFloat4Canvas& src1,
	TrueColorCanvas& dst
) {
	const float dw = 1.0f / dst.width;
	const float dh = 1.0f / dst.height;

	const qfloat2 dqx{ dw*2, 0 };
	const qfloat2 dqy{ 0, -dh*2 };
	qfloat2 q_row{
		mvec4f{rect.left.x / float(dst.width)} +mvec4f{dw}*FQX,
		mvec4f{(dst.height - 1 - rect.top.y) / float(dst.height)} - mvec4f{dh}*FQY
	};

	for (int y = rect.top.y; y < rect.bottom.y; y += 2, q_row+=dqy) {

		auto dst1 = &dst.ptr[y * dst.stride + rect.left.x];
		auto dst2 = dst1 + dst.stride;

		auto* px1 = &src1.ptr[(y >> 1) * (dst.width >> 1) + (rect.left.x >> 1)];

		qfloat2 q{ q_row };
		for (int x = rect.left.x; x < rect.right.x; x += 4, dst1+=4, dst2+=4) {
			mvec4i packed[2];
			for (int sub = 0; sub < 2; sub++, px1++, q+=dqx) {
				packed[sub] = shader(q, *px1); }

			_mm_stream_si128(reinterpret_cast<__m128i*>(dst1), _mm_unpacklo_epi64(packed[0].v, packed[1].v));
			_mm_stream_si128(reinterpret_cast<__m128i*>(dst2), _mm_unpackhi_epi64(packed[0].v, packed[1].v)); }}}
