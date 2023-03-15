#include "cl_defines.h"
#include "Farmer.h"
#if __has_include(<CL/opencl.hpp>)
# include <CL/opencl.hpp>
#else
# include "opencl.hpp"
#endif
#include <iostream>
#include <CL/cl.h>
#include <algorithm>
#include <numeric>
#include <limits>
#include <future>

/*
Purpose of this class is to detect least occupied device (OpenCL / SMP) and assign work.
compute_type sel_comp_type = desired computation type (OpenCL / SMP / both)
std::vector<cl_dev_stuff_struct> compute_cl_devices = vector with allowed OpenCL devices (size 0 if OpenCL not allowed)
*/
Farmer::Farmer(compute_type sel_comp_type, std::vector<cl_dev_stuff_struct> compute_cl_devices)
{
	this->sel_comp_type = sel_comp_type;
	this->cl_devices = compute_cl_devices;
}

/*
Returns vector with CL devices which are currently not working. Ie. latest defined task is finished.
*/
std::vector<cl_dev_stuff_struct*> Farmer::get_free_cl_devices() {
	std::vector<cl_dev_stuff_struct*> free_cl_devs;

	for (int i = 0; i < this->cl_devices.size(); i++) { //go through available devices and find free one
		cl_dev_stuff_struct* one_cl_dev = &this->cl_devices[i];
		if (!one_cl_dev->current_task.valid() || (one_cl_dev->current_task.wait_for(std::chrono::nanoseconds(0)) == std::future_status::ready)) {
			free_cl_devs.push_back(one_cl_dev);
		}
	}
	return free_cl_devs;
}

/*
Prepares devices for first round of algorithm (finding min + max, whether decimal point num is present). Ie. clears buffers etc.
double init_min_max_val = value to which minimum + maximum is inited
*/
void Farmer::prep_devs_min_max_dec_point_neg_num(double init_min_max_val){
	double init_val_min = DBL_MAX;
	double init_val_max = 0;
	bool init_bool = false;

	//init opencl
	for (int i = 0; i < this->cl_devices.size(); i++) { //go through all devices
		cl_dev_stuff_struct* one_cl_dev = &this->cl_devices[i];
		cl::CommandQueue* queue = &one_cl_dev->dev_queue;

		queue->enqueueWriteBuffer(one_cl_dev->res_min_pos_buf, CL_TRUE, 0, sizeof(double), &init_val_min); //min buf
		queue->enqueueWriteBuffer(one_cl_dev->res_max_pos_buf, CL_TRUE, 0, sizeof(double), &init_val_max); //max buf
		queue->enqueueWriteBuffer(one_cl_dev->res_min_neg_buf, CL_TRUE, 0, sizeof(double), &init_val_min); //min buf
		queue->enqueueWriteBuffer(one_cl_dev->res_max_neg_buf, CL_TRUE, 0, sizeof(double), &init_val_max); //max buf
		queue->enqueueWriteBuffer(one_cl_dev->res_dec_point_num_buf, CL_TRUE, 0, sizeof(int), &init_val_max); //decimal point num buf
		queue->finish();
	}

	//init smp
	this->min_value_global = tbb::enumerable_thread_specific<double>(init_val_min);
	this->max_value_global = tbb::enumerable_thread_specific<double>(init_val_max);
	this->dec_point_num_global = tbb::enumerable_thread_specific<bool>(init_bool);
	this->negative_num_global = tbb::enumerable_thread_specific<bool>(init_bool);
}

/*
Prepares devs for second round of algorithm - adding nums to intervals, finding average. Clears buffers for given values.
*/
void Farmer::prep_devs_intervals(int interval_count) {
	double init_val = 0;
	int init_val_int = 0;

	//init opencl
	for (int i = 0; i < this->cl_devices.size(); i++) { //go through all devices
		cl_dev_stuff_struct* one_cl_dev = &this->cl_devices[i];
		cl::CommandQueue* queue = &one_cl_dev->dev_queue;

		std::vector<int> output_intervals(MAX_OUTPUT_INTERVAL_COUNT, 0); //output buffer

		queue->enqueueWriteBuffer(one_cl_dev->output_intervals_buf, CL_TRUE, 0, sizeof(int) * output_intervals.size(), output_intervals.data()); //buffer for output interval
		queue->finish();
	}

	//init smp
	this->output_intervals_combined = std::vector<int>(interval_count, 0);
}

