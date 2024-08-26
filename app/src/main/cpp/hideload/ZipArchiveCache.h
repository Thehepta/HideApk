//
// Created by rzx on 2024/8/26.
//

#pragma once


#include "user_system.h"

const char* const kZipFileSeparator = "!/";


class ZipArchiveCache {
public:

    ZipArchiveCache();
    ~ZipArchiveCache();

    bool get_or_open(const char* zip_path, ZipArchiveHandle* handle);
private:
    DISALLOW_COPY_AND_ASSIGN(ZipArchiveCache);

//    std::unordered_map<std::string, void*> cache_;
};



