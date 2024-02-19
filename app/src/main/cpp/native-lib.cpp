#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <sys/mman.h>
#include <android/log.h>
#include "hideload/linker.h"
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
//    int a = addtext(1,2);
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
JNIEXPORT jobject JNICALL
Java_com_hepta_hideapk_MainActivity_GethideApkLoad(JNIEnv *env, jobject thiz, jstring source) {

    char* pkgName = const_cast<char *>(env->GetStringUTFChars(source, 0));
    auto classloader = env->FindClass("java/lang/ClassLoader");

    jobject g_currentDexLoad = hideLoadApkModule(env, pkgName);
//    jmethodID method_loadClass = env->GetMethodID(classloader,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");
//
//    jobject LoadEntrycls_obj = env->CallObjectMethod(g_currentDexLoad,method_loadClass,className);
//    const char* entry_method = env->GetStringUTFChars(method_name, nullptr);
//    jmethodID call_method_mth = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), entry_method, "(Landroid/content/Context;Ljava/lang/String;)V");
//    jstring aerg = env->NewStringUTF("load hiedapk is successful");
//    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_method_mth,aerg);
    return g_currentDexLoad;
}









extern "C"
JNIEXPORT void JNICALL
Java_com_hepta_hideapk_MainActivity_customhideApkLoad(JNIEnv *env, jobject thiz, jstring s) {

    char* pkgName = const_cast<char *>(env->GetStringUTFChars(s, 0));
    auto classloader = env->FindClass("java/lang/ClassLoader");

    jobject g_currentDexLoad = hideLoadApkModule(env, pkgName);
    jmethodID method_loadClass = env->GetMethodID(classloader,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");

    jstring LoadEntry_cls = env->NewStringUTF("com.hepta.fridaload.LoadEntry");
    jobject LoadEntrycls_obj = env->CallObjectMethod(g_currentDexLoad,method_loadClass,LoadEntry_cls);
    jmethodID call_method_mth = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "text", "(Ljava/lang/String;)V");
    jstring aerg = env->NewStringUTF("load hiedapk is successful");
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_method_mth,aerg);
    return;
}

struct android_namespace_user_t{


    int a;
private:
    bool is_isolated_;
    bool is_greylist_enabled_;
    bool is_also_used_as_anonymous_;
};


extern "C"
JNIEXPORT void JNICALL
Java_com_hepta_hideapk_MainActivity_soglobal(JNIEnv *env, jobject thiz, jstring source_dir) {
    // TODO: implement soglobal()

    std::string pkgName = const_cast<char *>(env->GetStringUTFChars(source_dir, 0));
    pkgName=pkgName+"!/lib/arm64-v8a/libnativeloader.so";
    void * handle = dlopen(pkgName.c_str(),RTLD_NOW);
    android_namespace_user_t **bbs1  = static_cast<android_namespace_user_t **>(dlsym(handle, "bbs"));
    int (*user_print)(void*)  = reinterpret_cast<int (*)(void *)>(dlsym(handle,"_ZN24android_namespace_user_t10user_printEv"));

    android_namespace_user_t *bbs = static_cast<android_namespace_user_t *>(*bbs1);
    LOGE("%d",bbs->a);
    int text = user_print(bbs);
    LOGE("%d",text);


}




__attribute__((constructor)) static void beforeFunction()
{
    LOGE("beforeFunction\n");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_hepta_hideapk_MainActivity_customhideSoLoad(JNIEnv *env, jobject thiz,
                                                     jstring libname_path) {
    // TODO: implement customhideSoLoad()
}