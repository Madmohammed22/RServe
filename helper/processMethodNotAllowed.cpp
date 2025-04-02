#include "../server.hpp"

int Server::processMethodNotAllowed(int fd, Server *server){
    std::string path1 = PATHE;
    std::string path2 = "405.html";
    std::string new_path = path1 + path2;
    std::string content = server->readFile(new_path);
    std::string httpResponse = server->methodNotAllowedResponse(server->getContentType(new_path), 0);
    
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
