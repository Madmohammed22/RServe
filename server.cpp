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
// test
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
        {
            return it->second;
        }
    }
    return "application/octet-stream";
}

std::string readFile(const std::string &path)
{
    std::stringstream content;
    if (path.empty())
        return "";

    std::ifstream infile(path.c_str(), std::ios::binary);
    if (!infile)
        return std::cerr << "Failed to open file: " << path << std::endl, "";

    char buffer[CHUNK_SIZE];
    while (infile.read(buffer, CHUNK_SIZE) || infile.gcount() > 0)
    {
        content.write(buffer, infile.gcount());
    }

    return content.str();
}

std::string Server::parseRequest(std::string request)
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

std::string Server::createHttpResponse(std::string contentType)
{
    return "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "\r\n\r\n";;
}

std::string errorPageNoteFound(std::string contentType)
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
    new_path = "/home/mmad/resources/" + filePath;
    std::ifstream file(new_path.c_str());
    if (!file.is_open())
        return std::cerr << "Failed to open file:: " << new_path << std::endl, false;
    filePath = new_path;
    return true;
}

int do_use_fd(int fd, Server *server, std::string request)
{
    if (request.empty())
        return -1;
    std::string filePath = server->parseRequest(request);
    std::string content;
    if (canBeOpen(filePath) == true)
    {
        content = readFile(filePath);
        std::string httpResponse = server->createHttpResponse(server->getContentType(filePath));
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send." << std::endl, -1;
        if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send" << std::endl, -1;
        send(fd, "\r\n", 2, MSG_NOSIGNAL);
    }
    else
    {
        std::string path1 = "/home/mmad/resources/";
        std::string path2 = "error.html";
        std::string new_path = path1 + path2;
        std::string content = readFile(new_path);
        std::string httpResponse = errorPageNoteFound(server->getContentType(new_path));
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send" << std::endl, -1;
        if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send" << std::endl, -1;
        send(fd, "\r\n", 2, MSG_NOSIGNAL);
    }
    return 0;
}

int Server::establishingServer(Server *server)
{
    int serverSocket = 0;
    serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, getprotobyname("tcp")->p_proto);
    if (serverSocket < 0)
        return delete server, std::cerr << "opening stream socket." << std::endl, EXIT_FAILURE;

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    int len = sizeof(serverAddress);
    int a = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &a, sizeof(int)) < 0)
        return perror(NULL), delete server, EXIT_FAILURE;
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
        return perror("binding stream socket"), delete server, EXIT_FAILURE;

    if (getsockname(serverSocket, (struct sockaddr *)&serverAddress, (socklen_t *)&len) == -1)
        return perror("getting socket name."), delete server, EXIT_FAILURE;
    std::cout << "Socket port " << ntohs(serverAddress.sin_port) << std::endl;

    if (listen(serverSocket, 5) < 0)
        return perror("listen stream socket"), delete server, EXIT_FAILURE;
    return serverSocket;
}

int handleClientConnections(Server *server, int listen_sock, struct epoll_event &ev, sockaddr_in &clientAddress, int epollfd, socklen_t &clientLen, std::map<int, std::string> &send_buffers)
{
    int conn_sock;
    char buffer[CHUNK_SIZE];
    std::string request;
    struct epoll_event events[MAX_EVENTS];
    // int timeout = -1;
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1)
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
            time_t now = time(0);

            if (now % 2 == 0)
            {
                close(events[i].data.fd);
                continue;
            }

            std::cout << "EPOLLIN" << std::endl;
            int bytes = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
            if (bytes == -1)
                return std::cerr << "recv" << std::endl, EXIT_FAILURE;
            else if (bytes == 0){
                // close(events[i].data.fd);
                continue; 
            }
            else{
                buffer[bytes] = '\0';
                request = buffer;
                send_buffers[events[i].data.fd] = request;
            }
        }
        if (events[i].events & EPOLLOUT)
        {
            request = send_buffers[events[i].data.fd];
            if (request.find("POST") != std::string::npos)
            {
                std::string body = request.substr(request.find("\r\n\r\n") + 4);
                send_buffers[events[i].data.fd] = body;
            }
            else if (request.find("DELETE") != std::string::npos)
            {
                std::string body = request.substr(request.find("\r\n\r\n") + 4);
                send_buffers[events[i].data.fd] = body;
            }
            else if (request.find("GET") != std::string::npos) 
            {
                do_use_fd(events[i].data.fd, server, request);
            }
            else 
                return -1;  
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
    listen_sock = server->establishingServer(server);
    std::cout << "Server is listening" << std::endl;

    epollfd = epoll_create1(0);
    if (epollfd < 0)
        return std::cerr << "Failed to create epoll file descriptor" << std::endl, EXIT_FAILURE;

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
        return std::cerr << "Failed to add file descriptor to epoll" << std::endl, EXIT_FAILURE;

    std::map<int, std::string> send_buffers;
    while (true)
    {
        if (handleClientConnections(server, listen_sock, ev, clientAddress, epollfd, clientLen, send_buffers) == 1)
            break;
    }

    close(listen_sock);
    if (close(epollfd) == -1)
        return std::cerr << "Failed to close epoll file descriptor" << std::endl, EXIT_FAILURE;
    delete server;
    return EXIT_SUCCESS;
}
