package com.example.heptalinker

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import com.example.heptalinker.databinding.ActivityMainBinding
import com.hepta.linker.NativeLib
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        val lib = NativeLib()
        // Example of a call to a native method
        binding.sampleText.text = lib.stringFromJNI()
    }

}