#include "cl_defines.h"
#include "Initializer.h"
#include "FileHelper.h"
#include "OpenCLManager.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "Farmer.h"

/*
Constructor accepts values specified by user at program execution.
char argc = count of arguments
char** argv = array with given arguments
OpenCLManager* openCLMan = manages OpenCL devices
*/
Initializer::Initializer(int argc, char** argv, OpenCLManager* openCLMan)
{
	this->argc = argc;
	this->argv = argv;
	this->openCLMan = openCLMan;
	this->sel_comp_type = compute_type::ALL;
}

/*
Initializes class variables which are related to input file info + computing type. Uses input arguments from user.
Validity of program arguments is checked before initialization. Program expects at least 3 arguments - program name + path to file + computing type.
returns = true if arguments are valid (init successful), else false
*/
bool Initializer::init_via_args()
{
	if (this->argc < 3) { //checks args count, must be >= 3
		std::cout << "ERROR: Invalid number of arguments. Usage: \"pprsolver.exe file processor[all | SMP | opencl_device_name]\"";
		return false;
	}

	if(is_file_available(argv[1]) == false) { //args count ok, check file existence
		std::cout << "ERROR: File with name " << argv[1] << " does not exist!";
		return false;
	}
	else {
		this->input_file_name = argv[1];
	}

	if (strcmp(argv[2], "SMP") != 0 && strcmp(argv[2], "smp") != 0) { //skip scan on SMP
		this->openCLMan->scan_cl_devs();
	}

	//args count ok, file exists ok, check computing type validity - START
	if (argc == 3) { //valid only: "all" or "SMP" or "opencl_device_name(s)"
		if (strcmp(argv[2], "all") == 0 || strcmp(argv[2], "ALL") == 0) { //calculate on all avail. devices
			this->sel_comp_type = compute_type::ALL;
			this->openCLMan->add_all_cl_dev();
			this->openCLMan->setup_added_dev();
		}
		else if (strcmp(argv[2], "SMP") == 0 || strcmp(argv[2], "smp") == 0) { //calculate on more CPU threads
			this->sel_comp_type = compute_type::SMP;
		}
		else if (openCLMan->add_sel_cl_dev(argv[2]) == true) { //calculate on specified OpenCL devices
			this->sel_comp_type = compute_type::OPENCL;
			this->sel_cl_devices.push_back(argv[2]);
			this->openCLMan->setup_added_dev();
		}
		else {
			//check whether third argument contain opencl in quotation marks - ie. "opencl1 opencl2 openclx" - START
			std::string pos_cl_dev; //token from third string argument (delimiter is whitespace)
			std::istringstream string_str(argv[2]); //convert input with desired computing devices into istringstream object
			while (std::getline(string_str, pos_cl_dev, ' ')) { //go through splitted third argument
				if (this->openCLMan->add_sel_cl_dev(pos_cl_dev) == false) { //at least one device is not valid OpenCL device, abort...
					std::cout << "ERROR: Device \"" << pos_cl_dev << "\" is not valid OpenCL device! Usage: \"pprsolver.exe file processor[all | SMP | opencl_device_name]\"" << std::endl;
					this->openCLMan->print_avail_cl_devs();
					return false;
				}
				this->sel_cl_devices.push_back(pos_cl_dev);
			}
			this->sel_comp_type = compute_type::OPENCL;
			this->openCLMan->setup_added_dev();
			//check whether third argument contain opencl in quotation marks - ie. "opencl1 opencl2 openclx" - END
		}
	}
	else { //argc > 3, expect more OpenCL devices...
		for (int i = 2; i < argc; i++) { //check validity for each OpenCL device
			if (this->openCLMan->add_sel_cl_dev(argv[i]) == false) {
				std::cout << "ERROR: Device \"" << argv[i] << "\" is not valid OpenCL device! Usage: \"pprsolver.exe file processor[all | SMP | opencl_device_name]\"" << std::endl;
				this->openCLMan->print_avail_cl_devs();
				return false;
			}
			this->sel_cl_devices.push_back(argv[i]);
		} //all listed OpenCL devices valid and present in system, assign computing type
		this->sel_comp_type = compute_type::OPENCL;
		this->openCLMan->setup_added_dev();
	}

	//args count ok, file exists ok, check computing type validity - END
	return true;
}

/*
Prints basic information regarding to program initialization. 
Ie. name of file to be parsed + computing type and eventually OpenCL device on which calculation will be performed.
*/
void Initializer::print_init_info()
{
	std::cout << "***Program init info - START***" << std::endl;
	std::cout << "file to parse: " << this->input_file_name << std::endl;

	std::cout << "selected computing type: ";
	switch (this->sel_comp_type) {
		case ALL:
			std::cout << "all available devices" << std::endl;
			break;
		case SMP:
			std::cout << "SMP - multithread CPU" << std::endl;
			break;
		case OPENCL:
			std::cout << "listed OpenCL devices (see below)" << std::endl;
			break;
		default:
			std::cout << "ERROR - unknown computing device" << std::endl;
	}

	for (int i = 0; i < this->sel_cl_devices.size(); i++) {
		std::cout << "using OpenCL device \"" << sel_cl_devices[i] << "\"" << std::endl;
	}
	std::cout << "***Program init info - END***" << std::endl;
}

/*
Function checks whether input file with given name is present on disk and is readable.
std::string file_name = name of file to check
return = true if file exists and is readable, else false
*/
bool Initializer::is_file_available(std::string file_name)
{
	std::ifstream fileStream(file_name, std::ios::binary);
	if (fileStream.good()) {
		return true;
	}
	else {
		return false;
	}
}

/*
Returns name of file using which was instance created.
*/
std::string Initializer::get_input_file_name()
{
	return this->input_file_name;
}

/*
Getter for desired computing type.
*/
compute_type Initializer::get_sel_comp_type()
{
	return this->sel_comp_type;
}
