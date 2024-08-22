package com.hepta.fridaload;

import android.content.Context;
import android.util.Log;

public class LoadEntry {

//    static {
//
//        System.loadLibrary("nativetext");
//    }

    public static void Entry(Context context , String source){
        LoadSo(context,source);
        test("load hiedapk is successful");
        test2("load hiedapk is successful");
        test3("load hiedapk is successful");
        test4("load hiedapk is successful");

    }

    private static void LoadSo(Context context, String source) {

        String abi= "arm64-v8a";
        if(!android.os.Process.is64Bit()){
            abi = "armeabi-v7a";
        }
        String str = source+"!/lib/"+abi+"/libnativetext.so";
        System.load(str);
    }

    public  static native void test(String str);
    public  static native void test2(String str);
    public  static native void test3(String str);
    public  static native void test4(String str);
    public  static void test_java_call_native(String str){
        Log.e("rzx",str);
        test("load hiedapk test_java_call_native");
        test2("load hiedapk test_java_call_native");
        test3("load hiedapk test_java_call_native");
        test4("load hiedapk test_java_call_native");
    }

    public static void text_java(){
        Log.e("rzx","text_java");

    }

}
