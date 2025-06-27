package com.hepta.linker

class NativeLib {

    /**
     * A native method that is implemented by the 'linker' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'linker' library on application startup.
        init {
            System.loadLibrary("linker")
        }
    }

}