/*
Finds out if given number represents minimum / maximum in dataset and updates respective variables. Also checks whether given value is decimal or negative. If so, updates corresponding bool variables.
std::vector<double> input_nums = numbers to be processed
*/
void Farmer::assign_min_max_dec_point_neg_num(std::vector<double> input_nums)
{
	if (this->cl_devices.size() != 0) { //cl devices allowed + present, check free ones
		std::vector<cl_dev_stuff_struct*> free_cl_devs = this->get_free_cl_devices();
		if (free_cl_devs.size() == 0) { //all openCL devices working
			if (this->sel_comp_type != OPENCL) { //assign to SMP, allowed
				smp_min_max_dec_point_neg_num(input_nums);
			}
			else { //only OpenCL devices allowed, wait for one..
				while (true) {
					free_cl_devs = this->get_free_cl_devices();
					if (free_cl_devs.size() != 0) {
						assign_min_max_dec_point_neg_num(input_nums);
						break;
					}
				}
			}
		}
		else { //split work into free cl devices / SMP
			int original_data_size = static_cast<int>(input_nums.size());
			int chunk_size = original_data_size / static_cast<int>(free_cl_devs.size());

			int i = 0;
			std::vector<double> chunk_data;
			for (; i < free_cl_devs.size() - 1; i++) { //assign OpenCL
				chunk_data = std::vector<double>{ input_nums.begin() + (chunk_size * i), input_nums.begin() + (chunk_size * (i + 1)) };
				cl_min_max_dec_point_neg_num(chunk_data, free_cl_devs[i]);
			}

			//assign last chunk
			chunk_data = std::vector<double>{ input_nums.begin() + (chunk_size * i), (input_nums.end()) };
			cl_min_max_dec_point_neg_num(chunk_data, free_cl_devs[i]);
		}
	}
	else { //Cl allowed but not found, use SMP
		smp_min_max_dec_point_neg_num(input_nums);
	}
}

/*
Assign the respective job to OpenCL device.
std::vector<double> input_nums = numbers to be processed
cl_dev_stuff_struct* least_occ_cl_dev = least occupied device
*/
void Farmer::cl_min_max_dec_point_neg_num(std::vector<double> input_nums, cl_dev_stuff_struct* least_occ_cl_dev) {
	cl::Kernel* kernel = &least_occ_cl_dev->ker_min_max_dec_point_neg_num;

	int input_nums_size = static_cast<int>(input_nums.size());
	kernel->setArg(6, input_nums_size);

	least_occ_cl_dev->current_task = std::async(std::launch::async, [least_occ_cl_dev, input_nums, input_nums_size]() {
		cl::Kernel* kernel = &least_occ_cl_dev->ker_min_max_dec_point_neg_num;
		cl::CommandQueue* queue = &least_occ_cl_dev->dev_queue;

		queue->enqueueWriteBuffer(least_occ_cl_dev->input_nums_buf, CL_TRUE, 0, input_nums_size * sizeof(double), input_nums.data());
		queue->enqueueNDRangeKernel(*kernel, cl::NullRange, cl::NDRange(input_nums.size()), cl::NullRange, NULL);
	});
}

/*
Assign the respective job to SMP device.
std::vector<double> input_nums = numbers to be processed
*/
void Farmer::smp_min_max_dec_point_neg_num(std::vector<double> input_nums) {

	auto tbb_first_pass_worker = [&](tbb::blocked_range<size_t> br) {
		//local values for one block - START
		double& min_value_local = min_value_global.local();
		double& max_value_local = max_value_global.local();
		bool& dec_point_num_local = dec_point_num_global.local();
		bool& negative_num_local = negative_num_global.local();
		//local values for one block - END

		for (size_t i = br.begin(); i < br.end(); i++) { //run on more threads
			if (input_nums[i] < min_value_local) { //num represents current minimum in dataset
				min_value_local = input_nums[i];
			}

			if (input_nums[i] > max_value_local) { //num represents current maximum in dataset
				max_value_local = input_nums[i];
			}

			if (fmod(input_nums[i], 1) != 0) { //num represents decimal point number
				dec_point_num_local = true;
			}

			if (input_nums[i] < 0) { //num is negative -> switch bool flag
				negative_num_local = true;
			}
		}
	};
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, input_nums.size()), tbb_first_pass_worker);
}

