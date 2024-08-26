#include <android/log.h>
#include <asm-generic/fcntl.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <zconf.h>
#include <asm-generic/mman.h>
#include <linux/mman.h>
#include <sys/mman.h>
//#include "jni_hook.h"
#include "jni.h"
#define LOG_TAG "Native"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)





 void LoadEntry_test(JNIEnv *env, jclass clazz, jstring str) {
    const char * string = env->GetStringUTFChars(str, nullptr);
    LOGE("LoadEntry_text_fun :%s",string);
}

void LoadEntry_test2(JNIEnv *env, jclass clazz, jstring str) {
    const char * string = env->GetStringUTFChars(str, nullptr);
    LOGE("LoadEntry_text_fun2 :%s",string);
}

void LoadEntry_test3(JNIEnv *env, jclass clazz, jstring str) {
    const char * string = env->GetStringUTFChars(str, nullptr);
    LOGE("LoadEntry_text_fun3 :%s",string);
}

void LoadEntry_test4(JNIEnv *env, jclass clazz, jstring str) {
    const char * string = env->GetStringUTFChars(str, nullptr);
    LOGE("LoadEntry_text_fun4 :%s",string);
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {

    LOGE("Call JNI_OnLoad");
    JNIEnv* env;
    if (vm->GetEnv( (void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
//    JavaVM * replace_vm = jni_hook_init(vm);
//    if(replace_vm->functions->GetEnv(replace_vm, reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK){
//        return -1;
//    }



    jclass classTest = env->FindClass("com/hepta/fridaload/LoadEntry");
    JNINativeMethod methods[]= {
            {"test", "(Ljava/lang/String;)V", (void*)LoadEntry_test},
            {"test2", "(Ljava/lang/String;)V", (void*)LoadEntry_test2},
            {"test3", "(Ljava/lang/String;)V", (void*)LoadEntry_test2},
            {"test4", "(Ljava/lang/String;)V", (void*)LoadEntry_test2},
    };
    env->RegisterNatives(classTest, methods, sizeof(methods)/sizeof(JNINativeMethod));
    // 在这里进行一些初始化工作
    return JNI_VERSION_1_6;
}


extern "C"
JNIEXPORT jobject JNICALL
Java_com_hepta_hideapk_MainActivity_zipLoadApk(JNIEnv *env, jobject thiz, jstring s) {
    const char* apk_path = env->GetStringUTFChars(s, nullptr);
    // 打开文件
    int fd = open(apk_path, O_RDONLY);
    if (fd == -1) {
        LOGE("Failed to open file: %s", strerror(errno));
        env->ReleaseStringUTFChars(s, apk_path);
    }

    return nullptr;
}