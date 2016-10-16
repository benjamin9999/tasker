#pragma once
#include <algorithm>
#include <random>
#include <tuple>

#include "cpu-colors.h"
#include "jobsys.h"
#include "render.h"
#include "vec.h"

#include "marzo.h"

namespace FxBalls {

const float ball_bound = 5.0f;
const int slices_per_task = 4;

const float magic = 0.35f;

static std::mutex slice_complete_lock;


struct ball {
	vec3 pos;
	vec3 dir;
	float vel;
	float r; };


struct soaballs {
	a64::vector<float> _posx;
	a64::vector<float> _posy;
	a64::vector<float> _posz;
	a64::vector<float> _dirx;
	a64::vector<float> _diry;
	a64::vector<float> _dirz;
	a64::vector<float> _vel;
	a64::vector<float> _r;

	void resize(const int ns) {
		_posx.resize(ns);
		_posy.resize(ns);
		_posz.resize(ns);
		_dirx.resize(ns);
		_diry.resize(ns);
		_dirz.resize(ns);
		_vel.resize(ns);
		_r.resize(ns); }

	void clear() {
		_posx.clear();
		_posy.clear();
		_posz.clear();
		_dirx.clear();
		_diry.clear();
		_dirz.clear();
		_vel.clear();
		_r.clear(); }

	void update(const float dt) {
		const int siz = _posx.size();
		const int rag = siz % 4;
		const int cnt = siz - rag;
		const mvec4f vdt{ dt };
		const mvec4f vbound{ ball_bound - 2.0f };
		int i;
		for (i = 0; i < cnt; i+=4) {
			//load
			mvec4f posx{ _mm_load_ps(&_posx[i]) };
			mvec4f posy{ _mm_load_ps(&_posy[i]) };
			mvec4f posz{ _mm_load_ps(&_posz[i]) };
			mvec4f vel{ _mm_load_ps(&_vel[i]) };
			mvec4f dirx{ _mm_load_ps(&_dirx[i]) };
			mvec4f diry{ _mm_load_ps(&_diry[i]) };
			mvec4f dirz{ _mm_load_ps(&_dirz[i]) };

			// update positions based on dt
			mvec4f dist = vel * vdt;
			posx += dirx * dist;
			posy += diry * dist;
			posz += dirz * dist;

			// if posx > limit
			auto mask = cmpgt(posx, vbound);
			//     dirx = -dirx
			dirx = selectbits(dirx, -dirx, mask);
			//     posx = limit + (limit - posx)
			posx = selectbits(posx, vbound + (vbound - posx), mask);
			// else if posx < -limit
			mask = cmplt(posx, -vbound);
			//     dirx = -dirx
			dirx = selectbits(dirx, -dirx, mask);
			//     posx = -limit + (-limit - posx)
			posx = selectbits(posx, -vbound + (-vbound - posx), mask);

			// repeat for y
			mask = cmpgt(posy, vbound);
			diry = selectbits(diry, -diry, mask);
			posy = selectbits(posy, vbound + (vbound - posy), mask);
			mask = cmplt(posy, -vbound);
			diry = selectbits(diry, -diry, mask);
			posy = selectbits(posy, -vbound + (-vbound - posy), mask);

			// repeat for z
			mask = cmpgt(posz, vbound);
			dirz = selectbits(dirz, -dirz, mask);
			posz = selectbits(posz, vbound + (vbound - posz), mask);
			mask = cmplt(posz, -vbound);
			dirz = selectbits(dirz, -dirz, mask);
			posz = selectbits(posz, -vbound + (-vbound - posz), mask);

			// store
			_mm_store_ps(&_posx[i], posx.v);
			_mm_store_ps(&_posy[i], posy.v);
			_mm_store_ps(&_posz[i], posz.v);
			_mm_store_ps(&_dirx[i], dirx.v);
			_mm_store_ps(&_diry[i], diry.v);
			_mm_store_ps(&_dirz[i], dirz.v);
		}
		// XXX no support for the rag
		//for (; i < siz; i++) {
		//}
	}