/*
Returns relevant results from first round of algorithm. Ie. gathers data from all devices which were used during computation and summarizes.
double* res_min_value = minimum value found accross all devices
double* res_max_value = maximum value found accross all devices
bool* res_dec_point_num = if decimal value found by atleast one device = true, else false
bool* res_negative_num = if negative value found by atleast one device = true, else false
*/
void Farmer::retr_min_max_dec_point_neg_num_res(double* res_min_value, double* res_max_value, bool* res_dec_point_num, bool* res_negative_num) {
	bool cl_vals_valid = false; //at least one opencl device used during computing
	bool smp_vals_valid = false; //at least one thread used during computing

	std::vector<double> cl_min_value_pos_all; //cl - minimum value of each device + 
	std::vector<double> cl_max_value_pos_all; //cl - max dataset value of each device +
	std::vector<double> cl_min_value_neg_all; //cl - minimum value of each device -
	std::vector<double> cl_max_value_neg_all; //cl - max dataset value of each device -
	std::vector<int> cl_dec_point_num_all; //cl - decimal point number present bool flag of each dev.

	for (int i = 0; i < this->cl_devices.size(); i++) { //go through all cl devices
		cl_dev_stuff_struct* one_cl_dev = &this->cl_devices[i];
		cl::CommandQueue* queue = &one_cl_dev->dev_queue;
		one_cl_dev->current_task.wait(); //wait for all tasks to complete

		double res_min_pos_cl; //min of device + sign
		double res_max_pos_cl; //max of device + sign
		double res_min_neg_cl; //min of device - sign
		double res_max_neg_cl; //max of device - sign
		int res_dec_point_num_cl; //decimal point value found by OpenCL device

		queue->enqueueReadBuffer(one_cl_dev->res_min_pos_buf, CL_TRUE, 0, sizeof(double), &res_min_pos_cl);
		queue->enqueueReadBuffer(one_cl_dev->res_max_pos_buf, CL_TRUE, 0, sizeof(double), &res_max_pos_cl);
		queue->enqueueReadBuffer(one_cl_dev->res_min_neg_buf, CL_TRUE, 0, sizeof(double), &res_min_neg_cl);
		queue->enqueueReadBuffer(one_cl_dev->res_max_neg_buf, CL_TRUE, 0, sizeof(double), &res_max_neg_cl);
		queue->enqueueReadBuffer(one_cl_dev->res_dec_point_num_buf, CL_TRUE, 0, sizeof(int), &res_dec_point_num_cl);

		cl_min_value_pos_all.push_back(res_min_pos_cl);
		cl_max_value_pos_all.push_back(res_max_pos_cl);
		cl_min_value_neg_all.push_back(res_min_neg_cl);
		cl_max_value_neg_all.push_back(res_max_neg_cl);
		cl_dec_point_num_all.push_back(res_dec_point_num_cl);
	}
	
	double cl_min_value_pos_total = 0; //minimal value retrieved by cl device +
	double cl_max_value_pos_total = 0; //maximum value retrieved by cl device +
	double cl_min_value_neg_total = 0; //minimal value retrieved by cl device -
	double cl_max_value_neg_total = 0; //maximum value retrieved by cl device -

	double cl_min_value_total = 0; //final min value (after sign fix)
	double cl_max_value_total = 0; //final max value (aft sign fix)

	bool cl_dec_point_num_found = false; //at least one cl dev found decimal point number
	bool cl_negative_num_found = false; //at least one cl dev found negative number
	if (cl_min_value_pos_all.size() > 0) { //at least one dev used during openCL computation
		cl_vals_valid = true;

		cl_min_value_pos_total = std::min_element<>(cl_min_value_pos_all.begin(), cl_min_value_pos_all.end())[0];
		cl_max_value_pos_total = std::max_element<>(cl_max_value_pos_all.begin(), cl_max_value_pos_all.end())[0];
		cl_min_value_neg_total = std::min_element<>(cl_min_value_neg_all.begin(), cl_min_value_neg_all.end())[0];
		cl_max_value_neg_total = std::max_element<>(cl_max_value_neg_all.begin(), cl_max_value_neg_all.end())[0];

		if (std::find(begin(cl_dec_point_num_all), end(cl_dec_point_num_all), 1) == end(cl_dec_point_num_all)) {
			cl_dec_point_num_found = false;
		}
		else {
			cl_dec_point_num_found = true;
		}

		//parse OpenCL results (+ - sign omitted)
		//edit retrieved values (because of sign...)
		if ((std::abs(DBL_MAX - cl_min_value_pos_total) > DBL_EPSILON || std::abs(cl_max_value_pos_total) > DBL_EPSILON) && (std::abs(DBL_MAX - cl_min_value_neg_total) < DBL_EPSILON && std::abs(cl_max_value_neg_total) < DBL_EPSILON)) { //+ boundaries changed, - not => only positive numbers
			cl_min_value_total = cl_min_value_pos_total;
			cl_max_value_total = cl_max_value_pos_total;
		}
		else if ((std::abs(DBL_MAX - cl_min_value_neg_total) > DBL_EPSILON || std::abs(cl_max_value_neg_total) > DBL_EPSILON) && (std::abs(DBL_MAX - cl_min_value_pos_total) < DBL_EPSILON && std::abs(cl_max_value_pos_total) < DBL_EPSILON)) { //- boundaries changed, + not => only negative numbers
			cl_min_value_total = -cl_min_value_neg_total; //add - sign
			cl_max_value_total = -cl_max_value_neg_total; //add - sign
		}
		else { //- to + boundaries
			cl_min_value_total = -cl_max_value_neg_total;
			cl_max_value_total = cl_max_value_pos_total;
		}
	}

	if (cl_min_value_total < 0) { //negative num detected by atleast one device
		cl_negative_num_found = true;
	}

	//retrieve SMP values
	double smp_min_value_total = 0;
	double smp_max_value_total = 0;
	bool smp_dec_point_num_found = false;
	bool smp_negative_num_found = false;

	if (min_value_global.size() > 0 && max_value_global.size() && dec_point_num_global.size() > 0 && negative_num_global.size() > 0) {
		smp_vals_valid = true;

		smp_min_value_total = std::min_element<>(min_value_global.begin(), min_value_global.end())[0];
		smp_max_value_total = std::max_element<>(max_value_global.begin(), max_value_global.end())[0];

		if (std::find(dec_point_num_global.begin(), dec_point_num_global.end(), true) == dec_point_num_global.end()) {
			smp_dec_point_num_found = false;
		}
		else {
			smp_dec_point_num_found = true;
		}

		if (std::find(negative_num_global.begin(), negative_num_global.end(), true) == negative_num_global.end()) {
			smp_negative_num_found = false;
		}
		else {
			smp_negative_num_found = true;
		}
	}

	if (cl_vals_valid == true && smp_vals_valid == false) { //only openCL used, return openCL values
		*res_min_value = cl_min_value_total;
		*res_max_value = cl_max_value_total;
		*res_dec_point_num = cl_dec_point_num_found;
		*res_negative_num = cl_negative_num_found;
	}
	else if (cl_vals_valid == false && smp_vals_valid == true) { //only SMP used, return SMP
		*res_min_value = smp_min_value_total;
		*res_max_value = smp_max_value_total;
		*res_dec_point_num = smp_dec_point_num_found;
		*res_negative_num = smp_negative_num_found;
	}
	else { //openCL + SMP used, pick suitable
		*res_min_value = std::fmin(cl_min_value_total, smp_min_value_total);
		*res_max_value = std::fmax(cl_max_value_total, smp_max_value_total);
		if (cl_dec_point_num_found == true || smp_negative_num_found == true) {
			*res_dec_point_num = true;
		}

		if (cl_negative_num_found == true || smp_negative_num_found == true) {
			*res_negative_num = true;
		}
	}
	//auto negative_num_combined = std::all_of(negative_num_global.begin(), negative_num_global.end(), [](bool part_bool) { return !part_bool; });
}

