cmake_minimum_required(VERSION 4.1)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(TestAffichageSynchrone)

set(CMAKE_CXX_STANDARD 23)

if (WIN32) # WINDOWS

    include(FetchContent)

    FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.0
    )

    FetchContent_MakeAvailable(raylib)

    set(RAYGUI_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/raygui.h")

    if(NOT EXISTS "${RAYGUI_HEADER_PATH}")
        message(STATUS "Téléchargement de raygui.h (version 4.0) depuis GitHub...")
        file(DOWNLOAD
                "https://raw.githubusercontent.com/raysan5/raygui/4.0/src/raygui.h"
                "${RAYGUI_HEADER_PATH}"
        )
    endif()

else () # LINUX

    find_package(raylib CONFIG REQUIRED)

endif ()

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

# Doxygen

find_package(Doxygen)

if (DOXYGEN_FOUND)
    # 1. Commande pour exécuter Doxygen
    add_custom_target(doxygen_generate
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/Doxyfile
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs
            COMMENT "Génération du XML Doxygen..."
    )

    # 2. Commande pour exécuter Sphinx
    add_custom_target(sphinx_generate
            COMMAND sphinx-build -b html source build/html
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs
            COMMENT "Génération du HTML Sphinx..."
            DEPENDS doxygen_generate # Sphinx attend que Doxygen ait fini
    )

    # Grouper dans "Documentation"
    set_target_properties(doxygen_generate sphinx_generate PROPERTIES FOLDER "Documentation")

endif ()