	void append(const vec3& pos, const vec3& dir, const float vel, const float r) {
		_posx.push_back(pos.x);
		_posy.push_back(pos.y);
		_posz.push_back(pos.z);
		_dirx.push_back(dir.x);
		_diry.push_back(dir.y);
		_dirz.push_back(dir.z);
		_vel.push_back(vel);
		_r.push_back(r); }

//	static vec4 sdSphere(const vec4& x, const vec4& y, const vec4& z, const vec4& r) {
//		return sqrt(x*x + y*y + z*z) - r; }

	static mvec4f C(const mvec4f& r, const mvec4f& d) {
		const auto zero = mvec4f::zero();
		auto mask = cmpge(r, d);
		if (movemask(mask) == 0) {
			return zero; }

		// within radius
		auto left = (2 * d*d*d) / (r*r*r);
		auto right = (3 * d*d) / (r*r);
		auto intensity = left - right + 1;

		return selectbits(zero, intensity, mask); }

	static float C(const float r, const float d) {
		if (r < d) {
			return 0; }
		else {
			// within radius
			auto left = (2 * d*d*d) / (r*r*r);
			auto right = (3 * d*d) / (r*r);
			return left - right + 1; }}

	std::tuple<float, float> sample(float sx, float sy, float sz) {
		const int siz = _posx.size();
		const int rag = siz % 4;
		const int cnt = siz - rag;

		const mvec4f vsx(sx), vsy(sy), vsz(sz);
		mvec4f ax_intensity(0);
		mvec4f ax_min_distance(100000);
		float distance, intensity;

		int i;
		for (i = 0; i < cnt; i += 4) {
			mvec4f ptx{ _mm_load_ps(&_posx[i]) };
			mvec4f pty{ _mm_load_ps(&_posy[i]) };
			mvec4f ptz{ _mm_load_ps(&_posz[i]) };
			mvec4f r{ _mm_load_ps(&_r[i]) };

			ptx = ptx - vsx;
			pty = pty - vsy;
			ptz = ptz - vsz;

			auto dist = sqrt(ptx*ptx + pty*pty + ptz*ptz);
			ax_min_distance = vmin(ax_min_distance, dist - r);
			ax_intensity += C(r, dist); }

		intensity = hadd(ax_intensity);
		distance = hmin(ax_min_distance);

		for (; i < siz; i++) {
			float& posx = _posx[i];
			float& posy = _posy[i];
			float& posz = _posz[i];
			float& r = _r[i];
			auto ptx = posx - sx;
			auto pty = posy - sy;
			auto ptz = posz - sz;
			auto dist = sqrt(ptx*ptx + pty*pty + ptz*ptz);
			distance = std::min(distance, dist - r);
			intensity += C(r, dist); }

		return std::make_tuple(distance, intensity); }

