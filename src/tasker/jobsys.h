#pragma once
#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include <mutex>

#include "stdtimer.h"
#include "barrier.h"

namespace jobsys {

#define member_size(type, member) sizeof(((type*)0)->member)

constexpr unsigned CACHE_LINE_SIZE = 64u;
constexpr unsigned NUMBER_OF_JOBS = 65536;
constexpr unsigned MASK = NUMBER_OF_JOBS - 1u;

typedef void(*JobFunction)(struct Job*, const unsigned thread_id, const void*);

void thread_main(unsigned id);


struct Job {
	JobFunction function;
	Job* parent;
	std::atomic<int> unfinished_jobs;
	int generation;
	char data[CACHE_LINE_SIZE - (sizeof(JobFunction) +
	                             sizeof(Job*) +
	                             sizeof(std::atomic<int>) +
	                             sizeof(int))];
};

static_assert(sizeof(Job) == CACHE_LINE_SIZE, "unexpected sizeof(Job)");

struct JobStat {
	double start_time;
	double end_time;
	uint32_t raw;
};


int generate_random_number(const int &min, const int &max);


class Queue {
public:
	Queue() :top(0), bottom(0) {}

	void push(Job* job) {
		auto b = bottom.load(std::memory_order_relaxed);
		jobs[b & MASK] = job;
		bottom.store(b + 1, std::memory_order_release); }

	Job* pop() {
		auto b = bottom.load(std::memory_order_relaxed) - 1;
		bottom.exchange(b);
		auto t = top.load(std::memory_order_relaxed);
		if (t <= b) {
			// non-empty queue
			Job* job = jobs[b & MASK];
			if (t != b) {
				// still more than one item left in the queue
				return job; }

			// this is the last item in the queue
			long tmp = t;
			if (!top.compare_exchange_weak(tmp, t + 1)) {
				// failed race against steal operation
				job = nullptr; }

			bottom.store(t + 1, std::memory_order_relaxed);
			return job; }
		else {
			// deque was already empty
			bottom.store(t, std::memory_order_relaxed);
			return nullptr; } }

