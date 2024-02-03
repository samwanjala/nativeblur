package com.example.nativeblur

import android.graphics.Bitmap
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.TextView
import androidx.core.graphics.drawable.toBitmap
import com.example.nativeblur.databinding.ActivityMainBinding
import com.google.android.renderscript.Toolkit

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val inputBitmap = binding.imageView.drawable.toBitmap(1920,1080)
        binding.imageView.setImageBitmap(inputBitmap)
//        val output = inputBitmap.copy(inputBitmap.config, true)

        binding.blurButton.setOnClickListener {
            Log.e("blurtask", "start")
//            nativeBlur(inputBitmap, output, 10)
            val output = Toolkit.blur(inputBitmap, 10)
            binding.imageView.setImageBitmap(output)
            Log.e("blurtask", "end")
        }

        binding.resetButton.setOnClickListener {
            binding.imageView.setImageBitmap(inputBitmap)
        }
    }

    private external fun nativeBlur(input: Bitmap, output:Bitmap, radius: Int)

    companion object {
        // Used to load the 'nativeblur' library on application startup.
        init {
            System.loadLibrary("nativeblur")
        }
    }
}