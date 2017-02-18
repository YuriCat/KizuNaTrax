/*
 string.hpp
 Katsuki Ohto
 */

// 文字列処理
// puyoAI
// https://github.com/puyoai/puyoai
// のbaseコードに
// 色々とつけたしたもの

#ifndef UTIL_STRING_HPP_
#define UTIL_STRING_HPP_

#include <string>
#include <sstream>
#include <valarray>

#include "../defines.h"
#include "../util/io.hpp"

static std::string fat(const std::string& s){
    return "\x1b[1m" + s + "\x1b[0m";
}

static bool isPrefix(const std::string& s,
                     const std::string& prefix){
    if (s.size() < prefix.size()){
        return false;
    }
    
    return s.substr(0, prefix.size()) == prefix;
}

static bool isSuffix(const std::string& s,
                     const std::string& suffix){
    if (s.size() < suffix.size()){
        return false;
    }
    
    return s.substr(s.size() - suffix.size()) == suffix;
}

static bool contains(const std::string& s,
                     const std::string& t){
    return s.find(t) != std::string::npos;
}

static std::string trim(const std::string& s, const char dlm = ' '){
    std::string::size_type p1 = s.find_first_not_of(dlm);
    if(p1 == std::string::npos){
        return std::string();
    }
    
    std::string::size_type p2 = s.find_last_not_of(dlm);
    return s.substr(p1, p2 - p1 + 1);
}

static std::vector<std::string> split(const std::string& s,
                                      char separator){
    std::vector<std::string> result;
    std::string::size_type p = 0;
    std::string::size_type q;
    while((q = s.find(separator, p)) != std::string::npos){
        result.emplace_back(s, p, q - p);
        p = q + 1;
    }
    
    result.emplace_back(s, p);
    return result;
}

static std::vector<std::string> splitByString(const std::string& s,
                                              const std::string& separator){
    std::vector<std::string> result;
    std::string::size_type p = 0;
    std::string::size_type q;
    while((q = s.find(separator, p)) != std::string::npos){
        result.emplace_back(s, p, q - p);
        p = q + 1;
    }
    
    result.emplace_back(s, p);
    return result;
}

static std::vector<std::string> split(const std::string& s,
                                      const std::string& separators){
    std::vector<std::string> result;
    std::string::size_type p = 0;
    std::string::size_type q;
    
    bool found;
    do{
        found = false;
        std::string::size_type minq = s.size();
        for(char sep : separators){
            //cerr << "separator : " << sep << endl;
            if((q = s.find(sep, p)) != std::string::npos){
                minq = min(minq, q);
                found = true;
            }
        }
        if(found){
            result.emplace_back(s, p, minq - p);
            p = minq + 1;
        }
    }while(found);
    result.emplace_back(s, p);
    return result;
}

static std::string join(const std::vector<std::string>& v,
                        const std::string& sep){
    if(v.size() == 0){
        return std::string();
    }
    
    std::ostringstream os;
    os << v[0];
    for(std::size_t i = 1; i < v.size(); ++i){
        os << sep << v[i];
    }
    return os.str();
}

static std::string lineUp(const std::vector<std::string>& v,
                          const std::string::size_type padding = 0,
                          const char separator = '\n'){
    std::vector<std::vector<std::string>> splitted;
    std::size_t maxComponents = 0;
    for(const auto& s : v){
        splitted.emplace_back(split(s, separator));
        maxComponents = std::max(maxComponents, splitted.back().size());
    }
    std::vector<std::string::size_type> maxLength;
    for(std::size_t i = 0; i < splitted.size(); ++i){
        maxLength.emplace_back(0);
        const auto& sp = splitted[i];
        for(const auto& s : sp){
            cerr << s.size() << s << endl;
            maxLength[i] = max(maxLength[i], s.size());
        }
    }
    //cerr << maxLength << endl;
    std::stringstream oss;
    for(std::size_t j = 0; j < maxComponents; ++j){
        for(std::size_t i = 0; i < splitted.size(); ++i){
            const auto& sp = splitted[i];
            if(j < sp.size()){
                oss << sp[j] << std::string(maxLength[i] - sp[j].size() + padding, ' ');
            }else{
                oss << std::string(maxLength[i] + padding, ' ');
            }
        }
        oss << std::endl;
    }
    return oss.str();
}

static std::string lineUp(const std::string& s0,
                          const std::string& s1,
                          const std::string::size_type padding = 0,
                          const char separator = '\n'){
    return lineUp({s0, s1}, padding, separator);
}

static std::string packLineUp(const std::vector<std::string>& v,
                              const int padding = 0,
                              const char separator = '\n'){
    std::vector<std::vector<std::string>> splitted;
    std::size_t maxComponents = 0;
    for(const auto& s : v){
        splitted.emplace_back(split(s, separator));
        maxComponents = std::max(maxComponents, splitted.back().size());
    }
    std::stringstream oss;
    for(std::size_t j = 0; j < maxComponents; ++j){
        for(std::size_t i = 0; i < splitted.size(); ++i){
            const auto& sp = splitted[i];
            if(j < sp.size()){
                oss << sp[j];
            }
            oss << std::string(padding, ' ');
        }
        oss << std::endl;
    }
    return oss.str();
}

static std::string packLineUp(const std::string& s0,
                              const std::string& s1,
                              const std::string::size_type padding = 0,
                              const char separator = '\n'){
    return packLineUp({s0, s1}, padding, separator);
}

#endif // UTIL_STRING_HPP_