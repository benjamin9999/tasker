#pragma once
#include <algorithm>
#include <array>
#include <map>

#include <PixelToaster.h>

#include "canvas.h"
#include "jobsys.h"

namespace jobsys {

namespace pxt = PixelToaster;

auto tc_mul(const pxt::TrueColorPixel& px, const float a) {
	auto mul = [](uint8_t a, float b) { return uint8_t(float(a) * b); };
	return pxt::TrueColorPixel(
		mul(px.r, a),
		mul(px.g, a),
		mul(px.b, a)); }


void render(const int left, const int top, const float xscale, TrueColorCanvas& canvas) {

	const int thick = 8;
	const int gap = 2;
	const auto scale = xscale * canvas.width;

	// some colors from I want hue, and a function to cycle over them
	unsigned color_idx = 0;
	constexpr int color_count = 16;
	constexpr int color_mask = color_count - 1;
	static const std::array<pxt::TrueColorPixel, color_count> task_colors{ {
		pxt::TrueColorPixel(0xd7de48), pxt::TrueColorPixel(0xda7bf5),
		pxt::TrueColorPixel(0x4ccbdb), pxt::TrueColorPixel(0xf67e77),
		pxt::TrueColorPixel(0x6dd671), pxt::TrueColorPixel(0x84a6e6),
		pxt::TrueColorPixel(0xee913c), pxt::TrueColorPixel(0xf084b4),
		pxt::TrueColorPixel(0x50d9a7), pxt::TrueColorPixel(0x66de3e),
		pxt::TrueColorPixel(0xd19bdf), pxt::TrueColorPixel(0xcfa93c),
		pxt::TrueColorPixel(0x9fc34f), pxt::TrueColorPixel(0xf179d9),
		pxt::TrueColorPixel(0xb4e532), pxt::TrueColorPixel(0xebc630)
	} };
	auto next_color = [&color_idx]() {
		auto idx = (color_idx++) % task_colors.size();
		return task_colors[idx]; };

	// create raster data from a JobStat instance
	struct job_span {
		int left;
		int right;
		pxt::TrueColorPixel color;
	};

	auto to_span = [&color_mask](const struct JobStat& jobstat, const float scale) {
		const auto left = int(jobstat.start_time * scale);
		const auto right = int(jobstat.end_time * scale);

		// use some random(ish) bits to make a stable color selection
		auto randomish_bits = uint32_t(jobstat.raw);
		uint8_t ax = 0;
		ax ^= (randomish_bits & 0xff);
		randomish_bits >>= 6;
		ax ^= (randomish_bits & 0xff);
		randomish_bits >>= 8;
		ax ^= (randomish_bits & 0xff);
		randomish_bits >>= 10;
		ax ^= (randomish_bits & 0xff);

		const auto color = task_colors[ax & color_mask];
		//const auto color = task_colors[(randomish_bits >> 11) & color_mask];
		return job_span{ left, right, color }; };

	// render to canvas
	int bar_top = 0;
	for (const auto &thread_telemetry : jobsys::telemetry_stores) {
		for (const auto &jobstat : thread_telemetry) {

			auto span = to_span(jobstat, scale);
			auto bright = 1.0f;
			auto delta = 1.0f / (span.right - span.left);

			for (int bx = span.left; bx <= span.right; bx++) {
				auto color = tc_mul(span.color, bright);

				for (int by = 0; by < thick; by++) {
					if (left + bx < canvas.width)
						canvas.ptr[(top + bar_top + by) * canvas.stride + left + bx] = color; }
				bright -= delta; }}

		bar_top += thick + gap; }}

}