	auto size() const {
		return _posx.size(); }};


void gather_balls(const float radius, const vec3& p, soaballs& src, soaballs& dst) {
	int culled = 0;
	using std::min;
	const int rag = src._posx.size() % 4;
	int cnt = src._posx.size() - rag;
	mvec4f vsx{ p.x }, vsy{ p.y }, vsz{ p.z };
	mvec4f vbr(radius);
	int i;
	for (i = 0; i < cnt; i += 4) {
		auto& posx = *reinterpret_cast<mvec4f*>(&src._posx[i]);
		auto& posy = *reinterpret_cast<mvec4f*>(&src._posy[i]);
		auto& posz = *reinterpret_cast<mvec4f*>(&src._posz[i]);
		auto& r = *reinterpret_cast<mvec4f*>(&src._r[i]);

		auto ptx = posx - vsx;
		auto pty = posy - vsy;
		auto ptz = posz - vsz;

		auto dist = sqrt(ptx*ptx + pty*pty + ptz*ptz) - vbr - r;
		auto mask = movemask(cmplt(dist, mvec4f::zero()));
		int bit = 1;
		for (int bi = 0; bi < 4; bi++, bit <<= 1) {
			if (mask & bit) {
				dst._posx.push_back(src._posx[i + bi]);
				dst._posy.push_back(src._posy[i + bi]);
				dst._posz.push_back(src._posz[i + bi]);
				dst._r.push_back(src._r[i + bi]); }
			else {
				culled++; }}}

	for (; i < src._posx.size(); i++) {
		float px = p.x - src._posx[i];
		float py = p.y - src._posy[i];
		float pz = p.z - src._posz[i];
		float dist = sqrt(px*px + py*py + pz*pz);
		if (dist - (radius + src._r[i]) <= 0.0f) {
			dst._posx.push_back(src._posx[i]);
			dst._posy.push_back(src._posy[i]);
			dst._posz.push_back(src._posz[i]);
			dst._r.push_back(src._r[i]); }
		else {
			culled++; }}

	if (0) {
		std::cout << "culled " << culled << " balls." << std::endl; }}


struct cell_job {
	// uniform
	GlPipeSet& pipes;
	const float delta;
	const soaballs& balls;

	// shared, writable
	float* const slice_intensity;
	float* const slice_distance;

	// local
	vec3 p;
	float target;
	int grid_size;

	void run(const unsigned tid) {
		thread_local SoaVertexBuffer vbo;
		vbo.clear();

		auto& pipe = pipes.get();

		pipe.glMaterial(103);
		pipe.glEnable(GL_CULL_FACE);
		pipe.glCullFace(GL_BACK);

		const int zstride = grid_size * grid_size;
		const int ystride = grid_size;

		const float sz = p.z;
		float sy = p.y;
		for (int y = 0; y < grid_size - 2; y++, sy -= delta) {
			if (y == 0) continue;

			float sx = p.x;
			for (int x = 0; x < grid_size - 2; x++, sx += delta) {
				if (x == 0) continue;

				const float * const bpi = slice_intensity + (y*grid_size) + x;
				const float * const bpd = slice_distance + (y*grid_size) + x;

				if (bpd[0] > delta) continue;
				const vec3 pos{ sx, sy - delta, sz };

				marzo::cell cd;
				auto fetch = [&bpi, &zstride, &ystride](int x, int y, int z) {
					return bpi[(z*zstride) + (y*ystride) + x]; };

				static const std::array<int, 24> ofs = { {
						0,1,0, 1,1,0, 1,0,0, 0,0,0, 0,1,1, 1,1,1, 1,0,1, 0,0,1
					} };

				int out_idx = 0;
				for (int oi = 0; oi < 24; oi += 3) {
					int px = ofs[oi], py = ofs[oi + 1], pz = ofs[oi + 2];
					float intensity = fetch(px, py, pz);
					float dx = (fetch(px + 1, py, pz) - fetch(px - 1, py, pz)) / delta;
					float dy = (fetch(px, py - 1, pz) - fetch(px, py + 1, pz)) / delta;
					float dz = (fetch(px, py, pz + 1) - fetch(px, py, pz - 1)) / delta;
					cd.value[out_idx] = intensity;
					cd.normal[out_idx] = vec3{ dx,dy,dz };
					out_idx++; }

				marzo::march_cell(vbo, pos, delta, cd, marzo::greater, target); }}

		vbo.pad();
		pipe.glDrawElements(vbo); }

	static void job(jobsys::Job* job, const unsigned tid, cell_job **self) {
		(**self).run(tid); }};




struct slice_job {
	// uniform
	GlPipeSet& pipes;
	const float delta;
	int grid_size;
	vec3 origin;
	soaballs& balls;

	// shared, writable
	float *grid_intensity;
	float *grid_distance;
	a64::vector<cell_job>& cell_jobs;
	unsigned char* slice_complete_flags;

