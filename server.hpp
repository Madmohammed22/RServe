#ifndef SERVER_HPP
#define SERVER_HPP

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
#include <map>
#include <vector>
#include <fstream>
#include <stack>
#include <poll.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <vector>
#include <sys/epoll.h>
#include <fcntl.h>
#include <string>


#define CHUNK_SIZE 1024
#define MAX_EVENTS 10

// curl -i -X GET http://localhost:4221/index.html


class Server
{
public:
    Server();
    Server(const Server& Init);
    Server& operator=(const Server& Init);
    ~Server();

public:
    int establishingServer(Server *server);

public:
    std::string parsRequest(std::string request);
    std::string parsRequest404(std::string request);
    std::string getContentType(const std::string &path);
    std::string creatHttpResponse(std::string contentType);
    std::string creatHttpResponseForPage404(std::string contentType, std::ifstream &file, std::string& s_content);
    std::string Server::parseRequest(std::string request);
    std::string Server::createHttpResponse(std::string contentType);
};

class Reader{
private :
    std::vector<std::string> buffer;

public:
    Reader();
    Reader(const Reader& Init);
    Reader& operator=(const Reader& Init);
    ~Reader();

public:
    bool TheBalancedParentheses(std::string file);
};


#endif

