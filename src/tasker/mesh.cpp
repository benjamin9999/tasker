#include "stdafx.h"
#include <unordered_map>
#include <iostream>
#include <string>
#include <map>

#include "material.h"
#include "texture.h"
#include "utils.h"
#include "mesh.h"
#include "obj.h"
#include "vec.h"

using namespace std;


void Mesh::print() const {
	cout << "mesh[" << this->name << "]:" << endl;
	cout << "  vert(" << this->points.size() << "), normal(" << this->normals.size() << "), uv(" << this->texcoords.size() << ")" << endl;
	cout << "  faces(" << this->faces.size() << ")" << endl; }


void Mesh::compute_bbox() {

	vec4 pmin = this->points[0];
	vec4 pmax = pmin;
	for (auto& item : this->points) {
		pmin = vmin(pmin, item);
		pmax = vmax(pmax, item); }

	bbox[0] = { pmin.x, pmin.y, pmin.z, 1 };
	bbox[1] = { pmin.x, pmin.y, pmax.z, 1 };
	bbox[2] = { pmin.x, pmax.y, pmin.z, 1 };
	bbox[3] = { pmin.x, pmax.y, pmax.z, 1 };
	bbox[4] = { pmax.x, pmin.y, pmin.z, 1 };
	bbox[5] = { pmax.x, pmin.y, pmax.z, 1 };
	bbox[6] = { pmax.x, pmax.y, pmin.z, 1 };
	bbox[7] = { pmax.x, pmax.y, pmax.z, 1 }; }


void Mesh::compute_face_and_vertex_normals() {

	vertex_normals.clear();
	for (int i = 0; i < points.size(); i++) {
		vertex_normals.push_back(vec4{ 0 }); }

	for (auto& face : faces) {
		vec4 v0v2 = points[face.point_idx[2]] - points[face.point_idx[0]];
		vec4 v0v1 = points[face.point_idx[1]] - points[face.point_idx[0]];
		vec4 dir = cross(v0v1, v0v2);
		face.normal = normalize(dir);

		vertex_normals[face.point_idx[0]] += dir;
		vertex_normals[face.point_idx[1]] += dir;
		vertex_normals[face.point_idx[2]] += dir; }

	for (auto& vn : vertex_normals) {
		vn = normalize(vn); }}


struct _edgedata {
	int face_id_a;
	int face_id_b;
	int edge_id;
};


unsigned make_edge_key(const unsigned vidx1, const unsigned vidx2) {
	if (vidx1 < vidx2)
		return vidx1<<16 | vidx2;
	else
		return vidx2<<16 | vidx1; }


void Mesh::compute_edges() {

	map<unsigned,_edgedata> edgemap;
	int next_id = 0;

	solid = true;

	/*
	 * pass 1
	 * create a table of unique edges, and the two faces that share them
	 */
	for (unsigned i=0; i<faces.size(); i++) {
		Face& face = faces[i];
		for (int ei=0; ei<3; ei++) {
			const unsigned edgekey = make_edge_key(face.point_idx[ei], face.point_idx[(ei+1)%3]);
			if (edgemap.count(edgekey) == 0) {
				// first time this edge has been seen
				int this_edge_id = next_id++;
				face.edge_ids[ei] = this_edge_id;
				edgemap[edgekey] = _edgedata{ int(i), -1, this_edge_id }; }
			else {
				// second time this edge has been seen
				if (edgemap[edgekey].face_id_b != -1) {
					// oops, we saw it more than twice
					solid = false;
					message = "edge over shared";
					return; }
				edgemap[edgekey].face_id_b = i;
				face.edge_ids[ei] = edgemap[edgekey].edge_id; }}}

	/*
	 * pass 2
	 * update each face to include the adjacent face for each edge
	 */
	for (unsigned i=0; i<faces.size(); i++) {
		Face& face = faces[i];
		for (int ei=0; ei<3; ei++) {
			const unsigned edgekey = make_edge_key(face.point_idx[ei], face.point_idx[(ei+1)%3]);
			auto& edgedata = edgemap[edgekey];
			if (edgedata.face_id_b == -1) {
				// oops, this edge has no adjacent face
				solid = false;
				message = "open edge";
				return; }

			// my neighbor is the face_id that isn't myself
			face.edge_faces[ei] = edgedata.face_id_a == i ? edgedata.face_id_b : edgedata.face_id_a; }}

	/*
	pass 3
	fill in SoA format point-in-face & face-normals for shadow-volumes
	for (auto& face : faces) {

		const auto& adj1 = faces[face.edge_faces[0]];
		const auto& adj2 = faces[face.edge_faces[1]];
		const auto& adj3 = faces[face.edge_faces[2]];

		// point-in-face for myself, and each adjacent face
		const auto& pi = points[face.point_idx[0]];
		const auto& p1 = points[adj1.point_idx[0]];
		const auto& p2 = points[adj2.point_idx[0]];
		const auto& p3 = points[adj3.point_idx[0]];

		// normal for myself, and each adjacent face
		const auto& ni = face.normal;
		const auto& n1 = adj1.normal;
		const auto& n2 = adj2.normal;
		const auto& n3 = adj3.normal;

		face.px = vec4( pi.x, p1.x, p2.x, p3.x );
		face.py = vec4( pi.y, p1.y, p2.y, p3.y );
		face.pz = vec4( pi.z, p1.z, p2.z, p3.z );

		face.nx = vec4( ni.x, n1.x, n2.x, n3.x );
		face.ny = vec4( ni.y, n1.y, n2.y, n3.y );
		face.nz = vec4( ni.z, n1.z, n2.z, n3.z );
	}*/
}



