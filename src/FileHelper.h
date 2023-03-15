#pragma once
#pragma warning(disable:4996)
#define FILE_READ_MODE "rb"
#include <vector>

//tools regarding to file read
class FileHelper {
    private:
        //constructor variables - START
        std::string file_name; //name of parsed file
        //constructor variables - END

        FILE* input_file_pointer; //pointer to file which should be parsed

    public:
        FileHelper(std::string file_name); //constructor expects just name of the file to read
        bool open_file_read(); //opens in rb mode
        bool close_file_read(); //closes file
        std::vector<double> read_part_file(size_t start_offset, size_t number_count); //read specified part of the file
        std::uintmax_t deter_file_size(); //gets file size
        bool is_valid_num(double num); //check if number is considered as valid (std::fpclassify is FP_NORMAL / FP_ZERO)
        std::string get_file_name(); //gets name of the file
};