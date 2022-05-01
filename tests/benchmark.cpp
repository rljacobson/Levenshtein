#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>

// The macros below are set in the CMakeLists.txt file. We give sensible
// defaults here in case one does not want to use CMake.
#ifndef WORDS_PATH
#define WORDS_PATH "/usr/share/dict/linux.words"
#endif

#ifndef WORD_COUNT
#define WORD_COUNT 10000ul
#endif

#ifndef BENCH_FUNCTION
    #define LEV_FUNCTION BENCH_FUNCTION
#else
    #define LEV_FUNCTION damlev
#endif
#include "testharness.hpp"

#define LEV_FUNCTION damlev2D
#include "testharness.hpp"

#include "benchtime.hpp"


extern "C" size_t lasm(const char *a, size_t alen, const char * b, size_t blen);

// Magic from SE:
// https://stackoverflow.com/questions/52699244/c-fast-way-to-load-large-txt-file-in-vectorstring
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range_core.hpp>

class LineIterator:
        public boost::iterator_facade<
            LineIterator,
            boost::iterator_range<char const*>,
            boost::iterators::forward_traversal_tag,
            boost::iterator_range<char const*>
        >{
public:
    LineIterator(char const* begin, char const* end)
    : p_(begin), q_(end) {
        // pass
    }

private:
    char const *p_, *q_;
    boost::iterator_range<char const*> dereference() const {
        return {p_, this->next()};
    }
    bool equal(LineIterator b) const {
        return p_ == b.p_;
    }
    void increment() {
        p_ = this->next();
    }
    char const* next() const {
        auto p = std::find(p_, q_, '\n');
        return p + (p != q_);
    }
    friend class boost::iterator_core_access;
};
inline boost::iterator_range<LineIterator> crange(boost::interprocess::mapped_region const& r) {
    auto p = static_cast<char const*>(r.get_address());
    auto q = p + r.get_size();
    return {LineIterator{p, q}, LineIterator{q, q}};
}
inline std::ostream& operator<<(std::ostream& s, boost::iterator_range<char const*> const& line) {
    return s.write(line.begin(), line.size());
}


int main(int argc, char *argv[]) {

    std::string     words_file_path {WORDS_PATH};
    unsigned        maximum_size    {WORD_COUNT};
    boost::interprocess::file_mapping text_file;

    // Magic from SE:
    try {
        text_file = boost::interprocess::file_mapping(WORDS_PATH, boost::interprocess::read_only);
    } catch (boost::interprocess::interprocess_exception &e){
        std::cerr << "Could not open file " << words_file_path << '.' << std::endl;
        return EXIT_FAILURE;
    }
    boost::interprocess::mapped_region text_file_buffer(text_file, boost::interprocess::read_only);

    std::cout << "Opened file: " << words_file_path << "." << std::endl;

    double timediff = 0;
    unsigned line_no = 0;
    Timer timer = Timer();

    for(auto a : crange(text_file_buffer)){
        damlev_setup();
        for(auto b : crange(text_file_buffer)){
            if (a==b)
                continue;
            ++line_no;
            damlev_call((char *)b.begin(), b.size(), (char *)a.begin(), a.size(), 1);
            //std::cout <<a<<" : "<<b;
        }
        damlev_teardown();

        if(line_no > maximum_size) goto breakA;

    }
breakA:
    std::cout << "Number of words: " << line_no-1 << std::endl;

    std::cout << "-----------\nDAMLEV\n-----------\n";
    timediff = timer.elapsed();
    std::cout << "Time elapsed:" << timediff << 's' << std::endl;


    line_no = 0;
    timer.reset();

    for(auto a : crange(text_file_buffer)){
        damlev2D_setup();
        for(auto b : crange(text_file_buffer)){
            if(a==b)
                continue;
            ++line_no;
            damlev2D_call((char *)b.begin(), b.size(), (char *)a.begin(), a.size(), 1);
        }
        damlev2D_teardown();
        if(line_no > maximum_size) goto breakB;

    }
breakB:

    std::cout << "-----------\n2 row approach\n-----------\n";
    std::cout << "Time elapsed:" << timer.elapsed() << "s\n-----------\n\n";
    std::cout << "We took " << timediff - timer.elapsed() ;
    std::cout << "s longer to run DAMLEVCONST than use a 2 row approach" ;
    std::cout << " checking " << line_no-1 << " words."<<std::endl;


}
