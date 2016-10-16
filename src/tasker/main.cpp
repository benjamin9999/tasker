#include "stdafx.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include <fmt/format.h>
#include <PixelToaster.h>

#include "aligned-containers.h"
#include "display-mode.h"
#include "jobsys-vis.h"
#include "material.h"
#include "barrier.h"
#include "profont.h"
#include "ptframe.h"
#include "texture.h"
#include "canvas.h"
#include "jobsys.h"
#include "kawase.h"
#include "render.h"
#include "bench.h"
#include "utils.h"
#include "mesh.h"
#include "main.h"

#include "fx-cubefield.h"
#include "fx-balls.h"
#include "fx-sd.h"

using namespace std;
using namespace PixelToaster;

const int measurement_samples = 1100;
const int scan_samples = 120;
const ivec2 scan_limit{ 16, 16 };
const double measurement_discard = 0.05;  // discard worst 5% because of os noise


std::vector<DisplayMode> modelist = {
	{640, 360},
	{640, 480},
	{800, 600},
	{1280, 720},
	{1280, 800},
	{1360, 768},
	{1920, 1080},
	{1920, 1200},
	{320, 240},
};


struct render_job_uniform {
	const bool disable_render;
	GlPipeSet& pipes;
	QuadFloat4Canvas& colorcanvas;
	QuadFloatCanvas& depthcanvas;
	TrueColorCanvas& outcanvas;
	FloatingPointCanvas& smallcanvas;
	MaterialStore& mstore;
	TextureStore& tstore;
	bool show_tiles;
	int *cpu_bin_assignments;
	bool output_small;
	bool output_final;

	void run(const unsigned tid, const int bin_idx) {
		auto rect = pipes.get_bin_rect(bin_idx);

		//fill_canvas_rect(rect, 1.0f, depthcanvas);

		if (!disable_render) {
			auto bkg_cubefield = vec4{ 0.31f, 0.41f, 0.51f, 1.0f };
			auto bkg_balls_dark = vec4{ 0.03f, 0.02f, 0.02f, 1.0f };
			auto bkg_balls_dark2 = vec4{ 0.01f, 0.01f, 0.01f, 1.0f };
			fill_canvas_rect(rect, bkg_balls_dark2, colorcanvas);
			pipes.render_bin(1, bin_idx, mstore, tstore, depthcanvas, colorcanvas); }

		if (output_small) {
			copy_canvas_rect_downsample(rect, colorcanvas, smallcanvas); }
		if (output_final) {
			canvas_shader_rect(rect, IqShader(), colorcanvas, outcanvas); }
		if (show_tiles) {
			cpu_bin_assignments[bin_idx] = tid; }}};


struct render_job {
	int bin_idx;
	struct render_job_uniform* rju;

	void run(const unsigned tid) {
		rju->run(tid, bin_idx); }

	static void job(jobsys::Job* job, const unsigned tid, render_job *ptr) {
		ptr->run(tid); }};


