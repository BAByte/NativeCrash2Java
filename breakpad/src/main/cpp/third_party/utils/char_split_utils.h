//
// Created by ba on 2/12/22.
//
#ifndef BANATIVECRASH_CHAR_SPLIT_UTILS_H
#define BANATIVECRASH_CHAR_SPLIT_UTILS_H
namespace babyte {
    extern std::vector<char *> SplitChar(char *str, char *delim) {
        std::vector<char *> result;
        try {
            if (nullptr == str) return result;
            char *splitRes = strtok(str, delim);
            while (splitRes) {
                result.push_back(splitRes);
                splitRes = strtok(NULL, delim);
            }
        } catch (std::exception) {
            //nothing todo
        }

        return result;
    }
}
#endif //BANATIVECRASH_CHAR_SPLIT_UTILS_H
