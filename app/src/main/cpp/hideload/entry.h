//
// Created by rzx on 2024/6/27.
//

#pragma once
#include "jni.h"
jobject FilehideLoadApkModule(JNIEnv *env, char * apkSource);
jobject memhideLoadApkModule(JNIEnv *env, unsigned char *apkSource, size_t i);
void *hide_dlopen(   const char *file_data);
void *hide_dlsym(void *si,char* syn_name) ;

