#pragma once
#include <algorithm>
#include <random>
#include <tuple>

#include "cpu-colors.h"
#include "jobsys.h"
#include "render.h"
#include "marzo.h"
#include "vec.h"

namespace FxSdf {

const float mc_bound = 5.0f;
const int slices_per_task = 4;

static std::mutex slice_complete_lock;


struct surface {
	float tm;

	static float sdSphere(const vec3& pos, const float r) {
		return length(pos) - r; }

	float sample(float sx, float sy, float sz) const {
		vec3 pos{ sx, sy, sz };
		auto distort = 0.30 * sin(5.0*(sx + tm / 4.0))* sin(2.0*(sy + (tm / 1.33))); // *sin(50.0*sz);
		return sdSphere(pos, 3.0f) + (distort * sin(tm / 2.0) + 1.0f); }

	void update(float new_time) {
		tm = new_time; }};


struct cell_job {
	// uniform
	GlPipeSet& pipes;
	const float delta;

	// shared, writable
	float* const slice_distance;

	// local
	vec3 p;
	int grid_size;

	void run(const unsigned tid) {
		auto& pipe = pipes.get();
		
		pipe.glMaterial(103);
		pipe.glEnable(GL_CULL_FACE);
		pipe.glCullFace(GL_BACK);
		pipe.glBegin(GL_TRIANGLES);

		const int zstride = grid_size * grid_size;
		const int ystride = grid_size;

		const float sz = p.z;
		float sy = p.y;
		for (int y = 0; y < grid_size - 2; y++, sy -= delta) {
			if (y == 0) continue;

			float sx = p.x;
			for (int x = 0; x < grid_size - 2; x++, sx += delta) {
				if (x == 0) continue;

				float *bpd = slice_distance + (y*grid_size) + x;

//				if (bpd[0] > delta) continue;
				vec3 pos{ sx, sy - delta, sz };

				marzo::cell cd;
				auto fetch = [&bpd,&zstride,&ystride](int x, int y, int z) {
					return bpd[(z*zstride) + (y*ystride) + x]; };

				static const std::array<int, 24> ofs = { {
						0,1,0, 1,1,0, 1,0,0, 0,0,0, 0,1,1, 1,1,1, 1,0,1, 0,0,1
					} };

				int out_idx = 0;
				for (int oi = 0; oi < 24; oi+=3) {
					//int px = 0, py = 1, pz = 0;
					int px = ofs[oi], py = ofs[oi + 1], pz = ofs[oi + 2];
					float distance = fetch(px, py, pz);
					float dx = (fetch(px+1, py, pz) - fetch(px-1, py, pz)) / delta;
					float dy = (fetch(px, py-1, pz) - fetch(px, py+1, pz)) / delta;
					float dz = (fetch(px, py, pz+1) - fetch(px, py, pz-1)) / delta;
					cd.value[out_idx] = distance;
					cd.normal[out_idx] = vec3{ dx,dy,dz };
					out_idx++; }
				
				marzo::march_cell(pipe, pos, delta, cd, marzo::less, 0.0f); } }

		pipe.glEnd(); }

	static void job(jobsys::Job* job, const unsigned tid, cell_job **self) {
		(**self).run(tid); } };




struct slice_job {
	// uniform
	GlPipeSet& pipes;
	const float delta;
	int grid_size;
	vec3 origin;
	const surface& field;

	// shared, writable
	float *grid_distance;
	a64::vector<cell_job>& cell_jobs;
	unsigned char* slice_complete_flags;

	// local
	int back;
	int front;

	void _fill_slice(
		float sx, float sy, const float sz, const float delta,
		float * const slice_distance
		) {
		float save_x = sx;
		for (int iy = 0; iy < grid_size; iy++, sy -= delta) {
			sx = save_x;
			for (int ix = 0; ix < grid_size; ix++, sx += delta) {
				float distance = field.sample(sx, sy, sz);
				slice_distance[iy*grid_size + ix] = distance; }}}

/*				if (distance > delta) {
					// "raymarching"
					const int many = distance / delta;
					for (int skip = 1; skip < many; skip++) {
						ix++;
						if (ix >= grid_size) break;
						slice_distance[iy*grid_size + ix] = distance;
						sx += delta; }}}}}*/

