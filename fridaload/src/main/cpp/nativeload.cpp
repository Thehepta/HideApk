#include <jni.h>
#include <android/log.h>

#define LOG_TAG "Native"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

extern "C"
JNIEXPORT void JNICALL
Java_com_hepta_fridaload_LoadEntry_text(JNIEnv *env, jclass clazz, jstring str) {
    const char * string = env->GetStringUTFChars(str, nullptr);
    LOGE("TEXT:%s",string);
}

void load_dex(JNIEnv *pEnv) {
    LOGD("load_dex\n");
    jobjectArray JAAR = nullptr;
    auto classloader = pEnv->FindClass("java/lang/ClassLoader");
    auto getsyscl_mid = pEnv->GetStaticMethodID(classloader, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
    auto sys_classloader = pEnv->CallStaticObjectMethod(classloader, getsyscl_mid);
    jmethodID method_loadClass = pEnv->GetMethodID(classloader,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");

    if (!sys_classloader){
        LOGE("getSystemClassLoader failed!!!");
        return;
    }
    auto in_memory_classloader = pEnv->FindClass( "dalvik/system/InMemoryDexClassLoader");
    auto initMid = pEnv->GetMethodID( in_memory_classloader, "<init>",
                                      "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
    auto byte_buffer_class = pEnv->FindClass("java/nio/ByteBuffer");
    auto dex_buffer = pEnv->NewDirectByteBuffer(dex_addr, dex_size);
    if (auto my_cl = pEnv->NewObject( in_memory_classloader, initMid,
                                      dex_buffer, sys_classloader)) {
        jobject  sand_class_loader_ = pEnv->NewGlobalRef( my_cl);

        jstring xposed = pEnv->NewStringUTF("com.rxposed.sandhooktextapp.XposedTest");
        jobject XposedTest = pEnv->CallObjectMethod(sand_class_loader_,method_loadClass,xposed);

        jclass Class_cls = pEnv->FindClass("java/lang/Class");
        jmethodID clsmethod_method = pEnv->GetMethodID(Class_cls,"getMethod","(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;");
        jclass Method_cls = pEnv->FindClass("java/lang/reflect/Method");
        jmethodID invoke_met =  pEnv->GetMethodID(Method_cls,"invoke","(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");

        jobject user_metohd = nullptr;
        jstring  call_method_name = pEnv->NewStringUTF("native_hook");
        user_metohd = pEnv->CallObjectMethod(XposedTest, clsmethod_method,call_method_name,JAAR);
        pEnv->CallObjectMethod(user_metohd, invoke_met,XposedTest,JAAR);

    } else {
        LOGE("InMemoryDexClassLoader creation failed!!!");
        return;
    }
}