	// local
	int back;
	int front;

	void _fill_slice(
		float sx, float sy, const float sz, const float delta,
		float * const slice_intensity, float * const slice_distance
		) {
		float save_x = sx;
		for (int iy = 0; iy < grid_size; iy++, sy -= delta) {
			sx = save_x;
			for (int ix = 0; ix < grid_size; ix++, sx += delta) {
				float intensity, distance;
				std::tie(distance, intensity) = balls.sample(sx, sy, sz);
				slice_intensity[iy*grid_size + ix] = intensity;
				slice_distance[iy*grid_size + ix] = distance;

				if (0) { //distance > delta) {
					// "raymarching"
					const int many = distance / delta;
					for (int skip = 1; skip < many; skip++) {
						ix++;
						if (ix >= grid_size) break;
						slice_intensity[iy*grid_size + ix] = 0; // magic;
						slice_distance[iy*grid_size + ix] = distance;
						sx += delta; }}}}}

	void run(jobsys::Job* self, const unsigned int tid) {
		const int slice_size = grid_size * grid_size;

		for (; back < front; back++) {
			vec3 so;
			so = origin;
			so += vec3{ 0,0, delta*back };
				//= origin + vec3{ 0, 0, delta * back };
			float *slice_intensity = &grid_intensity[back * slice_size];
			float *slice_distance = &grid_distance[back * slice_size];
			_fill_slice(so.x, so.y, so.z, delta, slice_intensity, slice_distance);

			std::lock_guard<std::mutex> guard(slice_complete_lock);
			slice_complete_flags[back] |= 1;
			for (int ci = 1; ci < grid_size - 2; ci++) {
				if ((slice_complete_flags[ci] & 2) > 0) continue;
				bool ready = true;
				for (int sub = -1; sub <= 2; sub++) {
					ready = ready && ((slice_complete_flags[ci + sub] & 1) > 0); }
				if (ready) {
					slice_complete_flags[ci] |= 2;
					vec3 blah;
					blah = origin;
					blah += vec3{ 0, -delta, ci*delta };
					cell_jobs.push_back(cell_job{
						pipes, delta, balls,
						&grid_intensity[ci * slice_size],
						&grid_distance[ci * slice_size],
						blah, //origin + vec3{0, -delta, ci*delta},
						magic,  // target
						grid_size });

					jobsys::Job *tmp = make_job_as_child(self->parent, cell_job::job, &cell_jobs.back());
					jobsys::run(tmp); }}}}

	static void job(jobsys::Job* job, const unsigned tid, slice_job **self) {
		(**self).run(job, tid); }};


struct block_job {
	// uniform
	GlPipeSet& pipes;
	soaballs& all_balls;

	// per block
	float delta;
	int cell_divisions;
	vec3 cell_origin;

	// private?
	soaballs balls;
	a64::vector<unsigned char> slice_complete_flags;
	a64::vector<float> grid_intensity;
	a64::vector<float> grid_distance;
	a64::vector<cell_job> cell_jobs;
	a64::vector<slice_job> slice_jobs;

