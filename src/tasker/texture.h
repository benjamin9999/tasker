#pragma once
#include <vector>

#include "../pixeltoaster/PixelToaster.h"

#include "aligned-containers.h"
#include "vec_soa.h"
#include "mvec4.h"

#define get_ul get_x
#define get_ur get_y
#define get_ll get_z
#define get_lr get_w


struct Texture {
	void maybe_make_mipmap();
	void save_tga(const std::string& fn) const;

	a64::vector<PixelToaster::FloatingPointPixel> buf;
	int width;
	int height;
	int stride;
	std::string name;
	int pow;
	bool mipmap; };


class TextureStore {
public:
	TextureStore();
	//      const Texture& get(string const key);
	void append(Texture t);
	const Texture* const find_by_name(const std::string& name) const;
	void load_dir(const std::string& prepend);
	void load_any(const std::string& prepend, const std::string& fname);
	void print();

private:
	std::vector<Texture> store; };


Texture load_png(const std::string filename, const std::string name, const bool premultiply);


inline void fpp_gather(
	const PixelToaster::FloatingPointPixel * const __restrict src,
	const mvec4i& offsets,
	mvec4f * const __restrict px
) {
	px[0].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.x]));
	px[1].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.y]));
	px[2].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.z]));
	px[3].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.w]));
	_MM_TRANSPOSE4_PS(px[0].v, px[1].v, px[2].v, px[3].v);
}


inline int clamp(const int val, const int lower, const int upper) {
	using std::min;
	using std::max;
	return min(max(val, lower), upper); }


//inline void mipcalc(const float u0, const float u1, int& offset, int& mip_size, int& stride)
//inline void mipcalc(const float dux, const float dvx, int* offset, int* mip_size, int* stride)

template<int pow>
inline void mipcalc(const float dux, const float dvx, const float duy, const float dvy, int& offset, int& mip_size, int& stride) {
	const int max_lod{ pow };
	const int image_dim{ 1 << pow };  // dimension of root image
	const int mipmap_height{ 2 << pow };  // total rows in mipmap

	const float v = abs(dux*dvy - duy*dvx);
	const int fast_lod = (int(v) - (127 << 23)) >> 24;
	const int lod = clamp(fast_lod, 0, pow);

	const int inverted_lod = max_lod - lod;

	// calculate offset for the first row by subtracting from the end
	const int row = mipmap_height - (2 << inverted_lod);

	stride = image_dim;
	offset = row * stride;
	mip_size = 1 << (pow - lod); }


template<int power>
struct ts_pow2_mipmap {

	const PixelToaster::FloatingPointPixel * __restrict texdata;
	const float fstride;

	ts_pow2_mipmap(const PixelToaster::FloatingPointPixel * const __restrict ptr)
		:texdata(ptr), fstride(float(1 << power)) {}

	inline qfloat4 fetch_texel(const int rowoffset, const int mipsize, const mvec4i& x, const mvec4i& y) const {
		const auto texmod = mvec4i(mipsize - 1);

		auto tx = mvec4i(          x & texmod);
		auto ty = mvec4i(texmod - (y & texmod));

		auto offset = mvec4i(shl<power>(ty) | tx) + mvec4i(rowoffset);

		__m128 r0 = _mm_load_ps(reinterpret_cast<const float*>(&texdata[offset.x]));
		__m128 r1 = _mm_load_ps(reinterpret_cast<const float*>(&texdata[offset.y]));
		__m128 r2 = _mm_load_ps(reinterpret_cast<const float*>(&texdata[offset.z]));
		__m128 r3 = _mm_load_ps(reinterpret_cast<const float*>(&texdata[offset.w]));
		_MM_TRANSPOSE4_PS(r0, r1, r2, r3);
		return qfloat4{ r0, r1, r2, r3 }; }

	inline qfloat4 sample(const qfloat2& texcoord) const {
		int offset, mip_size, stride;
		float dux = (texcoord.s.get_ur() - texcoord.s.get_ul()) * fstride;
		float duy = (texcoord.s.get_ll() - texcoord.s.get_ul()) * fstride;
		float dvx = (texcoord.t.get_ur() - texcoord.t.get_ul()) * fstride;
		float dvy = (texcoord.t.get_ll() - texcoord.t.get_ul()) * fstride;
		mipcalc<power>(dux, dvx, duy, dvy, offset, mip_size, stride);

		const mvec4f texsize{ float(mip_size) };

		// scale normalized coords to texture coords
		mvec4f up(texcoord.s * texsize);
		mvec4f vp(texcoord.t * texsize);

		// floor()
		mvec4i tx0(ftoi(up));
		mvec4i ty0(ftoi(vp));
		mvec4i tx1(tx0 + mvec4i(1));
		mvec4i ty1(ty0 + mvec4i(1));

		// get fractional parts
		const mvec4f fx(up - itof(tx0));
		const mvec4f fy(vp - itof(ty0));
		const mvec4f fx1(mvec4f(1.0f) - fx);
		const mvec4f fy1(mvec4f(1.0f) - fy);

		// calculate weights
		const mvec4f w1(fx1 * fy1);
		const mvec4f w2(fx  * fy1);
		const mvec4f w3(fx1 * fy);
		const mvec4f w4(fx  * fy);

		// temporary load p1 AoS data by lane, then transpose to make SoA rrrr/gggg/bbbb/aaaa
		auto p1 = fetch_texel(offset, mip_size, tx0, ty0);
		auto p2 = fetch_texel(offset, mip_size, tx1, ty0);
		auto p3 = fetch_texel(offset, mip_size, tx0, ty1);
		auto p4 = fetch_texel(offset, mip_size, tx1, ty1);

		return qfloat4{
			p1.x*w1 + p2.x*w2 + p3.x*w3 + p4.x*w4,
			p1.y*w1 + p2.y*w2 + p3.y*w3 + p4.y*w4,
			p1.z*w1 + p2.z*w2 + p3.z*w3 + p4.z*w4,
			p1.w*w1 + p2.w*w2 + p3.w*w3 + p4.w*w4
		}; }};


