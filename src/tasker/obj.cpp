#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <tuple>

#include "material.h"
#include "utils.h"
#include "mesh.h"
#include "vec.h"

using namespace std;


struct svec3 {
	float x, y, z;
	svec3() :x(0), y(0), z(0) {}
	svec3(float a) :x(a), y(a), z(a) {}
	svec3(float x, float y, float z) :x(x), y(y), z(z) {}

	vec4 xyz1() const {
		return vec4{x, y, z, 1}; }
	vec4 xyz0() const {
		return vec4{x, y, z, 0}; }
	vec3 xyz() const {
		return vec3{x, y, z}; } };


struct svec2 {
	float x, y;
	svec2() :x(0), y(0) {}
	svec2(float x, float y) :x(x), y(y) {}
	vec4 xy00() const {
		// the bilinear texture sampler
		// assumes that the texcoords
		// will be non-negative. this
		// offset prevents badness for
		// some meshes. XXX
		return vec4{ x+100.0f, y+100.0f, 0, 0 }; };
		//return vec4{ x, y, 0, 0 }; };
};


struct faceidx {
	int vv, vt, vn; };


svec2 ss_get_vec2(stringstream& ss) {
	float x, y;
	ss >> x >> y;
	return svec2{x, y}; }


svec3 ss_get_vec3(stringstream& ss) {
	float x, y, z;
	ss >> x >> y >> z;
	return svec3{x, y, z}; }


faceidx to_faceidx(const string& data) {
	using std::stol;
	auto tmp = explode(data, '/'); // "nn/nn/nn" or "nn//nn", 1+ indexed!!
	return faceidx{
		tmp[0].length() ? stol(tmp[0]) - 1 : 0, // vv
		tmp[1].length() ? stol(tmp[1]) - 1 : 0, // vt
		tmp[2].length() ? stol(tmp[2]) - 1 : 0  // vn
	}; }


vector<string> file_to_lines(const string& filename) {
	ifstream f(filename);
	vector<string> data;

	string ln;
	while (!getline(f, ln).eof()) {
		data.push_back(ln); }

	return data; }


struct ObjMaterial {
	string name;
	string texture;
	svec3 ka;
	svec3 kd;
	svec3 ks;
	float specpow;
	float density;

	void reset() {
		name = "__none__";
		texture = "";
		ka = svec3{0.0f};
		kd = svec3{0.0f};
		ks = svec3{0.0f};
		specpow = 1.0f;
		density = 1.0f; }

	Material to_material() const {
		Material mm;
		mm.ka = ka.xyz();
		mm.kd = kd.xyz();
		mm.ks = ks.xyz();
		mm.specpow = specpow;
		mm.d = density;
		mm.name = "obj-" + name;
		mm.imagename = texture;
		mm.shader = "obj";
		mm.pass = 0; // assume pass 0 by default
		return mm; }};


MaterialStore load_materials_from_file(const string& fn) {

	MaterialStore mlst;
	ObjMaterial m;

	auto push = [&mlst,&m]() {
		float p = 1.0f; // 2.2f;
		m.kd.x = pow(m.kd.x, p);
		m.kd.y = pow(m.kd.y, p);
		m.kd.z = pow(m.kd.z, p);
		mlst.store.push_back(m.to_material()); };

	auto lines = file_to_lines(fn);

	m.reset();
	for (auto& line : lines) {

		// remove comments, trim, skip empty lines
		auto tmp = line.substr(0, line.find('#'));
		tmp = trim(tmp);
		if (tmp.size() == 0) continue;

		// create a stream
		stringstream ss(tmp, stringstream::in);

		string cmd;
		ss >> cmd;
		if (cmd == "newmtl") {  //name
			if (m.name != "__none__") {
				push();
				m.reset(); }
			ss >> m.name; }
		else if (cmd == "Ka") { // ambient color
			m.ka = ss_get_vec3(ss); }
		else if (cmd == "Kd") { // diffuse color
			m.kd = ss_get_vec3(ss); }
		else if (cmd == "Ks") { // specular color
			m.ks = ss_get_vec3(ss); }
		else if (cmd == "Ns") { // phong exponent
			ss >> m.specpow; }
		else if (cmd == "d") { // opacity ("d-issolve")
			ss >> m.density; }
		else if (cmd == "map_Kd") { //diffuse texturemap
			ss >> m.texture; }}

	if (m.name != "__none__") {
		push(); }

	return mlst; }


std::tuple<Mesh,MaterialStore> load_obj(const string& prepend, const string& fn) {

	cout << "---- loading [" << fn << "]:" << endl;

	auto lines = file_to_lines(prepend + fn);

	string group_name("defaultgroup");
	int material_idx;

	MaterialStore materials;
	Mesh mesh;

	mesh.name = fn;

	for (auto& line : lines) {

		auto tmp = line.substr(0, line.find('#'));
		tmp = trim(tmp);
		if (tmp.size() == 0) continue;
		stringstream ss(tmp, stringstream::in);

		string cmd;
		ss >> cmd;

		if (cmd == "mtllib") {
			// material library
			string mtlfn;
			ss >> mtlfn;
			cout << "material library is [" << mtlfn << "]\n";
			materials = load_materials_from_file(prepend + mtlfn); }

		else if (cmd == "g") {
			// group, XXX unused
			ss >> group_name; }

		else if (cmd == "usemtl") {
			// material setting
			string material_name;
			ss >> material_name;
			material_idx = materials.find_by_name(string("obj-") + material_name); }

		else if (cmd == "v") {
			// vertex
			mesh.points.push_back(ss_get_vec3(ss).xyz1()); }

		else if (cmd == "vn") {
			// vertex normal
			mesh.normals.push_back(ss_get_vec3(ss).xyz0()); }

		else if (cmd == "vt") {
			// vertex uv
			mesh.texcoords.push_back(ss_get_vec2(ss).xy00()); }

		else if (cmd == "f") {
			// face indices
			string data = line.substr(line.find(' ') + 1, string::npos);
			auto faces = explode(data, ' ');
			vector<faceidx> indexes;
			for (auto& facechunk : faces) {
				indexes.push_back(to_faceidx(facechunk));
			}
			// triangulate and make faces
			for (unsigned i = 0; i < indexes.size() - 2; i++){
				Face fd;
				fd.front_material = material_idx;
				fd.point_idx = { { indexes[0].vv, indexes[i + 1].vv, indexes[i + 2].vv } };
				fd.normal_idx = { { indexes[0].vn, indexes[i + 1].vn, indexes[i + 2].vn } };
				fd.texcoord_idx = { { indexes[0].vt, indexes[i + 1].vt, indexes[i + 2].vt } };
				mesh.faces.push_back(fd); }}}

	mesh.compute_bbox();
	mesh.compute_face_and_vertex_normals();
	mesh.compute_edges();
	return std::tuple<Mesh,MaterialStore>(mesh, materials); }
