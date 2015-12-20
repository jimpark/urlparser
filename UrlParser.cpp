//
// Created by Jim on 12/18/2015.
//

#include "UrlParser.h"
#include <sstream>

namespace urlparser {
    using namespace std;
    using namespace parse;

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
            auto parseProtocol = Capture(protocol, OoM(w<char>())) + Str("://");
            auto parseUser = Capture(user, OoM(Not(ChSet<char>({':', '@'})))) +
                             ZoO(Ch(':') + Capture(password, OoM(Not(Ch('@'))))) + Ch('@');
            auto parsePort = Ch(':') + Capture(port, OoM(d<char>()));
            auto parseHost = Capture(host, OoM(Not(ChSet<char>({'/', ':'}))));

            auto parsePathElement = OoM(Not(ChSet<char>({'/', '#', '?'})));
            auto parsePath = Capture(path, parsePathElement + ZoM(Ch('/') + parsePathElement) + ZoO(Ch('/')));

            auto parseQuery = (OoM(Not(ChSet<char>({'=', '&', '#'})))) |
                              (OoM(Not(Ch('='))) + Ch('=') + OoM(Not(ChSet<char>({'&', '#'}))));
            auto parseQueryList = Ch('?') + Capture(queries, parseQuery + ZoM(Ch('&') + parseQuery));

            auto parseUrl = parseProtocol + ZoO(parseUser) + parseHost + ZoO(parsePort) + ZoO(Ch('/')) +
                                   ZoO(parsePath) + ZoO(parseQueryList) + End<char>();
            return parseUrl;
        }();

        urlParser = &p_;
    }

    bool UrlParser::parse(const std::string &url, UrlData &data) {
        return parse(url.begin(), url.end(), data);
    }

    bool UrlParser::parse(it i, it end, UrlData &data) {
        urlParser->reset();
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
        }

        if (!key.empty()) {
            result[key] = value;
        }

        return result;
    };
}
