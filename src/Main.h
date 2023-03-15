#pragma once
#pragma warning(disable:4996)
#include "cl_defines.h"
#include "DecisionDist.h"
#include "FileHelper.h"
#include "IntervalManager.h"
#include "ChiSquareManager.h"
#include "Farmer.h"
#include "Structures.h"

void perf_first_pass(FileHelper* fileHelper, DecisionDist* decisionDist, Farmer* farmer); //performs first pass of algorithm - dataset min / max number + valid nums count + check for negative / decimal point numbers
void print_first_pass_info(FileHelper* fileHelper, DecisionDist* decisionDist); //prints info gathered during first pass of algorithm
void perf_second_pass(FileHelper* fileHelper, IntervalManager* intervalManager, DecisionDist* decisionDist, Farmer* farmer); //performs second part of algo - sorts numbers into intervals, calc avg + std. dev.
void print_second_pass_info(IntervalManager* intervalManager, DecisionDist* decisionDist); //prints info gathered during second pass of algorithm
void perform_chi_square_calc(IntervalManager* intervalManager, DecisionDist* decisionDist, ChiSquareManager* chiSquareMan); //perform calculation using retrieved values