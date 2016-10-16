#pragma once
#include "vec_soa.h"
#include "mvec4.h"


struct SetBlendProgram {
	void operator()(const mvec4i& mask, qfloat4& npx, qfloat4* const __restrict tpx) const {
		tpx->r = selectbits(tpx->r, npx.r, mask);
		tpx->g = selectbits(tpx->g, npx.g, mask);
		tpx->b = selectbits(tpx->b, npx.b, mask); }};


struct UniformAlphaBlendProgram {
	mvec4f alpha;
	UniformAlphaBlendProgram(float alpha)
		:alpha(alpha) {}

	void operator()(const mvec4i& mask, qfloat4& npx, qfloat4* const __restrict tpx) const {
		tpx->r = lerp_premul(tpx->r, npx.r, selectbits(mvec4f{ 0 }, alpha, mask));
		tpx->g = lerp_premul(tpx->g, npx.g, selectbits(mvec4f{ 0 }, alpha, mask));
		tpx->b = lerp_premul(tpx->b, npx.b, selectbits(mvec4f{ 0 }, alpha, mask)); }};
