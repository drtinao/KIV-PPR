#include "cl_defines.h"
#include <iostream>
#include <fstream>
#include "FileHelper.h"
#include <cstdint>
#include <filesystem>

/*
Constructor takes name of file which should be parsed. File should contain 64bit doubles.
std::string file_name = name of file to parse
*/
FileHelper::FileHelper(std::string file_name) {
    this->file_name = file_name;
}

/*
Opens file for reading (in binary mode).
return = true if file opened successfully, else false (wrong permissions / not existing file etc.)
*/
bool FileHelper::open_file_read() {
    this->input_file_pointer = fopen(this->file_name.c_str(), FILE_READ_MODE); //open file for reading in binary mode
    if (this->input_file_pointer == NULL) { //error while opening file in bin mode
        return false;
    }
    else { //file opened successfully
        return true;
    }
}

/*
Closes input file which was previously opened for reading purposes.
*/
bool FileHelper::close_file_read() {
    if (fclose(this->input_file_pointer) != 0){ //error while closing file
        return false;
    }
    else { //file closed ok
        return true;
    }
}

/*
Reads part of file specified by arguments into buffer and returns vector object with desired content.
size_t start_offset = file offset from which reading should be performed
size_t number_count = number of doubles which should be retrieved from file; read sizeof(double) * number_count
return = vector with data retrieved from file
*/
std::vector<double> FileHelper::read_part_file(size_t start_offset, size_t number_count) {
    std::vector<double> byte_buffer(number_count, 0);
    fseek(input_file_pointer, static_cast<long>(start_offset), SEEK_SET); //move file pointer to desired position
    fread(&byte_buffer[0], sizeof(double), number_count, input_file_pointer);

    return byte_buffer;
}

/*
Determines size of input file. Size is used later for calculation of total read count from file.
*/
std::uintmax_t FileHelper::deter_file_size() {
    std::uintmax_t size = std::filesystem::file_size(this->file_name);
    return size;
}

/*
Checks if given number is considered as valid in terms of semestral project. Ie. function std::fpclassify(num) returns FP_NORMAL or FP_ZERO.
double num = number from dataset to check
*/
bool FileHelper::is_valid_num(double num)
{
    int input_num_class = std::fpclassify(num);

    if (input_num_class == FP_NORMAL || input_num_class == FP_ZERO) {
        return true;
    }
    else {
        return false;
    }
}

/*
Returns name of file using which was instance created.
*/
std::string FileHelper::get_file_name()
{
    return this->file_name;
}
