#include "../server.hpp"

std::string Server::getContentType(const std::string &path)
{
    std::map<std::string, std::string> extensionToType;
    extensionToType.insert(std::make_pair(std::string(".html"), std::string("text/html")));
    extensionToType.insert(std::make_pair(std::string(".css"), std::string("text/css")));
    extensionToType.insert(std::make_pair(std::string(".txt"), std::string("text/txt")));
    extensionToType.insert(std::make_pair(std::string(".js"), std::string("application/javascript")));
    extensionToType.insert(std::make_pair(std::string(".json"), std::string("application/json")));
    extensionToType.insert(std::make_pair(std::string(".cpp"), std::string("application/cpp")));
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

std::string Server::getCurrentTimeInGMT() {
    time_t t = time(0);
    tm *time_struct = gmtime(&t); // Use gmtime to get UTC time

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", time_struct);
    std::string date = buffer;
    return date;
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

std::string Server::createChunkedHttpResponse(std::string contentType)
{
    return "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "; charset=utf-8" + "\r\nTransfer-Encoding: chunked\r\n\r\n";
}

std::string Server::httpResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";
    return oss.str();
}


std::string Server::createNotFoundResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 404 Not Found\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";

    return oss.str();
}


std::string getCurrentTimeInGMT1() {
    time_t t = time(0);
    tm *time_struct = gmtime(&t); // Use gmtime to get UTC time

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", time_struct);
    std::string date = buffer;
    return date;
}


std::string Server::createBadResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 400 Bad Request\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT1() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";

    return oss.str();
}

std::string Server::methodNotAllowedResponse(std::string contentType, int contentLength)
{
    (void)contentLength;
    std::ostringstream oss;
    oss << "HTTP/1.1 405 Method Not Allowed\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Allow: GET, POST, DELETE\r\n\r\n";
        
    // << "Content-Length: " << contentLength << "\r\n"
    return oss.str();
}

std::string Server::createTimeoutResponse(std::string contentType, size_t contentLength)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 408 Request Timeout\r\n"
        << "Content-Type: " << contentType + "; charset=utf-8" << "\r\n"
        << "Last-Modified: " << getCurrentTimeInGMT() << "\r\n"
        << "Content-Length: " << contentLength << "\r\n\r\n";

    return oss.str();
}
