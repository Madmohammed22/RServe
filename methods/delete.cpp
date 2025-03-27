#include "../server.hpp"

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

int Server:: handle_delete_request(int fd, Server *server,std::string request){    
    std::cout << request << std::endl;
    std::string filePath = server->parseRequest(request, server);
    if (server->canBeOpen(filePath)) {
        if (server->getFileType(filePath) == 1) {
            deleteDirectoryContents(filePath.c_str());
        }
        if (DELETE(filePath) == -1) {
            std::cerr << "Failed to delete file or directory: " << filePath << std::endl;
            return close(fd), -1;
        }

        std::string contentType = "text/html";
        std::string message = server->createDeleteResponse(filePath);
        std::string httpResponse = server->generateHttpResponse(contentType, message.size());
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
    } else {
        std::string path1 = PATHE;
        std::string path2 = "404.html";
        close(fd);
        std::string new_path = path1 + path2;
        std::string content = server->readFile(new_path);
        std::string httpResponse = server->createNotFoundResponse(server->getContentType(new_path), content.length());
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