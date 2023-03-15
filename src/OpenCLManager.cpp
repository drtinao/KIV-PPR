#include "cl_defines.h"
#include "OpenCLManager.h"
#include "cl_src.h"
#include <iostream>

/*
Loads all available OpenCL platforms + devices.
*/
void OpenCLManager::scan_cl_devs() {
	std::vector<cl::Platform> detected_platforms = this->retr_cl_platforms();
	for (int i = 0; i < detected_platforms.size(); i++) { //go through detected platforms
		std::vector<cl::Device> detected_devices = this->retr_cl_dev_for_platf(detected_platforms[i]);
		for (int j = 0; j < detected_devices.size(); j++) { //go through detected devices for the platform
			this->det_cl_devices.push_back(detected_devices[j]);
		}
	}
}

/*
Used for retrieving vector with available OpenCL platforms present in system.
return = vector object with detected OpenCL devices
*/
std::vector<cl::Platform> OpenCLManager::retr_cl_platforms()
{
	std::vector<cl::Platform> detected_platforms;
	cl::Platform::get(&detected_platforms);

	return detected_platforms;
}

/*
Function retrieves OpenCL devices for specific platform - specified by function parameter.
cl::Platform cl_platform = platform for which devices should be determined
*/
std::vector<cl::Device> OpenCLManager::retr_cl_dev_for_platf(cl::Platform cl_platform)
{
	std::vector<cl::Device> detected_devices;
	cl_platform.getDevices(CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU, &detected_devices); //get all CPUs + GPUs for given platform

	return detected_devices;
}

/*
Function checks whether device with specified name is valid OpenCL device. If so, returns corresponding index to vector with all detected cl devices. If device not found, then returns -1.
std::string cl_dev_name = name of OpenCL device which should be checked
return = index of valid OpenCL device, -1 if device name not valid
*/
int OpenCLManager::retr_cl_dev_index(std::string cl_dev_name)
{
	for (int i = 0; i < this->det_cl_devices.size(); i++) {
		if (strcmp(this->det_cl_devices[i].getInfo<CL_DEVICE_NAME>().c_str(), cl_dev_name.c_str()) == 0) {
			return i; //specified device found, stop traversing
		}
	}
	return -1; //all traversed - OpenCL device with given name not found
}

/*
Builds program specified by program source code for specific OpenCL device.
std::string program_source = source code of the program
cl::Device& target_device = target device
cl::Context cl_device_context = context of the device
*/
cl::Program OpenCLManager::build_program_from_src(const std::string program_source, cl::Device& target_device, cl::Context cl_device_context) {
	cl::Program cl_program(cl_device_context, program_source);
	auto err = cl_program.build("-cl-std=CL2.0");

	if (err != CL_BUILD_SUCCESS) {
		std::cout << "ERROR: Building program for OpenCL device \"" << target_device.getInfo<CL_DEVICE_NAME>() << "\" failed." << std::endl;
		std::cout << "Log: " << cl_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(target_device) << std::endl;
	}

	return cl_program;
}

/*
Creates context for all selected OpenCL devices. Used later when building programs etc.
*/
void OpenCLManager::setup_dev_contexts()
{
	cl_dev_stuff_struct cl_dev_stuff;
	cl::Context device_context;

	for (int i = 0; i < this->sel_cl_devices.size(); i++) {
		cl_dev_stuff = cl_dev_stuff_struct{};

		device_context = { this->sel_cl_devices[i] };

		cl_dev_stuff.dev = this->sel_cl_devices[i];
		cl_dev_stuff.dev_context = device_context;
		this->compute_cl_devices.push_back(cl_dev_stuff);
	}
}

