#pragma once
#include <vector>


struct BenchStat {
	double _min, _max, _25th, _med, _75th, _mean, _sdev;
};

struct BenchStat calc_stat(const std::vector<double>& timings, const double discard);