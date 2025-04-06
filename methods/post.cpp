#include "../server.hpp"

// Read file in chunks with error handling
bool readFileChunk_post(const std::string &path, char *buffer, size_t offset, size_t chunkSize, size_t &bytesRead)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << path << std::endl;
        return false;
    }

    file.seekg(offset, std::ios::beg);
    file.read(buffer, chunkSize);
    bytesRead = file.gcount();
    return true;
}

// Send a chunk of data using chunked encoding.
bool sendChunk_post(int fd, const char *data, size_t size)
{
    try
    {
        // Send chunk size in hex
        std::ostringstream chunkHeader;
        chunkHeader << std::hex << size << "\r\n";
        std::string header = chunkHeader.str();

        // Combine chunk header, data, and terminator for atomic send
        std::vector<struct iovec> iov(3);
        iov[0].iov_base = const_cast<char *>(header.c_str());
        iov[0].iov_len = header.length();
        iov[1].iov_base = const_cast<char *>(data);
        iov[1].iov_len = size;
        char terminator[2] = {'\r', '\n'};
        iov[2].iov_base = terminator;
        iov[2].iov_len = 2;

        struct msghdr msg = {};
        msg.msg_iov = iov.data();
        msg.msg_iovlen = iov.size();

        ssize_t sentBytes = sendmsg(fd, &msg, MSG_NOSIGNAL);
        return sentBytes != -1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in sendChunk_post: " << e.what() << std::endl;
        return false;
    }
}

// Send the final empty chunk to indicate end of chunked transfer
bool sendFinalChunk_post(int fd)
{
    return sendChunk_post(fd, "", 0) &&
           send(fd, "\r\n", 2, MSG_NOSIGNAL) != -1;
}

int redirectTheParh(std::vector<char> buffer, std::string filePath, size_t bytesRead)
{
    // Define the redirect path
    const std::string REDIRECTPATH = PATHU;

    // Ensure the directory exists
    struct stat st;
    if (stat(REDIRECTPATH.c_str(), &st) != 0)
    { // Check if path exists
        if (mkdir(REDIRECTPATH.c_str(), 0777) != 0)
        { // Create directory
            std::cerr << "Failed to create directory: " << REDIRECTPATH
                      << " (" << strerror(errno) << ")" << std::endl;
            return -1;
        }
    }

    // Extract filename from the original path
    std::string filename = filePath.substr(filePath.find_last_of("/\\") + 1);

    // Construct the full new path
    std::string newFilePath = REDIRECTPATH + "/" + filename;

    // Save the file to the redirected path
    std::ofstream localFile(newFilePath.c_str(), std::ios::binary);
    if (!localFile)
    {
        std::cerr << "Failed to create file in redirected path: " << newFilePath << std::endl;
        return -1;
    }
    localFile.write(buffer.data(), bytesRead);
    localFile.close();
    std::cout << "File successfully saved to: " << newFilePath << std::endl;
    return 0;
}

int continueFileTransfer_post(int fd, Server *server)
{
    std::map<int, FileTransferState>::iterator transferIt = server->fileTransfers.find(fd);
    if (transferIt == server->fileTransfers.end())
    {
        std::cerr << "No file transfer in progress for fd: " << fd << std::endl;
        return -1;
    }

    FileTransferState &state = transferIt->second;
    if (state.isComplete)
    {
        server->fileTransfers.erase(transferIt);
        return 0;
    }

    char buffer[CHUNK_SIZE];
    size_t remainingBytes = state.fileSize - state.offset;
    size_t bytesToRead = std::min(remainingBytes, static_cast<size_t>(CHUNK_SIZE));
    size_t bytesRead = 0;

    if (!readFileChunk_post(state.filePath, buffer, state.offset, bytesToRead, bytesRead))
    {
        std::cerr << "Failed to read chunk from file: " << state.filePath << std::endl;
        server->fileTransfers.erase(transferIt);
        return -1;
    }

    if (!sendChunk_post(fd, buffer, bytesRead))
    {
        std::cerr << "Failed to send chunk." << std::endl;
        server->fileTransfers.erase(transferIt);
        return -1;
    }

    state.offset += bytesRead;
    // Check if we've sent the entire file
    if (state.offset >= state.fileSize)
    {
        if (!sendFinalChunk_post(fd))
        {
            std::cerr << "Failed to send final chunk." << std::endl;
            server->fileTransfers.erase(transferIt);
            return -1;
        }

        state.isComplete = true;
        server->fileTransfers.erase(transferIt);
    }
    std::vector<char> bufferVector(buffer, buffer + bytesRead);
    return redirectTheParh(bufferVector, server->fileTransfers[fd].filePath, bytesRead);
}

