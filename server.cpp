/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/18 03:11:14 by mmad              #+#    #+#             */
/*   Updated: 2025/03/27 08:37:53 by mmad             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

Server::Server()
{
    this->pageNotFound = 0;
    this->LARGE_FILE_THRESHOLD = 1024 * 1024;
}

Server::Server(const Server &Init)
{
    this->pageNotFound = Init.pageNotFound;
    this->LARGE_FILE_THRESHOLD = Init.LARGE_FILE_THRESHOLD;
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


// Helper function to get file size
std::ifstream::pos_type Server::getFileSize(const std::string &path)
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

int Server::getFileType(std::string path){
    struct stat s;
    if( stat(path.c_str(), &s) == 0 )
    {
        if( s.st_mode & S_IFDIR ) // dir
            return 1;
        if( s.st_mode & S_IFREG ) // file
            return 2;
    }
    return -1;
}

std::string Server::getCurrentTimeInGMT() {
    time_t t = time(0);
    tm *time_struct = gmtime(&t); // Use gmtime to get UTC time

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", time_struct);
    std::string date = buffer;
    return date;
}

std::string Server::createChunkedHttpResponse(std::string contentType)
{
    return "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "; charset=utf-8" + "\r\nTransfer-Encoding: chunked\r\n\r\n";
}

std::string Server::generateHttpResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
}


std::string Server::createNotFoundResponse(std::string contentType, int contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 404 Not Found\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";

    return oss.str();
}

std::string Server::generateMethodNotAllowedResponse(std::string contentType, int contentLength)
{
    (void)contentLength;
    std::ostringstream oss;
    oss << "HTTP/1.1 405 Method Not Allowed\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Allow: GET, POST, DELETE\r\n\r\n";
        
    // << "Content-Length: " << contentLength << "\r\n"
    return oss.str();
}

bool Server::canBeOpen(std::string &filePath)
{
    std::string new_path;
    if (getFileType("/" + filePath) == 2)
        new_path = "/" + filePath;
    else if (getFileType("/" + filePath) == 1){
        new_path = "/" + filePath;
    }
    else{
        std::cout << "I was here\n";
        new_path = PATHC + filePath;
    }
    std::ifstream file(new_path.c_str());
    if (!file.is_open()){
        // std::cout << getFileType(filePath) << std::endl;
        return std::cerr << "Failed to open file::::: " << new_path << std::endl, false;
    }
    filePath = new_path;
    return true;
}

bool sendChunk(int fd, const char* data, size_t size)
{
    std::ostringstream chunkHeader;
    chunkHeader << std::hex << size << "\r\n";
    std::string header = chunkHeader.str();
    
    send(fd, header.c_str(), header.length(), MSG_NOSIGNAL);
        
    // Send chunk data
    send(fd, data, size, MSG_NOSIGNAL);
    
    // Send chunk terminator
    send(fd, "\r\n", 2, MSG_NOSIGNAL);
    return true;
}

// Send the final empty chunk to indicate end of chunked transfer
bool sendFinalChunk(int fd)
{
    return sendChunk(fd, "", 0) && 
           send(fd, "\r\n", 2, MSG_NOSIGNAL) != -1;
}

// Continue sending chunks for an in-progress file transfer
int Server::continueFileTransfer(int fd, Server * server)
{
    if (server->fileTransfers.find(fd) == server->fileTransfers.end())
        return std::cerr << "No file transfer in progress for fd: " << fd << std::endl, -1;
    
    FileTransferState &state = server->fileTransfers[fd];
    if (state.isComplete)
        return server->fileTransfers.erase(fd), 0;
    
    char buffer[CHUNK_SIZE];
    size_t remainingBytes = state.fileSize - state.offset;
    size_t bytesToRead;
    if (remainingBytes > CHUNK_SIZE)
        bytesToRead = CHUNK_SIZE;
    else
        bytesToRead =  remainingBytes;
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
        // sendFinalChunk(fd);
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
int Server::handleFileRequest(int fd, Server *server, const std::string &filePath)
{
    std::string contentType = server->getContentType(filePath);
    size_t fileSize = server->getFileSize(filePath);
    
    if (fileSize == 0)
        return std::cerr << "Failed to get file size or empty file: " << filePath << std::endl, -1;
    
    // Determine if file is large enough to warrant chunked encoding
    const size_t LARGE_FILE_THRESHOLD = 1024 * 1024; // 1MB
    
    if (fileSize > LARGE_FILE_THRESHOLD)
    {
        std::string httpResponse = server->createChunkedHttpResponse(contentType);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send chunked HTTP header." << std::endl, -1;
        FileTransferState state;
        state.filePath = filePath;
        state.fileSize = fileSize;
        state.offset = 0;
        state.isComplete = false;
        server->fileTransfers[fd] = state;
        return server->continueFileTransfer(fd, server);
    }
    else
    {
        // Use standard Content-Length for smaller files
        std::string httpResponse = server->generateHttpResponse(contentType, fileSize);
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

// Original readFile function - kept for error pages
std::string Server::readFile(const std::string &path)
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
            server->setnonblocking(conn_sock);
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = conn_sock;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
                return std::cerr << "epoll_ctl: conn_sock" << std::endl, EXIT_FAILURE;
        }
        else if (events[i].events & EPOLLIN)
        {
            int bytes = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
            // if (bytes == -1)
            //     return std::cerr << "recv" << std::endl, EXIT_FAILURE;
            if (bytes == 0)
            {
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
                if (server->continueFileTransfer(events[i].data.fd, server) == -1)
                    return std::cerr << "Failed to continue file transfer" << std::endl, EXIT_FAILURE;
                continue;
            }
            
            // Otherwise process new request
            request = send_buffers[events[i].data.fd];
            if (request.empty())
                continue;

            if (request.find("POST") != std::string::npos)
            {
                if (server->handle_post_request(events[i].data.fd, server, request) == -1)
                    return EXIT_FAILURE;
            }
            else if (request.find("DELETE") != std::string::npos)
            {
                if (server->handle_delete_request(events[i].data.fd, server, request) == -1)
                    return EXIT_FAILURE;
            }
            else if (request.find("GET") != std::string::npos) 
            {
                if (server->serve_file_request(events[i].data.fd, server, request) == -1)
                    return EXIT_FAILURE;
            }
            else 
            {
                if (server->processMethodNotAllowed(events[i].data.fd, server) == -1)
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

int main(int argc, char **argv)
{
    (void)argc, (void)argv;
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
        if (handleClientConnections(server, listen_sock, ev, clientAddress, epollfd, clientLen, send_buffers) == EXIT_FAILURE)
            break;
    }
    
    for (std::map<int, FileTransferState>::iterator it = server->fileTransfers.begin(); it != server->fileTransfers.end(); ++it)
        close(it->first);
    server->fileTransfers.clear();
    close(listen_sock);
    if (close(epollfd) == -1)
        return std::cerr << "Failed to close epoll file descriptor" << std::endl, delete server, EXIT_FAILURE;
    delete server;
    return EXIT_SUCCESS;
}