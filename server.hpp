/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 03:11:18 by mmad              #+#    #+#             */
/*   Updated: 2025/04/08 09:31:44 by mmad             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <time.h>
#include <bits/types.h>
#include "server.hpp"
#include <unistd.h>
#include <iomanip>  
#include <filesystem>
#include <dirent.h>   
#include <cstring>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <map>
#include <stack>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>  
#include <errno.h>     
#include <string.h>
#include <set>
#define ERROR404 404
#define ERROR405 405
#define SUCCESS 200

#define PORT 8080 
#define MAX_EVENTS 10
#define CHUNK_SIZE 1024
#define TIMEOUT 10

#define PATHC "root/content/"
#define PATHE "root/error/" 
#define PATHU "root/UPLOAD"

// Structure to hold file transfer state
struct FileTransferState {
    time_t last_activity_time;
    std::string filePath;
    size_t offset;
    size_t endOffset;
    size_t fileSize;
    bool isComplete;
    int socket;
    int saveFd;
    int flag;
    std::set<std::string> knownPaths;
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

public:
    int pageNotFound;
    size_t LARGE_FILE_THRESHOLD;

public:
    std::string renderHtml(std::string path, Server *server);
    int handle_post_request(int fd, Server *server, std::string request);
    std::string readFile(const std::string &path);
    static std::string createNotFoundResponse(std::string contentType, size_t contentLength);
    std::string parseRequest(std::string request, Server *server);
    int getFileType(std::string path);
    bool canBeOpen(std::string &filePath);
    std::string createChunkedHttpResponse(std::string contentType);
    std::ifstream::pos_type getFileSize(const std::string &path);
    std::string httpResponse(std::string contentType, size_t contentLength);
    int handle_delete_request(int fd, Server *server,std::string request);
    int continueFileTransfer(int fd, Server * server);
    int handleFileRequest(int fd, Server *server, const std::string &filePath);
    int serve_file_request(int fd, Server *server, std::string request);
    std::string methodNotAllowedResponse(std::string contentType, int contentLength);
    void setnonblocking(int fd);
    int processMethodNotAllowed(int fd, Server *server);
    static std::string getCurrentTimeInGMT();
    std::string createTimeoutResponse(std::string contentType, size_t contentLength);
    static std::string createBadResponse(std::string contentType, size_t contentLength);
};


#endif

