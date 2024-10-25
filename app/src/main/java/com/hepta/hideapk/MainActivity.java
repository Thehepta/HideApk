package com.hepta.hideapk;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.hepta.hideapk.databinding.ActivityMainBinding;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;

import dalvik.system.DexFile;
import dalvik.system.PathClassLoader;

public class MainActivity extends AppCompatActivity {


    // Used to load the 'hideapk' library on application startup.
    static {
        System.loadLibrary("hideapk");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        Button LoadApkBtn = binding.LoadApk;
        LoadApkBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(){
                    @Override
                    public void run() {
                        try {
                            ApplicationInfo applicationInfo =  getApplication().getPackageManager().getApplicationInfo("com.hepta.fridaload", 0);
                            PathClassLoader pathClassLoader = new PathClassLoader(applicationInfo.sourceDir,null);
                            Class<?> LoadEntry = pathClassLoader.loadClass("com.hepta.fridaload.LoadEntry");
                            Method method = LoadEntry.getMethod("Entry", Context.class, String.class);
                            method.invoke(null,getApplicationContext(),applicationInfo.sourceDir);
                            Log.e("Rzx",applicationInfo.sourceDir);

                        } catch (PackageManager.NameNotFoundException  e) {
                            throw new RuntimeException(e);
                        } catch (ClassNotFoundException e) {
                            throw new RuntimeException(e);
                        } catch (InvocationTargetException e) {
                            throw new RuntimeException(e);
                        } catch (NoSuchMethodException e) {
                            throw new RuntimeException(e);
                        } catch (IllegalAccessException e) {
                            throw new RuntimeException(e);
                        }
                    }
                }.start();

            }
        });
        Button hideLoadApkBtn = binding.HideLoadApk;
        hideLoadApkBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.e("rzx","hideLoadApkBtn");
                ApplicationInfo applicationInfo = null;
                try {
                    applicationInfo = getApplication().getPackageManager().getApplicationInfo("com.hepta.fridaload", 0);
                } catch (PackageManager.NameNotFoundException e) {
                    throw new RuntimeException(e);
                }
                customMemhideApkLoad(applicationInfo.sourceDir);
            }
        });

        Button hideLoadSoBtn = binding.HideLoadSo;
        hideLoadSoBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.e("rzx","hideLoadSoBtn");
//                hideApk("com.hepta.adirf");
            }
        });


        Button rxposedModuleLoadBtn = binding.rxposedModuleLoad;
        rxposedModuleLoadBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.e("rzx","ExternhideLoadSoBtn");
                ApplicationInfo applicationInfo = null;
                try {
                    applicationInfo = getApplication().getPackageManager().getApplicationInfo("com.hepta.davlik", PackageManager.GET_META_DATA);
                } catch (PackageManager.NameNotFoundException e) {
                    throw new RuntimeException(e);
                }
                String clsname = applicationInfo.metaData.getString("rxposed_clsentry");
                String mtdname = applicationInfo.metaData.getString("rxposed_mtdentry");
                ClassLoader classLoader = GethideApkLoad(applicationInfo.sourceDir);
                try {
                    Class<?> cls = classLoader.loadClass(clsname);

                    Method entry_method = cls.getMethod(mtdname,Context.class,String.class);
                    entry_method.invoke(cls,getApplicationContext(),applicationInfo.sourceDir);
//                    Method entry_method = cls.getMethod("getClassLoaderList");
//                    entry_method.invoke(cls);


                } catch (ClassNotFoundException e) {
                    throw new RuntimeException(e);
                } catch (NoSuchMethodException e) {
                    throw new RuntimeException(e);
                } catch (InvocationTargetException e) {
                    throw new RuntimeException(e);
                } catch (IllegalAccessException e) {
                    throw new RuntimeException(e);
                }
            }
        });


        Button customLinkerLoadsoBtn = binding.customLinkerLoadso;
        customLinkerLoadsoBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                customLinkerLoadPathSO();
            }
        });
        Button ExternZlibLoadApk = binding.ExternZlibLoadApk;
        ExternZlibLoadApk.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.e("rzx","zipLoadApk");
                ApplicationInfo applicationInfo = null;
                try {
                    applicationInfo = getApplication().getPackageManager().getApplicationInfo("com.hepta.fridaload", 0);
                } catch (PackageManager.NameNotFoundException e) {
                    throw new RuntimeException(e);
                }
                zipLoadApk(applicationInfo.sourceDir);
            }
        });

        Button SystenStubLoadSo = binding.SystenStubLoadSo;
        SystenStubLoadSo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                SystenStubLoadSo();
            }
        });

        Button classloadDexmerge = binding.classloadDexmerge;
        classloadDexmerge.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.e("rzx","hideLoadApkBtn");
                ApplicationInfo applicationInfo = null;
                try {
                    applicationInfo = getApplication().getPackageManager().getApplicationInfo("com.hepta.fridaload", 0);
                } catch (PackageManager.NameNotFoundException e) {
                    throw new RuntimeException(e);
                }
                ClassLoader classLoader = new PathClassLoader(".",ClassLoader.getSystemClassLoader());
                classloadDexmerge(applicationInfo.sourceDir,classLoader);
                Class<?> LoadEntry = null;
                try {
                    LoadEntry = classLoader.loadClass("com.hepta.fridaload.LoadEntry");
                    Method method = LoadEntry.getMethod("text_java");
                    method.invoke(null);
                } catch (ClassNotFoundException | InvocationTargetException |
                         NoSuchMethodException | IllegalAccessException e) {
                    throw new RuntimeException(e);
                }

            }
        });

        Button javaDexmerge = binding.javaDexmerge;
        javaDexmerge.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ApplicationInfo applicationInfo = null;
                try {
                    applicationInfo = getApplication().getPackageManager().getApplicationInfo("com.hepta.fridaload", 0);
                } catch (PackageManager.NameNotFoundException e) {
                    throw new RuntimeException(e);
                }

                ClassLoader classLoader = Utils.mergeDex(getClassLoader(),applicationInfo);
                Class<?> LoadEntry = null;
                try {
                    LoadEntry = classLoader.loadClass("com.hepta.fridaload.LoadEntry");
                    Method method = LoadEntry.getMethod("text_java");
                    method.invoke(null);
                } catch (ClassNotFoundException | InvocationTargetException |
                         NoSuchMethodException | IllegalAccessException e) {
                    throw new RuntimeException(e);
                }
            }
        });
        Button pltHook = binding.pltHook;
        pltHook.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                plt_hook();
            }
        });

    }


    private native ClassLoader zipLoadApk(String s);
    private native void SystenStubLoadSo();
    private native void customLinkerLoadPathSO();

    private native void customFilehideApkLoad(String s);
    private native void customMemhideApkLoad(String s);
    private native ClassLoader GethideApkLoad(String s);
    private native void customhideSoLoad(String libnamePath);
    private native void classloadDexmerge(String s,ClassLoader classLoader);

    private native void plt_hook();
    /**
     * A native method that is implemented by the 'hideapk' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}