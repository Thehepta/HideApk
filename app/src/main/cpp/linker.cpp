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
#include<unordered_map>



android_namespace_t* g_default_namespace = static_cast<android_namespace_t *>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl_g_default_namespace"));

soinfo* (*soinf_alloc_fun)(android_namespace_t* , const char* ,const struct stat* , off64_t ,uint32_t ) = (soinfo* (*)(android_namespace_t* , const char* ,const struct stat* , off64_t ,uint32_t )) resolve_elf_internal_symbol(get_android_linker_path(),"__dl__Z12soinfo_allocP19android_namespace_tPKcPK4statlj");



const char* fix_dt_needed(const char* dt_needed, const char* sopath __unused) {
#if !defined(__LP64__)
  int app_target_api_level = android_get_application_target_sdk_version();
  if (app_target_api_level < 23) {
    const char* bname = basename(dt_needed);
    if (bname != dt_needed) {
      DL_WARN_documented_change(23,
                                "invalid-dt_needed-entries-enforced-for-api-level-23",
                                "library \"%s\" has invalid DT_NEEDED entry \"%s\"",
                                sopath, dt_needed, app_target_api_level);
//      add_dlwarning(sopath, "invalid DT_NEEDED entry",  dt_needed);
    }

    return bname;
  }
#endif
    return dt_needed;
}


template<typename F>
static void for_each_dt_needed(const ElfReader& elf_reader, F action) {
    for (const ElfW(Dyn)* d = elf_reader.dynamic(); d->d_tag != DT_NULL; ++d) {
        if (d->d_tag == DT_NEEDED) {
            action(fix_dt_needed(elf_reader.get_string(d->d_un.d_val), elf_reader.name()));
        }
    }
}



class LoadTask {
public:

    static LoadTask* create(const char* name,
                            soinfo* needed_by,
                            android_namespace_t* start_from,
                            std::unordered_map<const soinfo*, ElfReader>* readers_map) {
        return new  LoadTask(name, needed_by, start_from, readers_map);
    }




    LoadTask(const char* name,
             soinfo* needed_by,
             android_namespace_t* start_from,
             std::unordered_map<const soinfo*, ElfReader>* readers_map)
            : name_(name), needed_by_(needed_by), si_(nullptr),
              fd_(-1), close_fd_(false), file_offset_(0), elf_readers_map_(readers_map),
              is_dt_needed_(false), start_from_(start_from) {}

    // returns the namespace from where we need to start loading this.
    const android_namespace_t* get_start_from() const {
        return start_from_;
    }

    std::unordered_map<const soinfo*, ElfReader>* get_readers_map() {
        return elf_readers_map_;
    }

    soinfo* get_soinfo() const {
        return si_;
    }

    void set_file_offset(off64_t offset) {
        file_offset_ = offset;
    }

    off64_t get_file_offset() const {
        return file_offset_;
    }

    off64_t get_file_size() const {
        return file_size_;
    }
    void set_file_size(off64_t offset) {
        file_size_ = offset;
    }

    void set_soinfo(soinfo* si) {
        si_ = si;
    }

    bool read() {
        //    task->add_elf_readers_map()

        ElfReader &elf_reader = get_elf_reader();
        return elf_reader.Read("", fd_, file_offset_, file_size_);

    }


    bool load(address_space_params* address_space) {
        ElfReader& elf_reader = get_elf_reader();
        if (!elf_reader.Load(address_space)) {
            return false;
        }
        si_->base = elf_reader.load_start();
        si_->size = elf_reader.load_size();
        si_->set_mapped_by_caller(elf_reader.is_mapped_by_caller());
        si_->load_bias = elf_reader.load_bias();
        si_->phnum = elf_reader.phdr_count();
        si_->phdr = elf_reader.loaded_phdr();
        return true;
    }


    void set_fd(int fd, bool assume_ownership) {
        if (fd_ != -1 && close_fd_) {
            close(fd_);
        }
        fd_ = fd;
        close_fd_ = assume_ownership;
    }

