#include "../server.hpp"

std::string Server::parseRequest(std::string request, Server *server)
{
    (void)server;
    // std::cout << "-------( REQUEST PARSED )-------\n\n";
    // std::cout << request << std::endl;
    // std::cout << "-------( END OF REQUEST )-------\n\n";

    if (request.empty())
        return "";

    std::string filePath = "/index.html";

    // Handle GET requests
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
        return filePath;
    }

    // Handle DELETE requests
    startPos = request.find("DELETE /");
    if (startPos != std::string::npos)
    {
        startPos += 8;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            if (!requestedPath.empty() && requestedPath != "/")
            {
                filePath = requestedPath;
            }
        }
        return filePath;
    }

    // Handle POST requests
    startPos = request.find("POST /");
    if (startPos != std::string::npos)
    {
        startPos += 5;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            if (!requestedPath.empty() && requestedPath == "/")
            {
                filePath = PATHC;
            }
            else
                return requestedPath;
        }
        return filePath;
    }

    return filePath;
}
