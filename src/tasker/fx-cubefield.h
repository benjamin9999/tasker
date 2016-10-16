#pragma once
#include <random>

#include "cpu-colors.h"
#include "jobsys.h"
#include "render.h"
#include "marzo.h"
#include "vec.h"

namespace FxCubefield {

const auto tlf = vec3{ -1, 1,  1 };
const auto trf = vec3{ 1, 1,  1 };
const auto trb = vec3{ 1, 1, -1 };
const auto tlb = vec3{ -1, 1, -1 };
const auto blf = vec3{ -1,-1,  1 };
const auto brf = vec3{ 1,-1,  1 };
const auto brb = vec3{ 1,-1, -1 };
const auto blb = vec3{ -1,-1, -1 };


inline void draw_cube_with_strips(GlPipe& pipe) {
	const std::array<vec3, 14> lst{ {
		trb, tlb, trf, tlf, blf, tlb, blb,
		trb, brb, trf, brf, blf, brb, blb } };

	pipe.glBegin(GL_TRIANGLE_STRIP);
	for (const auto& vert : lst) {
		pipe.glVertex(vert); }
	pipe.glEnd(); }


inline void draw_cube_with_quads(GlPipe& pipe) {
	auto quad = [&pipe](const vec3& v1, const vec3& v2, const vec3& v3, const vec3& v4) {
		pipe.glVertex(v1);
		pipe.glVertex(v4);
		pipe.glVertex(v3);
		pipe.glVertex(v2); };

	pipe.glBegin(GL_QUADS);
	quad(tlf, trf, brf, blf);  // front
	quad(trb, tlb, blb, brb);  // back
	quad(tlb, trb, trf, tlf);  // top
	quad(blf, brf, brb, blb);  // bottom
	quad(tlb, tlf, blf, blb);  // left
	quad(trf, trb, brb, brf);  // right
	pipe.glEnd(); }


const int job_buckets = 32;

const int gran = 128;


struct Cube {
	vec3 rot;
	vec3 pos1, pos2;
	vec3 face_color;
	bool is_big; };


struct cube_job {
	double the_time;
	GlPipeSet& pipes;
	unsigned batch_size;
	Cube cube;

	void run(const unsigned int tid) {
		GlPipe& pipe = pipes.get();

		double extra_time = (cube.is_big ? 0.0 : 0);
		float stt = sin((the_time + extra_time) / 2.0f) * 0.5f + 0.5f;
		vec3 pos = lerp(cube.pos1, cube.pos2, stt);
		float sz = cube.is_big ? 0.3f : 0.1f;

		pipe.glColor(cube.face_color);
		pipe.glPushMatrix();
		pipe.glTranslate(pos);
		pipe.glRotate(the_time, cube.rot.x, cube.rot.y, cube.rot.z);
		pipe.glScale(sz, sz, sz);

		draw_cube_with_strips(pipe);
		//draw_cube_with_quads(pipe);

		pipe.glPopMatrix(); }

	static void job(jobsys::Job* job, const unsigned tid, void *rawptr) {
		auto cube_job_ptr = reinterpret_cast<cube_job**>(rawptr);
		auto cube_jobs = cube_job_ptr[0];
		auto& first = cube_jobs[0];
		auto batch_size = first.batch_size;  // first job has the batch-size
		auto& pipe = first.pipes.get();

		pipe.glMaterial(1);
		pipe.glPushMatrix();
		pipe.glEnable(GL_CULL_FACE);
		pipe.glCullFace(GL_BACK);
		for (unsigned i = 0; i < batch_size; i++) {
			cube_jobs[i].run(tid); }
		pipe.glPopMatrix(); }};


class Cubefield {
public:
	Cubefield(unsigned seed) {
		prng.seed(seed); }

	void run(jobsys::Job* parent, const int cubes_to_make, double the_time, GlPipeSet& pipes) {
		const int cubes_per_bucket = cubes_to_make / job_buckets;
		for (int bucket = 0; bucket < job_buckets; bucket++) {
			auto& cube_jobs = cube_job_buckets[bucket];

			jobsys::mark_start();

			// generate more cubes if needed
			for (auto i = cube_jobs.size(); i < cubes_per_bucket; i++) {
				auto cube = make_cube();
				cube_jobs.push_back(cube_job{ 0, pipes, 0, cube }); }

			// fill in per-frame values and queue jobs
			for (int i = 0; i < cubes_per_bucket; i+=gran) {
				int batch_size = 0;
				for (int p = 0; p < gran; p++) {
					if (i + p >= cubes_per_bucket) {
						break; }
					batch_size += 1;
					auto& job = cube_jobs[i+p];
					job.the_time = the_time; }

				auto& job = cube_jobs[i];
				job.batch_size = batch_size;  // store the batch size in the first job
				jobsys::Job *batch_job = jobsys::make_job_as_child(parent, cube_job::job, &job);
				jobsys::run(batch_job); }

			jobsys::mark_end(908467); }}

	Cube make_cube() {
		std::uniform_real_distribution<float> posx(-8, 8);
		std::uniform_real_distribution<float> posy(-6, 6);
		std::uniform_real_distribution<float> posz(-40, -4);
		std::uniform_real_distribution<float> two(-2, 2);
		std::uniform_real_distribution<float> one(0, 1);
		std::uniform_int_distribution<int> color_choice(0, 4);
		vec3 pos1{ posx(prng), posy(prng), posz(prng) };
		vec3 pos2{ posx(prng), posy(prng), posz(prng) };
		vec3 rot = normalize(vec3{ two(prng), two(prng), two(prng) });
		vec3 face_color;
		bool is_big;
		if (one(prng) >= 0.03) {
			face_color = vec3{ 0,0,0 };
			is_big = false; }
		else {
			face_color = vec3::from_rgb(cpu_colors[color_choice(prng)].integer) * vec3{3.0};
			is_big = true; }
		return Cube{rot, pos1, pos2, face_color, is_big}; }

private:
	a64::vector<cube_job> cube_job_buckets[job_buckets];
	std::minstd_rand prng; };

}
