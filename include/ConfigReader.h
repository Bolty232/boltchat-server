#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <string>
#include <map>
#include <optional>

std::optional<std::map<std::string, std::string>> readConfig(const std::string &path);


#endif //CONFIGREADER_H
