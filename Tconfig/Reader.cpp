#include "Reader.hpp"

Reader::Reader()
{
    // std::cout << "[Reader] constructor is called" << std::endl;
}

Reader::Reader(const Reader& Init){
    (void)Init;
    // std::cout << "[Reader] parametrize constructor is called" << std::endl;
}

Reader& Reader::operator=(const Reader& Init){
    if (this == &Init)
        return *this;
}
Reader::~Reader()
{
    // std::cout << "[Reader] distructor is called" << std::endl;
}