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
jint GetVersion(JNIEnv * env)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->GetVersion();
}

jclass DefineClass(JNIEnv * env,const char *name, jobject loader, const jbyte* buf,jsize bufLen){
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->DefineClass( name, loader, buf, bufLen);
}


jclass FindClass(JNIEnv * env,const char* name){
    HOOKLOGE("FindClass");
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->FindClass( name);
}
jmethodID FromReflectedMethod(JNIEnv * env,jobject method)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->FromReflectedMethod( method);
}
jfieldID FromReflectedField(JNIEnv * env,jobject field)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->FromReflectedField( field);
}
jobject ToReflectedMethod(JNIEnv * env,jclass cls, jmethodID methodID, jboolean isStatic)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->ToReflectedMethod( cls, methodID, isStatic);
}
jclass GetSuperclass(JNIEnv * env,jclass clazz)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->GetSuperclass( clazz);
}
jboolean IsAssignableFrom(JNIEnv * env,jclass clazz1, jclass clazz2)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->IsAssignableFrom(clazz1, clazz2);
}
jobject ToReflectedField(JNIEnv * env,jclass cls, jfieldID fieldID, jboolean isStatic)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->ToReflectedField( cls, fieldID, isStatic);
}

jint Throw(JNIEnv * env,jthrowable obj)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->Throw( obj);
}

jint ThrowNew(JNIEnv * env,jclass clazz, const char* message)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->ThrowNew( clazz, message);
}

jthrowable ExceptionOccurred(JNIEnv * env)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    return functions->ExceptionOccurred();
}

void ExceptionDescribe(JNIEnv * env)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    functions->ExceptionDescribe();
}

void ExceptionClear(JNIEnv * env)
{
    Linker_JNIEnv * linkerJniEnv = (Linker_JNIEnv *) env;
    JNIEnv * functions = linkerJniEnv->env;
    functions->ExceptionClear();
}
Linker_JNIEnv::Linker_JNIEnv(JNIEnv * env){
    this->env = env;
    functions = new Linker_JNINativeInterface();
    functions->GetVersion =  GetVersion;
    functions->DefineClass =  DefineClass;
    functions->FromReflectedMethod =  FromReflectedMethod;
    functions->FromReflectedField =  FromReflectedField;
    functions->ToReflectedMethod =  ToReflectedMethod;
    functions->GetSuperclass =  GetSuperclass;
    functions->IsAssignableFrom =  IsAssignableFrom;
    functions->ToReflectedField =  ToReflectedField;
    functions->Throw =  Throw;
    functions->ThrowNew =  ThrowNew;
    functions->ExceptionOccurred =  ExceptionOccurred;
    functions->ExceptionDescribe =  ExceptionDescribe;
    functions->ExceptionClear =  ExceptionClear;
    functions->GetStringUTFChars =  GetStringUTFChars;
    functions->FindClass = FindClass;
}



JavaVM* jni_hook_init(JavaVM* vm){
    Linker_JavaVM *linkerJavaVm = new Linker_JavaVM(vm);

    return (JavaVM*)linkerJavaVm;
}



