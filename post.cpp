#include "server.hpp"

// Read file in chunks
bool readFileChunk_post(const std::string &path, char *buffer, size_t offset, size_t chunkSize, size_t &bytesRead)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return false;
    
    file.seekg(offset);
    file.read(buffer, chunkSize);
    bytesRead = file.gcount();
    
    return true;
}

// Send a chunk of data using chunked encoding
bool sendChunk_post(int fd, const char* data, size_t size)
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
bool sendFinalChunk_post(int fd)
{
    return sendChunk_post(fd, "", 0) && 
           send(fd, "\r\n", 2, MSG_NOSIGNAL) != -1;
}


// Continue sending chunks for an in-progress file transfer
int continueFileTransfer_post(int fd, Server * server)
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
    
    if (!readFileChunk_post(state.filePath, buffer, state.offset, bytesToRead, bytesRead))
    {
        std::cerr << "Failed to read chunk from file: " << state.filePath << std::endl;
        server->fileTransfers.erase(fd);
        return -1;
    }
    
    if (!sendChunk_post(fd, buffer, bytesRead))
    {
        std::cerr << "Failed to send chunk." << std::endl;
        server->fileTransfers.erase(fd);
        return -1;
    }
    
    state.offset += bytesRead;
    
    // Check if we've sent the entire file
    if (state.offset >= state.fileSize)
    {
        if (!sendFinalChunk_post(fd))
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
int handleFileRequest_post(int fd, Server *server, const std::string &filePath)
{
    std::string contentType = server->getContentType(filePath);
    size_t fileSize = server->getFileSize(filePath);
    
    if (fileSize == 0)
        return std::cerr << "Failed to get file size or empty file: " << filePath << std::endl, -1;
    
    // Determine if file is large enough to warrant chunked encoding
    const size_t LARGE_FILE_THRESHOLD = 1024 * 1024; // 1MB
    
    if (fileSize > LARGE_FILE_THRESHOLD)
    {
        // Use chunked encoding for large files
        std::string httpResponse = server->createChunkedHttpResponse(contentType);
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
        return continueFileTransfer_post(fd, server);
    }
    else
    {
        // Use standard Content-Length for smaller files
        std::string httpResponse = server->generateHttpResponse(contentType, fileSize);
        std::cout << "[2][" << httpResponse << "]" << std::endl;
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send HTTP header." << std::endl, -1;
        
        // Read and send the entire file at once
        char* buffer = new char[fileSize];
        size_t bytesRead = 0;
        if (!readFileChunk_post(filePath, buffer, 0, fileSize, bytesRead))
            return std::cerr << "Failed to read file: " << filePath << std::endl, delete[] buffer, -1;
        
        int result = send(fd, buffer, bytesRead, MSG_NOSIGNAL);
        delete[] buffer;
        
        if (result == -1)
            return std::cerr << "Failed to send file content." << std::endl, -1;
        
        return 0;
    }
}
int Server::handle_post_request(int fd, Server *server, std::string request)
{
    if (request.empty())
        return -1;
    // Check if we already have a file transfer in progress
    if (server->fileTransfers.find(fd) != server->fileTransfers.end())
    {
        // Continue the existing transfer
        return continueFileTransfer_post(fd, server);
    }

    std::string filePath = server->parseRequest(request, server);
    if (canBeOpen(filePath) && getFileType(filePath) == 2)
    {
        std::cout << "I was here\n";
        return handleFileRequest_post(fd, server, filePath);
    }
    else
    {
        std::string path1 = PATHE;
        std::string path2 = "404.html";
        std::string new_path = path1 + path2;
        std::string content = readFile(new_path);
        std::string httpResponse = createNotFoundResponse(server->getContentType(new_path), content.length());
        
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