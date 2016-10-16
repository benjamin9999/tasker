/*
fps-style camera, as described in
http://www.opengl-tutorial.org/beginners-tutorials/tutorial-6-keyboard-and-mouse/
*/
#pragma once
#include "../pixeltoaster/PixelToaster.h"

#include "mat4.h"
#include "vec.h"


class HandyCam {
public:
	HandyCam() {
		pos = vec4{ 0,0,5,1 };
		ha = 3.14f;
		va = 0.0f;
		fov = 45.0f;
		speed = 3.0f;
		mouse_speed = 0.010f; }

	void update(const float dmx, const float dmy) {
		auto dt = 1.0f; // tm.delta();
		ha += mouse_speed * dt * dmx;
		va += mouse_speed * dt * dmy;

		// limit vertical range
		if (va > M_PI / 2.0f) {
			va = M_PI / 2.0f; }
		else if (va < -(M_PI / 2.0f)) {
			va = -(M_PI / 2.0f); }}

	void zoom(const int ticks) {
		fov += float(ticks); 
		std::cout << "fov(" << fov << ")" << std::endl; }

	vec4 make_dir() {
		return vec4{
			cos(va) * sin(ha),
			sin(va),
			cos(va) * cos(ha), 0 }; }

	vec4 make_right() {
		return vec4{
			sin(ha - 3.14f / 2.0f),
			0,
			cos(ha - 3.14f / 2.0f), 0 }; }

	mat4 get_matrix() {
		vec4 dir = make_dir();
		vec4 right = make_right();
		vec4 up = cross(right, dir);
		return look_at(pos, pos + dir, up); }

	void forward() {
		pos += make_dir(); }
	void backward() {
		pos -= make_dir(); }
	void right() {
		pos += make_right(); }
	void left() {
		pos -= make_right(); }
	void raise() {
		pos += vec4{ 0, 0.1f, 0, 0 }; }
	void lower() {
		pos -= vec4{ 0, 0.1f, 0, 0 }; }

public:
	vec4 pos;
	vec4 dir;
	float fov;
private:
	float ha, va;
	float speed;
	float mouse_speed;
	PixelToaster::Timer tm; };
