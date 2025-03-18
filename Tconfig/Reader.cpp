#include "Reader.hpp"

ConfigurationFile::ConfigurationFile()
{
    // std::cout << "[Reader] constructor is called" << std::endl;
}

ConfigurationFile::ConfigurationFile(const ConfigurationFile& Init){
    (void)Init;
    // std::cout << "[Reader] parametrize constructor is called" << std::endl;
}

ConfigurationFile& ConfigurationFile::operator=(const ConfigurationFile& Init){
    if (this == &Init)
        return *this;
}
ConfigurationFile::~ConfigurationFile()
{
    // std::cout << "[Reader] distructor is called" << std::endl;
}