#pragma once
#include <iostream>
#include <array>

#include "aligned-containers.h"
#include "guardband.h"
#include "material.h"
#include "texture.h"
#include "vec_soa.h"
#include "canvas.h"
#include "jobsys.h"
#include "mat4.h"
#include "vec.h"


constexpr int block_size = 8;
constexpr auto block_dimensions_in_pixels = ivec2{8, 8};

constexpr int GL_TRIANGLES = 0;
constexpr int GL_QUADS = 1;
constexpr int GL_TRIANGLE_STRIP = 2;
constexpr int GL_QUAD_STRIP = 3;

constexpr int GL_CW = 0;
constexpr int GL_CCW = 1;

constexpr int GL_CULL_FACE = 1;

constexpr int GL_FRONT = 1;
constexpr int GL_BACK = 2;
constexpr int GL_FRONT_AND_BACK = (GL_FRONT | GL_BACK);

constexpr int GL_MODELVIEW = 1;
constexpr int GL_PROJECTION = 2;
constexpr int GL_TEXTURE = 3;


struct SoaVertexBuffer {
	a64::vector<float> px, py, pz;
	a64::vector<float> nx, ny, nz;
	a64::vector<float> tx, ty;

	const int find(const vec4& p, const vec4& n, const vec4& t) {
		for (int i = 0; i < px.size(); i++) {
			if (almost_equal(px[i], p.x) &&
				almost_equal(py[i], p.y) &&
				almost_equal(pz[i], p.z) &&
				almost_equal(nx[i], n.x) &&
				almost_equal(ny[i], n.y) &&
				almost_equal(nz[i], n.z) &&
				almost_equal(tx[i], t.x) &&
				almost_equal(ty[i], t.y))
				return i; }
		return -1; }


	const void append(const vec4& p, const vec4& n, const vec4& t) {
		px.emplace_back(p.x); py.emplace_back(p.y); pz.emplace_back(p.z);
		nx.emplace_back(n.x); ny.emplace_back(n.y); nz.emplace_back(n.z);
		tx.emplace_back(t.x); ty.emplace_back(t.y); }

	void append(const vec3& p, const vec3& n) {
		px.emplace_back(p.x); py.emplace_back(p.y); pz.emplace_back(p.z);
		nx.emplace_back(n.x); ny.emplace_back(n.y); nz.emplace_back(n.z); }

	void clear() {
		px.clear(); py.clear(); pz.clear();
		nx.clear(); ny.clear(); nz.clear();
		tx.clear(); ty.clear(); }

	void pad() {
		const int rag = px.size() % 4;
		if (rag == 0)
			return;
		for (int i = rag; i < 4; i++) {
			px.push_back(0.0f); py.push_back(0.0f); pz.push_back(0.0f);
			nx.push_back(0.0f); ny.push_back(0.0f); nz.push_back(0.0f);
			tx.push_back(0.0f); ty.push_back(0.0f); }}


	const int upsert(const vec4& p, const vec4& n, const vec4& t) {
		int idx = find(p, n, t);
		if (idx == -1) {
			idx = px.size();
			append(p, n, t); }
		return idx; }};


struct GlVertexData {
	vec3 c;
	vec2 tc1;

	static inline GlVertexData clip(const GlVertexData& a, const GlVertexData& b, const float t) {
		return GlVertexData{
			lerp(a.c, b.c, t),
			lerp(a.tc1, b.tc1, t) };}};


struct ClipPair {
	vec4 position;
	GlVertexData data; };


struct GlVertexDataSoa {
	qfloat3 c;
	qfloat2 tc1;