	void run(jobsys::Job* self, const unsigned int tid) {
		const int slice_size = grid_size * grid_size;

		for (; back < front; back++) {
			vec3 so = origin + vec3{0, 0, delta * back};
			float *slice_distance = &grid_distance[back * slice_size];
			_fill_slice(so.x, so.y, so.z, delta, slice_distance);

			std::lock_guard<std::mutex> guard(slice_complete_lock);
			slice_complete_flags[back] |= 1;
			for (int ci = 1; ci < grid_size - 2; ci++) {
				if ((slice_complete_flags[ci] & 2) > 0) continue;
				bool ready = true;
				for (int sub = -1; sub <= 2; sub++) {
					ready = ready && ((slice_complete_flags[ci + sub] & 1) > 0); }
				if (ready) {
					slice_complete_flags[ci] |= 2;
					cell_jobs.push_back(cell_job{
						pipes, delta,
						&grid_distance[ci * slice_size],
						origin + vec3{0, -delta, ci*delta},
						grid_size});

					jobsys::Job *tmp = make_job_as_child(self->parent, cell_job::job, &cell_jobs.back());
					jobsys::run(tmp); }}}}

	static void job(jobsys::Job* job, const unsigned tid, slice_job **self) {
		(**self).run(job, tid); } };


struct block_job {
	// uniform
	GlPipeSet& pipes;
	const surface& field;

	// per block
	float delta;
	int cell_divisions;
	vec3 cell_origin;

	// private?
	a64::vector<unsigned char> slice_complete_flags;
	a64::vector<float> grid_distance;
	a64::vector<cell_job> cell_jobs;
	a64::vector<slice_job> slice_jobs;

	void run(jobsys::Job* self, const unsigned tid) {
		const int grid_size = cell_divisions + 3;

		// grid point one more delta top left back
		const vec3 grid_origin = cell_origin + (vec3{ -1, 1, -1 } * vec3{ delta });

		// reserve memory so nothing is realloc'd
		grid_distance.reserve(grid_size * grid_size * grid_size);
		cell_jobs.reserve(grid_size * grid_size);
		slice_jobs.reserve(grid_size);

		// reserve / clear flags
		slice_complete_flags.clear();
		for (int i=0; i<grid_size; i++) {
			slice_complete_flags.push_back(0); }

		const int slices_per_task = std::min(grid_size / jobsys::thread_count / 2, 1);

		cell_jobs.clear();
		slice_jobs.clear();
		for (int back = 0; back < grid_size; back += slices_per_task) {

			int front = std::min(back + slices_per_task, grid_size);

			slice_jobs.push_back(slice_job{
				pipes, delta, grid_size, grid_origin, field,
				grid_distance.data(), cell_jobs, slice_complete_flags.data(),
				back, front});

			jobsys::Job *tmp = jobsys::make_job_as_child(self->parent, slice_job::job, &slice_jobs.back());
			jobsys::run(tmp); }}

	static void job(jobsys::Job* job, const unsigned tid, block_job **self) {
		(**self).run(job, tid); } };


class FxSdf {
private:
	surface field;
public:
	FxSdf() {}

	void run(jobsys::Job* parent, const int cell_divisions, double the_time, GlPipeSet& pipes) {

		field.update(the_time);

		const float delta = (mc_bound * 2) / cell_divisions;

		// top left back of top left back cell
		const vec3 cell_origin = vec3{-mc_bound, mc_bound, -mc_bound};

		static a64::vector<block_job> block_jobs;
		if (block_jobs.size() < 8) {
			for (int i = block_jobs.size(); i < 8; i++) {
				block_jobs.push_back(block_job{pipes, field}); } }

		int ji = 0;
		for (int by=0; by<2; by++) {
			for (int bz=0; bz<2; bz++) {
				for (int bx=0; bx<2; bx++) {

					const int half_cells = cell_divisions >> 1;
					const vec3 offset = vec3{float(bx),float(by),float(bz)} * vec3{float(half_cells)} *(vec3{ delta } * vec3{ 1,-1,1 });

					const vec3 block_origin = cell_origin + offset;
			
					block_job& job = block_jobs[ji++];
					job.delta = delta;
					job.cell_divisions = cell_divisions >> 1;
					job.cell_origin = block_origin;

					jobsys::Job *tmp = make_job_as_child(parent, block_job::job, &job);
					jobsys::run(tmp);}}}} };

}
