/*
    Test harness for Levenshtein MySQL UDF.

    30 July 2019

    <hr>

    Copyright (C) 2019 Robert Jacobson. Released under the MIT license.

    Based on "Iosifovich", Copyright (C) 2019 Frederik Hertzum, which is
    licensed under the MIT license: https://bitbucket.org/clearer/iosifovich.

    The MIT License

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/


// We do not have `#pragma once` because we need to be able to include this file
// multiple times.

#include <iostream>

#include "../common.h"

#ifndef LEV_FUNCTION
#define LEV_FUNCTION damlev
#endif

/*
 * Concatenate preprocessor tokens A and B without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#ifndef PPCAT_NX
#define PPCAT_NX(A, B) A ## B
#endif
/*
 * Concatenate preprocessor tokens A and B after macro-expanding them.
 */
#ifndef PPCAT
#define PPCAT(A, B) PPCAT_NX(A, B)
#endif

#define LEV_ARGS PPCAT(LEV_FUNCTION,args)
#define LEV_INITID PPCAT(LEV_FUNCTION,initid)
#define LEV_MESSAGE PPCAT(LEV_FUNCTION,message)
#define LEV_CALL PPCAT(LEV_FUNCTION,_call)
#define LEV_INIT PPCAT(LEV_FUNCTION,_init)
#define LEV_DEINIT PPCAT(LEV_FUNCTION,_deinit)
#define LEV_SETUP PPCAT(LEV_FUNCTION,_setup)
#define LEV_TEARDOWN PPCAT(LEV_FUNCTION,_teardown)

// Use a "C" calling convention.
extern "C" {
bool LEV_INIT(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long LEV_FUNCTION(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void LEV_DEINIT(UDF_INIT *initid);
}

UDF_ARGS *LEV_ARGS;
UDF_INIT *LEV_INITID;
char *LEV_MESSAGE;

void LEV_SETUP(){
    LEV_ARGS = new UDF_ARGS;
    LEV_INITID = new UDF_INIT;
    LEV_MESSAGE = new char[512];

    // typedef struct st_udf_args
    // {
    //     unsigned int arg_count;		/* Number of arguments */
    //     enum Item_result *arg_type;		/* Pointer to item_results */
    //     char **args;				/* Pointer to argument */
    //     unsigned long *lengths;		/* Length of string arguments */
    //     char *maybe_null;			/* Set to 1 for all maybe_null args */
    // } UDF_ARGS;

    LEV_ARGS->arg_type = new Item_result[3];
    LEV_ARGS->args = new char*[3];
    LEV_ARGS->lengths = new unsigned long[3];
    LEV_ARGS->arg_count = 3;
    LEV_ARGS->arg_type[0] = STRING_RESULT;
    LEV_ARGS->arg_type[1] = STRING_RESULT;
    LEV_ARGS->arg_type[2] = INT_RESULT;

    // Don't forget to set args and lengths;

    // typedef struct st_udf_init
    // {
    //     my_bool maybe_null;			/* 1 if function can return NULL */
    //     unsigned int decimals;		/* for real functions */
    //     unsigned int max_length;		/* For string functions */
    //     char	  *ptr;				/* free pointer for function data */
    //     my_bool const_item;			/* 0 if result is independent of arguments */
    // } UDF_INIT;

    // Nothing to init for UDF_INIT.
    bool result = LEV_INIT(LEV_INITID, LEV_ARGS, LEV_MESSAGE);
    if(result == 1){
        std::cout << LEV_MESSAGE << std::endl;
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

// long long LEV_CALL(std::string_view subject, std::string_view query, long long max){
long long LEV_CALL(char * subject, size_t subject_len, char *query, size_t query_len, long long
max){
    long long result;

    LEV_ARGS->args[0] = subject;
    LEV_ARGS->lengths[0] = subject_len;
    LEV_ARGS->args[1] = query;
    LEV_ARGS->lengths[1] = query_len;
    LEV_ARGS->args[2] = (char *)(&max);
    LEV_ARGS->lengths[2] = sizeof(max);

    result = LEV_FUNCTION(LEV_INITID, LEV_ARGS, 0, LEV_MESSAGE);

    return result;
}










