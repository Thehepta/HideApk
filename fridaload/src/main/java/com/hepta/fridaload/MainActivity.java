package com.hepta.fridaload;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;

public class MainActivity extends AppCompatActivity {



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button btn = findViewById(R.id.loadfrida);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(){
                    @Override
                    public void run() {
//                        LoadEntry.Entry(getApplicationContext(),getApplicationInfo().sourceDir);
//                        System.loadLibrary("frida-gadget");
                        LoadEntry.text("rerewrewrew");
                    }
                }.start();
            }
        });

    }


}