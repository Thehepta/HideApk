//
// Created by thehepta on 2024/1/16.
//

#include "jni_hook.h"
#include "linker_debug.h"



jint  GetEnv(Linker_JNIInvokeInterface* javaVm, void** env, jint version){

    LOGE("hook GetEnv");
    return javaVm->reserved1->GetEnv(reinterpret_cast<JavaVM *>(javaVm->reserved1), env, version);
}


void* jni_hook_init(struct JNIInvokeInterface* jniInvokeInterface){
    Linker_JNIInvokeInterface *linkerJniInvokeInterface = new Linker_JNIInvokeInterface(jniInvokeInterface);
    linkerJniInvokeInterface->GetEnv = reinterpret_cast<jint (*)(JavaVM *, void **, jint)>(GetEnv);
    return linkerJniInvokeInterface;
}

//Linker_JNIInvokeInterface::Linker_JNIInvokeInterface(struct JNIInvokeInterface *java_vm) {
//    reserved1 = java_vm;
//}
