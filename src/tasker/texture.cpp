#include "stdafx.h"
#include <fstream>
#include <string>

#include <fmt/format.h>
#include <PixelToaster.h>
#include <picopng.h>

#include "aligned-containers.h"
#include "srgb-ryg.h"
#include "texture.h"
#include "utils.h"
#include "mvec4.h"
#include "vec.h"

using namespace PixelToaster;
using namespace std;


vector<uint8_t> load_file(const string& filename);


Texture load_png(const string filename, const string name, const bool premultiply) {
	using ryg::srgb8_to_float;

	std::vector<unsigned char> image;

	auto data = load_file(filename);

	unsigned long w, h;
	int error = decodePNG(image, w, h, data.empty() ? 0 : data.data(), (unsigned long)data.size());
	if (error != 0) {
		cout << "error(" << error << ") decoding png from [" << filename << "]" << endl;
		while (1); }

	a64::vector<FloatingPointPixel> pc;

	pc.resize(w * h);

	auto* ic = image.data();
	for (unsigned i = 0; i < w*h; i++) {
		auto& dst = pc[i];
		dst.r = srgb8_to_float(*(ic++));
		dst.g = srgb8_to_float(*(ic++));
		dst.b = srgb8_to_float(*(ic++));
		dst.a = *(ic++) / 255.0f; // XXX is alpha srgb?
		if (premultiply) {
			dst.r *= dst.a;
			dst.g *= dst.a;
			dst.b *= dst.a; }}

	return{ pc, int(w), int(h), int(w), name }; }


Texture load_any(const string& prefix, const string& fn, const string& name, const bool premultiply) {
	//	string tmp = fn;
	//	transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
	cout << "texturestore: loading " << prefix << fn << endl;
	string ext = fn.substr(fn.length() - 4, 4);
	if (ext == ".png") {
		return load_png(prefix + fn, name, premultiply); }
//	else if (ext == ".jpg") {
//		return loadJpg(prefix + fn, name);
//	}
	else {
		cout << "unsupported texture extension \"" << ext << "\"" << endl;
		while (1); }}


bool is_power_of_two(unsigned x) {
	while (((x & 1) == 0) && x > 1) {
		x >>= 1; }
	return x == 1; }


int ilog2(unsigned x) {
	int pow = 0;
	while (x) {
		x >>= 1;
		pow++; }
	return pow - 1; }


void Texture::maybe_make_mipmap() {

	if (width != height) {
		pow = -1;
		mipmap = false;
		return; }

	if (!is_power_of_two(width)) {
		pow = -1;
		mipmap = false;
		return; }

	pow = ilog2(width);
	if (pow > 9) {
		pow = -1;
		mipmap = false;
		return; }

	buf.resize(width * height * 2);

	int src_size = width;          // inital w/h of source
	int src = 0;                   // begin reading from the root image start
	int dst = src_size * src_size; // start output at the end of the root image
	for (int mip_level = pow - 1; mip_level >= 0; mip_level--) {
		//		cout << "making " << (sw >> 1) << endl;
		for (int src_y = 0; src_y < src_size; src_y += 2) {
			int dstrow = dst;
			for (int src_x = 0; src_x < src_size; src_x += 2) {

				const auto row1ofs = src + (src_y     *width) + src_x;
				const auto row2ofs = src + ((src_y + 1)*width) + src_x;

				const auto sum2x2 = (
					mvec4f(buf[row1ofs].v) + mvec4f(buf[row1ofs + 1].v) +
					mvec4f(buf[row2ofs].v) + mvec4f(buf[row2ofs + 1].v));

				const auto avg2x2 = sum2x2 / mvec4f(4.0f);

				buf[dstrow++].v = avg2x2.v;
			}
			dst += stride;
		}
		src += src_size * stride; // advance read pos to mip-level we just drew
		src_size >>= 1;

	}
	this->mipmap = true;
	//	this->height *= 2;  // this is probably only useful for viewing the mipmap itself
}


Texture checkerboard2x2() {
	a64::vector<FloatingPointPixel> db;
	db.push_back(FloatingPointPixel(0, 0, 0, 0));
	db.push_back(FloatingPointPixel(1, 0, 1, 0));
	db.push_back(FloatingPointPixel(1, 0, 1, 0));
	db.push_back(FloatingPointPixel(0, 0, 0, 0));
	return{ db, 2, 2, 2, "checkerboard" }; }


TextureStore::TextureStore() {
	this->append(checkerboard2x2()); }


void TextureStore::append(Texture t) {
	store.push_back(t); }


const Texture * const TextureStore::find_by_name(const string& name) const {
	for (auto& item : store) {
		if (item.name == name) return &item; }
	return nullptr; }


void TextureStore::load_any(const string& prepend, const string& fname) {
	auto existing = this->find_by_name(fname);
	if (existing != nullptr) {
		return; }

	Texture newtex = ::load_any(prepend, fname, fname, true);
	newtex.maybe_make_mipmap();
	this->append(newtex); }


void TextureStore::load_dir(const string& prepend) {
	static const vector<string> extensions{ "*.png" }; // , "*.jpg" };

	for (auto& ext : extensions) {
		for (auto& fn : fileglob(prepend + ext)) {
			//cout << "scanning [" << prepend << "][" << fn << "]" << endl;
			this->load_any(prepend, fn); }}}


void TextureStore::print() {
	int i = 0;
	for (const auto& item : store) {
		const void * const ptr = item.buf.data();
		fmt::printf("#% 3d \"%-20s\" % 4d x% 4d", i, item.name, item.width, item.height);
		fmt::printf("  data@ 0x%p\n", ptr);
		i++; }}


vector<uint8_t> load_file(const string& filename) {
	ifstream file(filename.c_str(), ios::in | ios::binary | ios::ate);

	// get filesize
	streamsize size = 0;
	if (file.seekg(0, ios::end).good()) {
		size = file.tellg(); }
	if (file.seekg(0, ios::beg).good()) {
		size -= file.tellg(); }

	vector<uint8_t> buffer;
	if (size > 0) {
		buffer.resize(size);
		file.read(reinterpret_cast<char*>(buffer.data()), size); }
	return buffer; }
