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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

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
                            PathClassLoader pathClassLoader = new PathClassLoader(applicationInfo.sourceDir,getClassLoader());
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
                customhideApkLoad(applicationInfo.sourceDir);
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
    }

    private native void customhideApkLoad(String s);
    private native void customhideSoLoad(String libnamePath);

    /**
     * A native method that is implemented by the 'hideapk' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}