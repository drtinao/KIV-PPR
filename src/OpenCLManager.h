#pragma once
#include "const.h"
#if __has_include(<CL/opencl.hpp>)
# include <CL/opencl.hpp>
#else
# include "opencl.hpp"
#endif
#include "Structures.h"
#include <vector>

//contains functions regarding to OpenCL devices
class OpenCLManager {
	private:
		//variables - START
		std::vector<cl::Device> det_cl_devices; //all OpenCL detected devices
		std::vector<cl::Device> sel_cl_devices; //list of OpenCL devices on which calculation should be performed
		std::vector<cl_dev_stuff_struct> compute_cl_devices; //contains struct for each OpenCL device used for computation - contains compiled programs for selected device, context etc. 
		//variables - END

		//functions - START
		std::vector<cl::Platform> retr_cl_platforms(); //gets available OpenCL platforms
		std::vector<cl::Device> retr_cl_dev_for_platf(cl::Platform cl_platform); //gets available OpenCL devices for specific platform
		int retr_cl_dev_index(std::string cl_dev_name); //checks whether device name is valid, returns index
		cl::Program build_program_from_src(const std::string program_source, cl::Device& target_device, cl::Context cl_device_context); //builds CL program from source
		void print_err(cl_int cl_err, std::string message); //prints errors related to OpenCL
		void setup_dev_contexts(); //setups CL devs context
		bool build_req_cl_programs(); //builds required CL programs for specified devices
		bool setup_cl_kernels(); //setups CL kernels for each CL dev.
		bool setup_cl_queues(); //setups CL queue for each CL dev.
		void alloc_cl_buffers(); //allocates required buffers for each CL device
		//functions - END
	public:
		void scan_cl_devs(); //performs system scan and retrieves available CL devices
		void alloc_add_nums_to_intervals_buffers(int output_intervals_count); //updates count of expected intervals for each CL dev.
		bool add_sel_cl_dev(std::string cl_dev_name); //if device name valid, adds to list of computing devices
		void add_all_cl_dev(); //adds all CL devices available in the system to list of computing devices
		void setup_added_dev(); //performs bulk setup of CL devices (context, queue, kernel...)
		void print_avail_cl_devs(); //prints available CL devices to user
		std::vector<cl_dev_stuff_struct> get_compute_cl_devices(); //returns all CL devices which are used during computation
};