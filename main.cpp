#include <iostream>
#include "UrlParser.h"

using namespace std;
using namespace urlparser;

int main() {
    UrlParser parser;
    UrlData data;
    string line;

    while (getline(cin, line)) {
        if (line == "quit") {
            return 0;
        }

        if (parser.parse(line, data)) {
            cout << data.ToJSON() << endl;
            data.clear();
        }
    }
}