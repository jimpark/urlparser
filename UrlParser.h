//
// Created by Jim on 12/18/2015.
//

#ifndef URLPARSER_URLPARSER_H
#define URLPARSER_URLPARSER_H

#include <string>
#include <unordered_map>

namespace urlparser {
    struct UrlData {
        UrlData();

        std::string protocol;
        std::string host;
        std::string user;
        std::string password;
        int port;
        std::string path;
        std::unordered_map<std::string, std::string> queries;
        std::string reference;
    };

    class UrlParser {
    public:
        using it = std::string::const_iterator;

        UrlParser();

        bool parse(const std::string &url, UrlData &data);

        bool parse(it i, it end, UrlData &data);

        bool parseProtocol(it &i, it end, std::string &protocol);
    };
}


#endif //URLPARSER_URLPARSER_H
