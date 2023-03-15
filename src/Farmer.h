#pragma once
#include <vector>
#include <thread>
#if __has_include(<CL/opencl.hpp>)
# include <CL/opencl.hpp>
#else
# include "opencl.hpp"
#endif
#include "Structures.h"
#include "const.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/combinable.h"

//detects least occupied devices and assigns work
class Farmer
{
	private:
		compute_type sel_comp_type; //selected type of computation - smp, all, spec. OpenCL devices
		std::vector<cl_dev_stuff_struct> cl_devices; //OpenCL devices on which computing should be performed

		tbb::enumerable_thread_specific<double> min_value_global; //minimum value for each SMP device
		tbb::enumerable_thread_specific<double> max_value_global; //maximum value for each SMP device
		tbb::enumerable_thread_specific<bool> dec_point_num_global; //thread found decimal point number flag - SMP
		tbb::enumerable_thread_specific<bool> negative_num_global; //thread found negative number flag - SMP
		std::vector<int> output_intervals_combined; //counters for each interval - SMP

		void cl_min_max_dec_point_neg_num(std::vector<double> input_nums, cl_dev_stuff_struct* least_occ_cl_dev); //assign the job to OpenCL device
		void smp_min_max_dec_point_neg_num(std::vector<double> input_nums); //assign the job to SMP device
		void cl_add_nums_to_intervals(std::vector<double> input_nums, double interval_size, double min_value_data, cl_dev_stuff_struct* least_occ_cl_dev); //assign the job to OpenCL device
		void smp_add_nums_to_intervals(std::vector<double> input_nums, double interval_size, double min_value_data, int interval_count); //assign the job to SMP device
	public:
		Farmer(compute_type sel_comp_type, std::vector<cl_dev_stuff_struct> compute_cl_devices); //constructor expects selected computing type + vector with allowed OpenCL devices
		std::vector<cl_dev_stuff_struct*> get_free_cl_devices(); //gets OpenCL devices which are not processing any data
		void prep_devs_min_max_dec_point_neg_num(double init_min_max_val); //init OpenCL + SMP for first round of algorithm
		void prep_devs_intervals(int interval_count); //init OpenCL + SMP for second round of algorithm
		void assign_min_max_dec_point_neg_num(std::vector<double> input_nums); //checks whether value is decimal / negative (useful for check if exponential + Poisson) + checks for minimum / maximum value
		void retr_min_max_dec_point_neg_num_res(double* res_min_value, double* res_max_value, bool* res_dec_point_num, bool* res_negative_num); //gets results of first round of algorithm
		void assign_add_nums_to_intervals(std::vector<double> input_nums, double interval_size, double min_value_data, int interval_count); //assign the job (second round of algorithm)
		void retr_add_nums_to_intervals_res(std::vector<int>* output_intervals, int interval_count); //get result of the job (second round of algorithm)
};

