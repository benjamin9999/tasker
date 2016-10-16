#include "stdafx.h"
#include <iostream>
#include <vector>
#include <thread>

#include <PixelToaster.h>

#include "jobsys.h"
#include "main.h"

using namespace std;

void test_all();


int main(int argc, char *argv[]) {

//	test_all();
//	return 0;

	vector<thread> threads;

	unsigned hardware_threads;
	hardware_threads = std::thread::hardware_concurrency();
	jobsys::init(hardware_threads);

	if (0) {
		PixelToaster::Timer tm;
		for (int frame = 0; frame < 6000; frame++) {
			jobsys::Job *root = jobsys::make_job(&jobsys::noop);
			for (int i = 0; i < 10000; i++) {
				jobsys::Job* job = jobsys::make_job_as_child(root, &jobsys::noop);
				jobsys::run(job); }
			jobsys::run(root);
			jobsys::wait(root);
			jobsys::reset(); }
		cout << "6000 frames, " << tm.time() << endl; }
//	return 0;


	Application app;
	app.run();

	jobsys::stop();
	jobsys::join();
	return 0; }
