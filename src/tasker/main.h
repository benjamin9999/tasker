#pragma once
#include <PixelToaster.h>

#include "display-mode.h"
#include "camera.h"
#include "bench.h"
#include "vec.h"


class Application : public PixelToaster::Listener {
public:
	void run();
	void set_dimensions(const int width, const int height) {
		frame_width = width;
		frame_height = height;
	}
private:
	bool defaultKeyHandlers();
	void onKeyPressed(PixelToaster::DisplayInterface& display, PixelToaster::Key key);
	void onKeyDown(PixelToaster::DisplayInterface& display, PixelToaster::Key key);
	void onKeyUp(PixelToaster::DisplayInterface& display, PixelToaster::Key key);
//	void onMouseButtonDown(PixelToaster::DisplayInterface & display, PixelToaster::Mouse mouse);
	void onMouseMove(PixelToaster::DisplayInterface & display, PixelToaster::Mouse mouse);
	void onMouseWheel(PixelToaster::DisplayInterface & display, PixelToaster::Mouse mouse, short wheel_amount);

	PixelToaster::Display display;

	int task_size;
	ivec2 tile_dim;
	int cubes_to_make;
	int vis_scale;
	bool debug_mode;
	bool show_shader_threads;
	bool paused;
	bool disable_render;
	bool capture_mouse;
	bool reset_mouse_next_frame = false;

	int frame_width, frame_height;

	int next_mode_idx;
	int mouse_pos_x, mouse_pos_y;

	bool measuring, start_measuring;
	bool scanning, start_scanning, stop_scanning;
	double scan_min_value;
	ivec2 scan_min_dim;

	std::vector<double> times;
	BenchStat last_stats;
	bool show_stats;
	DisplayMode cur_mode, change_mode;
	bool fullscreen;
	bool cur_fullscreen;
	bool show_mode_list;
	bool glow_effect;
	int grid_size;
	int ball_count;
	bool keys_shifted;

	HandyCam hc;
};
