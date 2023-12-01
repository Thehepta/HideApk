//
// Created by chic on 2023/11/26.
//

#include <cstdio>
#include <locale>
#include <linux/ashmem.h>
#include "linker.h"
#include "linker_phdr.h"
#include "linker_soinfo.h"
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <vector>
#include "elf_symbol_resolver.h"
#include "linker_debug.h"
#include <sys/syscall.h>
#include <sys/utsname.h>


static inline uintptr_t untag_address(uintptr_t p) {
#if defined(__aarch64__)
    return p & ((1ULL << 56) - 1);
#else
    return p;
#endif
}

template <typename T>
static inline T* untag_address(T* p) {
    return reinterpret_cast<T*>(untag_address(reinterpret_cast<uintptr_t>(p)));
}


soinfo* find_containing_library(const void* p) {

    static soinfo* (*solist_get_head)() = NULL;
    if (!solist_get_head)
        solist_get_head = (soinfo* (*)())resolve_elf_internal_symbol(get_android_linker_path(), "__dl__Z15solist_get_headv");

    static soinfo* (*solist_get_somain)() = NULL;
    if (!solist_get_somain)
        solist_get_somain = (soinfo* (*)())resolve_elf_internal_symbol(get_android_linker_path(), "__dl__Z17solist_get_somainv");

    ElfW(Addr) address = reinterpret_cast<ElfW(Addr)>(untag_address(p));
    for (soinfo* si = solist_get_head(); si != nullptr; si = si->next) {
        if (address < si->base || address - si->base >= si->size) {
            continue;
        }
        ElfW(Addr) vaddr = address - si->load_bias;
        for (size_t i = 0; i != si->phnum; ++i) {
            const ElfW(Phdr)* phdr = &si->phdr[i];
            if (phdr->p_type != PT_LOAD) {
                continue;
            }
            if (vaddr >= phdr->p_vaddr && vaddr < phdr->p_vaddr + phdr->p_memsz) {
                return si;
            }
        }
    }
    return nullptr;
}




soinfo* soinfo_alloc(ApkNativeInfo &apkNativeInfo){


    soinfo* (*soinf_alloc_fun)(android_namespace_t* , const char* ,const struct stat* , off64_t ,uint32_t ) = (soinfo* (*)(android_namespace_t* , const char* ,const struct stat* , off64_t ,uint32_t )) resolve_elf_internal_symbol(get_android_linker_path(),
                                                                                                                                                                                                                                     "__dl__Z12soinfo_allocP19android_namespace_tPKcPK4statlj");
    void (*protect_all)(void*,int prot) = (void (*)(void*,int prot))resolve_elf_internal_symbol(get_android_linker_path(),"__dl__ZN20LinkerBlockAllocator11protect_allEi");


    void** g_soinfo_allocator = static_cast<void **>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL18g_soinfo_allocator"));
    void** g_soinfo_links_allocator = static_cast<void **>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL24g_soinfo_links_allocator"));
    void** g_namespace_allocator = static_cast<void **>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL21g_namespace_allocator"));
    void** g_namespace_list_allocator = static_cast<void **>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL26g_namespace_list_allocator"));
    protect_all(*g_soinfo_allocator,PROT_READ | PROT_WRITE);
    protect_all(*g_soinfo_links_allocator,PROT_READ | PROT_WRITE);
    protect_all(*g_namespace_allocator,PROT_READ | PROT_WRITE);
    protect_all(*g_namespace_list_allocator,PROT_READ | PROT_WRITE);

//    struct stat file_stat;
//    Dl_info info;
//    void* addr = (void*)soinfo_alloc; // 你要查询的地址，这里以main函数的地址为例
//    soinfo * local_si = find_containing_library(addr);
    soinfo* si = soinf_alloc_fun(nullptr,apkNativeInfo.libname.c_str(), nullptr,0,RTLD_GLOBAL);
    protect_all(*g_soinfo_allocator,PROT_READ  );
    protect_all(*g_soinfo_links_allocator,PROT_READ  );
    protect_all(*g_namespace_allocator,PROT_READ  );
    protect_all(*g_namespace_list_allocator,PROT_READ  );


    return si;
}


