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
// #define CHUNK_SIZEE 1048576 


Server::Server()
{
}

Server::Server(const Server &Init)
{
    (void)Init;
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

std::string parseRequest(std::string request)
{
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
            if (!requestedPath.empty() && requestedPath != "/")
            {
                filePath = requestedPath;
            }
        }
    }
    return filePath;
}

std::string createChunkedHttpResponse(std::string contentType)
{
    return "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "\r\nTransfer-Encoding: chunked\r\n\r\n";
}

std::string createHttpResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << contentType << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
}

std::string errorPageNotFound(std::string contentType)
{
    return "HTTP/1.1 404 Not Found\r\nContent-Type: " + contentType + "\r\n\r\n";
}

void setnonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}

bool canBeOpen(std::string &filePath)
{
    std::string new_path;
    new_path = PATHC + filePath;
    std::ifstream file(new_path.c_str());
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << new_path << std::endl;
        return false;
    }
    filePath = new_path;
    return true;
}

// Structure to hold file transfer state
struct FileTransferState {
    std::string filePath;
    size_t offset;
    size_t fileSize;
    bool isComplete;
    
    FileTransferState() : offset(0), fileSize(0), isComplete(false) {}
};

// Map to keep track of file transfers for each client
std::map<int, FileTransferState> fileTransfers;

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
int continueFileTransfer(int fd)
{
    if (fileTransfers.find(fd) == fileTransfers.end())
    {
        std::cerr << "No file transfer in progress for fd: " << fd << std::endl;
        return -1;
    }
    
    FileTransferState &state = fileTransfers[fd];
    if (state.isComplete)
    {
        // Transfer already completed
        fileTransfers.erase(fd);
        return 0;
    }
    
    // const size_t CHUNK_SIZE = 8192; // 8KB chunks
    char buffer[CHUNK_SIZE];
    size_t remainingBytes = state.fileSize - state.offset;
    size_t bytesToRead = (remainingBytes > CHUNK_SIZE) ? CHUNK_SIZE : remainingBytes;
    size_t bytesRead = 0;
    
    if (!readFileChunk(state.filePath, buffer, state.offset, bytesToRead, bytesRead))
    {
        std::cerr << "Failed to read chunk from file: " << state.filePath << std::endl;
        fileTransfers.erase(fd);
        return -1;
    }
    
    if (!sendChunk(fd, buffer, bytesRead))
    {
        std::cerr << "Failed to send chunk." << std::endl;
        fileTransfers.erase(fd);
        return -1;
    }
    
    state.offset += bytesRead;
    
    // Check if we've sent the entire file
    if (state.offset >= state.fileSize)
    {
        if (!sendFinalChunk(fd))
        {
            std::cerr << "Failed to send final chunk." << std::endl;
            fileTransfers.erase(fd);
            return -1;
        }
        
        state.isComplete = true;
        fileTransfers.erase(fd);
    }
    
    return 0;
}

// Handle initial file request
int handleFileRequest(int fd, Server *server, const std::string &filePath)
{
    std::string contentType = server->getContentType(filePath);
    size_t fileSize = getFileSize(filePath);
    
    if (fileSize == 0)
    {
        std::cerr << "Failed to get file size or empty file: " << filePath << std::endl;
        return -1;
    }
    
    // Determine if file is large enough to warrant chunked encoding
    const size_t LARGE_FILE_THRESHOLD = 1024 * 1024; // 1MB
    
    if (fileSize > LARGE_FILE_THRESHOLD)
    {
        // Use chunked encoding for large files
        std::string httpResponse = createChunkedHttpResponse(contentType);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
        {
            std::cerr << "Failed to send chunked HTTP header." << std::endl;
            return -1;
        }
        
        // Setup the transfer state
        FileTransferState state;
        state.filePath = filePath;
        state.fileSize = fileSize;
        state.offset = 0;
        state.isComplete = false;
        fileTransfers[fd] = state;
        
        // Start sending the first chunk
        return continueFileTransfer(fd);
    }
    else
    {
        // Use standard Content-Length for smaller files
        std::string httpResponse = createHttpResponse(contentType, fileSize);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
        {
            std::cerr << "Failed to send HTTP header." << std::endl;
            return -1;
        }
        
        // Read and send the entire file at once
        char* buffer = new char[fileSize];
        size_t bytesRead = 0;
        if (!readFileChunk(filePath, buffer, 0, fileSize, bytesRead))
        {
            std::cerr << "Failed to read file: " << filePath << std::endl;
            delete[] buffer;
            return -1;
        }
        
        int result = send(fd, buffer, bytesRead, MSG_NOSIGNAL);
        delete[] buffer;
        
        if (result == -1)
        {
            std::cerr << "Failed to send file content." << std::endl;
            return -1;
        }
        
        return 0;
    }
}



// Original readFile function - kept for error pages
std::string readFile(const std::string &path)
{
    if (path.empty())
        return "";

    std::ifstream infile(path.c_str(), std::ios::binary);
    if (!infile)
    {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }

    std::ostringstream oss;
    oss << infile.rdbuf();
    return oss.str();
}

int do_use_fd(int fd, Server *server, std::string request)
{
    if (request.empty())
        return -1;
    
    // Check if we already have a file transfer in progress
    if (fileTransfers.find(fd) != fileTransfers.end())
    {
        // Continue the existing transfer
        return continueFileTransfer(fd);
    }
    
    std::string filePath = parseRequest(request);
    if (canBeOpen(filePath))
    {
        return handleFileRequest(fd, server, filePath);
    }
    else
    {
        // Send 404 error page
        std::string path1 = PATHE;
        std::string path2 = "index.html";
        std::string new_path = path1 + path2;
        std::string content = readFile(new_path);
        std::string httpResponse = errorPageNotFound(server->getContentType(new_path));
        
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
        {
            std::cerr << "Failed to send error response header" << std::endl;
            return -1;
        }
        
        if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
        {
            std::cerr << "Failed to send error content" << std::endl;
            return -1;
        }
        
        return 0;
    }
}



