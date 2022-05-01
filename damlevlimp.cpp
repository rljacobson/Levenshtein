/*
    Damerau–Levenshtein Edit Distance UDF for MySQL.

    17 January 2019

    This implementation is better than most others out there. It is extremely
    fast and efficient.
            __—R.__

    <hr>
    `DAMLEVLIMP()` computes the normalized Damarau Levenshtein edit distance between two
    strings, or a given number p, whichever is smaller.
    The normalization is the edit distance divided by the length of the longest string:
        ("edit distance")/("length of longest string").

    Syntax:

        DAMLEVLIMP(String1, String2, p);

    `String1`:  A string constant or column.
    `String2`:  A string constant or column to be compared to `String1`.
          `p`:  A number in the range [0,1] inclusive. If the distance between `String1` and
                `String2` is greater than `p`, `DAMLEVLIMP()` will stop its
                computation and return `p`. Make `p` as
                small as you can to improve speed and efficiency. For example,
                if you put `WHERE DAMLEVLIMP(...) < t` in a `WHERE`-clause, make
                `p` be `t`.

    Returns: Either a (floating point) number equal to the normalized edit distance between 
    `String1` and `String2` or `p`, whichever is smaller.

    Example Usage:

        SELECT Name, DAMLEVP(Name, "Vladimir Iosifovich Levenshtein", 0.2) AS
            EditDist FROM CUSTOMERS WHERE EditDist < 0.2;

    The above will return all rows `(Name, EditDist)` from the `CUSTOMERS` table
    where `Name` has edit distance within 20% of "Vladimir Iosifovich Levenshtein".

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

#include "common.h"

//#define PRINT_DEBUG
#ifdef PRINT_DEBUG
#include <iostream>
#endif

// Limits
#ifndef DAMLEVLIMP_BUFFER_SIZE
    // 640k should be good enough for anybody.
    #define DAMLEVLIMP_BUFFER_SIZE 512ull
#endif
constexpr long long DAMLEVLIMP_MAX_EDIT_DIST = std::max(0ull,
        std::min(16384ull, DAMLEVLIMP_BUFFER_SIZE));

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
        DAMLEVLIMP_ARG_NUM_ERROR[] = "Wrong number of arguments. DAMLEVLIMP() requires three arguments:\n"
                                 "\t1. A string\n"
                                 "\t2. A string\n"
                                 "\t3. A maximum percent difference (0.0 <= float < 1.0)";
constexpr const auto DAMLEVLIMP_ARG_NUM_ERROR_LEN = std::size(DAMLEVLIMP_ARG_NUM_ERROR) + 1;
constexpr const char DAMLEVLIMP_MEM_ERROR[] = "Failed to allocate memory for DAMLEVLIMP"
                                          " function.";
constexpr const auto DAMLEVLIMP_MEM_ERROR_LEN = std::size(DAMLEVLIMP_MEM_ERROR) + 1;
constexpr const char
        DAMLEVLIMP_ARG_TYPE_ERROR[] = "Arguments have wrong type. DAMLEVLIMP() requires three arguments:\n"
                                  "\t1. A string\n"
                                  "\t2. A string\n"
                                  "\t3. A maximum percent difference (0.0 <= float < 1.0)";
constexpr const auto DAMLEVLIMP_ARG_TYPE_ERROR_LEN = std::size(DAMLEVLIMP_ARG_TYPE_ERROR) + 1;

// Use a "C" calling convention.
extern "C" {
bool damlevlimp_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
double damlevlimp(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlevlimp_deinit(UDF_INIT *initid);
}

bool damlevlimp_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, DAMLEVLIMP_ARG_NUM_ERROR, DAMLEVLIMP_ARG_NUM_ERROR_LEN);
        return 1;
    }
        // The arguments needs to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT ||
            args->arg_type[2] != DECIMAL_RESULT) {
        strncpy(message, DAMLEVLIMP_ARG_TYPE_ERROR, DAMLEVLIMP_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Attempt to allocate a buffer.
    initid->ptr = (char *)new(std::nothrow) std::vector<size_t>(DAMLEVLIMP_MAX_EDIT_DIST);
    if (initid->ptr == nullptr) {
        strncpy(message, DAMLEVLIMP_MEM_ERROR, DAMLEVLIMP_MEM_ERROR_LEN);
        return 1;
    }

    // damlevlimp does not return null.
    initid->maybe_null = 0;
    return 0;
}

void damlevlimp_deinit(UDF_INIT *initid) {
    delete[] initid->ptr;
}

double damlevlimp(UDF_INIT *initid, UDF_ARGS *args, UNUSED char *is_null, UNUSED char *error) {
    // Retrieve the arguments.
    // Maximum edit distance.
    //TODO 3rd argument: args->args[2] is not being inputted.
    #ifdef PRINT_DEBUG

    std::cout <<" 3rd argument:"<< *(double *)(args->args[2])<<std::endl;
    std::cout << "Maximum edit distance:" <<  (int *)args->args[2]<<std::endl;
    std::cout << "DAMLEVLIM_MAX_EDIT_DIST:" <<DAMLEVLIMP_MAX_EDIT_DIST<<std::endl;
    std::cout << "Max String Length:" << static_cast<double>(std::max(args->lengths[0],
                                                                      args->lengths[1]))<<std::endl;
    #endif
    long long max_string_length = static_cast<double>(std::max(args->lengths[0],
                                                                  args->lengths[1]));
    auto max_p = *((double *)args->args[2]);
    std::cout << "max_p: " << max_p << std::endl;
    //auto max = roundl(max_p*max_string_length);

    auto max = std::min(static_cast<long long>(roundl(max_p*max_string_length)), DAMLEVLIMP_MAX_EDIT_DIST);

    std::cout << "max: " << max << std::endl;

    if (max == 0.0) {
        std::cout<<"max is 0.0, therefore returning: "<<static_cast<double>(max)<<std::endl;
        //TODO: this is not returning correct, returns a very large number  1.40462e+14
        return static_cast<double>(0.0);
    }
    // Check the arguments.
    if (args->lengths[0] == 0 || args->lengths[1] == 0 || args->args[1] == nullptr
            || args->args[0] == nullptr) {
        // Either one of the strings doesn't exist, or one of the strings has
        // length zero. In either case
        return 1.0;
    }
    // Save the original max string length for the normalization when we return.

    // Retrieve buffer.
    std::vector<size_t> &buffer = *(std::vector<size_t> *)initid->ptr;

    // Let's make some string views so we can use the STL.
    std::string_view subject{args->args[0], args->lengths[0]};
    std::string_view query{args->args[1], args->lengths[1]};


    // Skip any common prefix.
    auto[subject_begin, query_begin] =
    std::mismatch(subject.begin(), subject.end(), query.begin(), query.end());
    auto start_offset = (size_t)std::distance(subject.begin(), subject_begin);

    // If one of the strings is a prefix of the other, done.
    if (subject.length() == start_offset) {
#ifdef PRINT_DEBUG
        std::cout << subject << " is a prefix of " << query << ", bailing" << std::endl;
#endif
        return (size_t)(query.length() - start_offset);
    } else if (query.length() == start_offset) {
#ifdef PRINT_DEBUG
        std::cout << query << " is a prefix of " << subject << ", bailing" << std::endl;
#endif
        return (size_t)(subject.length() - start_offset);
    }
    // Skip any common suffix.
    auto[subject_end, query_end] = std::mismatch(
            subject.rbegin(), static_cast<decltype(subject.rend())>(subject_begin - 1),
            query.rbegin(), static_cast<decltype(query.rend())>(query_begin - 1));
    auto end_offset = std::min((size_t)std::distance(subject.rbegin(), subject_end),
                               (size_t)(subject.size() - start_offset));

    // Take the different part.
    subject = subject.substr(start_offset, subject.size() - end_offset - start_offset);
    query = query.substr(start_offset, query.size() - end_offset - start_offset);

    int trimmed_max = std::max(int(query.length()),int(subject.length()));
#ifdef PRINT_DEBUG
    std::cout << "trimmed max length:" <<trimmed_max<<std::endl;
    std::cout << "trimmed subject= " <<subject <<std::endl;
    std::cout <<"trimmed constant query= " <<query<<std::endl;
#endif

    // Make "subject" the smaller one.
    if (query.length() < subject.length()) {

        std::swap(subject, query);

    }


    // If one of the strings is a suffix of the other.
    if (subject.length() == 0) {
#ifdef PRINT_DEBUG
        std::cout << subject << " is a suffix of " << query << ", bailing" << std::endl;
#endif
        return query.length();
    }


    buffer.resize(trimmed_max+1);



    // Init buffer.
    std::iota(buffer.begin(), buffer.begin() + query.length() +1, 0);

#ifdef PRINT_DEBUG
    unsigned i = 0;
    std::cout <<"    ";
    for (auto a: query){
        if (i <10) std::cout << a << " ";
        else std::cout <<" "<< a<<" ";
        i++;
    }
    std::cout <<std::endl;
    std::cout <<"  ";
    for (auto a: buffer)
        std::cout << a << ' ';
    std::cout <<std::endl;
#endif
    size_t end_j; // end_j is referenced after the loop.
    size_t j;
    //this for makes the vertical direction.
    for (size_t i = 1; i < (subject.length() + 1); ++i) {


        // temp is what we're calling the current value.
        size_t temp = buffer[0]++; // counts up 1,2,3 ....


        // prior_temp is used to give us the UP-LEFT value.
        // The first UP-LEFT is always 0.
        size_t prior_temp;
        if (i ==1) {
            prior_temp = 0 ;}




        /* We don't need all the row data. We only needs to look in a window around answer it is
         * between i-max <= j <= i+max
         * The result of the max is positive,
         * but we need the second argument to be allowed to be negative.
         * Recall all the trimming we did, variable 'max' = trimmed_max
         */

        const size_t start_j = static_cast<size_t>(std::max(1ll, static_cast<long long>(i) -
                                                                 trimmed_max/2-1));
        end_j = std::min(static_cast<size_t>(query.length()+1),
                         static_cast<size_t >(i + trimmed_max/2)+1);

        size_t column_min = trimmed_max;     // Sentinels

        // this loop makes the horizontal data.
        for (size_t j = start_j; j < end_j; ++j) {
            //for (size_t j = 1; j < 6; ++j) {


            /*          a b c d
             *       0  1 2 3 4
             *    a  1  0 1 2 3
             *    b  2  1 0 1 2
             *    c  3  2 1 0 1
             *    d  4  3 2 1 0
             *
             * We only need three items to calculate the edit_distance.
             *
             * LEFT, UP, UP-LEFT (Diagonal up to the left).
             *
             * By rule, if the letters are the same  a = a then the answer is always UP-LEFT.
             *
             * Robert has decided to accomplish this with a single vector called: buffer.
             * Most edit_distance methods use the entire matrix or two rows.  This is a single row approach.
             * We can access the previous rows' data that hasn't been overwritten yet.
             *
             * UP: buffer[j] is the previous rows' data directly above the current
             * position, j
             *
             * LEFT: buffer[j-1] is the previous value to the left, same row,
             * of the current position j.
             *
             * UP-LEFT prior_temp is from the previous row left, we set it at the first for loop.
             * This gives us the left position of the previous rows, which is upper left from the current.
             *
             *
             */

            const size_t r = buffer[j]; // UP
            const size_t p = buffer[j-1]; // LEFT

            //std::cout <<"prior_temp: "<<prior_temp<<" r: "<<r<<" sub: "<<subject[i-1] <<" #";

            size_t cost; // need to set a cost of a mistake for transposition below.
            // are the values the same?
            if (subject[i-1] == query[j-1]) {
                temp = prior_temp; //UPPER LEFT
                cost =0;  // same, zero cost


            }

            else  {
                cost =1; // different cost of 1, this could be set to be something else.

                // answer is always the min of the three values we have.
                temp = std::min(std::min(r+ 1,   //  UP
                                         p + 1   // LEFT
                                ),
                                prior_temp + 1  //UPPER LEFT
                );
            }




            // We consider transposition,
            // flipping of a pair of letters to be a cost of error not two
            if( (i > 1) &&
                (j > 1) &&
                (subject[i-1] == query[j-2]) &&
                (subject[i-2] == query[j-1])
                    )
            {
                temp = std::min(
                        temp + cost,
                        prior_temp   // transposition
                );

            };

            // Keep track of minimum value in each row. why is it called 'column_min'
            // This will give us the ability to return if the edit distance is larger than the max
            if (temp < column_min) {

                column_min = temp;
            }
            // this sets UPPER LEFT for next loop
            // this is not UPPER LEFT at the beginning of the loop
            prior_temp = buffer[j];
            buffer[j] = temp; // this sets UP for the next loop



        }





        // max is the maximum edit_distance, trimmed max is the max(trimmed.subject,trimmed.query)
        // the max could be longer than the possible edit distance, so it would never bail early, likely not an issue, but..
        if (column_min > max) {
            // There is no way to get an edit distance > column_min.
            // We can bail out early.

            return 1.0;


        }
        // print out the row data,
#ifdef PRINT_DEBUG

        std::cout <<subject[i-1]<<" ";


        for (auto a: buffer) {
            std::cout << a << ' ';
        }
        std::cout <<std::endl;
#endif

    }


    double ld =static_cast<double> (buffer[end_j-1])/static_cast<double> (max_string_length);


    buffer.resize(DAMLEVLIMP_MAX_EDIT_DIST);
    return ld;
}