    void add_elf_readers_map(const soinfo* si, ElfReader elfReader){
        elf_readers_map_->emplace(si,elfReader);
    }

    ElfReader& get_elf_reader() {
        return (*elf_readers_map_)[si_];
    }


    ~LoadTask() {
        if (fd_ != -1 && close_fd_) {
            close(fd_);
        }
    }

    bool load(address_space_params* address_space) {
        ElfReader& elf_reader = get_elf_reader();
        if (!elf_reader.Load(address_space)) {
            return false;
        }

        si_->base = elf_reader.load_start();
        si_->size = elf_reader.load_size();
        si_->set_mapped_by_caller(elf_reader.is_mapped_by_caller());
        si_->load_bias = elf_reader.load_bias();
        si_->phnum = elf_reader.phdr_count();
        si_->phdr = elf_reader.loaded_phdr();

        return true;
    }

private:

    const char* name_;
    soinfo* needed_by_;
    soinfo* si_;
    const android_dlextinfo* extinfo_;
    int fd_;
    bool close_fd_;
    off64_t file_offset_;
    off64_t file_size_;
    std::unordered_map<const soinfo*, ElfReader>* elf_readers_map_;
    // TODO(dimitry): needed by workaround for http://b/26394120 (the grey-list)
    bool is_dt_needed_;
    // END OF WORKAROUND
    const android_namespace_t* const start_from_;

//    DISALLOW_IMPLICIT_CONSTRUCTORS(LoadTask);

};

typedef std::vector<LoadTask*> LoadTaskList;




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

void linker_protect(){


    void* g_soinfo_allocator = static_cast<void *>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL18g_soinfo_allocator"));
    void* g_soinfo_links_allocator = static_cast<void *>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL24g_soinfo_links_allocator"));
    void* g_namespace_allocator = static_cast<void *>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL21g_namespace_allocator"));
    void* g_namespace_list_allocator = static_cast<void *>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL26g_namespace_list_allocator"));


    void (*protect_all)(void*,int prot) = (void (*)(void*,int prot))resolve_elf_internal_symbol(get_android_linker_path(),"__dl__ZN20LinkerBlockAllocator11protect_allEi");
    protect_all(g_soinfo_allocator,PROT_READ | PROT_WRITE);      //arg1 = 0x73480D23D8
    protect_all(g_soinfo_links_allocator,PROT_READ | PROT_WRITE);
    protect_all(g_namespace_allocator,PROT_READ | PROT_WRITE);
    protect_all(g_namespace_list_allocator,PROT_READ | PROT_WRITE);
}

void linker_unprotect(){


    void* g_soinfo_allocator = static_cast<void *>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL18g_soinfo_allocator"));
    void* g_soinfo_links_allocator = static_cast<void *>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL24g_soinfo_links_allocator"));
    void* g_namespace_allocator = static_cast<void *>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL21g_namespace_allocator"));
    void* g_namespace_list_allocator = static_cast<void *>(resolve_elf_internal_symbol(get_android_linker_path(), "__dl__ZL26g_namespace_list_allocator"));


    void (*protect_all)(void*,int prot) = (void (*)(void*,int prot))resolve_elf_internal_symbol(get_android_linker_path(),"__dl__ZN20LinkerBlockAllocator11protect_allEi");
    protect_all(g_soinfo_allocator,PROT_READ  );
    protect_all(g_soinfo_links_allocator,PROT_READ  );
    protect_all(g_namespace_allocator,PROT_READ  );
    protect_all(g_namespace_list_allocator,PROT_READ  );
}


soinfo* soinfo_alloc(ApkNativeInfo &apkNativeInfo){


    soinfo* (*soinf_alloc_fun)(android_namespace_t* , const char* ,const struct stat* , off64_t ,uint32_t ) = (soinfo* (*)(android_namespace_t* , const char* ,const struct stat* , off64_t ,uint32_t )) resolve_elf_internal_symbol(get_android_linker_path(),"__dl__Z12soinfo_allocP19android_namespace_tPKcPK4statlj");
    soinfo* si = soinf_alloc_fun(g_default_namespace, apkNativeInfo.libname.c_str(), nullptr, 0, RTLD_GLOBAL);
    return si;
}