int Server::establishingServer(Server *server)
{
    (void)server;
    int serverSocket = 0;
    serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, getprotobyname("tcp")->p_proto);
    if (serverSocket < 0)
    {
        std::cerr << "Error opening stream socket." << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    int len = sizeof(serverAddress);
    int a = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &a, sizeof(int)) < 0)
    {
        perror("setsockopt failed");
        return EXIT_FAILURE;
    }
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("binding stream socket");
        return EXIT_FAILURE;
    }

    if (getsockname(serverSocket, (struct sockaddr *)&serverAddress, (socklen_t *)&len) == -1)
    {
        perror("getting socket name");
        return EXIT_FAILURE;
    }
    std::cout << "Socket port " << ntohs(serverAddress.sin_port) << std::endl;

    if (listen(serverSocket, 5) < 0)
    {
        perror("listen stream socket");
        return EXIT_FAILURE;
    }
    return serverSocket;
}

int handleClientConnections(Server *server, int listen_sock, struct epoll_event &ev, sockaddr_in &clientAddress, int epollfd, socklen_t &clientLen, std::map<int, std::string> &send_buffers)
{
    int conn_sock;
    char buffer[CHUNK_SIZE];
    std::string request;
    struct epoll_event events[MAX_EVENTS];
    int nfds;

    if ((nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) == -1)
    {
        std::cerr << "epoll_wait" << std::endl;
        return EXIT_FAILURE;
    }
    
    for (int i = 0; i < nfds; ++i)
    {
        if (events[i].data.fd == listen_sock)
        {
            conn_sock = accept(listen_sock, (struct sockaddr *)&clientAddress, &clientLen);
            if (conn_sock == -1)
            {
                std::cerr << "accept" << std::endl;
                return EXIT_FAILURE;
            }
            setnonblocking(conn_sock);
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = conn_sock;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
            {
                std::cerr << "epoll_ctl: conn_sock" << std::endl;
                return EXIT_FAILURE;
            }
        }
        else if (events[i].events & EPOLLIN)
        {
            std::cout << "EPOLLIN" << std::endl;
            int bytes = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
            if (bytes == -1)
            {
                std::cerr << "recv" << std::endl;
                return EXIT_FAILURE;
            }
            else if (bytes == 0)
            {
                // Clean up file transfer state if client disconnects
                if (fileTransfers.find(events[i].data.fd) != fileTransfers.end())
                {
                    fileTransfers.erase(events[i].data.fd);
                }
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
            if (fileTransfers.find(events[i].data.fd) != fileTransfers.end())
            {
                if (continueFileTransfer(events[i].data.fd) == -1)
                {
                    std::cerr << "Failed to continue file transfer" << std::endl;
                    return EXIT_FAILURE;
                }
                continue;
            }
            
            // Otherwise process new request
            request = send_buffers[events[i].data.fd];
            if (request.empty())
                continue;

            if (request.find("POST") != std::string::npos)
            {
                // Handle POST requests
                // std::string body = request.substr(request.find("\r\n\r\n") + 4);
                // send_buffers[events[i].data.fd] = body;
            }
            else if (request.find("DELETE") != std::string::npos)
            {
                // Handle DELETE requests
                // std::string body = request.substr(request.find("\r\n\r\n") + 4);
                // send_buffers[events[i].data.fd] = body;
            }
            else if (request.find("GET") != std::string::npos) 
            {
                if (do_use_fd(events[i].data.fd, server, request) == -1)
                {
                    return EXIT_FAILURE;
                }
            }
            else 
            {
                // Handle unknown request types
                std::cerr << "Unknown request type" << std::endl;
                continue;
            }
            
            // Clear the buffer after processing to avoid reprocessing
            if (fileTransfers.find(events[i].data.fd) == fileTransfers.end())
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

    listen_sock = server->establishingServer(server);
    if (listen_sock == EXIT_FAILURE)
    {
        delete server;
        return EXIT_FAILURE;
    }
    
    std::cout << "Server is listening" << std::endl;
    if ((epollfd = epoll_create1(0)) == -1)
    {
        std::cout << "Failed to create epoll file descriptor" << std::endl;
        close(listen_sock);
        delete server;
        return EXIT_FAILURE;
    }
    
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
    {
        std::cerr << "Failed to add file descriptor to epoll" << std::endl;
        close(listen_sock);
        close(epollfd);
        delete server;
        return EXIT_FAILURE;
    }

    std::map<int, std::string> send_buffers;
    while (true)
    {
        int result = handleClientConnections(server, listen_sock, ev, clientAddress, epollfd, clientLen, send_buffers);
        if (result == EXIT_FAILURE)
            break;
    }

    // Clean up any remaining file transfers
    for (std::map<int, FileTransferState>::iterator it = fileTransfers.begin(); it != fileTransfers.end(); ++it)
    {
        close(it->first);
    }
    fileTransfers.clear();

    close(listen_sock);
    if (close(epollfd) == -1)
    {
        std::cerr << "Failed to close epoll file descriptor" << std::endl;
        delete server;
        return EXIT_FAILURE;
    }
    
    delete server;
    return EXIT_SUCCESS;
}