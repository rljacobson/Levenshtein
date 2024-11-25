/*

Test harness for Levenshtein MySQL UDF. This defines the data structures and
a setup, teardown, and call method that allows you to use the UDF function
from your code.

*/


// We do not have `#pragma once` because we need to be able to include this file
// multiple times.

#include "../src/common.h"

// Default definitions in case CMake does not define them.
#ifndef LEV_FUNCTION
#define LEV_FUNCTION damlevlim
#endif

// This should be unconditional, as it may be a redefinition.
#define LEV_ALGORITHM_NAME AS_STRING(LEV_FUNCTION)

#ifndef LEV_ALGORITHM_COUNT
// Keep in synch with "default" LEV_FUNCTION
#define LEV_ALGORITHM_COUNT 3
#endif

// The following avoids name collisions
#define LEV_ARGS MACRO_CONCAT(LEV_FUNCTION,args)
#define LEV_INITID MACRO_CONCAT(LEV_FUNCTION,initid)
#define LEV_MESSAGE MACRO_CONCAT(LEV_FUNCTION,message)
#define LEV_CALL MACRO_CONCAT(LEV_FUNCTION,_call)
#define LEV_INIT MACRO_CONCAT(LEV_FUNCTION,_init)
#define LEV_DEINIT MACRO_CONCAT(LEV_FUNCTION,_deinit)
#define LEV_SETUP MACRO_CONCAT(LEV_FUNCTION,_setup)
#define LEV_TEARDOWN MACRO_CONCAT(LEV_FUNCTION,_teardown)

UDF_SIGNATURES(LEV_FUNCTION)

UDF_ARGS *LEV_ARGS;
UDF_INIT *LEV_INITID;
char *LEV_MESSAGE;

void LEV_SETUP(){
    LEV_ARGS = new UDF_ARGS;
    LEV_INITID = new UDF_INIT;
    LEV_MESSAGE = new char[512];

    // Initialize UDF_ARGS
    LEV_ARGS->arg_type    = new Item_result[LEV_ALGORITHM_COUNT];
    LEV_ARGS->args        = new char*[LEV_ALGORITHM_COUNT];
    LEV_ARGS->lengths     = new unsigned long[2]; // There are only ever two strings
    LEV_ARGS->arg_count   = LEV_ALGORITHM_COUNT;
    LEV_ARGS->arg_type[0] = STRING_RESULT;
    LEV_ARGS->arg_type[1] = STRING_RESULT;
#if LEV_ALGORITHM_COUNT==3
    LEV_ARGS->arg_type[2] = INT_RESULT;
#endif
    // LEV_ARGS->arg_type[2] = DECIMAL_RESULT;
    // Initialize UDF_INIT
    int result = LEV_INIT(LEV_INITID, LEV_ARGS, LEV_MESSAGE);
    if(result == 1){
        std::cout << "Initialization Message: " << LEV_MESSAGE << std::endl;
    }
}

void LEV_TEARDOWN(){
    LEV_DEINIT(LEV_INITID);

    delete[] LEV_ARGS->arg_type;
    if(nullptr != LEV_ARGS->args){
        delete[] LEV_ARGS->args;
    }
    if(nullptr != LEV_ARGS->lengths){
        delete[] LEV_ARGS->lengths;
    }
    delete LEV_INITID;
    delete[] LEV_MESSAGE;
}

// The following is for two argument algorithms
#if LEV_ALGORITHM_COUNT==2
long long LEV_CALL(char *subject, size_t subject_len, char *query, size_t query_len){
#else
long long LEV_CALL(char *subject, size_t subject_len, char *query, size_t query_len, long long max){
#endif
    long long result;

    LEV_ARGS->args[0] = subject;
    LEV_ARGS->lengths[0] = subject_len;
    LEV_ARGS->args[1] = query;
    LEV_ARGS->lengths[1] = query_len;

#if LEV_ALGORITHM_COUNT==3
    LEV_ARGS->args[2] = (char *)(&max);
#endif

    result = LEV_FUNCTION(LEV_INITID, LEV_ARGS, nullptr, LEV_MESSAGE);

    return result;
}