/*
Builds all required OpenCL programs for all selected devices on which computing will be performed.
*/
bool OpenCLManager::build_req_cl_programs()
{
	cl::Program program;

	std::cout << "OpenCL program build - START" << std::endl;
	for (int i = 0; i < this->compute_cl_devices.size(); i++) {
		std::cout << "Building required programs for OpenCL device \"" << this->compute_cl_devices[i].dev.getInfo<CL_DEVICE_NAME>() << "\", please wait..." << std::endl;
		program = this->build_program_from_src(cl_src_min_max_dec_point_neg_num, this->compute_cl_devices[i].dev, this->compute_cl_devices[i].dev_context);
		this->compute_cl_devices[i].prog_min_max_dec_point_neg_num = program;
		program = this->build_program_from_src(cl_src_add_nums_intervals, this->compute_cl_devices[i].dev, this->compute_cl_devices[i].dev_context);
		this->compute_cl_devices[i].prog_add_nums_intervals = program;
	}
	std::cout << "OpenCL program build - END" << std::endl;
	return false;
}

/*
Prepares kernel for each computing device.
*/
bool OpenCLManager::setup_cl_kernels()
{
	cl::Kernel kernel;

	for (int i = 0; i < this->compute_cl_devices.size(); i++) {
		kernel = cl::Kernel(this->compute_cl_devices[i].prog_min_max_dec_point_neg_num, "min_max_dec_point_neg_num");
		this->compute_cl_devices[i].ker_min_max_dec_point_neg_num = kernel;
		kernel = cl::Kernel(this->compute_cl_devices[i].prog_add_nums_intervals, "add_nums_intervals_avg");
		this->compute_cl_devices[i].ker_add_nums_intervals = kernel;
	}
	return false;
}

/*
Prepares command queue for each of computing devices.
*/
bool OpenCLManager::setup_cl_queues() {
	cl::CommandQueue queue;

	for (int i = 0; i < this->compute_cl_devices.size(); i++) {
		queue = cl::CommandQueue(this->compute_cl_devices[i].dev_context, this->compute_cl_devices[i].dev);
		this->compute_cl_devices[i].dev_queue = queue;
	}
	return false;
}

/*
Allocates required buffers on CL devices.
*/
void OpenCLManager::alloc_cl_buffers() {
	cl_int buffer_error = 0;

	for (int i = 0; i < this->compute_cl_devices.size(); i++) {
		this->compute_cl_devices[i].res_min_pos_buf = cl::Buffer(this->compute_cl_devices[i].dev_context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(double), NULL, &buffer_error); //result buffer - minimum value in dataset
		print_err(buffer_error, "ERROR: creation of OpenCL buffer for minimum value (first pass) failed.");
		this->compute_cl_devices[i].res_max_pos_buf = cl::Buffer(this->compute_cl_devices[i].dev_context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(double), NULL, &buffer_error); //result buffer - maximum value in dataset
		print_err(buffer_error, "ERROR: creation of OpenCL buffer for maximum value (first pass) failed.");
		this->compute_cl_devices[i].res_min_neg_buf = cl::Buffer(this->compute_cl_devices[i].dev_context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(double), NULL, &buffer_error); //result buffer - minimum value in dataset
		print_err(buffer_error, "ERROR: creation of OpenCL buffer for minimum value (first pass) failed.");
		this->compute_cl_devices[i].res_max_neg_buf = cl::Buffer(this->compute_cl_devices[i].dev_context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(double), NULL, &buffer_error); //result buffer - maximum value in dataset
		print_err(buffer_error, "ERROR: creation of OpenCL buffer for maximum value (first pass) failed.");
		this->compute_cl_devices[i].res_dec_point_num_buf = cl::Buffer(this->compute_cl_devices[i].dev_context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(int), NULL, &buffer_error); //result buffer - decimal point number present
		print_err(buffer_error, "ERROR: creation of OpenCL buffer for integer (0 / 1) indicating whether decimal point number is present (first pass) failed.");
		this->compute_cl_devices[i].input_nums_buf = cl::Buffer(this->compute_cl_devices[i].dev_context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, DOUBLE_READ_COUNT_ONCE * sizeof(double), NULL, &buffer_error); //input numbers - copy to cl, read only cl
		print_err(buffer_error, "ERROR: creation of OpenCL buffer for interval input numbers failed.");
		this->compute_cl_devices[i].output_intervals_buf = cl::Buffer(this->compute_cl_devices[i].dev_context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, MAX_OUTPUT_INTERVAL_COUNT * sizeof(int), NULL, &buffer_error); //output, interval counter - copy to cl, read only cl, host read only
		print_err(buffer_error, "ERROR: creation of OpenCL buffer for interval counters (output) failed.");

		this->compute_cl_devices[i].ker_min_max_dec_point_neg_num.setArg(0, this->compute_cl_devices[i].res_min_pos_buf);
		this->compute_cl_devices[i].ker_min_max_dec_point_neg_num.setArg(1, this->compute_cl_devices[i].res_max_pos_buf);
		this->compute_cl_devices[i].ker_min_max_dec_point_neg_num.setArg(2, this->compute_cl_devices[i].res_min_neg_buf);
		this->compute_cl_devices[i].ker_min_max_dec_point_neg_num.setArg(3, this->compute_cl_devices[i].res_max_neg_buf);
		this->compute_cl_devices[i].ker_min_max_dec_point_neg_num.setArg(4, this->compute_cl_devices[i].res_dec_point_num_buf);
		this->compute_cl_devices[i].ker_min_max_dec_point_neg_num.setArg(5, this->compute_cl_devices[i].input_nums_buf);

		this->compute_cl_devices[i].ker_add_nums_intervals.setArg(0, this->compute_cl_devices[i].input_nums_buf);
		this->compute_cl_devices[i].ker_add_nums_intervals.setArg(2, this->compute_cl_devices[i].output_intervals_buf);
	}
}

