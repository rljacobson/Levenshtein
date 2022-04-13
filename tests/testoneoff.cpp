#include <iostream>
#include <string>

#define xstr(s) str(s)
#define str(s) #s

//#define LEV_FUNCTION demlev
#define PRINT_DEBUG
#include "testharness.hpp"


int main(int argc, char *argv[]) {
    // std::string a {""};
    // std::string b {""};
    std::string a {"Acanthurus lineatus"};
    std::string b {"Acanthopagrus latus"};
    int c =3;
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