void* LoadNativeSoByMem(ApkNativeInfo &apkNativeInfo){

    ElfReader * elf_reader = new ElfReader();
    apkNativeInfo.libname = apkNativeInfo.file_stat.m_filename;
    if(!elf_reader->Read(apkNativeInfo.libname.c_str(),apkNativeInfo.fd,0,apkNativeInfo.file_stat.m_uncomp_size)){
        LOGE("elf_reader Read failed");
        return nullptr;
    }
    address_space_params  default_params;
    elf_reader->Load(&default_params);
    soinfo * si_ = soinfo_alloc(apkNativeInfo);
    si_->base = elf_reader->load_start();
    si_->size = elf_reader->load_size();
    si_->set_mapped_by_caller(elf_reader->is_mapped_by_caller());
    si_->load_bias = elf_reader->load_bias();
    si_->phnum = elf_reader->phdr_count();
    si_->phdr = elf_reader->loaded_phdr();

    si_->prelink_image();
    si_->set_dt_flags_1(si_->get_dt_flags_1() | DF_1_GLOBAL);
    si_->call_constructors();
    LOGE("%s","successful");
}


int memfd_create(const char* name, unsigned int flags) {
    // Check kernel version supports memfd_create(). Some older kernels segfault executing
    // memfd_create() rather than returning ENOSYS (b/116769556).
    static constexpr int kRequiredMajor = 3;
    static constexpr int kRequiredMinor = 17;
    struct utsname uts;
    int major, minor;
    if (uname(&uts) != 0 ||
        strcmp(uts.sysname, "Linux") != 0 ||
        sscanf(uts.release, "%d.%d", &major, &minor) != 2 ||
        (major < kRequiredMajor || (major == kRequiredMajor && minor < kRequiredMinor))) {
        errno = ENOSYS;
        return -1;
    }
    return syscall(__NR_memfd_create, name, flags);
}




uint8_t * Creatememfd(int *fd, int size){

    *fd = memfd_create("test", MFD_CLOEXEC);
    if (*fd < 0) {
        perror("Creatememfd Filede: open /dev/ashmem failed");
        exit(EXIT_FAILURE);
    }

    ftruncate(*fd, size);

    uint8_t *ptr = static_cast<uint8_t *>(mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, *fd,0));
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

bool compareSoDependency(ApkNativeInfo a, ApkNativeInfo b) {
    // Sort in descending order



//    return a > b;
}

bool LoadApkModule(char * apkSource){

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    mz_bool status = mz_zip_reader_init_file(&zip_archive, apkSource, 0);
    if (!status) {
        printf("Could not initialize zip reader.\n");
        return false;
    }
    std::vector<ApkNativeInfo> vec_apkNativeInfo;
    int file_count = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < file_count; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
            printf("Could not retrieve file info.\n");
            mz_zip_reader_end(&zip_archive);
            return false;
        }
        if(strstr(file_stat.m_filename,APK_NATIVE_LIB)!= NULL) {
            ApkNativeInfo apkNativeInfo;
            apkNativeInfo.file_stat = file_stat;
            apkNativeInfo.somem_addr = Creatememfd(&apkNativeInfo.fd, file_stat.m_uncomp_size);

            if (!apkNativeInfo.somem_addr) {
                printf("Failed to allocate memory.\n");
                mz_zip_reader_end(&zip_archive);
                return false;
            }
            if (!mz_zip_reader_extract_to_mem(&zip_archive, i, apkNativeInfo.somem_addr, file_stat.m_uncomp_size, 0)) {
                printf("Failed to extract file.\n");
                mz_zip_reader_end(&zip_archive);
                return false;
            }
            vec_apkNativeInfo.push_back(apkNativeInfo);
//            printf("Filename: \"%s\", Comment: \"%s\", Uncompressed size: %llu\n",file_stat.m_filename, file_stat.m_comment ? file_stat.m_comment : "",(mz_uint64) file_stat.m_uncomp_size);
        }
    }
    mz_zip_reader_end(&zip_archive);
//    std::sort(vec_apkNativeInfo.begin(), vec_apkNativeInfo.end(), compareSoDependency);

    for (auto curNativeInfo : vec_apkNativeInfo) {
        LoadNativeSoByMem(curNativeInfo);
    }



}





