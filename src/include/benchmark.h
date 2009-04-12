#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <string>
#include <sys/types.h>
#include "globals.h"

struct benchmark_params {
	trx_mode modem;
	int freq;
	bool afc, sql;
	double sqlevel;
	double src_ratio;
	int src_type;
	std::string input, output, buffer;
	size_t samples;
};
extern struct benchmark_params benchmark;

int setup_benchmark(void);
void do_benchmark(void);

#endif
