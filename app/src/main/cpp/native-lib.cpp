#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <sys/mman.h>
#include <android/log.h>
#include <linux/ashmem.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "hideload/entry.h"
#define LOG_TAG "Native"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define MAX_LINE_LENGTH 256
#define CHUNK_SIZE 4096  // 每次读取的块大小

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

    jobject g_currentDexLoad = FilehideLoadApkModule(env, pkgName);
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
Java_com_hepta_hideapk_MainActivity_customMemhideApkLoad(JNIEnv *env, jobject thiz, jstring s) {

    LOGE("Java_com_hepta_hideapk_MainActivity_customMemhideApkLoad");
    char* apkSource = const_cast<char *>(env->GetStringUTFChars(s, 0));
    auto classloader = env->FindClass("java/lang/ClassLoader");


    unsigned char* zip_data = NULL;
    size_t zip_size;

    FILE* file = fopen(apkSource, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        zip_size = ftell(file);
        rewind(file);

        zip_data = (unsigned char*)malloc(zip_size);
        if (zip_data) {
            if (fread(zip_data, 1, zip_size, file) != zip_size) {
                free(zip_data);
                zip_data = NULL;
            }
        }
        fclose(file);
    }
    jobject g_currentDexLoad = memhideLoadApkModule(env, zip_data, zip_size);
    jmethodID method_loadClass = env->GetMethodID(classloader,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");
    jstring LoadEntry_cls = env->NewStringUTF("com.hepta.fridaload.LoadEntry");
    jobject LoadEntrycls_obj = env->CallObjectMethod(g_currentDexLoad,method_loadClass,LoadEntry_cls);
    jstring arg1 = env->NewStringUTF("load hiedapk direct call jni native methdo successful");
    jmethodID call_jni_method_mth = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "test", "(Ljava/lang/String;)V");
    jmethodID call_jni_method_mth2 = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "test2", "(Ljava/lang/String;)V");
    jmethodID call_jni_method_mth3 = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "test3", "(Ljava/lang/String;)V");
    jmethodID call_jni_method_mth4 = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "test4", "(Ljava/lang/String;)V");
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_jni_method_mth,arg1);
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_jni_method_mth2,arg1);
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_jni_method_mth3,arg1);
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_jni_method_mth4,arg1);

    jmethodID call_java_method_mth = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "test_java_call_native", "(Ljava/lang/String;)V");
    jstring arg2 = env->NewStringUTF("load hiedapk direct call java methdo successful");
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_java_method_mth,arg2);

    jmethodID call_text_java_method_mth = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "text_java", "()V");
    jstring arg3 = env->NewStringUTF("load call java methdo successful");
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_text_java_method_mth,arg3);
    return;
}



