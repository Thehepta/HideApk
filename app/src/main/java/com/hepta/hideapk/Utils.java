package com.hepta.hideapk;

import static java.lang.ClassLoader.getSystemClassLoader;

import android.content.pm.ApplicationInfo;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import dalvik.system.BaseDexClassLoader;
import dalvik.system.DexFile;
import dalvik.system.PathClassLoader;

public class Utils {


    private static final String TAG = "hideapk_Utils";

    public static ClassLoader mergeDex(ClassLoader loader, ApplicationInfo applicationInfo){

        ByteBuffer[] dexBuffers = unzipdex(applicationInfo.sourceDir);

        try {
            PathClassLoader ret_classloader = new PathClassLoader(".",getSystemClassLoader());
            //TODO:to get 'pathList' field and 'dexElements' field by reflection.
            //private final DexPathList pathList;
            Field pathListField = BaseDexClassLoader.class.getDeclaredField("pathList");
            pathListField.setAccessible(true);
            Class<?> dexPathListClass = Class.forName("dalvik.system.DexPathList");
            Class<?> elementClass = Class.forName("dalvik.system.DexPathList$Element");
            Constructor<?> elementConstructor = elementClass.getConstructor(DexFile.class);
            elementConstructor.setAccessible(true);
            Field dexElementsField = dexPathListClass.getDeclaredField("dexElements");
            dexElementsField.setAccessible(true);
            //TODO:to get the value of host's dexElements field.
            Object hostPathList = pathListField.get(ret_classloader);
            Object[] replace_hostDexElements = (Object[]) dexElementsField.get(hostPathList);
            Class<?> DexFile_cls = DexFile.class;
            Constructor<?> dexFileConstructor = DexFile_cls.getDeclaredConstructor(
                    ByteBuffer[].class,
                    ClassLoader.class,
                    Class.forName("[Ldalvik.system.DexPathList$Element;")
            );
            dexFileConstructor.setAccessible(true);
            DexFile plug_dexfile = (DexFile) dexFileConstructor.newInstance(dexBuffers,null,null);
            Object pluginDexElements = elementConstructor.newInstance(plug_dexfile);
            Log.w(TAG, "return  1");
            Log.w(TAG, "replace_hostDexElements.length:"+replace_hostDexElements.length);
            Log.w(TAG, "pluginDexElements:"+pluginDexElements.toString());
            Class<?> new_type = pluginDexElements.getClass();
            Object[] new_dexElements = (Object[]) Array.newInstance(new_type, replace_hostDexElements.length + 1);
            System.arraycopy(
                    replace_hostDexElements,
                    0,
                    new_dexElements,
                    0,
                    replace_hostDexElements.length
            );
            Log.w(TAG, "return  2");
            new_dexElements[replace_hostDexElements.length] = pluginDexElements;
//            System.arraycopy(
//                    pluginDexElements,
//                    0,
//                    new_dexElements,
//                    replace_hostDexElements.length,
//                    1
//            );
            Log.w(TAG, "return  3");
            dexElementsField.set(hostPathList, new_dexElements);
            Log.w(TAG, "return  ret_classloader");
            return ret_classloader;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
    public static ByteBuffer[] unzipdex(String zippath){
        List<ByteBuffer> byteBufferList = new ArrayList<>();
        ZipFile zp=null;
        try{
            //指定编码，否则压缩包里面不能有中文目录
            zp=new ZipFile(zippath, Charset.forName("gbk"));
            //遍历里面的文件及文件夹
            Enumeration entries=zp.entries();

            while(entries.hasMoreElements()){
                ZipEntry entry= (ZipEntry) entries.nextElement();
                String zipEntryName=entry.getName();
                if(zipEntryName.contains(".dex")){
                    InputStream in=zp.getInputStream(entry);
                    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                    byte[] buffer=new byte[4096];
                    int len;
                    while ((len=in.read(buffer))>0){
                        outputStream.write(buffer, 0, len);
                    }
                    in.close();
                    byte[] byteArray = outputStream.toByteArray();
                    byteBufferList.add(ByteBuffer.wrap(byteArray));
                }

            }
            zp.close();
        }catch ( Exception e){
            e.printStackTrace();
        }

        return byteBufferList.toArray(new ByteBuffer[0]);
    }

}
