//
// Created by thehepta on 2024/1/16.
//

#include "jni_hook.h"

#include <android/log.h>
#define LOG_TAG "JNI_HOOK"
#define HOOKLOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

jint  GetEnv(JavaVM * vm, void** env, jint version){

    HOOKLOGE("GetEnv");
    Linker_JavaVM* linkerJavaVm = (Linker_JavaVM *) vm;
    int re = linkerJavaVm->vm->GetEnv(env, version);
    Linker_JNIEnv * linkerJniEnv = new Linker_JNIEnv((JNIEnv *)*env);
    *env = linkerJniEnv;
    return re;
}
jint DestroyJavaVM(JavaVM*vm ){

    HOOKLOGE("DestroyJavaVM");
    Linker_JavaVM* linkerJavaVm = (Linker_JavaVM *) vm;
    return linkerJavaVm->vm->DestroyJavaVM();
}

jint AttachCurrentThread(JavaVM*vm,JNIEnv** p_env, void* thr_args){
    Linker_JavaVM* linkerJavaVm = (Linker_JavaVM *) vm;
    return linkerJavaVm->vm->AttachCurrentThread( p_env, thr_args);
}
jint DetachCurrentThread(JavaVM*vm){
    Linker_JavaVM* linkerJavaVm = (Linker_JavaVM *) vm;
    return linkerJavaVm->vm->DetachCurrentThread();
}

jint AttachCurrentThreadAsDaemon(JavaVM*vm,JNIEnv** p_env, void* thr_args){
    Linker_JavaVM* linkerJavaVm = (Linker_JavaVM *) vm;
    return linkerJavaVm->vm->AttachCurrentThreadAsDaemon( p_env, thr_args);
}


Linker_JavaVM::Linker_JavaVM(JavaVM *vm) {
    this->vm = vm;
    functions = new Linker_JNIInvokeInterface();
    functions->GetEnv = GetEnv;
    functions->DestroyJavaVM = DestroyJavaVM;
    functions->AttachCurrentThread = AttachCurrentThread;
    functions->DetachCurrentThread = DetachCurrentThread;
    functions->AttachCurrentThreadAsDaemon = AttachCurrentThreadAsDaemon;

}



const char* GetStringUTFChars(JNIEnv * env,jstring string, jboolean* isCopy){
    HOOKLOGE("GetStringUTFChars");
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->GetStringUTFChars( string, isCopy);

}

jclass FindClass(JNIEnv * env,const char* name){
    HOOKLOGE("FindClass");
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->FindClass( name);
}

Linker_JNIEnv::Linker_JNIEnv(JNIEnv * env){
    this->env = env;
    functions = new Linker_JNINativeInterface();
    functions->GetStringUTFChars = GetStringUTFChars;
    functions->FindClass = FindClass;
}



JavaVM* jni_hook_init(JavaVM* vm){
    Linker_JavaVM *linkerJavaVm = new Linker_JavaVM(vm);

    return (JavaVM*)linkerJavaVm;
}



