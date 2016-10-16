/*
thread synchronization barrier, same as in boost
*/
#pragma once
#include <mutex>


class Barrier {
public:
	Barrier(unsigned count) :threshold(count), count(count), generation(0) {
		_ASSERT(count > 0); }

	bool wait() {
		std::unique_lock<std::mutex> lock(my_mutex);
		unsigned gen = generation;
		if (--count == 0) {
			generation++;
			count = threshold;
			cond.notify_all();
			return true; }

		while (gen == generation) {
			cond.wait(lock); }

		return false; }

private:
	std::mutex my_mutex;
	std::condition_variable cond;
	unsigned threshold;
	unsigned count;
	unsigned generation; };
