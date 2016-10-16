#include "stdafx.h"
#include "utils.h"
#include "mat4.h"
#include "vec.h"

using namespace std;


void ok() {
	cout << "."; }

void fail() {
	cout << "F"; }


void assertEqual(const string& a, const string& b) {
	if (a == b) {
		ok();
		return; }
	cout << "error, strings not equal " << a << " != " << b << endl; }


void assertAlmostEqual(float a, float b) {
	static const auto ep = 0.001f;
	if (abs(a - b) < ep) {
		ok();
		return; }
	cout << "error, not almost equal " << a << ", !~= " << b << endl; }


void assertAlmostEqual(const vec4& a, const vec4& b) {
	static const auto ep = 0.001f;
	float dx = abs(a.x - b.x);
	float dy = abs(a.y - b.y);
	float dz = abs(a.z - b.z);
	float dw = abs(a.w - b.w);
	if (dx < ep && dy < ep && dz < ep && dw < ep) {
		ok();
		return; }
	cout << "error, not almost equal " << a << ", !~= " << b << endl; }


void test_vec4_basic_math() {
	vec4 a{ 1, 2, 3, 4 };
	vec4 b{ 12, 13, 14, 15 };
	vec4 res = a + b;

//	cout << "test basic math 1" << endl;
	assertAlmostEqual(res.x, 13);
	assertAlmostEqual(res.y, 15);
	assertAlmostEqual(res.z, 17);
	assertAlmostEqual(res.w, 19);

	res = b - a;
//	cout << "test basic math 2" << endl;
	assertAlmostEqual(res.x, 11);
	assertAlmostEqual(res.y, 11);
	assertAlmostEqual(res.z, 11);
	assertAlmostEqual(res.w, 11);

	res = b * a;
//	cout << "test basic math 3" << endl;
	assertAlmostEqual(res.x, 12);
	assertAlmostEqual(res.y, 26);
	assertAlmostEqual(res.z, 42);
	assertAlmostEqual(res.w, 60);

	res = b / a;
//	cout << "test basic math 4" << endl;
	assertAlmostEqual(res.x, 12);
	assertAlmostEqual(res.y, 6.5);
	assertAlmostEqual(res.z, 4.666);
	assertAlmostEqual(res.w, 3.75);

	vec4 one{ 1 };
	vec4 two{ 2 };
	res = one + two;
//	cout << "test basic math 5" << endl;
	assertAlmostEqual(res.x, 3);
	assertAlmostEqual(res.y, 3);
	assertAlmostEqual(res.z, 3);
	assertAlmostEqual(res.w, 3);

	vec4 three{ 3 };
	vec4 started_default;

	started_default.x = -2;
	started_default.y = -3;
	started_default.z = -4;
	started_default.w = 10;
	res = three + started_default;
//	cout << "test basic math 6" << endl;
	assertAlmostEqual(res.x, 1);
	assertAlmostEqual(res.y, 0);
	assertAlmostEqual(res.z, -1);
	assertAlmostEqual(res.w, 13);
	
	cout << endl;
}


void test_vec3_basic_math() {
	vec3 a{ 1, 2, 3 };
	vec3 b{ 12, 13, 14 };
	vec3 res = a + b;

//	cout << "test basic math 1" << endl;
	assertAlmostEqual(res.x, 13);
	assertAlmostEqual(res.y, 15);
	assertAlmostEqual(res.z, 17);

	res = b - a;
//	cout << "test basic math 2" << endl;
	assertAlmostEqual(res.x, 11);
	assertAlmostEqual(res.y, 11);
	assertAlmostEqual(res.z, 11);

	res = b * a;
//	cout << "test basic math 3" << endl;
	assertAlmostEqual(res.x, 12);
	assertAlmostEqual(res.y, 26);
	assertAlmostEqual(res.z, 42);

	res = b / a;
//	cout << "test basic math 4" << endl;
	assertAlmostEqual(res.x, 12);
	assertAlmostEqual(res.y, 6.5);
	assertAlmostEqual(res.z, 4.666);

	vec3 one{ 1 };
	vec3 two{ 2 };
	res = one + two;
//	cout << "test basic math 5" << endl;
	assertAlmostEqual(res.x, 3);
	assertAlmostEqual(res.y, 3);
	assertAlmostEqual(res.z, 3);

	vec3 three{ 3 };
	vec3 started_default;

	started_default.x = -2;
	started_default.y = -3;
	started_default.z = -4;
	res = three + started_default;
//	cout << "test basic math 6" << endl;
	assertAlmostEqual(res.x, 1);
	assertAlmostEqual(res.y, 0);
	assertAlmostEqual(res.z, -1);
	
	cout << endl;
}


