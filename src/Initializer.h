#pragma once
#include <string>
#include <vector>
#include "Structures.h"
#include "OpenCLManager.h"

//takes care of program initialization
class Initializer
{
	private:
		//constructor variables - START
		int argc; //count of arguments given by user
		char** argv; //array with arguments specified by user
		OpenCLManager* openCLMan; //for managing openCL devs
		//constructor variables - END

		std::string input_file_name; //name of file which is supposed to be parsed
		compute_type sel_comp_type; //desired computing type defined by user (enum compute_type)
		std::vector<std::string> sel_cl_devices; //array which contains OpenCL devices on which calculation should be performed - used only if selCompType is OPENCL / ALL

	public:
		Initializer(int argc, char** argv, OpenCLManager* openCLMan); //constructor takes just reference to given values, instances
		bool init_via_args(); //check user arguments and init program
		void print_init_info(); //prints information regarding to init
		bool is_file_available(std::string file_name); //checks if file is readable
		std::string get_input_file_name(); //name of file specified by user
		compute_type get_sel_comp_type(); //desired computing type
};

