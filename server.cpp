/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 03:11:14 by mmad              #+#    #+#             */
/*   Updated: 2025/03/18 21:42:34 by mmad             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>  // Added for getprotobyname
#include <iomanip>  // Added for std::hex, std::setw, std::setfill
#include <sys/stat.h>  // Added for stat function
#include <filesystem>
#include <dirent.h>     // Added for directory operations

Server::Server()
{
    this->pageNotFound = 0;
}

Server::Server(const Server &Init)
{
    this->pageNotFound = Init.pageNotFound;
}

Server &Server::operator=(const Server &Init)
{
    if (this == &Init)
        return *this;
    return *this;
}

Server::~Server()
{
    std::cout << "[Server] Destructor is called" << std::endl;
}

std::string Server::getContentType(const std::string &path)
{
    std::map<std::string, std::string> extensionToType;
    extensionToType.insert(std::make_pair(std::string(".html"), std::string("text/html")));
    extensionToType.insert(std::make_pair(std::string(".css"), std::string("text/css")));
    extensionToType.insert(std::make_pair(std::string(".js"), std::string("application/javascript")));
    extensionToType.insert(std::make_pair(std::string(".json"), std::string("application/json")));
    extensionToType.insert(std::make_pair(std::string(".xml"), std::string("application/xml")));
    extensionToType.insert(std::make_pair(std::string(".mp4"), std::string("video/mp4")));
    extensionToType.insert(std::make_pair(std::string(".mp3"), std::string("audio/mpeg")));
    extensionToType.insert(std::make_pair(std::string(".wav"), std::string("audio/wav")));
    extensionToType.insert(std::make_pair(std::string(".ogg"), std::string("audio/ogg")));
    extensionToType.insert(std::make_pair(std::string(".png"), std::string("image/png")));
    extensionToType.insert(std::make_pair(std::string(".jpg"), std::string("image/jpeg")));
    extensionToType.insert(std::make_pair(std::string(".jpeg"), std::string("image/jpeg")));
    extensionToType.insert(std::make_pair(std::string(".gif"), std::string("image/gif")));
    extensionToType.insert(std::make_pair(std::string(".svg"), std::string("image/svg+xml")));
    extensionToType.insert(std::make_pair(std::string(".ico"), std::string("image/x-icon")));

    size_t dotPos = path.rfind('.');
    if (dotPos != std::string::npos)
    {
        std::string extension = path.substr(dotPos);
        std::map<std::string, std::string>::const_iterator it = extensionToType.find(extension);
        if (it != extensionToType.end())
            return it->second;
    }
    return "application/octet-stream";
}

// Helper function to get file size
std::ifstream::pos_type getFileSize(const std::string &path)
{
    std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return 0;
    
    return file.tellg();
}

// Read file in chunks
bool readFileChunk(const std::string &path, char *buffer, size_t offset, size_t chunkSize, size_t &bytesRead)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return false;
    
    file.seekg(offset);
    file.read(buffer, chunkSize);
    bytesRead = file.gcount();
    
    return true;
}

int getFileType(std::string path){
    struct stat s;
    if( stat(path.c_str(), &s) == 0 )
    {
        if( s.st_mode & S_IFDIR ) // dir
            return 1;
        else if( s.st_mode & S_IFREG ) // file
            return 2;
        else
            return 3;
    }
    return -1;
}
std::string parseRequest(std::string request)
{
    std::cout << "------------------------------------\n";
    std::cout <<request << std::endl;
    std::cout << "------------------------------------\n";

    if (request.empty())
        return "";
    std::string filePath = "/index.html";
    size_t startPos = request.find("GET /");
    if (startPos != std::string::npos)
    {
        startPos += 5;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            if (requestedPath.empty()){
                return filePath;
            }
            if (!requestedPath.empty() && requestedPath != "/")
            {
                filePath = requestedPath;
            }
        }
    }
    else{
        filePath.clear();
        size_t startPos = request.find("DELETE /");
        startPos += 8;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            if (!requestedPath.empty() && requestedPath != "/")
            {
                filePath = requestedPath;
            }
        }
    }
    return filePath;
}

std::string getCurrentTimeInGMT() {
    time_t t = time(0);
    tm *time_struct = gmtime(&t);

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", time_struct);
    std::string date = buffer;
    return date;
}

std::string createChunkedHttpResponse(std::string contentType)
{
    return "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "; charset=utf-8" + "\r\nTransfer-Encoding: chunked\r\n\r\n";
}

std::string createHttpResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
}

std::string errorPageNotFound(std::string contentType, int contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 404 Not Found\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";

    return oss.str();
}

std::string errorMethodNotAllowed(std::string contentType, int contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 405 Method Not Allowed\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";

    return oss.str();

    return "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: " + contentType + "\r\n\r\n";
}
void setnonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        (perror("fcntl"), exit(EXIT_FAILURE));
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}

bool canBeOpen(std::string &filePath)
{  
    std::string new_path;
    if (getFileType("/" + filePath) == 2)
        new_path = "/" + filePath;
    else if (getFileType("/" + filePath) == 1){
        new_path = "/" + filePath;
    }
    else
        new_path = PATHC + filePath;
    std::ifstream file(new_path.c_str());
    if (!file.is_open()){
        // std::cout << getFileType(filePath) << std::endl;
        return std::cerr << "Failed to open file: " << new_path << std::endl, false;
    }
    filePath = new_path;
    return true;
}

// Send a chunk of data using chunked encoding
bool sendChunk(int fd, const char* data, size_t size)
{
    // Send chunk size in hex
    std::ostringstream chunkHeader;
    chunkHeader << std::hex << size << "\r\n";
    std::string header = chunkHeader.str();
    
    if (send(fd, header.c_str(), header.length(), MSG_NOSIGNAL) == -1)
        return false;
    
    // Send chunk data
    if (size > 0) {
        if (send(fd, data, size, MSG_NOSIGNAL) == -1)
            return false;
    }
    
    // Send chunk terminator
    if (send(fd, "\r\n", 2, MSG_NOSIGNAL) == -1)
        return false;
    
    return true;
}

// Send the final empty chunk to indicate end of chunked transfer
bool sendFinalChunk(int fd)
{
    return sendChunk(fd, "", 0) && 
           send(fd, "\r\n", 2, MSG_NOSIGNAL) != -1;
}

// Continue sending chunks for an in-progress file transfer
int continueFileTransfer(int fd, Server * server)
{
    if (server->fileTransfers.find(fd) == server->fileTransfers.end())
        return std::cerr << "No file transfer in progress for fd: " << fd << std::endl, -1;
    
    FileTransferState &state = server->fileTransfers[fd];
    if (state.isComplete) // Transfer already completed
        return server->fileTransfers.erase(fd), 0;
    
    // const size_t CHUNK_SIZE = 8192; // 8KB chunks
    char buffer[CHUNK_SIZE];
    size_t remainingBytes = state.fileSize - state.offset;
    size_t bytesToRead = (remainingBytes > CHUNK_SIZE) ? CHUNK_SIZE : remainingBytes;
    size_t bytesRead = 0;
    
    if (!readFileChunk(state.filePath, buffer, state.offset, bytesToRead, bytesRead))
    {
        std::cerr << "Failed to read chunk from file: " << state.filePath << std::endl;
        server->fileTransfers.erase(fd);
        return -1;
    }
    
    if (!sendChunk(fd, buffer, bytesRead))
    {
        std::cerr << "Failed to send chunk." << std::endl;
        server->fileTransfers.erase(fd);
        return -1;
    }
    
    state.offset += bytesRead;
    
    // Check if we've sent the entire file
    if (state.offset >= state.fileSize)
    {
        if (!sendFinalChunk(fd))
        {
            std::cerr << "Failed to send final chunk." << std::endl;
            server->fileTransfers.erase(fd);
            return -1;
        }
        
        state.isComplete = true;
        server->fileTransfers.erase(fd);
    }
    
    return 0;
}

// Handle initial file request
int handleFileRequest(int fd, Server *server, const std::string &filePath)
{
    std::string contentType = server->getContentType(filePath);
    size_t fileSize = getFileSize(filePath);
    
    if (fileSize == 0)
        return std::cerr << "Failed to get file size or empty file: " << filePath << std::endl, -1;
    
    // Determine if file is large enough to warrant chunked encoding
    const size_t LARGE_FILE_THRESHOLD = 1024 * 1024; // 1MB
    
    if (fileSize > LARGE_FILE_THRESHOLD)
    {
        // Use chunked encoding for large files
        std::string httpResponse = createChunkedHttpResponse(contentType);
        std::cout << "[1][" << httpResponse << "]" << std::endl;
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send chunked HTTP header." << std::endl, -1;
        
        // Setup the transfer state
        FileTransferState state;
        state.filePath = filePath;
        state.fileSize = fileSize;
        state.offset = 0;
        state.isComplete = false;
        server->fileTransfers[fd] = state;
        // Start sending the first chunk
        return continueFileTransfer(fd, server);
    }
    else
    {
        // Use standard Content-Length for smaller files
        std::string httpResponse = createHttpResponse(contentType, fileSize);
        std::cout << "[2][" << httpResponse << "]" << std::endl;
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send HTTP header." << std::endl, -1;
        
        // Read and send the entire file at once
        char* buffer = new char[fileSize];
        size_t bytesRead = 0;
        if (!readFileChunk(filePath, buffer, 0, fileSize, bytesRead))
            return std::cerr << "Failed to read file: " << filePath << std::endl, delete[] buffer, -1;
        
        int result = send(fd, buffer, bytesRead, MSG_NOSIGNAL);
        delete[] buffer;
        
        if (result == -1)
            return std::cerr << "Failed to send file content." << std::endl, -1;
        
        return 0;
    }
}

void deleteDirectoryContents(const std::string& dir)
{
    DIR* dp = opendir(dir.c_str());
    if (dp == NULL) {
        std::cerr << "Error: Unable to open directory " << dir << std::endl;
        return;
    }

    try
    {
        struct dirent* entry;
        while ((entry = readdir(dp)) != NULL) {
            // Skip the special entries "." and ".."
            if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..") {
                continue;
            }

            struct stat entryStat;
            std::string entryPath = dir + "/" + entry->d_name;
            if (stat(entryPath.c_str(), &entryStat) == -1) {
                std::cerr << "Error: Unable to stat " << entryPath << std::endl;
                continue;
            }

            if (S_ISDIR(entryStat.st_mode)) {
                // If it's a directory, use recursive removal
                if (rmdir(entryPath.c_str()) == -1) {
                    std::cerr << "Error: Unable to remove directory " << entryPath << std::endl;
                }
            } else {
                // If it's a file, remove it
                if (unlink(entryPath.c_str()) == -1) {
                    std::cerr << "Error: Unable to remove file " << entryPath << std::endl;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    closedir(dp);
    (void)rmdir(dir.c_str());
}

// Original readFile function - kept for error pages
std::string readFile(const std::string &path)
{
    if (path.empty())
        return "";

    std::ifstream infile(path.c_str(), std::ios::binary);
    if (!infile)
        return std::cerr << "Failed to open file: " << path << std::endl, "";

    std::ostringstream oss;
    oss << infile.rdbuf();
    return oss.str();
}
int DELETE(std::string request){
    const char* filename = request.c_str();
    if (unlink(filename) == -1) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
std::string getFileTypeDescription(std::string filePath){
    if (getFileType(filePath) == 1)
        return "Directory";
    else if (getFileType(filePath) == 2)
        return "File";
    else
        return "Content";
    return NULL; // potencial error
}

int handle_delete_request(int fd, Server *server,std::string request){    
    std::cout << request << std::endl;
    std::string filePath = parseRequest(request);
    // just to get the result if the file can be open.
    if (canBeOpen(filePath)) {
        if (getFileType(filePath) == 1) {
            deleteDirectoryContents(filePath.c_str());
        }
        if (DELETE(filePath) == -1) {
            std::cerr << "Failed to delete file or directory: " << filePath << std::endl;
            return close(fd), -1;
        }

        std::string contentType = "text/html";
        std::string message = "<html>\n"
                              "  <body>\n"
                              "    <h1>" + getFileTypeDescription(filePath) + " " + filePath + "\" deleted successfully.</h1>\n"
                              "  </body>\n"
                              "</html>";
        std::string httpResponse = createHttpResponse(contentType, message.size());
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1) {
            std::cerr << "Failed to send HTTP header." << std::endl;
            return close(fd), -1;
        }
        if (send(fd, message.c_str(), message.length(), MSG_NOSIGNAL) == -1) {
            std::cerr << "Failed to send response message." << std::endl;
            return close(fd), -1;
        }
        if (send(fd, "\r\n\r\n", 2, MSG_NOSIGNAL) == -1) {
            std::cerr << "Failed to send final header." << std::endl;
            return close(fd), -1;
        }
        server->fileTransfers.erase(fd);
        close(fd);
    } else {
        std::string path1 = PATHE;
        std::string path2 = "404.html";
        std::string new_path = path1 + path2;
        std::string content = readFile(new_path);
        std::string httpResponse = errorPageNotFound(server->getContentType(new_path), content.length());
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send error response header" << std::endl, -1;
        
        if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send error content" << std::endl, -1;
        if (send(fd, "\r\n\r\n", 2, MSG_NOSIGNAL) == -1)
            return -1;
        server->fileTransfers.erase(fd);
        close(fd);
        std::cout << "File does not exist: " << filePath << std::endl;
    }
    return 0;
}
int serve_file_request(int fd, Server *server, std::string request)
{
    if (request.empty())
        return -1;
    // Check if we already have a file transfer in progress
    if (server->fileTransfers.find(fd) != server->fileTransfers.end())
    {
        // Continue the existing transfer
        return continueFileTransfer(fd, server);
    }
    
    std::string filePath = parseRequest(request);
    if (canBeOpen(filePath) && getFileType(filePath) == 2)
    {
        return handleFileRequest(fd, server, filePath);
    }
    else
    {
        std::string path1 = PATHE;
        std::string path2 = "404.html";
        std::string new_path = path1 + path2;
        std::string content = readFile(new_path);
        std::string httpResponse = errorPageNotFound(server->getContentType(new_path), content.length());
        
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send error response header" << std::endl, close(fd), -1;
        if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send error content" << std::endl, close(fd), -1;
        if (send(fd, "\r\n\r\n", 2, MSG_NOSIGNAL) == -1)
            return close(fd), -1;
        server->fileTransfers.erase(fd);
        close(fd);
        return 0;
    }
}

int Server::establishingServer()
{
    int serverSocket = 0;
    serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, getprotobyname("tcp")->p_proto);
    if (serverSocket < 0)
        return std::cerr << "Error opening stream socket." << std::endl, EXIT_FAILURE;

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    int len = sizeof(serverAddress);
    int a = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &a, sizeof(int)) < 0)
        return perror("setsockopt failed"), EXIT_FAILURE;
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
        return perror("binding stream socket"), EXIT_FAILURE;

    if (getsockname(serverSocket, (struct sockaddr *)&serverAddress, (socklen_t *)&len) == -1)
        return perror("getting socket name"), EXIT_FAILURE;
    std::cout << "Socket port " << ntohs(serverAddress.sin_port) << std::endl;

    if (listen(serverSocket, 5) < 0)
        return perror("listen stream socket"), EXIT_FAILURE;
    return serverSocket;
}
int processMethodNotAllowed(int fd, Server *server,std::string request){
    (void)request;
    std::string path1 = PATHE;
    std::string path2 = "405.html";
    std::string new_path = path1 + path2;
    std::string content = readFile(new_path);
    std::string httpResponse = errorMethodNotAllowed(server->getContentType(new_path));
    
    if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
        return std::cerr << "Failed to send error response header" << std::endl, close(fd), -1;
    
    if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
        return std::cerr << "Failed to send error content" << std::endl, close(fd), -1;
    if (send(fd, "\r\n\r\n", 2, MSG_NOSIGNAL) == -1)
        return close(fd), -1;
    server->fileTransfers.erase(fd);
    close(fd);        
    return 0;
}
int handleClientConnections(Server *server, int listen_sock, struct epoll_event &ev
    , sockaddr_in &clientAddress, int epollfd, socklen_t &clientLen, std::map<int, std::string> &send_buffers)
{
    int conn_sock;
    char buffer[CHUNK_SIZE];
    std::string request;
    struct epoll_event events[MAX_EVENTS];
    int nfds;

    if ((nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) == -1)
        return std::cerr << "epoll_wait" << std::endl, EXIT_FAILURE;
    
    for (int i = 0; i < nfds; ++i)
    {
        if (events[i].data.fd == listen_sock)
        {
            conn_sock = accept(listen_sock, (struct sockaddr *)&clientAddress, &clientLen);
            if (conn_sock == -1)
                return std::cerr << "accept" << std::endl, EXIT_FAILURE;
            setnonblocking(conn_sock);
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = conn_sock;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
                return std::cerr << "epoll_ctl: conn_sock" << std::endl, EXIT_FAILURE;
        }
        else if (events[i].events & EPOLLIN)
        {
            int bytes = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
            if (bytes == -1)
                return std::cerr << "recv" << std::endl, EXIT_FAILURE;
            else if (bytes == 0)
            {
                // Clean up file transfer state if client disconnects
                if (server->fileTransfers.find(events[i].data.fd) != server->fileTransfers.end())
                    server->fileTransfers.erase(events[i].data.fd);
                close(events[i].data.fd);
                continue; 
            }
            else
            {
                buffer[bytes] = '\0';
                request = buffer;
                send_buffers[events[i].data.fd] = request;
            }
        }
        else if (events[i].events & EPOLLOUT)
        {
            // Check if we have an ongoing file transfer
            if (server->fileTransfers.find(events[i].data.fd) != server->fileTransfers.end())
            {
                if (continueFileTransfer(events[i].data.fd, server) == -1)
                    return std::cerr << "Failed to continue file transfer" << std::endl, EXIT_FAILURE;
                continue;
            }
            
            // Otherwise process new request
            request = send_buffers[events[i].data.fd];
            if (request.empty())
                continue;

            if (request.find("POST") != std::string::npos)
            {
                std::cout << request << std::endl;
                // Handle POST requests
                // std::string body = request.substr(request.find("\r\n\r\n") + 4);
                // send_buffers[events[i].data.fd] = body;
            }
            else if (request.find("DELETE") != std::string::npos)
            {
                if (handle_delete_request(events[i].data.fd, server, request) == -1)
                    return EXIT_FAILURE;
            }
            else if (request.find("GET") != std::string::npos) 
            {
                if (serve_file_request(events[i].data.fd, server, request) == -1)
                    return EXIT_FAILURE;
            }
            else 
            {
                if (processMethodNotAllowed(events[i].data.fd, server, request) == -1)
                    return EXIT_FAILURE;
            }
            
            // Clear the buffer after processing to avoid reprocessing
            if (server->fileTransfers.find(events[i].data.fd) == server->fileTransfers.end())
            {
                // Only erase if we're not in the middle of a chunked transfer
                send_buffers.erase(events[i].data.fd);
            }
        }
    }

    return EXIT_SUCCESS;
}

int main()
{
    Server *server = new Server();

    sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);
    int listen_sock, epollfd;
    struct epoll_event ev;

    if ((listen_sock = server->establishingServer()) == EXIT_FAILURE)
        return delete server, EXIT_FAILURE;
    
    std::cout << "Server is listening" << std::endl;
    if ((epollfd = epoll_create1(0)) == -1)
    {
        return std::cout << "Failed to create epoll file descriptor" << std::endl,
        close(listen_sock), delete server, EXIT_FAILURE;
    }
    
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
    {
        return std::cerr << "Failed to add file descriptor to epoll" << std::endl,
        close(listen_sock), close(epollfd), delete server, EXIT_FAILURE;
    }

    std::map<int, std::string> send_buffers;
    while (true)
    {
        int result = handleClientConnections(server, listen_sock, ev, clientAddress, epollfd, clientLen, send_buffers);
        if (result == EXIT_FAILURE)
            break;
    }

    // Clean up any remaining file transfers
    for (std::map<int, FileTransferState>::iterator it = server->fileTransfers.begin(); it != server->fileTransfers.end(); ++it)
    {
        close(it->first);
    }
    server->fileTransfers.clear();

    close(listen_sock);
    if (close(epollfd) == -1)
        return std::cerr << "Failed to close epoll file descriptor" << std::endl, delete server, EXIT_FAILURE;
    
    delete server;
    return EXIT_SUCCESS;
}