	inline void push_back_into(a64::vector<GlVertexData>& buf) {
		/*convert SoA data to AoS and push into a vector
		
		the source data is destroyed in the process due to in-place transpose*/
		qfloat& c1 = c.x;
		qfloat& c2 = c.y;
		qfloat& c3 = c.z;
		qfloat  c4 = mvec4f{ 1.0f };
		_MM_TRANSPOSE4_PS(c1.v, c2.v, c3.v, c4.v);

		qfloat& tc1_1 = tc1.x;
		qfloat& tc1_2 = tc1.y;
		qfloat  tc1_3 = mvec4f{ 0 };
		qfloat  tc1_4 = mvec4f{ 1.0f };
		_MM_TRANSPOSE4_PS(tc1_1.v, tc1_2.v, tc1_3.v, tc1_4.v);

		buf.emplace_back(GlVertexData{ c1.xyz(), tc1_1.xy() });
		buf.emplace_back(GlVertexData{ c2.xyz(), tc1_2.xy() });
		buf.emplace_back(GlVertexData{ c3.xyz(), tc1_3.xy() });
		buf.emplace_back(GlVertexData{ c4.xyz(), tc1_4.xy() }); }};


struct GlPipeFace {
	/*packed face data to store in bins
	
	24-bits index for vertex0, and 16-bit deltas for vertex1&2
	
	7 bits for material_id, and 1 for backfacing flag*/
	uint32_t bf_mat_i0;
	short d1, d2;

	static GlPipeFace make(const int i0, const int i1, const int i2, const int material, const bool backfacing) {
		_ASSERT(material < 128);
		_ASSERT(abs(i1 - i0) < 32727);
		_ASSERT(abs(i2 - i0) < 32727);
		_ASSERT(i0 < 0xffffff);
		uint8_t mat = material;
		if (backfacing) {
			mat |= 0x80; }
		uint32_t packed = (mat << 24) | i0;
		return GlPipeFace{ packed, short(i1 - i0), short(i2 - i0) }; }

	void unmake(int& i0, int &i1, int &i2, int& material, bool& backfacing) const {
		auto _mat = bf_mat_i0 >> 24;
		backfacing = (_mat & 0x80) > 0;
		material = _mat & 0x7f;

		i0 = bf_mat_i0 & 0xffffff;
		i1 = i0 + d1;
		i2 = i0 + d2; } };


class Tilebin {
public:
	Tilebin(const int id, const irect& rect) :id(id), rect(rect) {}

	void clear() {
		gldata.clear(); }

public:
	int id;
	irect rect;
	a64::vector<GlPipeFace> gldata; };


class Binner {
public:
	Binner() :buffer_dimensions_in_pixels({ 0, 0 }) {}

	void reset(const ivec2& new_buffer_dimensions_in_pixels,
		const ivec2& new_tile_dimensions_in_blocks) {
		if (new_buffer_dimensions_in_pixels != buffer_dimensions_in_pixels ||
			new_tile_dimensions_in_blocks != tile_dimensions_in_blocks) {

			buffer_dimensions_in_pixels = new_buffer_dimensions_in_pixels;
			tile_dimensions_in_blocks = new_tile_dimensions_in_blocks;
			buffer_dimensions_in_pixels_vec4 = vec4{ float(buffer_dimensions_in_pixels.x),
			                                         float(buffer_dimensions_in_pixels.y), 0, 0 };
			retile(); }
		for (auto& bin : bins) {
			bin.clear(); }}

	void insert(
		const vec4& p1, const vec4& p2, const vec4& p3,
		const int i0, const int i1, const int i2,
		const int material_id,
		const bool backfacing);

private:
	void retile();

public:
	std::vector<Tilebin> bins;

private:
	ivec2 buffer_dimensions_in_pixels;
	ivec2 buffer_dimensions_in_tiles;
	ivec2 tile_dimensions_in_blocks;
	ivec2 tile_dimensions_in_pixels;
	vec4 buffer_dimensions_in_pixels_vec4; };


#define VERTEX_SHADER_ARGUMENTS                \
	const vec3& gl_Vertex,                     \
	const vec3& gl_Normal,                     \
	const vec3& gl_Color,                      \
	const vec2& gl_MultiTexCoord0,             \
	const mat4& gl_NormalMatrix,               \
	const mat4& gl_ModelViewMatrix,            \
	const mat4& gl_ProjectionMatrix,           \
	const mat4& gl_ModelViewProjectionMatrix,  \
	vec4& gl_Position,                         \
	GlVertexData& data