template<int power>
struct ts_pow2_mipmap_nearest {

	const PixelToaster::FloatingPointPixel * __restrict texdata;
	const float fstride;

	ts_pow2_mipmap_nearest(const PixelToaster::FloatingPointPixel * const __restrict ptr) :texdata(ptr), fstride(float(1<<power)) {}

	inline qfloat4 fetch_texel(const int rowoffset, const int mipsize, const mvec4i& x, const mvec4i& y) const {
		const auto texmod = mvec4i(mipsize - 1);

		auto tx = mvec4i(x & texmod);
		auto ty = mvec4i(texmod - (y & texmod));

		auto offset = mvec4i(shl<power>(ty) | tx) + mvec4i(rowoffset);

		__m128 r0 = _mm_load_ps((float*)&texdata[offset.x]);
		__m128 r1 = _mm_load_ps((float*)&texdata[offset.y]);
		__m128 r2 = _mm_load_ps((float*)&texdata[offset.z]);
		__m128 r3 = _mm_load_ps((float*)&texdata[offset.w]);
		_MM_TRANSPOSE4_PS(r0, r1, r2, r3);
		return qfloat4{ r0, r1, r2, r3 }; }

	inline qfloat4 sample(const qfloat2& texcoord) const {

		int offset, mip_size, stride;
		float dux = (texcoord.s.y - texcoord.s.x)*fstride;
		float duy = (texcoord.s.z - texcoord.s.x)*fstride;
		float dvx = (texcoord.t.y - texcoord.t.x)*fstride;
		float dvy = (texcoord.t.z - texcoord.t.x)*fstride;
		mipcalc<power>(dux, dvx, duy, dvy, offset, mip_size, stride);

		const mvec4f texsize(itof(mvec4i(mip_size)));

		// scale normalized coords to texture coords
		mvec4f up(texcoord.s * texsize);
		mvec4f vp(texcoord.t * texsize);

		// temporary load p1 AoS data by lane, then transpose to make SoA rrrr/gggg/bbbb/aaaa
		return fetch_texel(offset, mip_size, ftoi(up), ftoi(vp)); }};


template<int power>
struct ts_pow2_direct_nearest {

	const PixelToaster::FloatingPointPixel * __restrict texdata;
	const float fstride;
	const mvec4i texmod;

	ts_pow2_direct_nearest(const PixelToaster::FloatingPointPixel * const __restrict ptr) :texdata(ptr), fstride(float(1<<power)), texmod((2<<power)-1) {}

	inline qfloat4 fetch_texel(const mvec4i& x, const mvec4i& y) const {
		auto tx = mvec4i(x & texmod);
		auto ty = mvec4i(texmod - (y & texmod));

		auto offset = mvec4i(shl<power>(ty) | tx);

		__m128 r0 = _mm_load_ps((float*)&texdata[offset.x]);
		__m128 r1 = _mm_load_ps((float*)&texdata[offset.y]);
		__m128 r2 = _mm_load_ps((float*)&texdata[offset.z]);
		__m128 r3 = _mm_load_ps((float*)&texdata[offset.w]);
		_MM_TRANSPOSE4_PS(r0, r1, r2, r3);
		return qfloat4{ r0, r1, r2, r3 }; }

	inline void sample(const qfloat2& texcoord) const {
		vec4 up(texcoord.u * fstride);
		vec4 vp(texcoord.v * fstride);
		return fetch_texel(ftoi(up), ftoi(vp)); }};


struct ts_any_direct_nearest {

	const PixelToaster::FloatingPointPixel * __restrict texdata;
	const float fw, fh;
	const int width;
	const int height;

	ts_any_direct_nearest(const PixelToaster::FloatingPointPixel * const __restrict ptr, const int width, const int height)
		:texdata(ptr),
		fw(float(width)),
		fh(float(height)),
		width(width),
		height(height)
	{}

	inline qfloat4 fetch_texel(const mvec4i& x, const mvec4i& y) const {
		__m128 r[4];

		for (int i = 0; i < 4; i++) {
			auto tx = x.si[i]; // v.m128i_i32[i];
			auto ty = y.si[i]; // v.m128i_i32[i]; XXX better method?

			if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
				const int offset = ty*width + tx;
				r[i] = mvec4f::load(reinterpret_cast<const __m128*>(&texdata[offset])).v; }
			else {
				r[i] = _mm_setzero_ps(); }}

		_MM_TRANSPOSE4_PS(r[0], r[1], r[2], r[3]);
		return qfloat4{ r[0], r[1], r[2], r[3] }; }

	inline qfloat4 sample(const qfloat2& texcoord) const {
		const mvec4f up{texcoord.s *  fw};
		const mvec4f vp{texcoord.t * -fh};
		return fetch_texel(ftoi(up), ftoi(vp)); }};
