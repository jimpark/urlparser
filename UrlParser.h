//
//  UrlParser.h
//
//  Created by Jim Park on 12/18/15.
//  Copyright © 2015 Jim Park. All rights reserved.
//

#ifndef URLPARSER_URLPARSER_H
#define URLPARSER_URLPARSER_H

#include <string>
#include <unordered_map>
#include "parse.h"

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

        std::string ToJSON() const;
        void clear();
    };

    class UrlParser {
    public:
        using it = std::string::const_iterator;

        UrlParser();

        bool parse(const std::string &url, UrlData &data);

        bool parse(it i, it end, UrlData &data);

        std::unordered_map<std::string, std::string> parseQueries(
               const std::string& queries);

    private:
        parse::Node<char>* urlParser;
        std::string protocol;
        std::string host;
        std::string user;
        std::string password;
        std::string port;
        std::string path;
        std::string queries;
        std::string reference;
    };
}

#endif //URLPARSER_URLPARSER_H
