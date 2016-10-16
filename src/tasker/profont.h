#pragma once
#include <vector>
#include <map>

#include "../pixeltoaster/PixelToaster.h"

#include "vec.h"

/*
SPACE,!,",#,$,%,&,',(,),*,+,COMMA,-,.,/,0,1,2,3,4,5,6,7,8,:,;,<,=,>,?
@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_
`abcdefghijklmnopqrstuvwxyz{|}~MISSING

other interesting chars..

"degrees" y=4, x=1
"notequal" y=4, x=13
bullet y=4, x=5
plusminus, y=4, x=17
*/

class ProPrinter {
public:
	ProPrinter();
	unsigned charmap_to_offset(const ivec2& cxy) const;
	unsigned char_to_offset(const char ch) const;
	void draw_glyph(const char ch, int left, int top, struct TrueColorCanvas& canvas) const;
	void write(const std::string str, int left, int top, struct TrueColorCanvas& canvas) const;

private:
	const ivec2 glyph_dimensions;
	std::map<char, ivec2> charmap;
	std::vector<uint8_t> bitmap; };
