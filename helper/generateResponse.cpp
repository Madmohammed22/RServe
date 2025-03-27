#include "../server.hpp"

 std::string Server::renderHtml(std::string path, Server *server){
    (void)server;
    std::string state = "Success";
    std::string coler = "rgb(50, 150, 0)";
    std::string title = "Delete Confirmation";
    
    std::string message = "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "    <style>\n"
    "        * {\n"
    "            margin: 0;\n"
    "            padding: 0;\n"
    "            box-sizing: border-box;\n"
    "        }\n"
    "        body {\n"
    "            font-family: Arial, sans-serif;\n"
    "            background-color: #D3D3D3;\n"
    "            display: flex;\n"
    "            justify-content: center;\n"
    "            align-items: center;\n"
    "            height: 100vh;\n"
    "        }\n"
    "        .error-container {\n"
    "            text-align: center;\n"
    "            background-color: #fff;\n"
    "            padding: 20px;\n"
    "            border-radius: 5px;\n"
    "            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);\n"
    "        }\n"
    "        h1 {\n"
    "            font-size: 5rem;\n"
    "            color:" + coler + ";\n"
    "        }\n"
    "        p {\n"
    "            font-size: 1.5rem;\n"
    "            color: #333;\n"
    "            margin-bottom: 20px;\n"
    "        }\n"
    "    </style>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "    <title>" + title + "</title>\n"
    "</head>\n"
    "<body>\n"
    "    <div class=\"error-container\">\n"
    "        <h1> " + state + "</h1>\n"

    "        <p>" + path + "\" deleted successfully.</p>\n"
    "    </div>\n"
    "</body>\n"
    "</html>";
    return message;
}
