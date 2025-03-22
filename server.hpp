/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 03:11:18 by mmad              #+#    #+#             */
/*   Updated: 2025/03/18 03:17:22 by mmad             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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

#define PORT 8080 
#define MAX_EVENTS 10 
#define CHUNK_SIZE 8192 
// /home/mmad/Desktop/webserve/root/content/testing
#define PATHC "/home/mmad/Desktop/webserve/root/content/static/"
// #define PATHC "/home/mmad/Desktop/webserve/root/content/testing/"
#define PATHE "/home/mmad/Desktop/webserve/root/error/" 

// Structure to hold file transfer state
struct FileTransferState {
    std::string filePath;
    size_t offset;
    size_t fileSize;
    bool isComplete;
    
    FileTransferState() : offset(0), fileSize(0), isComplete(false) {}
};

class Server
{
public:
    Server();
    Server(const Server& Init);
    Server& operator=(const Server& Init);
    ~Server();
public:
    // Map to keep track of file transfers for each client
    std::map<int, FileTransferState> fileTransfers;
public:
    int establishingServer();

public:
    std::string parsRequest(std::string request);
    std::string parsRequest404(std::string request);
    std::string getContentType(const std::string &path);
};

class ConfigurationFile{
private :
    std::vector<std::string> buffer;

public:
    ConfigurationFile();
    ConfigurationFile(const ConfigurationFile& Init);
    ConfigurationFile& operator=(const ConfigurationFile& Init);
    ~ConfigurationFile();

public:
    bool TheBalancedParentheses(std::string file);
};


#endif

