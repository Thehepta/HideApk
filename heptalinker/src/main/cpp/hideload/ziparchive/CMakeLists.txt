# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(base)
include_directories(incfs_support/include   include base/include)

add_library( # Sets the name of the library.
        ziparchive
        # Sets the library as a shared library.
        STATIC
        # Provides a relative path to your source file(s).
        "zip_archive.cc"
        "zip_archive_stream_entry.cc"
        "zip_cd_entry_map.cc"
        "zip_error.cpp"
        "zip_writer.cc"
        incfs_support/signal_handling.cpp
        )

target_link_libraries( # Specifies the target library.
        ziparchive
        # Links the target library to the log library
        # included in the NDK.
        base
        log)


