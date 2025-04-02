#include "../server.hpp"

int Server::serve_file_request(int fd, Server *server, std::string request)
{
    // Check if we already have a file transfer in progress
    if (server->fileTransfers.find(fd) != server->fileTransfers.end())
    {
        // Continue the existing transfer
        return server->continueFileTransfer(fd, server);
    }
    
    std::string filePath = server->parseRequest(request,server);
    if (server->canBeOpen(filePath) && server->getFileType(filePath) == 2)
    {
        return server->handleFileRequest(fd, server, filePath);
    }
    else
    {
        std::string path1 = PATHE;
        std::string path2 = "404.html";
        std::string new_path = path1 + path2;
        std::string content = server->readFile(new_path);
        std::string httpResponse = server->createNotFoundResponse(server->getContentType(new_path), content.length());
        
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send error response header" << std::endl, close(fd), -1;
        if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send error content" << std::endl, close(fd), -1;
        if (send(fd, "\r\n\r\n", 2, MSG_NOSIGNAL) == -1)
            return close(fd), -1;
        server->fileTransfers.erase(fd);
        close(fd);
    }
    return 0;
}
