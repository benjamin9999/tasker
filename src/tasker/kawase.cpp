#include "stdafx.h"
#include <array>

#include "canvas.h"
#include "mvec4.h"

using std::array;
using std::min;
using std::max;

struct ofs {
	char x, y;
};


constexpr array<const array<ofs, 4>, 5> kawase_offsets = { {
	{{ {-1, -1}, {0, -1}, {-1, 0}, {0, 0} }},
	{{ {-2, -2}, {1, -2}, {-2, 1}, {1, 1} }},
	{{ {-3, -3}, {2, -3}, {-3, 2}, {2, 2} }},
	{{ {-4, -4}, {3, -4}, {-4, 3}, {3, 3} }},
	{{ {-5, -5}, {4, -5}, {-5, 4}, {4, 4} }},
} };

const mvec4f FACTOR{ 1.0f / 16.0f };

constexpr int SAFE_ZONE = 5;


inline int clamp_int(const int a, const int l, const int h) {
	return min(max(a, l), h); }


void blur_kawase(const int y0, const int y1, const int dist, const FloatingPointCanvas& src, FloatingPointCanvas& dst) {
	auto& offsets = kawase_offsets[dist];
	auto in = reinterpret_cast<const mvec4f*>(src.ptr);

	auto sample_clamp = [&src, &in](int x, int y) {
		const int _x = clamp_int(x, 0, src.width - 1);
		const int _y = clamp_int(y, 0, src.height - 1);
		return in[_y * src.stride + _x]; };

	auto sample_safe = [&src, &sample_clamp](int x, int y) {
		return (sample_clamp(x + 0, y + 0) + sample_clamp(x + 1, y + 0) +
		        sample_clamp(x + 0, y + 1) + sample_clamp(x + 1, y + 1)); };

	auto sample_fast = [&src, &in](int x, int y) {
		auto ptr = &in[y * src.stride + x];
		return (ptr[0]          + ptr[1] +
		        ptr[src.stride] + ptr[src.stride + 1]); };

	auto out = reinterpret_cast<mvec4f*>(dst.ptr + y0 * dst.stride);

	for (int y = y0; y < y1; y++) {
		if (y < SAFE_ZONE || y > dst.height - SAFE_ZONE) {
			for (int x = 0; x < dst.width; x++) {
				mvec4f ax{ 0 };
				for (const auto& ofs : offsets) {
					ax += sample_safe(x + ofs.x, y + ofs.y); }
				(ax * FACTOR).store(&out[x].v); }}
		else {
			int x;
			for (x = 0; x < SAFE_ZONE; x++) {
				mvec4f ax{ 0 };
				for (const auto& ofs : offsets) {
					ax += sample_safe(x + ofs.x, y + ofs.y); }
				(ax * FACTOR).store(&out[x].v); }
			for (; x < dst.width - SAFE_ZONE; x++) {
				mvec4f ax{ 0 };
				for (const auto& ofs : offsets) {
					ax += sample_fast(x + ofs.x, y + ofs.y); }
				(ax * FACTOR).store(&out[x].v); }
			for (; x < dst.width; x++) {
				mvec4f ax{ 0 };
				for (const auto& ofs : offsets) {
					ax += sample_safe(x + ofs.x, y + ofs.y); }
				(ax * FACTOR).store(&out[x].v); }}
		out += dst.stride; }}
