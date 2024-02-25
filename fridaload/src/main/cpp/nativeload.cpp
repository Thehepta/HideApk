#include <android/log.h>
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