	void run(jobsys::Job* self, const unsigned tid) {
		const int grid_size = cell_divisions + 3;

		// grid point one more delta top left back
		const vec3 grid_origin = cell_origin + (vec3{ -1, 1, -1 } *vec3{ delta });

		// find point in the center of our block
		const float hb = float(cell_divisions >> 1);  // half block
		const vec3 center_offset = vec3{ hb, -hb, hb } *delta;
		const vec3 block_center = cell_origin + center_offset;
		const float block_radius = delta * ((cell_divisions + 2.1f) * sqrt(3.0) / 2.0f);  // radius includes extra slice

		// gather balls that are within range
		balls.clear();
		gather_balls(block_radius, block_center, all_balls, balls);

		// reserve memory so nothing is realloc'd
		grid_intensity.reserve(grid_size * grid_size * grid_size);
		grid_distance.reserve(grid_size * grid_size * grid_size);
		cell_jobs.reserve(grid_size * grid_size);
		slice_jobs.reserve(grid_size);

		// reserve / clear flags
		slice_complete_flags.clear();
		for (int i = 0; i < grid_size; i++) {
			slice_complete_flags.push_back(0); }

		const int slices_per_task = std::min(grid_size / jobsys::thread_count / 2, 1);

		cell_jobs.clear();
		slice_jobs.clear();
		for (int back = 0; back < grid_size; back += slices_per_task) {

			int front = std::min(back + slices_per_task, grid_size);

			slice_jobs.push_back(slice_job{
				pipes, delta, grid_size, grid_origin, balls,
				grid_intensity.data(), grid_distance.data(), cell_jobs, slice_complete_flags.data(),
				back, front });

			jobsys::Job *tmp = jobsys::make_job_as_child(self->parent, slice_job::job, &slice_jobs.back());
			jobsys::run(tmp); }}

	static void job(jobsys::Job* job, const unsigned tid, block_job **self) {
		(**self).run(job, tid); }};


class FxBalls {
public:
	FxBalls(unsigned seed) {
		last_time = 0;
		prng.seed(seed); }

	void run(jobsys::Job* parent, const int balls_to_make, const int cell_divisions, double the_time, GlPipeSet& pipes) {

		const float delta = (ball_bound * 2) / cell_divisions;

		// top left back of top left back cell
		const vec3 cell_origin = vec3{ -ball_bound, ball_bound, -ball_bound };

		// generate more balls if needed
		if (balls_to_make < balls.size()) {
			balls.resize(balls_to_make); }
		for (auto i = balls.size(); i < balls_to_make; i++) {
			auto ball = make_ball();
			balls.append(ball.pos, ball.dir, ball.vel, ball.r); }

		// update balls
		if (last_time != 0) {
			balls.update(the_time - last_time); }
		last_time = the_time;

		static a64::vector<block_job> block_jobs;
		if (block_jobs.size() < 8) {
			for (int i = block_jobs.size(); i < 8; i++) {
				block_jobs.push_back(block_job{ pipes, balls }); }}

		int ji = 0;
		for (int by = 0; by < 2; by++) {
			for (int bz = 0; bz < 2; bz++) {
				for (int bx = 0; bx < 2; bx++) {

					const int half_cells = cell_divisions >> 1;
					const vec3 offset = vec3{ float(bx),float(by),float(bz) } *vec3{ float(half_cells) } *(vec3{ delta } *vec3{ 1,-1,1 });

					const vec3 block_origin = cell_origin + offset;

					block_job& job = block_jobs[ji++];
					job.delta = delta;
					job.cell_divisions = cell_divisions >> 1;
					job.cell_origin = block_origin;

					jobsys::Job *tmp = make_job_as_child(parent, block_job::job, &job);
					jobsys::run(tmp); }}}}

	ball make_ball() {
		std::uniform_real_distribution<float> posxyz(-ball_bound, ball_bound);
		std::uniform_real_distribution<float> two(-2, 2);
		std::uniform_real_distribution<float> one(0, 1);
		std::uniform_real_distribution<float> one_to_three(0.8, 1.2);
		std::uniform_int_distribution<int> color_choice(0, 4);

		vec3 pos{ posxyz(prng), posxyz(prng), posxyz(prng) };
		vec3 dir = normalize(vec3{ two(prng), two(prng), two(prng) });
		float vel = 0.3f + one(prng);
		float r = one_to_three(prng); // 0.5f;

//		return ball{ vec4{0,0,0,1}, vec4(0,0,0,0), 0.5, 2.0f};
		return ball{ pos, dir, vel, r }; }

private:
	double last_time;
	soaballs balls;
	std::minstd_rand prng; };

}