void make_indexed_vertex_buffer(const Mesh& m, SoaVertexBuffer& d, a64::vector<int>& idx) {

	auto search_vbo = [&d](float px, float py, float pz, float nx, float ny, float nz, float tx, float ty) {
		for (int i = 0; i < d.px.size(); i++) {
			if (almost_equal(d.px[i], px) &&
				almost_equal(d.py[i], py) &&
				almost_equal(d.pz[i], pz) &&
				almost_equal(d.nx[i], nx) &&
				almost_equal(d.ny[i], ny) &&
				almost_equal(d.nz[i], nz) &&
				almost_equal(d.tx[i], tx) &&
				almost_equal(d.ty[i], ty)) {
				return i; }}
		return -1; };

//	int start_size = vbo.size();
	for (const auto& face : m.faces) {
		idx.push_back(face.front_material);
		for (int i = 0; i < 3; i++) {
			auto point = m.points[face.point_idx[i]];
			auto normal = m.normals[face.normal_idx[i]];
			auto texcoord = m.texcoords[face.texcoord_idx[i]];
			int vbo_idx = d.upsert(point, normal, texcoord);
			idx.push_back(vbo_idx); }}

	const int siz = d.px.size();
	const int rag = siz % 4;
	const int needed = 4 - rag;
	//cout << "vbo has " << d.px.size() << " vertices" << endl;
	for (int i = rag; i < 4; i++) {
		d.px.push_back(0);
		d.py.push_back(0);
		d.pz.push_back(0);
		d.nx.push_back(0);
		d.ny.push_back(0);
		d.nz.push_back(0);
		d.tx.push_back(0);
		d.ty.push_back(0); }
	/*
	int end_size = vbo.size() - start_size;
	int faces = idx.size() / 3;
	cout << "vbo created with " << end_size << " entries" << endl;
	cout << "    index list has " << faces << " faces" << endl;
	cout << "    worst case is " << (faces * 3) << " vertex entries" << endl;
	*/
}


void MeshStore::load_dir(const string& prefix, MaterialStore& materialstore, TextureStore& texturestore) {

	const string spec = "*.obj";

	for (auto& fn : fileglob(prefix + spec)) {
		cout << "loading mesh [" << prefix << "][" << fn << "]" << endl;

		Mesh tmp_mesh;
		MaterialStore tmp_materials;
		tie(tmp_mesh, tmp_materials) = load_obj(prefix, fn);

		int material_base_idx = int(materialstore.store.size());

		for (auto& item : tmp_materials.store) {
			if (item.imagename != "") {
				texturestore.load_any(prefix, item.imagename); }
			materialstore.store.push_back(item); }

		for (auto& item : tmp_mesh.faces) {
			item.front_material += material_base_idx; }

		store.push_back(tmp_mesh); }}
