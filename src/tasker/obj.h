#pragma once
#include <string>
#include <tuple>

std::tuple<struct Mesh, class MaterialStore> load_obj(const std::string& prepend, const std::string& fn);