#define VERTEX_SHADER_ARGUMENTS_SOA            \
	const qfloat3& gl_Vertex,                  \
	const qfloat3& gl_Normal,                  \
	const qfloat3& gl_Color,                   \
	const qfloat2& gl_MultiTexCoord0,          \
	const qmat4& gl_NormalMatrix,              \
	const qmat4& gl_ModelViewMatrix,           \
	const qmat4& gl_ProjectionMatrix,          \
	const qmat4& gl_ModelViewProjectionMatrix, \
	qfloat4& gl_Position,                      \
	GlVertexDataSoa& data


struct EnvMapVertexShader {
	void operator()(VERTEX_SHADER_ARGUMENTS) {
		vec4 position = mul_w1(gl_ModelViewMatrix, gl_Vertex);
		//vec4 position = gl_ModelViewMatrix * gl_Vertex;
		vec3 e = normalize(position.xyz());
		vec3 n = normalize((mul_w0(gl_NormalMatrix, gl_Normal)).xyz());
		//vec3 n = normalize((gl_NormalMatrix * gl_Normal).xyz());
		vec3 r = reflect(e, n);

		float rx = r.x, ry = r.y, rz = r.z;
		float m = 2.0f * sqrt((rx*rx) + (ry*ry) + ((rz + 1.0f)*(rz + 1.0f)));
		float uu = rx / m + 0.5f;
		float vv = ry / m + 0.5f;
		data.tc1 = vec2{ uu, vv };

		gl_Position = mul_w1(gl_ModelViewProjectionMatrix, gl_Vertex); }
		//gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; }

	void operator()(VERTEX_SHADER_ARGUMENTS_SOA) {
		qfloat4 position = mul_w1(gl_ModelViewMatrix, gl_Vertex);
		qfloat3 e = normalize(position.xyz());
		qfloat3 n = normalize(mul_w0(gl_NormalMatrix, gl_Normal));
		qfloat3 r = reflect(e, n);

		qfloat m = qfloat{ 2.0f } *sqrt((r.x*r.x) + (r.y*r.y) + ((r.z + 1.0f)*(r.z + 1.0f)));
		qfloat uu = r.x / m + 0.5f;
		qfloat vv = r.y / m + 0.5f;
		data.tc1 = qfloat2{ uu, vv };

		gl_Position = mul_w1(gl_ModelViewProjectionMatrix, gl_Vertex); }};


struct DefaultVertexShader {
	void operator()(VERTEX_SHADER_ARGUMENTS) {
		//gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
		gl_Position = mul_w1(gl_ModelViewProjectionMatrix, gl_Vertex);
		data.c = gl_Color;
		//data.n = gl_normal;
		data.tc1 = gl_MultiTexCoord0; }
	void operator()(VERTEX_SHADER_ARGUMENTS_SOA) {
		//gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
		gl_Position = mul_w1(gl_ModelViewProjectionMatrix, gl_Vertex);
		data.c = gl_Color;
		//data.n = gl_normal;
		data.tc1 = gl_MultiTexCoord0; }};




class GlPipe {
public:
	GlPipe() :stack_active(&stack_modelview) {}

	void reset(const ivec2& buffer_dimensions_in_pixels,
	           const ivec2& tile_dimensions_in_blocks) {
		device_matrix = make_device_matrix(buffer_dimensions_in_pixels.x, buffer_dimensions_in_pixels.y);
		binner.reset(buffer_dimensions_in_pixels,
		             tile_dimensions_in_blocks);
		glReset(); }

	inline void glReset() {
		stack_modelview.clear();
		stack_modelview.load(mat4::ident());
		stack_projection.clear();
		stack_projection.load(mat4::ident());
		stack_texture.clear();
		stack_texture.load(mat4::ident());
		gl_matrix_mode = GL_MODELVIEW;
		gl_culling = false;
		gl_cull_face = GL_BACK;
		stack_active = &stack_modelview;
		gl_program = 0;
		pv.clear();
		dv.clear();}

