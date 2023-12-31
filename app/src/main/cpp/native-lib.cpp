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

void load_dex(JNIEnv *pEnv) {
    jobjectArray JAAR = nullptr;
    auto classloader = pEnv->FindClass("java/lang/ClassLoader");
    auto getsyscl_mid = pEnv->GetStaticMethodID(classloader, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
    auto sys_classloader = pEnv->CallStaticObjectMethod(classloader, getsyscl_mid);
    jmethodID method_loadClass = pEnv->GetMethodID(classloader,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");

    if (!sys_classloader){
        LOGE("getSystemClassLoader failed!!!");
        return;
    }
    auto in_memory_classloader = pEnv->FindClass( "dalvik/system/InMemoryDexClassLoader");
    auto initMid = pEnv->GetMethodID( in_memory_classloader, "<init>",
                                      "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
    auto byte_buffer_class = pEnv->FindClass("java/nio/ByteBuffer");
    auto dex_buffer = pEnv->NewDirectByteBuffer(dex_addr, dex_size);
    if (auto my_cl = pEnv->NewObject( in_memory_classloader, initMid, dex_buffer, sys_classloader)) {
        jobject  sand_class_loader_ = pEnv->NewGlobalRef( my_cl);

        jstring xposed = pEnv->NewStringUTF("com.rxposed.sandhooktextapp.XposedTest");
        jobject XposedTest = pEnv->CallObjectMethod(sand_class_loader_,method_loadClass,xposed);

        jclass Class_cls = pEnv->FindClass("java/lang/Class");
        jmethodID clsmethod_method = pEnv->GetMethodID(Class_cls,"getMethod","(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;");
        jclass Method_cls = pEnv->FindClass("java/lang/reflect/Method");
        jmethodID invoke_met =  pEnv->GetMethodID(Method_cls,"invoke","(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");

        jobject user_metohd = nullptr;
        jstring  call_method_name = pEnv->NewStringUTF("native_hook");
        user_metohd = pEnv->CallObjectMethod(XposedTest, clsmethod_method,call_method_name,JAAR);
        pEnv->CallObjectMethod(user_metohd, invoke_met,XposedTest,JAAR);

    } else {
        LOGE("InMemoryDexClassLoader creation failed!!!");
        return;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_hepta_hideapk_MainActivity_hideApk(JNIEnv *env, jobject thiz, jstring s) {

    char* pkgName = const_cast<char *>(env->GetStringUTFChars(s, 0));

    LoadApkModule(pkgName);
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