extern "C"
JNIEXPORT void JNICALL
Java_com_hepta_hideapk_MainActivity_customFilehideApkLoad(JNIEnv *env, jobject thiz, jstring s) {

    LOGE("Java_com_hepta_hideapk_MainActivity_customFilehideApkLoad");

    char* pkgName = const_cast<char *>(env->GetStringUTFChars(s, 0));
    auto classloader = env->FindClass("java/lang/ClassLoader");

    jobject g_currentDexLoad = FilehideLoadApkModule(env, pkgName);
    jmethodID method_loadClass = env->GetMethodID(classloader,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");
    jstring LoadEntry_cls = env->NewStringUTF("com.hepta.fridaload.LoadEntry");
    jobject LoadEntrycls_obj = env->CallObjectMethod(g_currentDexLoad,method_loadClass,LoadEntry_cls);
    jstring arg1 = env->NewStringUTF("load hiedapk direct call jni native methdo successful");
    jmethodID call_jni_method_mth = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "test", "(Ljava/lang/String;)V");
    jmethodID call_jni_method_mth2 = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "test2", "(Ljava/lang/String;)V");
    jmethodID call_jni_method_mth3 = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "test3", "(Ljava/lang/String;)V");
    jmethodID call_jni_method_mth4 = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "test4", "(Ljava/lang/String;)V");
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_jni_method_mth,arg1);
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_jni_method_mth2,arg1);
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_jni_method_mth3,arg1);
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_jni_method_mth4,arg1);

    jmethodID call_java_method_mth = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "test_java_call_native", "(Ljava/lang/String;)V");
    jstring arg2 = env->NewStringUTF("load hiedapk direct call java methdo successful");
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_java_method_mth,arg2);

    jmethodID call_text_java_method_mth = env->GetStaticMethodID(static_cast<jclass>(LoadEntrycls_obj), "text_java", "()V");
    jstring arg3 = env->NewStringUTF("load call java methdo successful");
    env->CallStaticVoidMethod(static_cast<jclass>(LoadEntrycls_obj), call_text_java_method_mth,arg3);
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
extern "C"
JNIEXPORT void JNICALL
Java_com_hepta_hideapk_MainActivity_SystenStubLoadSo(JNIEnv *env, jobject thiz) {

    const char *libc_name = "/apex/com.android.runtime/lib64/bionic/libdl.so";  // libc 的路径，可能因系统而异
    int libc_fd = open(libc_name, O_RDONLY);
    if (libc_fd < 0) {
        perror("open");
        return ;
    }

    // 获取文件信息
    struct stat file_stat;
    if (fstat(libc_fd, &file_stat) == -1) {
        perror("fstat");
        close(libc_fd);
        return ;
    }
    size_t libc_file_size = file_stat.st_size;

    const char *libdl_name = "/apex/com.android.runtime/lib64/bionic/libc.so";  // libc 的路径，可能因系统而异
    int libdl_fd = open(libdl_name, O_RDONLY);
    if (libdl_fd < 0) {
        perror("open");
        return ;
    }

    // 映射文件到内存
    uint8_t *map = static_cast<uint8_t *>(mmap(NULL, libc_file_size, PROT_READ | PROT_WRITE,
                                               MAP_PRIVATE, libdl_fd, 0));
    if (map == MAP_FAILED) {
        perror("mmap");
        return ;
    }
    ssize_t bytes_read;
    ssize_t file_offset = 0;
    while ((bytes_read = read(libc_fd, map + file_offset, CHUNK_SIZE)) > 0) {
        LOGE("copy = %zd",bytes_read);
        file_offset += bytes_read;
    }
    LOGE("read size = %zd",file_offset);
    LOGE("libc_file_size  = %zu",libc_file_size);
    void *ret_si = load_so_by_fd(libdl_fd);
    void *dlsym_addr = custom_dlsym(ret_si, "dlsym");
    LOGE("libc_file_size  = %p",dlsym_addr);

    return;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_hepta_hideapk_MainActivity_customLinkerLoadPathSO(JNIEnv *env, jobject thiz) {
    // TODO: implement customLinkerLoadPathSO()
    const char *libdl_name = "/apex/com.android.runtime/lib64/bionic/libdl.so";  // libc 的路径，可能因系统而异
    int libdl_fd = open(libdl_name, O_RDONLY);
    if (libdl_fd < 0) {
        perror("open");
        return ;
    }
    void *ret_si = load_so_by_fd(libdl_fd);
    void *dlsym_addr = custom_dlsym(ret_si, "dlsym");
    close(libdl_fd);
    LOGE("libc_file_size  = %p",dlsym_addr);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_hepta_hideapk_MainActivity_classloadDexmerge(JNIEnv *env, jobject thiz, jstring source,
                                                      jobject class_loader) {
    char* pkgName = const_cast<char *>(env->GetStringUTFChars(source, nullptr));
    Class_DexFile_Merge(env,pkgName,class_loader);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_hepta_hideapk_MainActivity_plt_1hook(JNIEnv *env, jobject thiz) {
    PLT_HOOK();
}