	inline void glPushMatrix() {
		stack_active->push(); }

	inline void glPopMatrix() {
		stack_active->pop(); }

	inline void glTranslate(const vec4& a) {
		stack_active->mul(mat4::translate(a)); }

	inline void glTranslate(const vec3& a) {
		stack_active->mul(mat4::translate(a)); }

	inline void glTranslate(const float x, const float y, const float z) {
		stack_active->mul(mat4::translate(x, y, z)); }

	inline void glScale(const vec4& a) {
		stack_active->mul(mat4::scale(a)); }

	inline void glScale(const float x, const float y, const float z) {
		stack_active->mul(mat4::scale(x, y, z)); }

	inline void glRotate(const float theta, const float x, const float y, const float z) {
		stack_active->mul(mat4::rotate(theta, x, y, z)); }

	inline void glLoadIdentity() {
		stack_active->load(mat4::ident()); }

	inline void glLoadMatrix(const mat4& m) {
		stack_active->load(m); }

	inline void glMatrixMode(const int value) {
		if (value == GL_MODELVIEW) {
			stack_active = &stack_modelview; }
		else if (value == GL_PROJECTION) {
			stack_active = &stack_projection; }
		else if (value == GL_TEXTURE) {
			stack_active = &stack_texture; }
		else {
			throw std::exception("unknown matrix mode"); }}

	inline void glEnable(const int value) {
		if (value == GL_CULL_FACE) {
			gl_culling = true; }
		else {
			throw std::exception("unknown glEnable value"); }}

	inline void glDisable(const int value) {
		if (value == GL_CULL_FACE) {
			gl_culling = false; }
		else {
			throw std::exception("unknown glEnable value"); }}

	inline void glCullFace(const int value) {
		gl_cull_face = value; }

	inline void glBegin(const int mode) {
		_mm_setcsr(_mm_getcsr() | 0x8040);
		m_modelview = stack_modelview.top();
		m_projection = stack_projection.top();
		m_normal = transpose(inverse(m_modelview));
		m_modelviewprojection = m_projection * m_modelview;

		_ASSERT(clip_queue.size() == 0);
		_ASSERT(clip_faces.size() == 0);

		gl_strip_loaded = false;
		gl_mode = mode;
		gl_front_face_winding_order = GL_CCW;
		gl_total_poly = 0;
		if (gl_mode == GL_TRIANGLES || gl_mode == GL_TRIANGLE_STRIP) {
			gl_needed = 3; }
		else if (gl_mode == GL_QUADS || gl_mode == GL_QUAD_STRIP) {
			gl_needed = 4; }
		else {
			throw std::exception("bad gl vertex mode"); }}

	inline void glEnd() {
		clip_finish();
	}

	inline void glColor(const vec3& v) {
		gl_color = v; }

	inline void glColor(const float r, const float g, const float b) {
		gl_color = vec3{ r, g, b }; }

	inline void glNormal(const vec4& v) {
		_ASSERT(v.w == 0);
		gl_normal = vec3{ v.x, v.y, v.z }; }

	inline void glNormal(const vec3& v) {
		gl_normal = v; }

	inline void glTexCoord(const float s, const float t) {
		gl_texcoord = vec2{ s, t }; }

	inline void glMaterial(const int v) {
		gl_material = v; }

	inline void glUseProgram(const int v) {
		gl_program = v; }

#define FIXED_PARAMS m_normal, m_modelview, m_projection, m_modelviewprojection, out_position, data
#define FIXED_PARAMS_SOA qm_normal, qm_modelview, qm_projection, qm_modelviewprojection, out_position, data

