#ifndef READER_HPP
#define READER_HPP

#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <vector>

using namespace std;

class Reader
{
private:
    std::vector<std::string> buffer;    
public:
    Reader();
    Reader(const Reader& Init);
    Reader& operator=(const Reader& Init);
    ~Reader();
};



#endif