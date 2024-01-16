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


    return linkerJavaVm->vm->GetEnv(env, version);
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

JavaVM* jni_hook_init(JavaVM* vm){
    Linker_JavaVM *linkerJavaVm = new Linker_JavaVM(vm);
    linkerJavaVm->functions->GetEnv = GetEnv;
    linkerJavaVm->functions->DestroyJavaVM = DestroyJavaVM;
    linkerJavaVm->functions->AttachCurrentThread = AttachCurrentThread;
    linkerJavaVm->functions->DetachCurrentThread = DetachCurrentThread;
    linkerJavaVm->functions->AttachCurrentThreadAsDaemon = AttachCurrentThreadAsDaemon;
    return (JavaVM*)linkerJavaVm;
}

LinkerJNIInvokeInterface::LinkerJNIInvokeInterface(struct JNIInvokeInterface *java_vm) {
    reserved0 = java_vm;
}