void Application::run() {
	task_size = 6;
	tile_dim = ivec2{ 12, 4 };
	cubes_to_make = 25000;
	debug_mode = false;
	show_shader_threads = false;
	paused = false;
	disable_render = false;
	capture_mouse = false;
	vis_scale = 30;

	next_mode_idx = 0;
	cur_mode = DisplayMode{ 0, 0 };
	cur_fullscreen = false;
	fullscreen = false;
	change_mode = modelist[0];
	
	measuring = false;
	start_measuring = false;
	show_stats = false;
	show_mode_list = false;
	keys_shifted = false;
	scanning = false;
	stop_scanning = false;
	start_scanning = false;
	glow_effect = true;

	grid_size = 64;
	ball_count = 64;


	MaterialStore materialstore;
	TextureStore texturestore;
	MeshStore meshstore;

	texturestore.load_dir("data\\textures\\");
	meshstore.load_dir("data\\meshes\\", materialstore, texturestore);

	materialstore.print();

	auto& m = meshstore.get("rcube1.obj");
	SoaVertexBuffer rcube_vbo;
	a64::vector<int> rcube_idx;
	make_indexed_vertex_buffer(m, rcube_vbo, rcube_idx);

	auto& room = meshstore.get("map1.obj");
	SoaVertexBuffer map_vbo;
	a64::vector<int> map_idx;
	make_indexed_vertex_buffer(room, map_vbo, map_idx);

#define SAMPLE(src, a) (a = (a*0.9) + (src.delta() * 1000 * 0.1))

	PixelToaster::Timer gt;
	PixelToaster::Timer frametimer;
	PixelToaster::Timer frametimer2;
	double ax_frame = 0.0;
	double ax_render = 0.0;

	ProPrinter pp;

	a64::vector<qfloat4> colorbuffer;
	a64::vector<qfloat> depthbuffer;
	a64::vector<FloatingPointPixel> smallbuffer1;
	a64::vector<FloatingPointPixel> smallbuffer2;

	GlPipeSet pipes(jobsys::thread_count);

	FxCubefield::Cubefield fx_cubefield(0x123456);
	FxBalls::FxBalls fx_balls(0x123456);
	FxSdf::FxSdf fx_sdf;

	struct binstat {
		int bin_id;
		int total_size;
	};
	vector<binstat> binstats;
	vector<int> cpu_bin_assignment;
	int frame_counter{ 0 };

	try {
		while (true) {

			if (cur_mode != change_mode || cur_fullscreen != fullscreen) {
				cur_mode = change_mode;
				cur_fullscreen = fullscreen;
				display.close();
				display.open("rqdq 2016", cur_mode.width, cur_mode.height, cur_fullscreen ? Output::Fullscreen : Output::Windowed, Mode::TrueColor);
				display.listener(this); }

			const int total_pixels = cur_mode.width * cur_mode.height;
			colorbuffer.reserve(total_pixels >> 2);

			depthbuffer.reserve(total_pixels >> 2);

			smallbuffer1.reserve(total_pixels >> 2);
			smallbuffer2.reserve(total_pixels >> 2);

			auto depthcanvas = QuadFloatCanvas(depthbuffer.data(), cur_mode.width, cur_mode.height);
			auto colorcanvas = QuadFloat4Canvas(colorbuffer.data(), cur_mode.width, cur_mode.height);
			auto smallcanvas1 = FloatingPointCanvas(smallbuffer1.data(), cur_mode.width / 2, cur_mode.height / 2);
			auto smallcanvas2 = FloatingPointCanvas(smallbuffer2.data(), cur_mode.width / 2, cur_mode.height / 2);

			auto frame = Frame(display);

			frametimer.reset();
			frametimer2.reset();

			auto canvas = frame.canvas();
			float the_time;
			if (paused) {
				the_time = 1.333f; }
			else {
				the_time = float(gt.time()); }


			jobsys::Job *root_job = jobsys::make_job(&jobsys::noop);
			jobsys::work_start();
			jobsys::reset();

			if (capture_mouse && reset_mouse_next_frame) {
				reset_mouse_next_frame = false;
				display.center_mouse(); }


			jobsys::mark_start();

			pipes.reset(ivec2{ canvas.width, canvas.height }, tile_dim);
			pipes.glMatrixMode(GL_PROJECTION);
			pipes.glLoadMatrix(make_gluPerspective(hc.fov, canvas.aspect, 1, 100));
			pipes.glMatrixMode(GL_MODELVIEW);

			pipes.glLoadMatrix(hc.get_matrix());
//			pipes.glTranslate(0, 0, -hc.pos.z);

			cpu_bin_assignment.reserve(pipes.get_bin_count());

			jobsys::mark_end(1487);

			if (0) {
				pipes.glUseProgram(0);
				jobsys::Job *spawn = jobsys::make_job_as_child(root_job, jobsys::noop);
				fx_cubefield.run(spawn, cubes_to_make, the_time, pipes);
				jobsys::run(spawn);
				jobsys::wait(spawn); }

			if (1) {
				pipes.glUseProgram(1);
				pipes.glPushMatrix();
//				pipes.glTranslate(0, 5, 0);
				jobsys::Job *spawn = jobsys::make_job_as_child(root_job, jobsys::noop);
				fx_balls.run(spawn, ball_count, grid_size, the_time, pipes);
				jobsys::run(spawn);
				jobsys::wait(spawn);
				pipes.glPopMatrix();
			}

			if (0) {
				pipes.glUseProgram(1);
				jobsys::Job *spawn = jobsys::make_job_as_child(root_job, jobsys::noop);
				fx_sdf.run(spawn, grid_size, the_time, pipes);
				jobsys::run(spawn);
				jobsys::wait(spawn); }

			if (1) {
				pipes.glUseProgram(0);
				auto& p = pipes.get();
				p.glEnable(GL_CULL_FACE);
				p.glColor(0, 1, 0);
				p.glMaterial(1);
				p.glBegin(GL_QUADS);
				p.glVertex({ 0.5, 0, 0 });
				p.glVertex({ 0.5, 0, -5 });
				p.glVertex({ -0.5, 0, -5 });
				p.glVertex({ -0.5, 0, 0 });
				p.glEnd();
			}

			if (0) {
				pipes.glUseProgram(0);
				auto& p = pipes.get();
				p.glEnable(GL_CULL_FACE);
				p.glColor(0.6, 0.6, 0);
				p.glMaterial(1);
				p.glBegin(GL_QUADS);
				p.glVertex({ 100, 0, 100 });
				p.glVertex({ 100, 0, -100 });
				p.glVertex({ -100, 0, -100 });
				p.glVertex({ -100, 0, 100 });
				p.glEnd();
			}

			if (1) {
				pipes.glUseProgram(0);
				auto& p = pipes.get();
				p.glEnable(GL_CULL_FACE);
				p.glPushMatrix();
				p.glTranslate(0, -7, 0);
				p.glDrawElements(map_vbo, map_idx, true);
				p.glPopMatrix();
			}


			struct thing_job {
				static void job(jobsys::Job* job, const unsigned tid, thing_job *self) {
					self->run(tid); }

				GlPipeSet& pipes;
				const float the_time;
				SoaVertexBuffer& rcube_vbo;
				const a64::vector<int>& rcube_idx;
				const int imod;
				const int ival;

				void run(const unsigned tid) {
					auto& pipe = pipes.get();
					pipe.glUseProgram(1);
					pipe.glEnable(GL_CULL_FACE);
					vec3 dir = normalize(vec3{ 0, 0, 1 });
					pipe.glPushMatrix();
					pipe.glTranslate(-6 * 12/4.0, -6, -6 * 12/4.0);
					int instance_id = 0;
					for (int z = 5; z >= -5; z--) {
						pipe.glTranslate(0, 0, 12/4.0);
						pipe.glPushMatrix();
						for (int x = -5; x <= 5; x++) {
							pipe.glTranslate(12/4.0, 0, 0);
							pipe.glPushMatrix();
//							pipe.glRotate(the_time / 2.0f + (z/2.0) + (x/10.0f), dir.x, dir.y, dir.z);
							pipe.glMaterial(103);
							pipe.glColor(vec3{ 1,1,1 });
							pipe.glPushMatrix();
							pipe.glScale(1.0/4, 0.20/4, 1.00/4);
							if (instance_id % imod == ival) {
								pipe.glDrawElements(rcube_vbo, rcube_idx, false);
							}
							pipe.glPopMatrix();
/*							pipe.glMaterial(4);
							pipe.glTranslate(0, -1.0/8, 0);
							pipe.glColor(vec4{ 1,1,1,1 });
							pipe.glScale(1.0/4, 0.20/4, 1.00/4);
							if (instance_id % imod == ival) {
								pipe.glDrawElements(rcube_vbo, rcube_idx);
							}*/
							instance_id++;
							pipe.glPopMatrix(); }
						pipe.glPopMatrix(); }
					pipe.glPopMatrix(); }
			};
			if (1) {
				const int subdiv = 16;
//				auto& m = meshstore.get("rcube1.obj");
				jobsys::Job *spawn = jobsys::make_job_as_child(root_job, jobsys::noop, nullptr);
				for (int i = 0; i < subdiv; i++) { // jobsys::thread_count; i++) {
					jobsys::Job *tmp = jobsys::make_job_as_child(spawn, thing_job::job,
						thing_job{ pipes, the_time, rcube_vbo, rcube_idx, subdiv, i });
					jobsys::run(tmp);
				}
				jobsys::run(spawn);
				jobsys::wait(spawn); }


			jobsys::mark_start();
			binstats.clear();
			const int bincnt = pipes.get_bin_count();
			for (int bi = 0; bi < bincnt; bi++) {
				int ax = 0;
				ax += pipes.get_bin_size(bi);
				binstats.push_back(binstat{ bi, ax }); }
			std::sort(binstats.begin(),
				binstats.end(),
				[](const binstat& a, const binstat& b) {return a.total_size > b.total_size; });
			jobsys::mark_end(5782589);


			jobsys::mark_start();
			render_job_uniform rju{
				disable_render,
				pipes,
				colorcanvas,
				depthcanvas,
				canvas,
				smallcanvas1,
				materialstore,
				texturestore,
				show_shader_threads,
				cpu_bin_assignment.data(),
				glow_effect,
				!glow_effect };
			for (int _bi = 1; _bi < bincnt; _bi++) {
				int bi = binstats[_bi].bin_id;
				jobsys::Job* job = jobsys::make_job_as_child(root_job, render_job::job, render_job{
					bi, &rju });
				jobsys::run(job); }
			{
				int _bi = 0;
				int bi = binstats[_bi].bin_id;
				jobsys::Job* job = jobsys::make_job_as_child(root_job, render_job::job, render_job{
					bi, &rju });
				jobsys::run(job); }
			jobsys::mark_end(3927672);


			jobsys::run(root_job);
			jobsys::wait(root_job);

			if (glow_effect) {
				struct blur_job {
					FloatingPointCanvas& in;
					FloatingPointCanvas& out;
					int y0, y1, dist;
					void run() {
						blur_kawase(y0, y1, dist, in, out); }
					static void job(jobsys::Job* self, const int tid, blur_job *ptr) {
						ptr->run(); } };

				FloatingPointCanvas* blur_src = &smallcanvas1;
				FloatingPointCanvas* blur_dst = &smallcanvas2;
				std::vector<int> passes = {
					1, 2, 3
				};
				const int blur_task_lines = 8;
				for (const auto &dist : passes) {
					jobsys::Job *blurp = jobsys::make_job(jobsys::noop);
					for (int y = 0; y < smallcanvas1.height; y += blur_task_lines) {
						int top = y;
						int bot = min(y + blur_task_lines, smallcanvas1.height);
						jobsys::Job* job = jobsys::make_job_as_child(blurp, blur_job::job, blur_job{ *blur_src, *blur_dst, top, bot, dist });
						jobsys::run(job); }
					jobsys::run(blurp);
					jobsys::wait(blurp);
					std::swap(blur_src, blur_dst); }
				// now blur_src points to the finished blur

				struct post_job {
					QuadFloat4Canvas& in_color;
					FloatingPointCanvas& in_blur;
					TrueColorCanvas& out;
					int y0, y1;
					void run() {
						canvas_shader_rows(y0, y1, GlowShader(), in_color, in_blur, out); }
						//canvas_shader_rows(y0, y1, DofShader(), in_color, in_blur, out); }
					static void job(jobsys::Job* self, const int tid, post_job *ptr) {
						ptr->run(); } };
				jobsys::Job *postp = jobsys::make_job(jobsys::noop);
				for (int y = 0; y < canvas.height; y += 8) {
					int top = y;
					int bot = min(y + 8, canvas.height);
					jobsys::Job* job = jobsys::make_job_as_child(postp, post_job::job, post_job{ colorcanvas, *blur_src, canvas, top, bot });
					jobsys::run(job); }
				jobsys::run(postp);
				jobsys::wait(postp);
			} // if glow_effect


			jobsys::work_end();
			SAMPLE(frametimer, ax_frame);
			SAMPLE(gt, ax_render);

			if (show_shader_threads) {
				for (int i = 0; i < pipes.get_bin_count(); i++) {
					auto rect = pipes.get_bin_rect(i);
					draw_border(rect, cpu_colors[cpu_bin_assignment[i]], canvas); } }

			if (show_mode_list) {
				int idx = 1;
				int top = canvas.height / 2.0;
				for (const auto& mode : modelist) {
					stringstream ss;
					ss << idx << ": " << mode.width << "x" << mode.height;
					if (cur_mode == mode) {
						ss << " (current)"; }
					pp.write(ss.str(), 16, top, canvas);
					top += 10;
					idx += 1; } }

			if (start_measuring) {
				measuring = true;
				start_measuring = false;
				show_stats = false;
				times.clear(); }

			if (measuring) {
				if (times.size() == measurement_samples) {
					measuring = false;
					last_stats = calc_stat(times, measurement_discard);
					show_stats = true; }
				else {
					times.push_back(frametimer2.delta() * 1000.0);
					stringstream ss;
					ss << "measuring, " << times.size() << " / " << measurement_samples;
					auto canvas = frame.canvas();
					pp.write(ss.str(), 16, 100, canvas); } }

			if (start_scanning) {
				scanning = true;
				start_scanning = false;
				tile_dim = ivec2{ 2, 2 };
				scan_min_dim = ivec2{ 2, 2 };
				scan_min_value = 10000.0;
				times.clear(); }

			if (stop_scanning) {
				stop_scanning = false;
				scanning = false;
				tile_dim = scan_min_dim; }

			if (scanning) {
				{
					int top = 100;
					stringstream ss;
					auto canvas = frame.canvas();
					ss << "probing for fastest tile dimensions: " << (((tile_dim.y - 1) * 16) + tile_dim.x - 1) << " / " << (16 * 16) << "   ";
					pp.write(ss.str(), 16, top, canvas);  top += 10;
					ss.str("");
					ss << "                     fastest so far: " << scan_min_dim.x << "x" << scan_min_dim.y << "   ";
					pp.write(ss.str(), 16, top, canvas);  top += 10;
					ss.str("");
					ss << "          press s to stop            ";
					pp.write(ss.str(), 16, top, canvas);  top += 10;
				}
				if (times.size() == scan_samples) {
					last_stats = calc_stat(std::vector<double>(times.begin() + 60, times.end()), measurement_discard);
					times.clear();
					if (last_stats._mean < scan_min_value) {
						scan_min_value = last_stats._mean;
						scan_min_dim = tile_dim; }
					tile_dim.x += 2;
					if (tile_dim.x > scan_limit.x) {
						tile_dim.x = 2;
						tile_dim.y += 2; }
					if (tile_dim.y > scan_limit.y) {
						scanning = false;
						tile_dim = scan_min_dim; } }
				else {
					times.push_back(frametimer2.delta() * 1000.0); } }

			if (debug_mode) {
				auto canvas = frame.canvas();
				double fps = 1.0 / ax_render * 1000;
				auto s = fmt::sprintf("% 6.2f ms, fps: %.0f", ax_frame, fps);
				pp.write(s, 16, 16, canvas); }

			if (debug_mode) {
				auto canvas = frame.canvas();
				stringstream ss;
				ss << "tile size: " << tile_dim.x << "x" << tile_dim.y << ", balls: " << ball_count << ", grid: " << grid_size;
				ss << ", ";
				ss << "visu scale: " << vis_scale;
				if (paused) {
					ss << "   PAUSED"; }
				if (disable_render) {
					ss << "   NORENDER"; }
				pp.write(ss.str(), 16, 27, canvas); }

			//if (debug_mode) {
			//    pp.write(std::string("+"), mouse_pos_x, mouse_pos_y, frame.canvas()); }

			if (debug_mode && show_stats) {
				auto dst = frame.canvas();
				int top = dst.height / 2;
				pp.write("   min    25th     med    75th     max    mean    sdev", 32, top, dst);
				top += 10;
				fmt::MemoryWriter out;
				out.write("{: 6.2f}  ", last_stats._min);
				out.write("{: 6.2f}  ", last_stats._25th);
				out.write("{: 6.2f}  ", last_stats._med);
				out.write("{: 6.2f}  ", last_stats._75th);
				out.write("{: 6.2f}  ", last_stats._max);
				out.write("{: 6.2f}  ", last_stats._mean);
				out.write("{: 6.2f}  ", last_stats._sdev);
				pp.write(out.str(), 32, top, dst);
				top += 10;
				pp.write(string("press C to clear"), 32, top, dst); }

			if (debug_mode) {
				auto canvas = frame.canvas();
				stringstream ss;
				ss << "F1 debug        -&+ ball count    [&] tile size    ,&. vis scale       ";
				pp.write(ss.str(), 16, -32, canvas);
				ss.str("");
				ss << " p toggle pause  m  change mode    r  measure       n  wasd capture    ";
				pp.write(ss.str(), 16, -42, canvas);
				ss.str("");
				ss << " f fullscreen   F2  show tiles     g toggle post    shift -&+ grid size";
				pp.write(ss.str(), 16, -52, canvas); }
			else if (frame_counter < (5*60)) {
				auto canvas = frame.canvas();
				stringstream ss;
				int top = canvas.height - 11;
				ss << "F1 debug";
				pp.write(ss.str(), 0, top, canvas); }

			if (debug_mode) {
				auto canvas = frame.canvas();
				jobsys::render(20, 40, float(vis_scale), canvas); } 

			frame_counter++; }}

	catch (WindowClosed &e) {
		_unused(e);
		cout << "window was closed" << endl; }

	catch (WrongFormat &e) {
		_unused(e);
		cout << "framebuffer not rgba8888" << endl; }

	cout << "terminated." << endl; }