void* LoadNativeSoByMem(ApkNativeInfo &apkNativeInfo){







//    LoadTask task = new LoadTask();


    soinfo * si_ = soinfo_alloc(apkNativeInfo);
    if (si_ == nullptr) {
        return nullptr;
    }

//    task->set_soinfo(si);



    ElfReader * elf_reader = new ElfReader();
    apkNativeInfo.libname = apkNativeInfo.file_stat.m_filename;

    if(!elf_reader->Read(apkNativeInfo.libname.c_str(),apkNativeInfo.fd,0,apkNativeInfo.file_stat.m_uncomp_size)){
        LOGE("elf_reader Read failed");
        return nullptr;
    }



    address_space_params  default_params;
    elf_reader->Load(&default_params);



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



bool find_library_internal(const android_namespace_t *ns, LoadTask *task,
                           LoadTaskList *load_tasks) {






    return true;
}









bool LoadApkModule(char * apkSource){


//    soinfo *si = find_containing_library((const void*)linker_protect);
//    return true;


    std::unordered_map<const soinfo*, ElfReader> readers_map;
    LoadTaskList load_tasks;

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
            int fd;
            uint8_t * somem_addr = Creatememfd(&fd, file_stat.m_uncomp_size);

            if (!somem_addr) {
                printf("Failed to allocate memory.\n");
                mz_zip_reader_end(&zip_archive);
                return false;
            }
            if (!mz_zip_reader_extract_to_mem(&zip_archive, i, somem_addr, file_stat.m_uncomp_size, 0)) {
                printf("Failed to extract file.\n");
                mz_zip_reader_end(&zip_archive);
                return false;
            }

            LoadTask* task =  LoadTask::create(file_stat.m_filename, nullptr,g_default_namespace, &readers_map);
            task->set_fd(fd, false);
            task->set_file_offset(0);
            task->set_file_size(file_stat.m_uncomp_size);
            load_tasks.push_back(task);
//            printf("Filename: \"%s\", Comment: \"%s\", Uncompressed size: %llu\n",file_stat.m_filename, file_stat.m_comment ? file_stat.m_comment : "",(mz_uint64) file_stat.m_uncomp_size);
        }
    }
    mz_zip_reader_end(&zip_archive);


    linker_protect();


    for (size_t i = 0; i<load_tasks.size(); ++i) {

        LoadTask* task = load_tasks[i];

        soinfo* si = soinf_alloc_fun(g_default_namespace, ""/*real path*/, nullptr, 0, RTLD_GLOBAL);
        if (si == nullptr) {
            return false;
        }
        task->set_soinfo(si);

        if (!task->read()) {
//            soinfo_free(si);
            task->set_soinfo(nullptr);
            return false;
        }
        const ElfReader& elf_reader = task->get_elf_reader();
        for (const ElfW(Dyn)* d = elf_reader.dynamic(); d->d_tag != DT_NULL; ++d) {
            if (d->d_tag == DT_RUNPATH) {
                si->set_dt_runpath(elf_reader.get_string(d->d_un.d_val));
            }
            if (d->d_tag == DT_SONAME) {
                si->set_soname(elf_reader.get_string(d->d_un.d_val));
            }
        }

        for_each_dt_needed(task->get_elf_reader(), [&](const char* name) {
            LOGE("NEED name: %s",name);
//            load_tasks.push_back(LoadTask::create(name, si, const_cast<android_namespace_t *>(task->get_start_from()), task->get_readers_map()));
        });
    }
    address_space_params  default_params;
    for (auto&& task : load_tasks) {
        task->load(&default_params);
    }


    linker_unprotect();


}






















































