/*
 io.hpp
 Katsuki Ohto
 */

#ifndef UTIL_IO_HPP_
#define UTIL_IO_HPP_

#include <dirent.h>
#include <sys/stat.h>

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

// 入力
std::vector<std::string> getFilePathVector(const std::string& ipath,
                                           const std::string& condition = ""){
    // 指定されたディレクトリ内のファイルのパス一覧を作る
    std::vector<std::string> fileNames;
    
    DIR *pdir;
    dirent *pentry;
    
    pdir = opendir(ipath.c_str());
    if(pdir == nullptr){
        return fileNames;
    }
    do{
        pentry = readdir(pdir);
        if(pentry != nullptr){
            const std::string name = std::string(pentry->d_name);
            const std::string fullPath = ipath + name;
            struct stat st;
            int result = stat(fullPath.c_str(), &st);
            if((st.st_mode & S_IFMT) == S_IFDIR){ // is directory
            }else if(condition.size() == 0 || name.find(condition) != std::string::npos){
                fileNames.emplace_back(fullPath);
            }
        }
    }while(pentry != nullptr);
    return fileNames;
}

std::vector<std::string> getFilePathVectorRecursively(const std::string& ipath,
                                                      const std::string& condition = ""){
    // 指定されたディレクトリ内のファイルのパス一覧を再帰的に作る
    std::vector<std::string> fileNames;
    
    DIR *pdir;
    dirent *pentry;
    
    pdir = opendir(ipath.c_str());
    if(pdir == nullptr){
        return fileNames;
    }
    do{
        pentry = readdir(pdir);
        if(pentry != nullptr){
            const std::string name = std::string(pentry->d_name);
            const std::string fullPath = ipath + name;
            struct stat st;
            int result = stat(fullPath.c_str(), &st);
            if((st.st_mode & S_IFMT) == S_IFDIR){ // is directory
                if(name != "." && name != ".."){
                    std::vector<std::string> tfileNames = getFilePathVectorRecursively(fullPath + "/", condition);
                    fileNames.insert(fileNames.end(), tfileNames.begin(), tfileNames.end()); // add vector
                }
            }else if(condition.size() == 0 || name.find(condition) != std::string::npos){
                fileNames.emplace_back(fullPath);
            }
        }
    }while(pentry != nullptr);
    return fileNames;
}

#endif // UTIL_IO_HPP_