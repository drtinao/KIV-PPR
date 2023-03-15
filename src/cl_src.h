#pragma once
#include <string>
#include <CL/cl.h>

const std::string cl_src_add_nums_intervals = R"CLC(
#pragma OPENCL EXTENSION cl_khr_int64_base_atomics: enable
#pragma OPENCL EXTENSION cl_khr_int64_extended_atomics: enable

__kernel void add_nums_intervals_avg(__global double* input_nums, int input_nums_size, __global int* output_intervals, int output_intervals_size, double interval_size, double min_value_data){
	int index = get_global_id(0);
	double input_num = input_nums[index];	

	int index_to_inc;
	if (min_value_data < 0) { //dataset minimum is < 0
		double part_index_1 = input_num / interval_size;
		double part_index_2 = fabs(min_value_data) / interval_size;
		index_to_inc = part_index_1 + part_index_2;
	}
	else {
		double part_index_1 = input_num / interval_size;
		double part_index_2 = min_value_data / interval_size;
		index_to_inc = part_index_1 - part_index_2;
	}

	if ((int)index_to_inc == output_intervals_size) { //last interval - include upper boundary
		index_to_inc = output_intervals_size - 1;
	}

	atom_inc(&output_intervals[index_to_inc]);
}
)CLC";

const std::string cl_src_min_max_dec_point_neg_num = R"CLC(
#pragma OPENCL EXTENSION cl_khr_int64_base_atomics: enable
#pragma OPENCL EXTENSION cl_khr_int64_extended_atomics: enable

__kernel void min_max_dec_point_neg_num(__global ulong* res_min_value_pos, __global ulong* res_max_value_pos, __global ulong* res_min_value_neg, __global ulong* res_max_value_neg, __global int* res_dec_point_num, volatile __global double* input_nums, int input_nums_size)
{
	int index = get_global_id(0);
	double input_num_orig = input_nums[index];
	ulong input_num_mod = as_ulong(fabs(input_num_orig));

	int dec_point_num = 0;

	if(input_num_orig < 0){
		atom_min(res_min_value_neg, input_num_mod);
		atom_max(res_max_value_neg, input_num_mod);
	}else{
		atom_min(res_min_value_pos, input_num_mod);
		atom_max(res_max_value_pos, input_num_mod);
	}

	if (fmod(input_num_orig, 1) != 0) { //num represents decimal point number
		dec_point_num = 1;
	}

	atom_max(res_dec_point_num, dec_point_num); //atleast one number is decimal (1)
}
)CLC";