	inline void glDrawElements(SoaVertexBuffer& s, const a64::vector<int>& idx, const bool include_material) {
		thread_local a64::vector<unsigned char> cfbuf;
		thread_local a64::vector<vec4> pbuf;

		_mm_setcsr(_mm_getcsr() | 0x8040);
		cfbuf.clear();
		pbuf.clear();

		//clip_reset();
		_ASSERT(clip_queue.size() == 0);
		_ASSERT(clip_faces.size() == 0);

		const qmat4 qm_modelview{ stack_modelview.top() };
		const qmat4 qm_projection{ stack_projection.top() };
		const qmat4 qm_normal{ transpose(inverse(stack_modelview.top())) };
		const qmat4 qm_modelviewprojection{ stack_projection.top() * stack_modelview.top() };
		const qmat4 qm_device{ device_matrix };
		GlVertexDataSoa data;
		qfloat4 out_position;

		const int bp = pv.size();

		const int siz = s.px.size();
		const int rag = siz % 4;
		_ASSERT(rag == 0);
		int vi = 0;
		int fi = 0;
		for (; vi < siz && fi < idx.size(); vi += 4) {

			if ((vi & 0x1111111) == 0) {
				//std::cout << "trying a batch" << std::endl;
				while (1) {
					if (fi >= idx.size()) {
						break; }
					if (include_material)
						gl_material = idx[fi];
					const int& i0 = idx[fi + 1];
					const int& i1 = idx[fi + 2];
					const int& i2 = idx[fi + 3];
					if (i0 < vi && i1 < vi && i2 < vi) {
						process_gl_triangle(
							cfbuf[i0], cfbuf[i1], cfbuf[i2],
							pbuf[i0], pbuf[i1], pbuf[i2],
							bp+i0, bp+i1, bp+i2);
						fi += 4; }
					else {
						break; }}}

			qfloat3 vpos{
				_mm_load_ps(&s.px[vi]),
				_mm_load_ps(&s.py[vi]),
				_mm_load_ps(&s.pz[vi]) };

			qfloat3 vnor{
				_mm_load_ps(&s.nx[vi]),
				_mm_load_ps(&s.ny[vi]),
				_mm_load_ps(&s.nz[vi]) };

			qfloat2 vtex;

			if (gl_program == 0) {
				vtex = qfloat2{
					_mm_load_ps(&s.tx[vi]),
					_mm_load_ps(&s.ty[vi]) }; }
			else {
				vtex = qfloat2{
					qfloat(gl_texcoord.x),
					qfloat(gl_texcoord.y) }; }

			qfloat3 vcol;

			if (gl_program == 0) {
				DefaultVertexShader()(vpos, vnor, vcol, vtex, FIXED_PARAMS_SOA); }
			else if (gl_program == 1) {
				EnvMapVertexShader()(vpos, vnor, vcol, vtex, FIXED_PARAMS_SOA); }
			else {
				throw std::exception("unknown vertex program"); }

			//pbuf.emplace_back(out_position);
			auto flags = Guardband::clip_point(out_position);
			cfbuf.emplace_back(flags.x);
			cfbuf.emplace_back(flags.y);
			cfbuf.emplace_back(flags.z);
			cfbuf.emplace_back(flags.w);

			auto devpos = pdiv(mul_w1(qm_device, out_position));
			{
				auto& p1 = devpos.x;
				auto& p2 = devpos.y;
				auto& p3 = devpos.z;
				auto& p4 = devpos.w;
				_MM_TRANSPOSE4_PS(p1.v, p2.v, p3.v, p4.v);
				dv.emplace_back(p1.xyzw());
				dv.emplace_back(p2.xyzw());
				dv.emplace_back(p3.xyzw());
				dv.emplace_back(p4.xyzw()); }
			{
				auto& p1 = out_position.x;
				auto& p2 = out_position.y;
				auto& p3 = out_position.z;
				auto& p4 = out_position.w;
				_MM_TRANSPOSE4_PS(p1.v, p2.v, p3.v, p4.v);
				pbuf.emplace_back(p1.xyzw());
				pbuf.emplace_back(p2.xyzw());
				pbuf.emplace_back(p3.xyzw());
				pbuf.emplace_back(p4.xyzw()); }
			data.push_back_into(pv); }

		while (1) {
			if (fi >= idx.size()) {
				break; }
			if (include_material)
				gl_material = idx[fi];
			const int& i0 = idx[fi + 1];
			const int& i1 = idx[fi + 2];
			const int& i2 = idx[fi + 3];
			if (i0 < vi && i1 < vi && i2 < vi) {
				process_gl_triangle(
					cfbuf[i0], cfbuf[i1], cfbuf[i2],
					pbuf[i0], pbuf[i1], pbuf[i2],
					bp+i0, bp+i1, bp+i2);
				fi += 4; }
			else {
				break; }}

		clip_finish(); }