/*
Assigns task which adds number from dataset to corresponding interval to least occupied device.
std::vector<double> input_nums = numbers to be processed
double interval_size = size of each interval
double min_value_data = minimum value found in data
int interval_count = number of intervals
*/
void Farmer::assign_add_nums_to_intervals(std::vector<double> input_nums, double interval_size, double min_value_data, int interval_count)
{
	if (this->cl_devices.size() != 0) { //cl devices allowed + present, check free ones
		std::vector<cl_dev_stuff_struct*> free_cl_devs = this->get_free_cl_devices();
		if (free_cl_devs.size() == 0) { //all openCL devices working
			if (this->sel_comp_type != OPENCL) { //assign to SMP, allowed
				smp_add_nums_to_intervals(input_nums, interval_size, min_value_data, interval_count);
			}
			else { //only OpenCL devices allowed, wait for one..
				while (true) {
					free_cl_devs = this->get_free_cl_devices();
					if (free_cl_devs.size() != 0) {
						assign_add_nums_to_intervals(input_nums, interval_size, min_value_data, interval_count);
						break;
					}
				}
			}
		}
		else { //split work into free cl devices / SMP
			int original_data_size = static_cast<int>(input_nums.size());
			int chunk_size = original_data_size / static_cast<int>(free_cl_devs.size());

			int i = 0;
			std::vector<double> chunk_data;
			for (; i < free_cl_devs.size() - 1; i++) { //assign OpenCL
				chunk_data = std::vector<double>{ input_nums.begin() + (chunk_size * i), input_nums.begin() + (chunk_size * (i + 1)) };
				cl_add_nums_to_intervals(chunk_data, interval_size, min_value_data, free_cl_devs[i]);
			}

			//assign last chunk
			chunk_data = std::vector<double>{ input_nums.begin() + (chunk_size * i), (input_nums.end()) };
			cl_add_nums_to_intervals(chunk_data, interval_size, min_value_data, free_cl_devs[i]);
		}
	}
	else { //Cl allowed but not found, use SMP
		smp_add_nums_to_intervals(input_nums, interval_size, min_value_data, interval_count);
	}
}

