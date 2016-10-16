#pragma once
#include "vec.h"


struct irect {
	union {
		ivec2 top_left;
		ivec2 top;
		ivec2 left; };
	union {
		ivec2 bottom_right;
		ivec2 bottom;
		ivec2 right; };

	irect(const ivec2& tl, const ivec2& br) :top_left(tl), bottom_right(br) {}

	int height() const {
		return bottom.y - top.y; }

	int width() const {
		return right.x - left.x; } };

/*
struct ibox3 {
	union {
		ivec3 top_left_back;
		ivec3 top;
		ivec3 left;
		ivec3 back; };
	union {
		ivec3 bottom_right_front;
		ivec3 bottom;
		ivec3 right;
		ivec3 front; };

	ibox3(const ivec3& tlb, const ivec3& brf)
		:top_left_back(tlp), bottom_right_front(brf) {}

	int height() const {
		return bottom.y - top.y; }

	int width() const {
		return right.x - left.x; }

	int depth() const {
		return front.z - back.z; } };
*/
