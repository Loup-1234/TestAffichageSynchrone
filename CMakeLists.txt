cmake_minimum_required(VERSION 4.1)
project(TestAffichageSynchrone)

set(CMAKE_CXX_STANDARD 23)

find_package(raylib CONFIG REQUIRED)

set(SOURCE_FILES
        src/main.cpp
        src/AffichageSynchrone.cpp
        include/SynchroniseurMultiVideo/SynchroniseurMultiVideo.cpp
)

set(HEADER_FILES
        include/AffichageSynchrone/AffichageSynchrone.h
        include/SynchroniseurMultiVideo/SynchroniseurMultiVideo.h
)

add_executable(TestAffichageSynchrone ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(TestAffichageSynchrone PRIVATE include)

if (WIN32) # WINDOWS

    message(STATUS "Configuration pour Windows...")

    target_include_directories(TestAffichageSynchrone PRIVATE "include/LibVLC/include")
    target_link_directories(TestAffichageSynchrone PRIVATE "include/LibVLC/lib")

    target_link_libraries(TestAffichageSynchrone PRIVATE
            raylib
            winmm
            gdi32
            libvlc
            libvlccore
    )

else () # LINUX

    message(STATUS "Configuration pour Linux...")

    find_package(PkgConfig REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)

    pkg_check_modules(LIBVLC REQUIRED libvlc)

    target_include_directories(TestAffichageSynchrone PRIVATE ${LIBVLC_INCLUDE_DIRS})

    target_link_libraries(TestAffichageSynchrone PRIVATE
            raylib
            ${LIBVLC_LIBRARIES}
            glfw
            OpenGL::GL
            Threads::Threads
            m
            dl
            rt
    )

endif ()
