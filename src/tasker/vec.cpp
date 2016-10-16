#include "stdafx.h"
#include <ostream>

#include <fmt/format.h>

#include "vec.h"

using namespace std;
using fmt::format;


ostream& operator<<(ostream& os, const vec4& v) {
	os << format("{:.4f},{:.4f},{:.4f},{:.4f})", v.x, v.y, v.z, v.w);
	return os; }


ostream& operator<<(ostream& os, const vec3& v) {
	auto str = format("({:.4f},{:.4f},{:.4f})", v.x, v.y, v.z);
	os << str;
	return os; }


ostream& operator<<(ostream& os, const ivec2& v) {
	os << "<ivec2 " << v.x << ", " << v.y << ">";
	return os; }
