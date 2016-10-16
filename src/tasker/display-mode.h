#pragma once


struct DisplayMode {
	int width;
	int height; };


inline bool operator==(const DisplayMode& lhs, const DisplayMode& rhs) {
	return lhs.width == rhs.width &&
		lhs.height == rhs.height;
}


inline bool operator!=(const DisplayMode& lhs, const DisplayMode& rhs) {
	return !operator==(lhs, rhs); }