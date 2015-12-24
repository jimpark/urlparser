//
//  UrlParser.cpp
//
//  Created by Jim Park on 12/18/15.
//  Copyright © 2015 Jim Park. All rights reserved.
//

#include "UrlParser.h"
#include <sstream>

namespace urlparser {
    using namespace std;
    using namespace parse;
    using namespace parse::u8;

    UrlData::UrlData()
            : port(-1) {
    }

    std::string UrlData::ToJSON() const {
        ostringstream o;
        o << "{\n";
        o << "\t\"protocol\":" << "\"" << protocol << "\",\n";
        o << "\t\"host\":" << "\"" << host << "\",\n";
        o << "\t\"user\":" << "\"" << user << "\",\n";
        o << "\t\"password\":" << "\"" << password << "\",\n";
        o << "\t\"port\":" << port << "\n";
        o << "\t\"path\":" << "\"" << path << "\",\n";
        o << "\t\"queries\":" << "{\n";

        for (const auto& i : queries) {
            o << "\t\t\"" << i.first << "\": \"" << i.second << "\",\n";
        }

        o << "\t},\n";
        o << "\t\"reference\":" << "\"" << reference << "\",\n";
        o << "}" << endl;
        return o.str();
    }

    void UrlData::clear() {
        protocol.clear();
        host.clear();
        user.clear();
        password.clear();
        port = -1;
        path.clear();
        queries.clear();
        reference.clear();
    }

    UrlParser::UrlParser() {
        static auto p_ = [=](){
            auto parseProtocol = Capture(protocol, OoM(w())) + str("://");
            auto parseUser = Capture(user, OoM(!(cs({':', '@'})))) +
                             ZoO(c(':') + Capture(password, OoM(!(c('@'))))) + c('@');
            auto parsePort = c(':') + Capture(port, OoM(d()));
            auto parseHost = Capture(host, OoM(!(cs({'/', ':', '?', '#'}))));

            auto parsePathElement = OoM(!(cs({'/', '#', '?'})));
            auto parsePath = c('/') + Capture(path, ZoM((parsePathElement + c('/')) | parsePathElement) + ZoO(c('/')));

            auto parseQuery = (OoM(!(c('='))) + c('=') + OoM(!(cs({'&', '#'})))) |
                              (OoM(!(cs({'=', '&', '#'}))));
            auto parseQueryList = c('?') + Capture(queries, parseQuery + ZoM(c('&') + parseQuery));

            auto parseRef = c('#') + Capture(reference, OoM(any()));
            auto parseUrl = parseProtocol + ZoO(parseUser) + parseHost + ZoO(parsePort) +
                                   ZoO(parsePath) + ZoO(parseQueryList) + ZoO(parseRef) + stop();
            return parseUrl;
        }();

        urlParser = &p_;
    }

    bool UrlParser::parse(const std::string &url, UrlData &data) {
        return parse(url.begin(), url.end(), data);
    }

    bool UrlParser::parse(it i, it end, UrlData &data) {
        if ((*urlParser)(i, end)) {
            data.protocol = protocol;
            data.host = host;
            data.user = user;
            data.password = password;
            if (port.empty()) {
                data.port = -1;
            } else {
                data.port = atoi(port.c_str());
            }
            data.path = path;
            if (!queries.empty()) {
                data.queries = parseQueries(queries);
            }
            data.reference = reference;
            return true;
        }
        return false;
    }

    unordered_map<string, string> UrlParser::parseQueries(const std::string& queries) {
        unordered_map<string, string> result;
        char c;
        auto i = queries.begin();
        auto end = queries.end();
        enum State { Key, Value };
        State s = Key;
        string key;
        string value;

        while (i != end) {
            c = *i;
            switch (c) {
                case '&':
                    result[key] = value;
                    key.clear();
                    value.clear();
                    s = Key;
                    break;
                case '=':
                    s = Value;
                    break;
                default:
                    if (s == Key) {
                        key.push_back(c);
                    } else {
                        value.push_back(c);
                    }
                    break;
            }
            ++i;
        }

        if (!key.empty()) {
            result[key] = value;
        }

        return result;
    };
}
