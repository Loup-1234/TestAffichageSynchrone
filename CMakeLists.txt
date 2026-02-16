cmake_minimum_required(VERSION 4.1)
project(TestAffichageSynchrone)

set(CMAKE_CXX_STANDARD 23)

find_package(raylib CONFIG REQUIRED)

set(SOURCE_FILES
        src/main.cpp
        src/AffichageSynchrone.cpp
        include/raymedia/raymedia.c
)

set(HEADER_FILES
        include/AffichageSynchrone/AffichageSynchrone.h
        include/raymedia/raymedia.h
)

add_executable(TestAffichageSynchrone ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(TestAffichageSynchrone PRIVATE include)

if (WIN32) # WINDOWS

    message(STATUS "Configuration pour Windows...")

    set(FFMPEG_DIR "${CMAKE_SOURCE_DIR}/include/ffmpeg")

    target_include_directories(TestAffichageSynchrone PRIVATE ${FFMPEG_DIR}/include)

    target_link_directories(TestAffichageSynchrone PRIVATE ${FFMPEG_DIR}/lib)

    target_compile_definitions(TestAffichageSynchrone PRIVATE RAYMEDIA_STATIC)

    target_link_libraries(TestAffichageSynchrone PRIVATE
            raylib
            avcodec
            avformat
            avutil
            swscale
            swresample
            winmm
            gdi32
    )

else () # LINUX

    message(STATUS "Configuration pour Linux...")

    find_package(PkgConfig REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)

    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
            libavcodec
            libavformat
            libavutil
            libswscale
            libswresample
    )

    target_link_libraries(TestAffichageSynchrone PRIVATE
            raylib
            PkgConfig::FFMPEG
            glfw
            OpenGL::GL
            Threads::Threads
            m
            dl
            rt
    )

endif ()