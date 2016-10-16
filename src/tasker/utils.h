#pragma once
#include <vector>
#include <string>

#define _unused(x) ((void)x)

std::vector<std::string> fileglob(const std::string& pathpat);
std::vector<std::string> explode(const std::string& str, char ch);
std::string trim(const std::string& s);