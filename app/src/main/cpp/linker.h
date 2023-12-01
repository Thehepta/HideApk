//
// Created by chic on 2023/11/25.
//

#pragma once

#include <dlfcn.h>
#include <android/dlext.h>
#include <elf.h>
#include <inttypes.h>
#include <link.h>
#include <sys/stat.h>
#include <unistd.h>
#include "miniz.h"


#if defined(__LP64__)
#define ELFW(what) ELF64_ ## what
#define APK_NATIVE_LIB "lib/arm64-v8a"
#else
#define ELFW(what) ELF32_ ## what
#define APK_NATIVE_LIB "lib/armeabi-v7a"
#endif


static int get_android_system_version() {
    char os_version_str[100];
    __system_property_get("ro.build.version.sdk", os_version_str);
    int os_version_int = atoi(os_version_str);
    return os_version_int;
}


static const char *get_android_linker_path() {
#if __LP64__
    if (get_android_system_version() >= 29) {
        return (const char *)"/apex/com.android.runtime/bin/linker64";
    } else {
        return (const char *)"/system/bin/linker64";
    }
#else
    if (get_android_system_version() >= 29) {
        return (const char *)"/apex/com.android.runtime/bin/linker";
    } else {
        return (const char *)"/system/bin/linker";
    }
#endif
}




#define SUPPORTED_DT_FLAGS_1 (DF_1_NOW | DF_1_GLOBAL | DF_1_NODELETE | DF_1_PIE)


struct ApkNativeInfo {
    mz_zip_archive_file_stat file_stat;
    uint8_t * somem_addr;
    std::string libname;
    int fd;
};
void* LoadNativeSoByMem(uint8_t * soArrayMem,int length);
uint8_t * Creatememfd(int fd, int size);
bool LoadApkModule(char * apkSource);
struct address_space_params {
    void* start_addr = nullptr;
    size_t reserved_size = 0;
    bool must_use_address = false;
};

#define PAGE_START(x) ((x) & PAGE_MASK)

// Returns the offset of address 'x' in its page.
#define PAGE_OFFSET(x) ((x) & ~PAGE_MASK)

// Returns the address of the next page after address 'x', unless 'x' is
// itself at the start of a page.
#define PAGE_END(x) PAGE_START((x) + (PAGE_SIZE-1))


static constexpr uintptr_t align_up(uintptr_t p, size_t align) {
      return (p + align - 1) & ~(align - 1);
}

template <typename T>
static inline T* align_up(T* p, size_t align) {
      return reinterpret_cast<T*>(align_up(reinterpret_cast<uintptr_t>(p), align));
}

static constexpr uintptr_t align_down(uintptr_t p, size_t align) {
      return p & ~(align - 1);
}

template <typename T>
static inline T* align_down(T* p, size_t align) {
      return reinterpret_cast<T*>(align_down(reinterpret_cast<uintptr_t>(p), align));
}


constexpr unsigned kLibraryAlignmentBits = 18;
constexpr size_t kLibraryAlignment = 1UL << kLibraryAlignmentBits;


