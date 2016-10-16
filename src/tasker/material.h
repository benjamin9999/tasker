#pragma once
#include <string>

#include "aligned-containers.h"
#include "vec.h"


struct Material {
	void print() const;

	vec3 ka;
	vec3 kd;
	vec3 ks;
	float specpow;
	float d;
	int pass;
	int invert;
	std::string name;
	std::string imagename;
	std::string shader; };


class MaterialStore {
public:
	void print() const;
	int find_by_name(const std::string& name) const;

	const Material& get(const int id) const {
		return store[id]; }

	a64::vector<Material> store; };
