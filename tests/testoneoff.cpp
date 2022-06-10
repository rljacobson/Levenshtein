// HELLO LEV_ARGS needs to be adjusted if you're going to change from damlevconst
//damlevconst INT required testharness.hpp --> LEV_ARGS->arg_count = 3;
// damlev testharness.hpp --> LEV_ARGS->arg_count = 2;
//damlevlimp FLOAT required testharness.hpp LEV_ARGS->arg_count = 3 A maximum percent difference (0.0 <= float < 1.0);

#include <iostream>
#include <string>

#define xstr(s) str(s)
#define str(s) #s

//#define LEV_FUNCTION demlev
#define PRINT_DEBUG
#include "testharness.hpp"
std::string search_term {"mustifaspiata"};
std::string db_return {"multifasciatus"};
std::string term1 {"aMartha"};
std::string term2 {"bMarbtahab"};
std::string empty1 {""};
std::string empty2 {""};

int main(int argc, char *argv[]) {
    std::string b = search_term;
    std::string a = db_return;
    long long c =9;
    long long result = 0;


    LEV_SETUP();

    result = LEV_CALL(const_cast<char*>(a.c_str()),
                      a.size(),
                      const_cast<char*>(b.c_str()),
                      b.size(),
                      c
                      );

    std::cout << xstr(LEV_FUNCTION) "(" << a << ", " << b << ") =  " << result << std::endl;

    LEV_TEARDOWN();
}