/*
Allocates buffer for second round of algorithm (adding numbers into respetive intervals).
int output_intervals_count = number of intervals (size for output buffer)
*/
void OpenCLManager::alloc_add_nums_to_intervals_buffers(int output_intervals_count) {
	std::vector<int> output_intervals(output_intervals_count, 0); //output buffer
	cl_int buffer_error = 0;

	for (int i = 0; i < this->compute_cl_devices.size(); i++) {
		this->compute_cl_devices[i].ker_add_nums_intervals.setArg(3, output_intervals_count);
	}
}

/*
At first, functions checks whether given OpenCL device name is valid. If it is, adds the device to list with devices on which computing should be performed.
std::string cl_dev_name = OpenCL device which should be added
*/
bool OpenCLManager::add_sel_cl_dev(std::string cl_dev_name)
{
	int cl_dev_index = retr_cl_dev_index(cl_dev_name);
	if (cl_dev_index == -1) { //not valid OpenCL device name
		return false;
	}
	else { //device is valid, add to list
		this->sel_cl_devices.push_back(this->det_cl_devices[cl_dev_index]);
		return true;
	}
}

/*
Adds all available OpenCL devices to list of devices which will compute.
*/
void OpenCLManager::add_all_cl_dev()
{
	this->sel_cl_devices = this->det_cl_devices;
	//this->sel_cl_devices.pop_back();
}

/*
Prepares all added OpenCL devices for operation. Ie. setups device context, builds required programs, setups kernels + buffers etc.
*/
void OpenCLManager::setup_added_dev() {
	this->setup_dev_contexts();
	this->build_req_cl_programs();
	this->setup_cl_kernels();
	this->setup_cl_queues();
	this->alloc_cl_buffers();
}

/*
Prints all available OpenCL devices on particular computer to screen. Useful for informing user - if wrong device name is entered, show available devices.
*/
void OpenCLManager::print_avail_cl_devs() {
	std::cout << "***List of available OpenCL devices***" << std::endl;
	for (int i = 0; i < this->det_cl_devices.size(); i++) {
		std::cout << "available: " << this->det_cl_devices[i].getInfo<CL_DEVICE_NAME>().c_str() << ", version: " << this->det_cl_devices[i].getInfo<CL_DEVICE_VERSION>().c_str() << std::endl;
	}
}

/*
Prints error related to OpenCL devices. Creation of buffers etc.
cl_int cl_err = specific OpenCL error code
std::string message = message wehich should be printed to user
*/
void OpenCLManager::print_err(cl_int cl_err, std::string message) {
	if (cl_err != CL_SUCCESS) {
		std::cout << message << std::endl;
	}
}

/*
Getter for computing devices vector.
*/
std::vector<cl_dev_stuff_struct> OpenCLManager::get_compute_cl_devices()
{
	return this->compute_cl_devices;
}
