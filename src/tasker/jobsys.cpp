#include "stdafx.h"
#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include <mutex>

#include "stdtimer.h"
#include "barrier.h"
#include "jobsys.h"

namespace jobsys {

#define member_size(type, member) sizeof(((type *)0)->member)


using _random_source = std::minstd_rand;
//using _random_source = std::mt19937;

int generate_random_number(const int &min, const int &max) {
	static thread_local _random_source * generator = nullptr;
	if (generator == nullptr) {
		auto my_hash = std::hash<std::thread::id>()(std::this_thread::get_id());
		generator = new _random_source(unsigned(clock() + my_hash)); }

	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(*generator); }


std::vector<Job*> jobpools;
std::vector<std::unique_ptr<Queue>> jobqueues;

std::vector<std::vector<struct JobStat>> telemetry_stores;
std::vector<stdtimer::Timer> telemetry_timers;


thread_local unsigned thread_id;
unsigned generation;
unsigned total_jobs;
thread_local uint32_t pool_idx;
std::atomic<bool> should_work{ false };
std::atomic<bool> should_quit{ false };
int thread_count;

std::vector<std::thread> pool;

Barrier *start_barrier;
Barrier *end_barrier;

thread_local double mark_start_time;


void init(const unsigned threads) {
	_ASSERT(sizeof(jobsys::Job) == CACHE_LINE_SIZE);

	//	std::cout << "extra bytes: " << member_size(Job, data) << std::endl;

	thread_id = 0; // init should be run in main thread...
	generation = 1;
	total_jobs = 0;
	thread_count = threads;

	jobpools.clear();
	jobqueues.clear();
	for (unsigned ti = 0; ti < threads; ti++) {
		auto telemetry = std::vector<struct JobStat>(NUMBER_OF_JOBS);
		auto timer = stdtimer::Timer();
		telemetry_stores.push_back(telemetry);
		telemetry_timers.push_back(timer);

		void * ptr = _aligned_malloc(NUMBER_OF_JOBS * sizeof(Job), 64);
		auto job_pool_ptr = reinterpret_cast<Job*>(ptr);
		for (int pi = 0; pi < NUMBER_OF_JOBS; pi++) {
			job_pool_ptr[pi].generation = 0;
		}
		jobpools.push_back(job_pool_ptr);
		jobqueues.push_back(std::make_unique<Queue>());
	}

	start_barrier = new Barrier(threads);
	end_barrier = new Barrier(threads);

	for (unsigned ti = 0; ti < threads; ti++) {
		if (ti > 0)
			pool.push_back(std::thread(thread_main, ti));
	}
}


void thread_main(unsigned id) {
	thread_id = id;
	while (true) {
		start_barrier->wait();
		if (should_quit) break;
		while (should_work == true) {
			help();
		}
		end_barrier->wait();
	}
}


void noop(jobsys::Job *job, const int tid, void *) {}

}//namespace jobsys
