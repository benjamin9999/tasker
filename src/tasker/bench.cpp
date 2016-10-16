#include "stdafx.h"
#include <algorithm>
#include <iostream>
#include <vector>

#include "bench.h"

using namespace std;


struct BenchStat calc_stat(const vector<double>& _timings, const double discard) {

	vector<double> timings(_timings);
	sort(timings.begin(), timings.end());
	timings.resize(int(timings.size() * (1.0 - discard)));

	auto len = timings.size();

	double ax;

	auto _min = timings[0];
	auto _max = timings[len - 1];
	auto idx_25 = len >> 2;
	auto _25th = timings[idx_25];
	auto idx_75 = idx_25 * 3;
	auto _med = timings[len >> 1];
	auto _75th = timings[idx_75];

	ax = 0;
	for (const auto &val : timings) {
		ax += val; }
	auto _mean = ax /= len;

	ax = 0;
	for (const auto &val : timings) {
		double tmp = val - _mean;
		ax += (tmp * tmp); }
	ax /= len;
	auto _sdev = sqrt(ax);

	return{ _min, _max, _25th, _med, _75th, _mean, _sdev }; }