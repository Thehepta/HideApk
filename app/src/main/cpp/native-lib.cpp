#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <sys/mman.h>
#include <android/log.h>
#include "linker.h"

#define LOG_TAG "Native"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define MAX_LINE_LENGTH 256
struct Module {
    char name[MAX_LINE_LENGTH];
    intptr_t start_address;
    intptr_t end_address;
    uint8_t perms;
};



void maps_hide(Module module) {

    size_t size = module.end_address - module.start_address;
    void *addr = reinterpret_cast<void *>(module.start_address);

    void *copy = mmap(nullptr, size, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if ((module.perms & PROT_READ) == 0) {
        mprotect(addr, size, PROT_READ);
    }
    memcpy(copy, addr, size);
    mremap(copy, size, size, MREMAP_MAYMOVE | MREMAP_FIXED, addr);
    mprotect(addr, size, module.perms);
    munmap(copy,size);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_hepta_hideapk_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

#include <sys/mman.h>
#include <linux/ashmem.h>
#include <sys/ioctl.h>
#include <fcntl.h>


uint8_t *create_ashmem_fd(int size){
//    android_dlextinfo extinfo;
    int fd = open("/dev/ashmem", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if (ioctl(fd, ASHMEM_SET_SIZE, size) < 0) {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    uint8_t *ptr = static_cast<uint8_t *>(mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0));
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    return ptr;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_hepta_hideapk_MainActivity_hideApk(JNIEnv *env, jobject thiz, jstring s) {

    char* pkgName = const_cast<char *>(env->GetStringUTFChars(s, 0));
    LoadApkModule(pkgName);

    return;
}