	inline void glDrawElements(SoaVertexBuffer& s) {
		thread_local a64::vector<unsigned char> cfbuf;
		thread_local a64::vector<vec4> pbuf;

		_mm_setcsr(_mm_getcsr() | 0x8040);
		cfbuf.clear();
		pbuf.clear();

		//clip_reset();
		_ASSERT(clip_queue.size() == 0);
		_ASSERT(clip_faces.size() == 0);

		const qmat4 qm_modelview{ stack_modelview.top() };
		const qmat4 qm_projection{ stack_projection.top() };
		const qmat4 qm_normal{ transpose(inverse(stack_modelview.top())) };
		const qmat4 qm_modelviewprojection{ stack_projection.top() * stack_modelview.top() };
		const qmat4 qm_device{ device_matrix };
		GlVertexDataSoa data;
		qfloat4 out_position;

		const int bp = pv.size();

		const int siz = s.px.size();
		const int rag = siz % 4;
		_ASSERT(rag == 0);
		int vi = 0;
		int fi = 0;
		for (; vi < siz && fi < siz; vi += 4) {

			if ((vi & 0x1111111) == 0) {
				//std::cout << "trying a batch" << std::endl;
				while (1) {
					if (fi >= siz) {
						break; }
					const int& i0 = fi + 0;
					const int& i1 = fi + 1;
					const int& i2 = fi + 2;
					if (i0 < vi && i1 < vi && i2 < vi) {
						process_gl_triangle(
							cfbuf[i0], cfbuf[i1], cfbuf[i2],
							pbuf[i0], pbuf[i1], pbuf[i2],
							bp+i0, bp+i1, bp+i2);
						fi += 3; }
					else {
						break; }}}

			qfloat3 vpos{
				_mm_load_ps(&s.px[vi]),
				_mm_load_ps(&s.py[vi]),
				_mm_load_ps(&s.pz[vi]) };

			qfloat3 vnor{
				_mm_load_ps(&s.nx[vi]),
				_mm_load_ps(&s.ny[vi]),
				_mm_load_ps(&s.nz[vi]) };

			qfloat2 vtex;

			if (gl_program == 0) {
				vtex = qfloat2{
					_mm_load_ps(&s.tx[vi]),
					_mm_load_ps(&s.ty[vi]) }; }
			else {
				vtex = qfloat2{
					gl_texcoord.x,
					gl_texcoord.y }; }

			qfloat3 vcol;

			if (gl_program == 0) {
				DefaultVertexShader()(vpos, vnor, vcol, vtex, FIXED_PARAMS_SOA); }
			else if (gl_program == 1) {
				EnvMapVertexShader()(vpos, vnor, vcol, vtex, FIXED_PARAMS_SOA); }
			else {
				throw std::exception("unknown vertex program"); }

			//pbuf.emplace_back(out_position);
			auto flags = Guardband::clip_point(out_position);
			cfbuf.emplace_back(flags.x);
			cfbuf.emplace_back(flags.y);
			cfbuf.emplace_back(flags.z);
			cfbuf.emplace_back(flags.w);

			auto devpos = pdiv(mul_w1(qm_device, out_position));
			{
				auto& p1 = devpos.x;
				auto& p2 = devpos.y;
				auto& p3 = devpos.z;
				auto& p4 = devpos.w;
				_MM_TRANSPOSE4_PS(p1.v, p2.v, p3.v, p4.v);
				dv.emplace_back(p1.xyzw());
				dv.emplace_back(p2.xyzw());
				dv.emplace_back(p3.xyzw());
				dv.emplace_back(p4.xyzw()); }
			{
				auto& p1 = out_position.x;
				auto& p2 = out_position.y;
				auto& p3 = out_position.z;
				auto& p4 = out_position.w;
				_MM_TRANSPOSE4_PS(p1.v, p2.v, p3.v, p4.v);
				pbuf.emplace_back(p1.xyzw());
				pbuf.emplace_back(p2.xyzw());
				pbuf.emplace_back(p3.xyzw());
				pbuf.emplace_back(p4.xyzw()); }
			data.push_back_into(pv); }

		while (1) {
			if (fi >= siz) {
				break; }
			const int& i0 = fi + 0;
			const int& i1 = fi + 1;
			const int& i2 = fi + 2;
			if (i0 < vi && i1 < vi && i2 < vi) {
				process_gl_triangle(
					cfbuf[i0], cfbuf[i1], cfbuf[i2],
					pbuf[i0], pbuf[i1], pbuf[i2],
					bp+i0, bp+i1, bp+i2);
				fi += 3; }
			else {
				break; }}

		clip_finish(); }

