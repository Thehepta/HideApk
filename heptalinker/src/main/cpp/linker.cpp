#include <jni.h>
#include <string>
#include "hideload/entry.h"
extern "C" JNIEXPORT jstring JNICALL
Java_com_hepta_linker_NativeLib_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++===";
    return env->NewStringUTF(hello.c_str());
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
