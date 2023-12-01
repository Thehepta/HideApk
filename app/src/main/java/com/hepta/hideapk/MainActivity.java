package com.hepta.hideapk;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

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
        Button tv = binding.sampleText;
        tv.setText(stringFromJNI());
        tv.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(){
                    @Override
                    public void run() {
                        try {

                            ApplicationInfo applicationInfo =  getApplication().getPackageManager().getApplicationInfo("com.hepta.adirf", 0);
//                            PathClassLoader pathClassLoader = new PathClassLoader(applicationInfo.sourceDir,getClassLoader());
//                            Class<?> LoadEntry = pathClassLoader.loadClass("com.hepta.adirf.LoadEntry");
//
//                            Method method = LoadEntry.getMethod("Entry", Context.class, String.class);
//                            method.invoke(null,getApplicationContext(),applicationInfo.sourceDir);
//                            Log.e("Rzx",applicationInfo.sourceDir);
                            hideApk(applicationInfo.sourceDir);

                        } catch (PackageManager.NameNotFoundException  e) {
                            throw new RuntimeException(e);
                        }
                    }
                }.start();

            }
        });
        Button btn2 = binding.sampleText2;
        btn2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
//                hideApk("com.hepta.adirf");
            }
        });
    }

    private native void hideApk(String s);

    /**
     * A native method that is implemented by the 'hideapk' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}