bool Application::defaultKeyHandlers() {
	return false; }


void Application::onKeyPressed(DisplayInterface& display, Key key) {
	switch (key) {
	case Key::OpenBracket:
		task_size = max(4, task_size - 1);
		tile_dim = ivec2{ task_size, task_size };
		break;
	case Key::CloseBracket:
		task_size = min(task_size + 1, 128);
		tile_dim = ivec2{ task_size, task_size };
		break;
	case Key::Separator:
		//cubes_to_make = max(10, int(cubes_to_make - (cubes_to_make * 0.10f)));
		if (keys_shifted) {
			grid_size = max(10, grid_size - 1); }
		else {
			ball_count = max(8, ball_count - 4); }
		break;
	case Key::Equals:
		//cubes_to_make = min(100000, int(cubes_to_make + (cubes_to_make * 0.10f)));
		if (keys_shifted) {
			grid_size = min(256, grid_size + 1); }
		else {
			ball_count = min(2048, ball_count + 4); }
		break;
	case Key::F1:
		debug_mode = !debug_mode;
		break;
	case Key::F2:
		show_shader_threads = !show_shader_threads;
		break;
	case Key::P:
		if (keys_shifted) {
			disable_render = !disable_render; }
		else {
			paused = !paused; }
		break;
	case Key::G:
		glow_effect = !glow_effect;
		break;
	case Key::W: hc.forward(); break;
	case Key::S: hc.backward(); break;
	case Key::A: hc.left(); break;
	case Key::D: hc.right(); break;
	case Key::E: hc.raise();  break;
	case Key::Q: hc.lower();  break;
	case Key::Period:
		vis_scale += max(vis_scale / 10, 1);
		break;
	case Key::Comma:
		vis_scale = max(1, vis_scale - (vis_scale / 10));
		break;
	case Key::R:
		start_measuring = true;
		break;
	case Key::C:
		show_stats = false;
		break;
	case Key::F:
		fullscreen = !fullscreen;
		break;
	case Key::N:
		capture_mouse = !capture_mouse;
		if (capture_mouse) {
			display.capture_mouse(); }
		else {
			display.uncapture_mouse(); }
		break;
/*
	case Key::S:
		if (!measuring && !scanning) {
			start_scanning = true; }
		else if (scanning) {
			stop_scanning = true; }
		break;
*/
	default:
//		cout << "key was " << key << endl;
		if (show_mode_list && (key >= Key::One && key < (Key::One + modelist.size()))) {
			int modeidx = key - Key::One;
			change_mode = modelist[modeidx]; }}}


void Application::onKeyDown(DisplayInterface& display, Key key) {
	switch (key) {
	case Key::M:
		show_mode_list = true;
		break;
	case Key::Shift:
		keys_shifted = true;
		break;
	}}


void Application::onKeyUp(DisplayInterface& display, Key key) {
	switch (key) {
	case Key::M:
		show_mode_list = false;
		break;
	case Key::Shift:
		keys_shifted = false;
		break;
	}}


void Application::onMouseMove(DisplayInterface& display, Mouse mouse) {
	mouse_pos_x = mouse.x;
	mouse_pos_y = mouse.y;
	float dmx = (display.width() / 2) - mouse.x;
	float dmy = (display.height() / 2) - mouse.y;
	if (capture_mouse) {
		//cout << "mm(" << dmx << ", " << dmy << ")" << endl;
		hc.update(dmx, dmy);
		reset_mouse_next_frame = true;
	}
}

void Application::onMouseWheel(DisplayInterface& display, Mouse mouse, const short wheel_amount) {
	int ticks = wheel_amount / 120;
	hc.zoom(ticks);
}
