#include <iostream>
#include <string>

#define xstr(s) str(s)
#define str(s) #s

// #define LEV_FUNCTION damlevlim
#define PRINT_DEBUG
#include "testharness.hpp"


int main(int argc, char *argv[]) {
    // std::string a {"haupt"};
    // std::string b {"hautp"};
    std::string a {"or"};
    std::string b {"ro"};
    long long result = 0;

    LEV_SETUP();

    result = LEV_CALL(const_cast<char*>(a.c_str()), a.size(), const_cast<char*>(b.c_str()), b.size
    (), 17);

    std::cout << xstr(LEV_FUNCTION) "(" << a << ", " << b << ") =  " << result << std::endl;

    LEV_TEARDOWN();
}
