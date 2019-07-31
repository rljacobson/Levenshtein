/*

    This testcases file has been adapted from:

    Iosifovich - A blazingly fast and efficient Levenshtein distance function.

    Created by Frederik Hertzum on 18 June 2018.

    18 January 2019 - Robert Jacobson:
        * Refactored build system to use CMake.
        * Used std::mismatch with the reverse iterators rbegin/rend to find common suffix.
        * Added transpositions.
        * The function now  takes two `string_view`'s instead of `string`'s
        * Preallocated the buffer

    19 January 2019 - Robert Jacobson:
        * Implemented a common early bailout optimization: If the caller only cares about edit distance less than k, we only need to look at the range i - k <= j <= i + k, and if the min of values in that range is >= k, we return k (or infinity).

    21 January 2019 - Frederik Hertzum:
        * Relicensed under the MIT license.

    21 January 2019 - Robert Jacobson:
        * Actually, more is true. One can actually restrict attention only to the range i - k/2 <= j <= i + k/2

    30 July 2019 - Robert Jacobson:
        * Adapted to the Levenshtein MySQL UDF implementation.


    Copyright (c) 2019 Frederik Hertzum

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


#ifndef LEV_FUNCTION
#define LEV_FUNCTION damlevconst
#endif

#include "testharness.hpp"



#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("empty strings are distance 0")
{
	REQUIRE(LEV_CALL("", "") == 0);
}


TEST_CASE("identical strings are distance 0")
{
	REQUIRE(LEV_CALL("", "") == 0);
	REQUIRE(LEV_CALL("-", "-") == 0);
	REQUIRE(LEV_CALL("--", "--") == 0);
	REQUIRE(LEV_CALL("---", "---") == 0);
	REQUIRE(LEV_CALL("----", "----") == 0);
	REQUIRE(LEV_CALL("-----", "-----") == 0);
}


TEST_CASE("empty strings are distance n from strings of length n")
{
	REQUIRE(LEV_CALL("", "-") == 1);
	REQUIRE(LEV_CALL("", "--") == 2);
	REQUIRE(LEV_CALL("", "---") == 3);
	REQUIRE(LEV_CALL("", "----") == 4);
	REQUIRE(LEV_CALL("", "-----") == 5);
	
	
	REQUIRE(LEV_CALL("-", "") == 1);
	REQUIRE(LEV_CALL("--", "") == 2);
	REQUIRE(LEV_CALL("---", "") == 3);
	REQUIRE(LEV_CALL("----", "") == 4);
	REQUIRE(LEV_CALL("-----", "") == 5);
}


TEST_CASE("strings with one inserted character has a distance of one")
{
	REQUIRE(LEV_CALL("", "-") == 1);
	REQUIRE(LEV_CALL("-", "-x") == 1);
	REQUIRE(LEV_CALL("--", "-x-") == 1);
	REQUIRE(LEV_CALL("---", "-x--") == 1);
	REQUIRE(LEV_CALL("----", "--x--") == 1);
	
	
	REQUIRE(LEV_CALL("-", "") == 1);
	REQUIRE(LEV_CALL("-x", "-") == 1);
	REQUIRE(LEV_CALL("-x-", "--") == 1);
	REQUIRE(LEV_CALL("-x--", "---") == 1);
	REQUIRE(LEV_CALL("--x--", "----") == 1);
}


TEST_CASE("strings with one prepended character has a distance of 1")
{
	REQUIRE(LEV_CALL("x", "") == 1);
	REQUIRE(LEV_CALL("x-", "-") == 1);
	REQUIRE(LEV_CALL("x--", "--") == 1);
	REQUIRE(LEV_CALL("x---", "---") == 1);
	REQUIRE(LEV_CALL("x----", "----") == 1);
	
	REQUIRE(LEV_CALL("", "x") == 1);
	REQUIRE(LEV_CALL("-", "x-") == 1);
	REQUIRE(LEV_CALL("--", "x--") == 1);
	REQUIRE(LEV_CALL("---", "x---") == 1);
	REQUIRE(LEV_CALL("----", "x----") == 1);
}


TEST_CASE("strings with one appended character has a distance of 1")
{
	REQUIRE(LEV_CALL("x", "") == 1);
	REQUIRE(LEV_CALL("-x", "-") == 1);
	REQUIRE(LEV_CALL("--x", "--") == 1);
	REQUIRE(LEV_CALL("---x", "---") == 1);
	REQUIRE(LEV_CALL("----x", "----") == 1);
	
	REQUIRE(LEV_CALL("", "x") == 1);
	REQUIRE(LEV_CALL("-", "-x") == 1);
	REQUIRE(LEV_CALL("--", "--x") == 1);
	REQUIRE(LEV_CALL("---", "---x") == 1);
	REQUIRE(LEV_CALL("----", "----x") == 1);
}


TEST_CASE("strings with two appended character has a distance of 2")
{
	REQUIRE(LEV_CALL("xx", "") == 2);
	REQUIRE(LEV_CALL("-xx", "-") == 2);
	REQUIRE(LEV_CALL("--xx", "--") == 2);
	REQUIRE(LEV_CALL("---xx", "---") == 2);
	REQUIRE(LEV_CALL("----xx", "----") == 2);
	
	REQUIRE(LEV_CALL("", "xx") == 2);
	REQUIRE(LEV_CALL("-", "-xx") == 2);
	REQUIRE(LEV_CALL("--", "--xx") == 2);
	REQUIRE(LEV_CALL("---", "---xx") == 2);
	REQUIRE(LEV_CALL("----", "----xx") == 2);
}


TEST_CASE("strings with one appended and one prepended character has a distance of 2")
{
	REQUIRE(LEV_CALL("xx", "") == 2);
	REQUIRE(LEV_CALL("x-x", "-") == 2);
	REQUIRE(LEV_CALL("x--x", "--") == 2);
	REQUIRE(LEV_CALL("x---x", "---") == 2);
	REQUIRE(LEV_CALL("x----x", "----") == 2);

	REQUIRE(LEV_CALL("", "xx") == 2);
	REQUIRE(LEV_CALL("-", "x-x") == 2);
	REQUIRE(LEV_CALL("--", "x--x") == 2);
	REQUIRE(LEV_CALL("---", "x---x") == 2);
	REQUIRE(LEV_CALL("----", "x----x") == 2);
}


TEST_CASE("strings with one appended, one inserted and one prepended character has a distance of 3")
{
	REQUIRE(LEV_CALL("xxx", "") == 3);
	REQUIRE(LEV_CALL("x-xx", "-") == 3);
	REQUIRE(LEV_CALL("x-x-x", "--") == 3);
	REQUIRE(LEV_CALL("x-x--x", "---") == 3);
	REQUIRE(LEV_CALL("x--x--x", "----") == 3);

	REQUIRE(LEV_CALL("", "xxx") == 3);
	REQUIRE(LEV_CALL("-", "x-xx") == 3);
	REQUIRE(LEV_CALL("--", "x-x-x") == 3);
	REQUIRE(LEV_CALL("---", "x-x--x") == 3);
	REQUIRE(LEV_CALL("----", "x--x--x") == 3);
}