	inline void glVertex(const vec3& p) {
		thread_local static std::array<unsigned char, 4> pflags;
		thread_local static std::array<vec4, 4> lastp;

		GlVertexData data;
		vec4 out_position;

		if (gl_program == 0) {
			DefaultVertexShader()(p, gl_normal, gl_color, gl_texcoord, FIXED_PARAMS); }
		else if (gl_program == 1) {
			EnvMapVertexShader()(p, gl_normal, gl_color, gl_texcoord, FIXED_PARAMS); }
		else {
			throw std::exception("unknown vertex program"); }

		// push newest clip flags & device point onto the clip-flag stack
		std::copy(pflags.begin() + 1, pflags.end(), pflags.begin());
		std::copy(lastp.begin() + 1, lastp.end(), lastp.begin());
		pflags.back() = Guardband::clip_point(out_position);
		lastp.back() = out_position;

		const int vidx = pv.size();
		dv.emplace_back(pdiv(device_matrix * out_position));
		pv.emplace_back(data);
		gl_needed--;

		if (gl_needed > 0) return;

		if (gl_mode == GL_TRIANGLES) {
			process_gl_triangle(
				pflags[3 - 2], pflags[3 - 1], pflags[3 - 0],
				lastp[3 - 2], lastp[3 - 1], lastp[3 - 0],
				vidx - 2, vidx - 1, vidx - 0);
			gl_needed = 3; }

		else if (gl_mode == GL_TRIANGLE_STRIP) {
			if ((gl_total_poly & 0x1) == 0)
				process_gl_triangle(
					pflags[3 - 2], pflags[3 - 1], pflags[3 - 0],
					lastp[3 - 2], lastp[3 - 1], lastp[3 - 0],
					vidx - 2, vidx - 1, vidx);
			else
				process_gl_triangle(
					pflags[3 - 1], pflags[3 - 2], pflags[3 - 0],
					lastp[3 - 1], lastp[3 - 2], lastp[3 - 0],
					vidx - 1, vidx - 2, vidx);
			gl_total_poly++; }

		else if (gl_mode == GL_QUAD_STRIP) {
			process_gl_quad(
				pflags[3 - 3], pflags[3 - 2], pflags[3 - 0], pflags[3 - 1],
				lastp[3 - 3], lastp[3 - 2], lastp[3 - 0], lastp[3 - 1],
				vidx - 3, vidx - 2, vidx - 0, vidx - 1);
			gl_needed = 2; }

		else if (gl_mode == GL_QUADS) {
			process_gl_quad(
				pflags[3 - 3], pflags[3 - 2], pflags[3 - 1], pflags[3 - 0],
				lastp[3 - 3], lastp[3 - 2], lastp[3 - 1], lastp[3 - 0],
				vidx - 3, vidx - 2, vidx - 1, vidx - 0);
			gl_needed = 4; }}

	const irect get_bin_rect(const int bin_idx) const {
		return binner.bins[bin_idx].rect; }

	const int get_bin_size(const int bin_idx) const {
		return int(binner.bins[bin_idx].gldata.size()); }

