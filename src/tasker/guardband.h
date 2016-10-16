#pragma once
#include "aligned-containers.h"
#include "vec_soa.h"
#include "mvec4.h"
#include "vec.h"

namespace Guardband {

const unsigned CLIP_LEFT   = 1 << 0;
const unsigned CLIP_BOTTOM = 1 << 1;
const unsigned CLIP_NEAR   = 1 << 2;
const unsigned CLIP_RIGHT  = 1 << 3;
const unsigned CLIP_TOP    = 1 << 4;
const unsigned CLIP_FAR    = 1 << 5;

const float GUARDBAND_FACTOR = 1.0f;


//const mvec4f GUARDBAND_WWWW{GUARDBAND_FACTOR, GUARDBAND_FACTOR, 1.0f, 0.0f};
/*inline unsigned clip_point(const mvec4f& p) {
	const auto zero = mvec4f::zero();
	auto www = GUARDBAND_WWWW * p.wwww();  // guardband vector is (gbf,gbf,1,0)
	auto lbn_mask = movemask(cmple(www+p, zero)) & 0x7;  // left, bottom, near
	auto rtf_mask = movemask(cmple(www-p, zero)) & 0x3;  // right, top, far
	return lbn_mask | (rtf_mask << 3); }*/


inline unsigned clip_point(const vec4& p) {
	auto left = (p.w + p.x < 0) << 0;
	auto bottom = (p.w + p.y < 0) << 1;
	auto near = (p.w + p.z < 0) << 2;

	auto right = (p.w - p.x < 0) << 3;
	auto top = (p.w - p.y < 0) << 4;
	//auto far = (p.w - p.z < 0) << 5;
	return left | bottom | near | right | top; } // | far


inline mvec4i clip_point(const qfloat4& p) {
	const auto zero = mvec4f::zero();

	auto left   =        shr<31>(float2bits(cmple(p.w + p.x, zero)));
	auto bottom = shl<1>(shr<31>(float2bits(cmple(p.w + p.y, zero))));
	auto near   = shl<2>(shr<31>(float2bits(cmple(p.w + p.z, zero))));

	auto right  = shl<3>(shr<31>(float2bits(cmple(p.w - p.x, zero))));
	auto top    = shl<4>(shr<31>(float2bits(cmple(p.w - p.y, zero))));
	//auto far    = shl<5>(shr<31>(float2bits(cmple(p.w - p.z, zero))));

	auto all = left | bottom | near | right | top;  // | far
	return all;}


inline const char *plane_name(const int plane) {
	switch (plane) {
	case CLIP_LEFT:   return "left";
	case CLIP_RIGHT:  return "right";
	case CLIP_BOTTOM: return "bottom";
	case CLIP_TOP:    return "top";
	case CLIP_NEAR:   return "near";
	case CLIP_FAR:    return "far"; }
	_ASSERT(0);
	return nullptr; }  // silence warning


inline bool is_inside(const unsigned clipplane, const vec4& p) {
	const float wscaled = p.w * GUARDBAND_FACTOR;
	switch (clipplane) {
	case CLIP_LEFT:   return wscaled + p.x >= 0;
	case CLIP_RIGHT:  return wscaled - p.x >= 0;
	case CLIP_BOTTOM: return wscaled + p.y >= 0;
	case CLIP_TOP:    return wscaled - p.y >= 0;
	case CLIP_NEAR:   return p.w+p.z >= 0;
	case CLIP_FAR:    return p.w-p.z >= 0; }
	_ASSERT(0);
	return 0; }  // silence warning


inline float clip_line(const unsigned clipplane, const vec4& a, const vec4& b) {
	const float aws = a.w * GUARDBAND_FACTOR;
	const float bws = b.w * GUARDBAND_FACTOR;
	switch (clipplane) {
	case CLIP_LEFT:   return (aws+a.x) / ((aws+a.x)-(bws+b.x));
	case CLIP_RIGHT:  return (aws-a.x) / ((aws-a.x)-(bws-b.x));
	case CLIP_BOTTOM: return (aws+a.y) / ((aws+a.y)-(bws+b.y));
	case CLIP_TOP:    return (aws-a.y) / ((aws-a.y)-(bws-b.y));
	case CLIP_NEAR:   return (a.w+a.z) / ((a.w+a.z)-(b.w+b.z));
	case CLIP_FAR:    return (a.w-a.z) / ((a.w-a.z)-(b.w-b.z)); }
	_ASSERT(0);
	return 0; }  // silence warning

}
