/*
 io.hpp
 Katsuki Ohto
*/

#ifndef UTIL_IO_HPP_
#define UTIL_IO_HPP_

#include <iostream>
#include <array>
#include <vector>
#include <valarray>

#include "../defines.h"

// 出力簡略化
template<class T, std::size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& arg){
    os << "{";
    for(int i = 0; i < (int)N - 1; ++i){
        os << arg[i] << ", ";
    }
    if(arg.size() > 0){
        os << arg[N - 1];
    }
    os << "}";
    return os;
}

template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& arg){
    os << "{";
    for(int i = 0; i < (int)arg.size() - 1; ++i){
        os << arg[i] << ", ";
    }
    if(arg.size() > 0){
        os << arg[arg.size() - 1];
    }
    os << "}";
    return os;
}

template<class T>
std::ostream& operator<<(std::ostream& os, const std::valarray<T>& arg){
    os << "{";
    for(int i = 0; i < (int)arg.size() - 1; ++i){
        os << arg[i] << ", ";
    }
    if(arg.size() > 0){
        os << arg[arg.size() - 1];
    }
    os << "}";
    return os;
}

template<class T, std::size_t N>
std::string toString(const std::array<T, N>& arg, const std::string& format = ""){
    std::ostringstream os;
    if(format.size() < 1){
        os << "{";
    }else if(format.size() > 2){
        os << format[0];
    }
    for(int i = 0; i < (int)N - 1; ++i){
        os << arg[i];
        if(format.size() < 1){
            os << ", ";
        }else if(format.size() > 2){
            os << format.substr(1, format.size() - 2);
        }else{
            os << format[0];
        }
    }
    if(arg.size() > 0){
        os << arg[arg.size() - 1];
    }
    if(format.size() < 1){
        os << "}";
    }else if(format.size() > 2){
        os << format[format.size() - 1] << endl;
    }
    return os.str();
}

template<class T>
std::string toString(const T arg[], const std::size_t n, const std::string& format = ""){
    std::ostringstream os;
    if(format.size() < 1){
        os << "{";
    }else if(format.size() > 2){
        os << format[0];
    }
    for(int i = 0; i < (int)n - 1; ++i){
        os << arg[i];
        if(format.size() < 1){
            os << ", ";
        }else if(format.size() > 2){
            os << format.substr(1, format.size() - 2);
        }else{
            os << format[0];
        }
    }
    if(n > 0){
        os << arg[n - 1];
    }
    if(format.size() < 1){
        os << "}";
    }else if(format.size() > 2){
        os << format[format.size() - 1] << endl;
    }
    return os.str();
}

template<class T>
std::string toString(const std::vector<T>& arg, const std::string& format = ""){
    std::ostringstream os;
    if(format.size() < 1){
        os << "{";
    }else if(format.size() > 2){
        os << format[0];
    }
    for(int i = 0; i < (int)arg.size() - 1; ++i){
        os << arg[i];
        if(format.size() < 1){
            os << ", ";
        }else if(format.size() > 2){
            os << format.substr(1, format.size() - 2);
        }else{
            os << format[0];
        }
    }
    if(arg.size() > 0){
        os << arg[arg.size() - 1];
    }
    if(format.size() < 1){
        os << "}";
    }else if(format.size() > 2){
        os << format[format.size() - 1] << endl;
    }
    return os.str();
}

template<class T>
std::string toString(const std::valarray<T>& arg, const std::string& format = ""){
    std::ostringstream os;
    if(format.size() < 1){
        os << "{";
    }else if(format.size() > 2){
        os << format[0];
    }
    for(int i = 0; i < (int)arg.size(); ++i){
        os << arg[i];
        if(format.size() < 1){
            os << ", ";
        }else if(format.size() > 2){
            os << format.substr(1, format.size() - 2);
        }else{
            os << format[0];
        }
    }
    if(arg.size() > 0){
        os << arg[arg.size() - 1];
    }
    if(format.size() < 1){
        os << "}";
    }else if(format.size() > 2){
        os << format[format.size() - 1] << endl;
    }
    return os.str();
}

#endif // UTIL_IO_HPP_