#pragma once
#include <string>
#include <array>

#include "aligned-containers.h"
#include "render.h"
#include "vec.h"


struct Face {
	std::array<int, 3> point_idx;
	std::array<int, 3> texcoord_idx;
	std::array<int, 3> normal_idx;
	vec4 normal;
	int front_material;
//	int back_material;

	std::array<int, 3> edge_ids;  // edge_id for each edge
	std::array<int, 3> edge_faces;  // adjacent face_id for each edge

	/*
	soa point-in-face & adjacent-normals for shadow volumes
	vec4 nx,ny,nz;
	vec4 px,py,pz;
	*/
};


struct Mesh {
	vec4 bbox[8];

	a64::vector<vec4> points;
	a64::vector<vec4> normals;
	a64::vector<vec4> texcoords;

	a64::vector<vec4> vertex_normals;
	a64::vector<Face> faces;

	std::string name;

	bool solid;
	std::string message;

	void print() const;
	void compute_bbox();
	void compute_face_and_vertex_normals();
	void compute_edges(); };


/*inline bool almost_equal(const vbo_vertex& a, const vbo_vertex& b) {
	return (
		almost_equal(a.point, b.point) &&
		almost_equal(a.normal, b.normal) &&
		almost_equal(a.texcoord, b.texcoord)); }*/


void make_indexed_vertex_buffer(const Mesh& m, SoaVertexBuffer&, a64::vector<int>&);


class MeshStore {
public:
	const Mesh& get(const std::string& name) const {
		for (const auto& mesh : store) {
			if (mesh.name == name) {
				return mesh; }}
		throw std::exception("mesh not found"); }

	void print() const;
	void load_dir(const std::string& prepend, MaterialStore& materialstore, class TextureStore& texturestore);

private:
	a64::vector<Mesh> store; };
