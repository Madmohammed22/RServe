#include "server.hpp"

ConfigurationFile::ConfigurationFile()
{
}

ConfigurationFile::ConfigurationFile(const ConfigurationFile &Init)
{
    (void)Init;
}

ConfigurationFile &ConfigurationFile::operator=(const ConfigurationFile &Init)
{
    if (this == &Init)
        return *this;

    return *this;
}

ConfigurationFile::~ConfigurationFile()
{
}

bool ConfigurationFile::TheBalancedParentheses(std::string file)
{
    std::ifstream fileName(file.c_str());
    std::stack<char> s;
    std::string line;
    std::vector<std::string> chaine;
    std::string buffer;
    while (std::getline(fileName, line))
    {
        chaine.push_back(line);
        for (size_t i = 0; i < line.size(); i++)
            buffer.push_back(line.at(i));
    }
    for (size_t i = 0; i < buffer.size(); i++)
    {
        if (buffer.at(i) == '{')
            s.push(buffer.at(i));
        else if (buffer.at(i) == '}')
        {
            if (s.empty() == true)
                return false;
            s.pop();
        }
    }
    return s.empty();
}