void test_vec2_basic_math() {
	vec2 a{ 1, 2 };
	vec2 b{ 12, 13 };
	vec2 res = a + b;

//	cout << "test basic math 1" << endl;
	assertAlmostEqual(res.x, 13);
	assertAlmostEqual(res.y, 15);

	res = b - a;
//	cout << "test basic math 2" << endl;
	assertAlmostEqual(res.x, 11);
	assertAlmostEqual(res.y, 11);

	res = b * a;
//	cout << "test basic math 3" << endl;
	assertAlmostEqual(res.x, 12);
	assertAlmostEqual(res.y, 26);

	res = b / a;
//	cout << "test basic math 4" << endl;
	assertAlmostEqual(res.x, 12);
	assertAlmostEqual(res.y, 6.5);

	vec2 one{ 1 };
	vec2 two{ 2 };
	res = one + two;
//	cout << "test basic math 5" << endl;
	assertAlmostEqual(res.x, 3);
	assertAlmostEqual(res.y, 3);

	vec2 three{ 3 };
	vec2 started_default;

	started_default.x = -2;
	started_default.y = -3;
	res = three + started_default;
//	cout << "test basic math 6" << endl;
	assertAlmostEqual(res.x, 1);
	assertAlmostEqual(res.y, 0);
	
	cout << endl;
}


/*void test_vec4_shuffles() {
	mvec4f a{ 2, 3, 4, 5 };
	mvec4f res;

	res = a.xxxx();
	assertAlmostEqual(res, vec4{ 2,2,2,2 });
	res = a.yyyy();
	assertAlmostEqual(res, vec4{ 3,3,3,3 });
	res = a.zzzz();
	assertAlmostEqual(res, vec4{ 4,4,4,4 });
	res = a.wwww();
	assertAlmostEqual(res, vec4{ 5,5,5,5 });

	res = a.yzxw();
	assertAlmostEqual(res, vec4{ 3, 4, 2, 5 });
	res = a.zxyw();
	assertAlmostEqual(res, vec4{ 4, 2, 3, 5 });

	res = a.xyxy();
	assertAlmostEqual(res, vec4{ 2, 3, 2, 3 });
	res = a.zwzw();
	assertAlmostEqual(res, vec4{ 4, 5, 4, 5 });
	cout << endl;
}*/


void test_vec3_more() {
	assertAlmostEqual(hmax(vec3{ 10,20,30 }), 30);
	assertAlmostEqual(hmax(vec3{ 30,20,10 }), 30);
	assertAlmostEqual(hmax(vec3{ 10,30,20 }), 30);
	assertAlmostEqual(hmin(vec3{ 10,20,30 }), 10);
	assertAlmostEqual(hmin(vec3{ 30,20,10 }), 10);
	assertAlmostEqual(hmin(vec3{ 10,30,20 }), 10);
}

