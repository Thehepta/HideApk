# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.22.1)

include_directories(include)

add_library( # Sets the name of the library.
        base
        # Sets the library as a shared library.
        STATIC
        "abi_compatibility.cpp"
        "chrono_utils.cpp"
        "cmsg.cpp"
        "file.cpp"
        "hex.cpp"
        "logging.cpp"
        "mapped_file.cpp"
        "parsebool.cpp"
        "parsenetaddress.cpp"
        "posix_strerror_r.cpp"
        "process.cpp"
        "properties.cpp"
        "stringprintf.cpp"
        "strings.cpp"
        "threads.cpp"
        "test_utils.cpp"
        )

set_property(TARGET base PROPERTY CXX_STANDARD 17)

