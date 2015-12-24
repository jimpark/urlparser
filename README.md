# urlparser
Parse URLs using C++

Simply add the three files, Parse.h, UrlParser.h and UrlParser.cpp to your project to parse URLs. You can look at the example usage in main.cpp to see it generate a JSON output of the parsed URL.

The Parse.h contains a PEG-like parser engine. It can be used to parse other things but as-is it's roughly equal in power to a regular expression engine. I was just using this project to explore PEG parsing and marrying it with C++ templates.