int handleFileRequest_post(int fd, Server *server, const std::string &filePath)
{
    try
    {
        std::string contentType = server->getContentType(filePath);
        size_t fileSize = server->getFileSize(filePath);

        // if (fileSize == 0) {
        //     std::cerr << "Failed to get file size or empty file: " << filePath << std::endl;
        //     return -1;
        // }

        if (fileSize > server->LARGE_FILE_THRESHOLD)
        {
            std::string httpResponse = server->createChunkedHttpResponse(contentType);
            if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            {
                std::cerr << "Failed to send chunked HTTP header." << std::endl;
                return -1;
            }

            FileTransferState state;
            state.filePath = filePath;
            state.fileSize = fileSize;
            state.offset = 0;
            state.isComplete = false;
            server->fileTransfers[fd] = state;

            return continueFileTransfer_post(fd, server);
        }
        else
        {
            std::string httpResponse = server->httpResponse(contentType, fileSize);

            if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            {
                std::cerr << "Failed to send HTTP header." << std::endl;
                return -1;
            }

            std::vector<char> buffer(fileSize);
            size_t bytesRead = 0;

            if (!readFileChunk_post(filePath, buffer.data(), 0, fileSize, bytesRead))
            {
                std::cerr << "Failed to read file: " << filePath << std::endl;
                return -1;
            }

            if (send(fd, buffer.data(), bytesRead, MSG_NOSIGNAL) == -1)
            {
                std::cerr << "Failed to send file content." << std::endl;
                return -1;
            }

            return redirectTheParh(buffer, filePath, bytesRead);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in handleFileRequest_post: " << e.what() << std::endl;
        return -1;
    }
}

std::pair<std::string, std::string> ft_parseRequest(std::string request)
{
    std::pair<std::string, std::string> pair_request(request.substr(0, request.find("\r\n\r\n", 0)),
                                                     request.substr(request.find("\r\n\r\n", 0), request.length()));
    return pair_request;
}

size_t returnTargetFromRequest(std::string request, std::string target)
{
    std::set<std::string> node;
    char *result = strstr((char *)request.c_str(), (char *)target.c_str());
    size_t j = 0;
    for (size_t i = 0; i < strlen(result); i++)
    {
        if (result[i] == '\n')
        {
            node.insert(static_cast<std::string>(result).substr(j, i));
            j = i + 1;
        }
    }
    std::set<std::string>::iterator it = node.begin();
    size_t n;
    while (it != node.end())
    {
        n = static_cast<size_t>(atoi((it->substr((it->find((char *)(" "), 0)) + 1, it->length())).c_str()));
        it++;
    }
    return n;
}

int Server::handle_post_request(int fd, Server *server, std::string request)
{
    std::pair<std::string, std::string> pair_request = ft_parseRequest(request);
    if (returnTargetFromRequest(pair_request.first, "Content-Length") == 0)
    {
        //--------
    }

    // Check if we already have a file transfer in progress
    if (server->fileTransfers.find(fd) != server->fileTransfers.end())
    {
        // Continue the existing transfer
        return continueFileTransfer_post(fd, server);
    }

    std::string filePath = server->parseRequest(request, server);
    if (canBeOpen(filePath) && getFileType(filePath) == 2)
    {
        return handleFileRequest_post(fd, server, filePath);
    }
    else
    {
        // Handle 404 Not Found scenario
        std::string path1 = PATHE;
        std::string path2 = "404.html";
        std::string new_path = path1 + path2;
        std::string content = readFile(new_path);
        std::string httpResponse = createNotFoundResponse(server->getContentType(new_path), content.length());

        // Consolidated error handling
        try
        {
            if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            {
                throw std::runtime_error("Failed to send error response header");
            }

            if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
            {
                throw std::runtime_error("Failed to send error content");
            }

            if (send(fd, "\r\n\r\n", 2, MSG_NOSIGNAL) == -1)
            {
                throw std::runtime_error("Failed to send final CRLF");
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
            close(fd);
            return -1;
        }

        server->fileTransfers.erase(fd);
        close(fd);
    }
    return 0;
}