	Job* steal() {
		auto t = top.load(std::memory_order_relaxed);
		std::atomic_thread_fence(std::memory_order_acquire);
		auto b = bottom.load(std::memory_order_relaxed);
		if (t < b) {
			Job* job = jobs[t & MASK];
			if (!top.compare_exchange_weak(t, t + 1)) {
				return nullptr; }
			return job; }
		else {
			// queue is empty
			return nullptr; } }

private:
	Job* jobs[NUMBER_OF_JOBS];
	std::atomic<long> top;
	std::atomic<long> bottom; };



extern std::vector<Job*> jobpools;
extern std::vector<std::unique_ptr<Queue>> jobqueues;



extern thread_local unsigned thread_id;
extern unsigned generation;
extern unsigned total_jobs;
extern thread_local uint32_t pool_idx;
extern std::atomic<bool> should_work;
extern std::atomic<bool> should_quit;

extern std::vector<std::thread> pool;

extern Barrier* start_barrier;
extern Barrier* end_barrier;

extern int thread_count;
extern std::vector<std::vector<struct JobStat>> telemetry_stores;
extern std::vector<stdtimer::Timer> telemetry_timers;


void init(const unsigned threads);

inline void reset() {
	// XXX in debug, search pool for unfinished jobs
	for (int ti = 0; ti < thread_count; ti++) {
		telemetry_stores[ti].clear();
		telemetry_timers[ti].reset(); }
	generation++;
	total_jobs = 0; }


inline Job* allocate_job() {
	auto my_store = jobpools[thread_id];
	_ASSERT(total_jobs < NUMBER_OF_JOBS);
	total_jobs += 1;
	const uint32_t index = pool_idx++;
	auto job = &my_store[index & (NUMBER_OF_JOBS - 1)];
	_ASSERT(job->generation != generation);
	job->generation = generation;
	return job; }


template <typename T>
Job* make_job(T function) {
	Job* job = allocate_job();
	job->function = reinterpret_cast<JobFunction>(function);
	job->parent = nullptr;
	job->unfinished_jobs.store(1);
	return job; }


template <typename T, typename D>
Job* make_job(T function, D data) {
	Job* job = allocate_job();
	job->function = reinterpret_cast<JobFunction>(function);
	job->parent = nullptr;
	job->unfinished_jobs.store(1);
	_ASSERT(sizeof(data) <= member_size(Job, data));
	memcpy(job->data, &data, sizeof(data));
	return job; }


template <typename T>
Job* make_job_as_child(Job* parent, T function) {
	parent->unfinished_jobs++;

	Job* job = allocate_job();
	job->function = reinterpret_cast<JobFunction>(function);
	job->parent = parent;
	job->unfinished_jobs.store(1);
	return job; }


template <typename T, typename D>
Job* make_job_as_child(Job* parent, T function, D data) {
	parent->unfinished_jobs++;

	Job* job = allocate_job();
	job->function = reinterpret_cast<JobFunction>(function);
	job->parent = parent;
	job->unfinished_jobs.store(1);
	_ASSERT(sizeof(data) <= member_size(Job, data));
	memcpy(job->data, &data, sizeof(data));
	return job; }


inline bool is_empty_job(const Job* const job) {
	return job == nullptr; }


inline bool has_job_completed(const Job* const job) {
	return job->unfinished_jobs == 0; }


inline bool can_execute(const Job* const job) {
	return job->unfinished_jobs == 1; }


inline void sleep(unsigned ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }


inline void yield() {
	sleep(0); }


inline Queue* get_my_queue() {
	return jobqueues[thread_id].get(); }


inline Queue* get_random_queue() {
	unsigned random_idx = generate_random_number(0, thread_count - 1);
	return jobqueues[random_idx].get(); }


inline Job* get_job() {
	auto my_queue = get_my_queue();

	auto job = my_queue->pop();
	if (is_empty_job(job)) {
		// our queue is empty, try to steal
		auto steal_queue = get_random_queue();
		if (steal_queue == my_queue) {
			yield();  // can't steal from ourselves
			return nullptr; }

		auto stolen_job = steal_queue->steal();
		if (is_empty_job(stolen_job)) {
			yield();  // steal failed
			return nullptr; }

		return stolen_job; }

	return job; }


void run(Job* job);
void help();
void wait(const Job* job);
void finish(Job* job);
void execute(Job* job);


inline void execute(Job* job) {
	while (!can_execute(job)) {
		help(); }

	double start_time = telemetry_timers[thread_id].elapsed();
	(job->function)(job, thread_id, job->data);
	double end_time = telemetry_timers[thread_id].elapsed();
	telemetry_stores[thread_id].push_back({
		start_time, end_time,
		reinterpret_cast<uint32_t>(job->function)
	});

	finish(job); }


extern thread_local double mark_start_time;
inline void mark_start() {
	mark_start_time = telemetry_timers[thread_id].elapsed(); }

inline void mark_end(const uint32_t bits) {
	double mark_end_time = telemetry_timers[thread_id].elapsed();
	telemetry_stores[thread_id].push_back({
		mark_start_time, mark_end_time, bits }); }


inline void run(Job* job) {
	get_my_queue()->push(job);}


inline void wait(const Job* job) {
	while (!has_job_completed(job)) {
		Job* next_job = get_job();
		if (next_job != nullptr) {
			execute(next_job);}}}


inline void finish(Job* job) {
	const int32_t unfinished_jobs = --job->unfinished_jobs;
	if ((unfinished_jobs == 0) && (job->parent != nullptr)) {
		finish(job->parent);}}


inline void help() {
	Job* job = get_job();
	if (job != nullptr) {
		execute(job);}}


void thread_main(unsigned id);


inline void work_start() {
	should_work.store(true);
	start_barrier->wait();}


inline void work_end() {
	should_work.store(false);
	end_barrier->wait();}


inline void stop() {
	should_work.store(false);
	should_quit.store(true);
	start_barrier->wait();}


inline void join() {
	for (auto &thread : pool) {
		thread.join();}}


void noop(jobsys::Job* job, const int tid, void*);

#undef member_size
}
