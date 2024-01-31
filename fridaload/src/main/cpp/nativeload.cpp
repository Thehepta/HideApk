#include <android/log.h>
//#include "jni_hook.h"
#include "jni.h"
#define LOG_TAG "Native"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)






jint LoadEntry_text(JNIEnv *env, jclass clazz, jstring str) {
    const char * string = env->GetStringUTFChars(str, nullptr);
    LOGE("TEXT:%s",string);
}


JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {

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
            {"text", "(Ljava/lang/String;)V", (void*)LoadEntry_text},
    };
    env->RegisterNatives(classTest, methods, sizeof(methods)/sizeof(JNINativeMethod));
    // 在这里进行一些初始化工作
    LOGE("TEXT:%s","JNI_OnLoad");
    return JNI_VERSION_1_6;
}

