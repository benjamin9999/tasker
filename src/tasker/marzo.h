#pragma once
#include "render.h"
#include "vec.h"

namespace marzo {

extern const std::array<vec3, 8> vertex_offset;
extern const std::array<vec3, 12> edge_direction;
extern const std::array<short, 256> cube_edge_flags;
extern const std::array<std::array<char, 2>, 12> edge_connection;
extern const std::array<std::array<char, 16>, 256> tritable;


struct cell {
	/*marching cube cell data

	order must match vertex_offset table:

	ccw, starting from left/bottom/back:
	left/bottom/back, right/bottom/back, right/top/back, left/top/back

	then again for the front.
	*/
	float value[8];
	vec3 normal[8]; };


/*comparitors for surface detection*/
static const std::less<float> less;
static const std::greater<float> greater;


inline float calc_offset(const float a, const float b, const float target) {
	if (1) {
		return (target - a) / (b - a); }
	else {
		double delta = b - a;
		if (delta == 0.0) {
			return 0.5f; }
		else {
			return (target - a) / delta; }}}


template <typename _cmp>
void march_cell(
	GlPipe& pipe,
	const vec3& pos,
	const float delta,
	const cell& cd,
	const _cmp cmp,
	const float target
) {
	/*march one cell, output using glBegin api

	Args:
		pipe: GlPipe context, ready to receive glVertex commands
		pos: position of lower, back, left point of the cell data
		delta: distance between grid points
		cd: cube/cell data low/left/back ccw, then low/left/front ccw
		cmp: surface detection comparison functor
		target: surface detection comparison threshold
	*/
	vec3 edge_vertex[12];
	vec3 edge_normal[12];

	const vec3 vdelta{ delta };

	int flag_index = 0;
	for (int iv = 0; iv<8; iv++) {
		if (cmp(cd.value[iv], target)) {
			flag_index |= 1 << iv; }}

	auto edge_flags = cube_edge_flags[flag_index];

	if (!edge_flags) return;

	for (int edge = 0; edge<12; edge++) {
		if (edge_flags & (1 << edge)) {
			auto offset = calc_offset(
				cd.value[edge_connection[edge][0]],
				cd.value[edge_connection[edge][1]],
				target
			);
			edge_vertex[edge] = pos + ((vertex_offset[edge_connection[edge][0]] + vec3{offset} * edge_direction[edge]) * vdelta);
			edge_normal[edge] = normalize(lerp(cd.normal[edge_connection[edge][0]], cd.normal[edge_connection[edge][1]], offset)); } }

	for (int tri = 0; tri<5; tri++) {
		if (tritable[flag_index][3 * tri] < 0) {
			break; } // end of the list
		for (int corner = 2; corner >= 0; corner--) {
			auto vertex = tritable[flag_index][3 * tri + corner];
			pipe.glNormal(edge_normal[vertex]);
			pipe.glVertex(edge_vertex[vertex]); }}}


template <typename _cmp>
void march_cell(
	SoaVertexBuffer& vbo,
	const vec3& pos,
	const float delta,
	const cell& cd,
	const _cmp cmp,
	const float target
) {
	/*march one cell, append triangles to SoA vertex buffer

	Args:
		vbo: SoAVertexBuffer that is ready to append triangle data
		pos: position of lower, back, left point of the cell data
		delta: distance between grid points
		cd: cube/cell data low/left/back ccw, then low/left/front ccw
		cmp: surface detection comparison functor
		target: surface detection comparison threshold
	*/
	vec3 edge_vertex[12];
	vec3 edge_normal[12];

	const vec3 vdelta{ delta, delta, delta };

	int flag_index = 0;
	for (int iv = 0; iv<8; iv++) {
		if (cmp(cd.value[iv], target)) {
			flag_index |= 1 << iv; }}

	auto edge_flags = cube_edge_flags[flag_index];

	if (!edge_flags) return;

	for (int edge = 0; edge<12; edge++) {
		if (edge_flags & (1 << edge)) {
			auto offset = calc_offset(
				cd.value[edge_connection[edge][0]],
				cd.value[edge_connection[edge][1]],
				target
			);
			edge_vertex[edge] = pos + ((vertex_offset[edge_connection[edge][0]] + vec3{ offset } * edge_direction[edge]) * vdelta);
			edge_normal[edge] = normalize(lerp(cd.normal[edge_connection[edge][0]], cd.normal[edge_connection[edge][1]], offset)); } }

	for (int tri = 0; tri < 5; tri++) {
		if (tritable[flag_index][3 * tri] < 0) {
			break; } // end of the list
		for (int corner = 2; corner >= 0; corner--) {
			auto vertex = tritable[flag_index][3 * tri + corner];
			vbo.append(edge_vertex[vertex], edge_normal[vertex]); }}}

}