	const int get_bin_count() const {
		return int(binner.bins.size()); }

	void render_bin(
		const int pass,
		const int bin_idx,
		const MaterialStore& mstore,
		const TextureStore& tstore,
		QuadFloatCanvas& dbc,
		QuadFloat4Canvas& cbc);

private:
	void process_gl_quad(
		unsigned char cf0, unsigned char cf1, unsigned char cf2, unsigned char cf3,
		const vec4& p0, const vec4& p1, const vec4& p2, const vec4& p3,
		int i0, int i1, int i2, int i3);

	void process_gl_triangle(
		unsigned char cf0, unsigned char cf1, unsigned char cf2,
		const vec4& p0, const vec4& p1, const vec4& p2,
		int i0, int i1, int i2);

	inline void clip_reset() {
		clip_queue.clear();
		clip_faces.clear(); }

	void clip_gl_triangle(
		unsigned char cf,
		const vec4& p0, const vec4& p1, const vec4& p2,
		int i0, int i1, int i2);

	void clip_finish();

	Mat4Stack stack_modelview;
	Mat4Stack stack_projection;
	Mat4Stack stack_texture;
	Mat4Stack* stack_active;
	mat4 device_matrix;
	int device_width, device_height;

	int gl_mode;
	int gl_matrix_mode;
	vec3 gl_normal;
	vec3 gl_color;
	vec2 gl_texcoord;
	int gl_material;
	bool gl_strip_loaded;
	int gl_needed;
	int gl_total_poly;
	int gl_front_face_winding_order;
	bool gl_culling;
	int gl_cull_face;
	int gl_program;

	mat4 m_modelview;
	mat4 m_projection;
	mat4 m_modelviewprojection;
	mat4 m_normal;

	a64::vector<GlVertexData> pv;
	a64::vector<vec4> dv;

	a64::vector<ClipPair> clip_queue;
	a64::vector<int> clip_faces;

	Binner binner; };


class GlPipeSet {
public:
	GlPipeSet(const int many) {
		for (int i = 0; i < many; i++) {
			pipes.push_back(GlPipe{}); }}
	
	GlPipe& get() {
		return pipes[jobsys::thread_id]; }

	int get_bin_size(const int bin_idx) const {
		int ax = 0;
		for (const auto& pipe : pipes) {
			ax += pipe.get_bin_size(bin_idx); }
		return ax; }

	auto get_bin_rect(const int bin_idx) const {
		return pipes[0].get_bin_rect(bin_idx); }

	auto get_bin_count() {
		return pipes[0].get_bin_count(); }

	void render_bin(
		const int pass,
		const int bin_idx,
		const MaterialStore& mstore,
		const TextureStore& tstore,
		QuadFloatCanvas& dbc,
		QuadFloat4Canvas& cbc) {
		for (auto& pipe : pipes) {
			pipe.render_bin(pass, bin_idx, mstore, tstore, dbc, cbc); }}

	void reset(const ivec2& new_buffer_dimensions_in_pixels, const ivec2& new_tile_dimensions_in_blocks) {
		for (auto& pipe : pipes) {
			pipe.reset(new_buffer_dimensions_in_pixels, new_tile_dimensions_in_blocks); }}

	void glMatrixMode(const int value) {
		for (auto& pipe : pipes) {
			pipe.glMatrixMode(value); }}

	void glLoadMatrix(const mat4& m) {
		for (auto& pipe : pipes) {
			pipe.glLoadMatrix(m); }}

	void glTranslate(const float x, const float y, const float z) {
		for (auto& pipe : pipes) {
			pipe.glTranslate(x, y, z); }}

	void glPushMatrix() {
		for (auto& pipe : pipes) {
			pipe.glPushMatrix(); }}

	void glPopMatrix() {
		for (auto& pipe : pipes) {
			pipe.glPopMatrix(); }}

	void glUseProgram(const int v) {
		for (auto& pipe : pipes) {
			pipe.glUseProgram(v); }}

private:
	a64::vector<GlPipe> pipes; };
