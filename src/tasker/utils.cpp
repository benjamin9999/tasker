#include "stdafx.h"
#include <algorithm>
#include <codecvt>
#include <cctype>
#include <string>
#include <vector>

#include <Windows.h>
#undef min
#undef max

using namespace std;


wstring s2ws(const string& str) {
	typedef codecvt_utf8<wchar_t> convert_typeX;
	wstring_convert<convert_typeX, wchar_t> converterX;
	return converterX.from_bytes(str); }


string ws2s(const wstring& wstr) {
	typedef codecvt_utf8<wchar_t> convert_typeX;
	wstring_convert<convert_typeX, wchar_t> converterX;
	return converterX.to_bytes(wstr); }


vector<string> fileglob(const string& pathpat) {
	vector<string> lst;
	WIN32_FIND_DATA ffd;

	HANDLE hFind = FindFirstFile(s2ws(pathpat).c_str(), &ffd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
			lst.push_back(ws2s(wstring(ffd.cFileName)));
		} while (FindNextFile(hFind, &ffd) != 0); }
	return lst; }


vector<string> explode(const string& str, char ch) {
	vector<string> items;
	string src(str);
	auto nextmatch = src.find(ch);
	while (1) {
		string item = src.substr(0, nextmatch);
		items.push_back(item);
		if (nextmatch == string::npos) { break; }
		src = src.substr(nextmatch + 1);
		nextmatch = src.find(ch); }

	return items; }


string trim(const string &s) {
	string::const_iterator it = s.begin();
	while (it != s.end() && isspace(*it))
		it++;

	string::const_reverse_iterator rit = s.rbegin();
	while (rit.base() != it && isspace(*rit))
		rit++;

	return string(it, rit.base()); }
