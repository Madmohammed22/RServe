#include "../server.hpp"

std::string generateDeleteHttpResponse(Server* server)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 204 No Content\r\n"
        << "Last-Modified: " << server->getCurrentTimeInGMT() << "\r\n\r\n";
    return oss.str();
}

std::string generateGoneHttpResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 410 Gone\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
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

int DELETE(std::string request){
    const char* filename = request.c_str();
    if (unlink(filename) == -1) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

bool searchOnPath(std::vector<std::string> nodePath, std::string filePath){
    std::vector<std::string>::iterator begin = nodePath.begin();
    while(begin != nodePath.end())
        if (*begin == filePath)
            return true;
    return false;
}

int Server:: handle_delete_request(int fd, Server *server,std::string request){
    std::string filePath = server->parseRequest(request, server);
    std::vector<std::string> nodePath;
    std::cout << "[[" << server->getFileType("/home/mmad/Desktop/webserve/root/PATH/message.txt") << std::endl;
    if (server->canBeOpen(filePath)) {
        std::cout << "[1]\n";
        nodePath.push_back(filePath);
        if (server->getFileType(filePath) == 1) {
            deleteDirectoryContents(filePath.c_str());
        }
        if (DELETE(filePath) == -1) {
            return std::cerr << "Failed to delete file or directory: " << filePath << std::endl,
            close(fd), -1;
        }

        std::string httpResponse = generateDeleteHttpResponse(server);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1) {
            std::cerr << "Failed to send HTTP header." << std::endl;
            return close(fd), -1;
        }
        server->fileTransfers.erase(fd);
        // close(fd);
    }

    if  (searchOnPath(nodePath, filePath) == false && server->canBeOpen(filePath) == false){
        std::cout << "[2]\n";
        std::string path1 = PATHE;
        std::string path2 = "404.html";
        std::string new_path = path1 + path2;
        std::string content = server->readFile(new_path);
        std::string httpResponse = server->createNotFoundResponse(server->getContentType(new_path), content.length());
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send error response header [1]" << std::endl, -1;
        
        if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send error content" << std::endl, -1;
        if (send(fd, "\r\n\r\n", 2, MSG_NOSIGNAL) == -1)
            return -1;
        server->fileTransfers.erase(fd);
        close(fd);
        std::cout << "File does not exist: " << filePath << std::endl;
        return 0;
    }

    if (searchOnPath(nodePath, filePath) == true && server->canBeOpen(filePath) == false)
    {
        std::cout << "[3]\n";
        std::string path1 = PATHE;
        std::string path2 = "410.html";
        std::string new_path = path1 + path2;
        std::string content = server->readFile(new_path);

        std::string httpResponse = generateGoneHttpResponse(server->getContentType(new_path), content.length());
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send error response header [2]" << std::endl, -1;
        
        if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send error content" << std::endl, -1;
        if (send(fd, "\r\n\r\n", 2, MSG_NOSIGNAL) == -1)
            return -1;
        server->fileTransfers.erase(fd);
        nodePath.clear();
    }
    close(fd);
    return 0;
}