/*void test_vec4_more() {
	vec4 a{ 4, 4, 0, 0 };
	auto len = length(a);

	assertAlmostEqual(len, 5.656);

	vec4 b{ 1, 1, 1, 0 };
	len = length(b);
	assertAlmostEqual(len, 1.732);

	vec4 c{ 1, 1, 1, 1 };
	len = length(c);
	assertAlmostEqual(len, 2);

	vec4 d{ 2, 3, 4, 5 };
	vec4 neg = -d;
	assertAlmostEqual(neg, vec4{ -2, -3, -4, -5 });

	vec4 e{ -10, -20, 30, -40 };
	vec4 f{ 100, -234, 0, 3.14f };
	assertAlmostEqual(abs(e), vec4{ 10, 20, 30, 40 });
	assertAlmostEqual(abs(f), vec4{ 100, 234, 0, 3.14f });

	vec4 da{ 2, 3, 4, 0 };
	vec4 db{ 3, 4, 5, 0 };
	assertAlmostEqual(dot(da, db), 3 * 2 + 4 * 3 + 5 * 4);

	vec4 g{ 16 };
	assertAlmostEqual(sqrt(g), vec4{ 4,4,4,4 });

	vec4 h{ 10,10,10,0 };
	assertAlmostEqual(normalize(h), vec4{ 0.5772f, 0.5772f, 0.5772f, 0 });
	assertAlmostEqual(length(normalize(h)), 1.0);

	vec4 i{ 4, 13, 8, -10 };
	assertAlmostEqual(hadd(i), 4 + 13 + 8 + (-10));
	assertAlmostEqual(hmin(i), -10);
	assertAlmostEqual(hmax(i), 13);

	assertAlmostEqual(hmax(vec4{ 10,20,30,40 }), 40);
	assertAlmostEqual(hmax(vec4{ 30,20,40,10 }), 40);
	assertAlmostEqual(hmax(vec4{ 40,30,20,10 }), 40);
	assertAlmostEqual(hmax(vec4{ 30,40,20,10 }), 40);
	assertAlmostEqual(hmin(vec4{ 10,20,30,40 }), 10);
	assertAlmostEqual(hmin(vec4{ 30,20,40,10 }), 10);
	assertAlmostEqual(hmin(vec4{ 10,40,20,30 }), 10);
	assertAlmostEqual(hmin(vec4{ 40,10,20,30 }), 10);

	vec4 ca{ 3, 4, 5, 0 };
	vec4 cb{ 4, 5, 6, 0 };
	assertAlmostEqual(cross(ca,cb), vec4{ -1, 2, -1,0 });
	cb = vec4{ 4, 5, -6, 0 };
	assertAlmostEqual(cross(ca, cb), vec4{ -49, 38, -1, 0 });
	cout << endl;


	vec4 pa{ 10,20,30,10 };
	assertAlmostEqual(perspective_divide(pa), vec4{ 1,2,3,1.0f / 10.0f });

	vec4 za{ 1, 2, 3, 4 };
	assertAlmostEqual(za.xyz0(), vec4{ 1,2,3,0 });
}*/


void test_mat4_basic() {
	mat4 m = mat4::ident();

	vec4 a{ 3, 4, 5, 6 };
	assertAlmostEqual(m*a, vec4{ 3,4,5,6 });

	vec4 b{ 10, 10, 10, 1 };
	m = mat4::scale(vec4{ 1, 2, 3, 4 });  // w component should be dropped
	assertAlmostEqual(m*b, vec4{ 10, 20, 30, 1 });

	a = vec4{ 3, 4, 5, 1 };
	m = mat4::translate(0, 0, -10);
	assertAlmostEqual(m*a, vec4{ 3, 4, -5, 1 });

	m = mat4{
		0,1,2,3,
		4,5,6,7,
		8,9,10,11,
		12,13,14,15 };
	assertAlmostEqual(m.ff[0], 0);
	assertAlmostEqual(m.ff[1], 4);
	assertAlmostEqual(m.ff[2], 8);
	assertAlmostEqual(m.ff[3], 12);

	assertAlmostEqual(m.ff[4], 1);
	assertAlmostEqual(m.ff[5], 5);
	assertAlmostEqual(m.ff[6], 9);
	assertAlmostEqual(m.ff[7], 13);

	// skip the third col...

	assertAlmostEqual(m.ff[12], 3);
	assertAlmostEqual(m.ff[13], 7);
	assertAlmostEqual(m.ff[14], 11);
	assertAlmostEqual(m.ff[15], 15);
}


void test_rotate_does_what_i_think() {
	array<int, 4> a = {{ 4, 5, 6, 7 }};
	//std::rotate(a.begin(), a.begin() + 1, a.end());
	std::copy(a.begin() + 1, a.end(), a.begin());
	cout << "a:" << a[0] << "," << a[1] << "," << a[2] << "," << a[3] << endl;
	//while (1) {}
}


void test_trim() {
	assertEqual(trim("   hey   "), "hey");
	assertEqual(trim("   hey"), "hey");
	assertEqual(trim("hey   "), "hey");
	assertEqual(trim("hey"), "hey");
}


void test_all() {
	test_vec4_basic_math();
	test_vec3_basic_math();
	test_vec2_basic_math();
//	test_vec4_shuffles();
//	test_vec4_more();
	test_mat4_basic();
//    test_rotate_does_what_i_think();
	test_trim();
}
