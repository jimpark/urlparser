//
// Created by Jim on 12/18/2015.
//

#include "UrlParser.h"

namespace urlparser {
    using namespace std;

    UrlData::UrlData()
            : port(-1) {
    }

    UrlParser::UrlParser() {
    }

    bool UrlParser::parse(const std::string &url, UrlData &data) {
        return parse(url.begin(), url.end(), data);
    }

    bool UrlParser::parse(it i, it end, UrlData &data) {
        if (i == end) { return false; }
    }

    bool UrlParser::parseProtocol(it &i, it end, std::string &protocol) {
        string result;
        while (i != end && *i != ':')
        {
            result.push_back(*i);
            ++i;
        }

        if (i == end)
        {
            return false;
        }

        protocol = std::move(result);
        return true;
    }
}