/*
Assigns respective task to CL device (private).
*/
void Farmer::cl_add_nums_to_intervals(std::vector<double> input_nums, double interval_size, double min_value_data, cl_dev_stuff_struct* least_occ_cl_dev)
{
	cl::Kernel* kernel = &least_occ_cl_dev->ker_add_nums_intervals;

	int input_nums_size = static_cast<int>(input_nums.size());
	kernel->setArg(1, input_nums_size);
	kernel->setArg(4, interval_size);
	kernel->setArg(5, min_value_data);

	least_occ_cl_dev->current_task = std::async(std::launch::async, [least_occ_cl_dev, input_nums, input_nums_size, interval_size, min_value_data]() {
		cl::Kernel* kernel = &least_occ_cl_dev->ker_add_nums_intervals;
		cl::CommandQueue* queue = &least_occ_cl_dev->dev_queue;

		queue->enqueueWriteBuffer(least_occ_cl_dev->input_nums_buf, CL_TRUE, 0, input_nums_size * sizeof(double), input_nums.data());
		queue->enqueueNDRangeKernel(*kernel, cl::NullRange, cl::NDRange(input_nums.size()), cl::NullRange, NULL);
	});
}

/*
Assigns respective task to SMP device (private).
*/
void Farmer::smp_add_nums_to_intervals(std::vector<double> input_nums, double interval_size, double min_value_data, int interval_count)
{
	std::vector<int> output_intervals(interval_count, 0); //output buffer

	tbb::combinable<std::vector<int>> output_intervals_combinable(output_intervals); //output of each thread
	auto tbb_add_nums_to_intervals = [&](tbb::blocked_range<size_t> br) {
		//local values for one block - START
		std::vector<int>& output_intervals_local = output_intervals_combinable.local();
		//local values for one block - END

		for (size_t i = br.begin(); i < br.end(); i++) { //run on more threads
			double index_to_inc; //calc corresponding index of interval which should be increased
			if (min_value_data < 0) { //dataset minimum is < 0
				double part_index_1 = input_nums[i] / interval_size;
				double part_index_2 = fabs(min_value_data) / interval_size;
				index_to_inc = part_index_1 + part_index_2;
			}
			else {
				double part_index_1 = input_nums[i] / interval_size;
				double part_index_2 = min_value_data / interval_size;
				index_to_inc = part_index_1 - part_index_2;
			}

			if ((int)index_to_inc == interval_count) { //last interval - include upper boundary
				int latest_valid_interv = interval_count - 1;
				index_to_inc = latest_valid_interv;
			}

			output_intervals_local[(int)index_to_inc] += 1;
		}
	};

	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, input_nums.size()), tbb_add_nums_to_intervals); //execute SMP task

	//combine products of threads
	output_intervals_combinable.combine_each([&](std::vector<int> block_result) {
		std::transform(
			output_intervals_combined.begin(),
			output_intervals_combined.end(),
			block_result.begin(),
			output_intervals_combined.begin(),
			std::plus<>()
		);
		});
}

/*
Returns relevant results from second round of algorithm. Ie. gets data from all computing devices and adds count of occurrences for each interval (across devices).
std::vector<int>* output_intervals = global occurrences for each interval
int interval_count = count of created intervals
*/
void Farmer::retr_add_nums_to_intervals_res(std::vector<int>* output_intervals, int interval_count) {
	//add openCL results (SMP already added on every chunk job finish)
	for (int i = 0; i < this->cl_devices.size(); i++) { //go through available devices and find least occupied / free
		cl_dev_stuff_struct* one_cl_dev = &this->cl_devices[i];
		cl::CommandQueue* queue = &one_cl_dev->dev_queue;
		one_cl_dev->current_task.wait(); //wait for all tasks to complete

		std::vector<int> output_intervals_cl(interval_count, 0); //results from one device
		queue->enqueueReadBuffer(one_cl_dev->output_intervals_buf, CL_TRUE, 0, interval_count * sizeof(int), output_intervals_cl.data());

		std::transform(
			output_intervals_combined.begin(),
			output_intervals_combined.end(),
			output_intervals_cl.begin(),
			output_intervals_combined.begin(),
			std::plus<>()
		);
	}

	*output_intervals = output_intervals_combined;
}