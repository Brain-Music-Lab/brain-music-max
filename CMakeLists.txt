cmake_minimum_required(VERSION 3.19)
project(Min-DevKit)

# set(USER_HOME)
# if (NOT USER_HOME)
#     message(FATAL_ERROR "Variable 'USER_HOME' not set or incorrectly set. Rerun the cmake command with '-D USER_HOME=$HOME' appended.")
# endif (NOT USER_HOME)

set(BUILD_SHARED_LIBS OFF)

add_subdirectory(deps/liblsl)
add_subdirectory(deps/bml-dsp)

enable_testing()

string(REGEX REPLACE "(.*)/" "" THIS_PACKAGE_NAME "${CMAKE_CURRENT_SOURCE_DIR}")

# add_library(lib_fftw STATIC IMPORTED)
# add_library(lib_lsl STATIC IMPORTED)
# set_property(TARGET lib_fftw PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/deps/bml-dsp/vendor/fftw/lib/libfftw3-3.lib")
# set_property(TARGET lib_lsl PROPERTY IMPORTED_LOCATION "C:\\Program Files (x86)\\liblsl\\lib\\lsl.lib")

# if (APPLE)
#     if (${CMAKE_GENERATOR} MATCHES "Xcode")
#             if (${XCODE_VERSION} VERSION_LESS 10)
#                 message(STATUS "Xcode 10 or higher is required. Please install from the Mac App Store.")
#                 return ()
#             elseif(${XCODE_VERSION} VERSION_GREATER_EQUAL 12)
#                 set(C74_BUILD_FAT YES)
#             endif ()
#     endif ()

#     if (NOT CMAKE_OSX_ARCHITECTURES)
#         if(C74_BUILD_FAT)
#             set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architecture" FORCE)
#         else()
#             set(CMAKE_OSX_ARCHITECTURES ${CMAKE_SYSTEM_PROCESSOR} CACHE STRING "macOS architecture" FORCE)
#         endif()
#         message("CMAKE_OSX_ARCHITECTURES set to ${CMAKE_OSX_ARCHITECTURES}")
#     endif()
# endif()


# Misc setup and subroutines
include(${CMAKE_CURRENT_SOURCE_DIR}/source/min-api/script/min-package.cmake)

# Add the Lib, if it exists
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/min-lib/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/min-lib)
endif ()

find_package(Threads REQUIRED)

# Generate a project for every folder in the "source/projects" folder
SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/source/projects)
foreach (project_dir ${PROJECT_DIRS})
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir}/CMakeLists.txt")
        message("Generating: ${project_dir}")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir})

        target_compile_options(${project_dir} PUBLIC /MTd)

        # target_include_directories
        target_link_libraries(${project_dir}
            PUBLIC LSL::lsl
            PUBLIC bml-dsp
        )

        # find_package(LSL 1.13.0 REQUIRED
        #     HINTS "C:\\Program Files (x86)\\liblsl\\lib\\cmake\\LSL")
    
        # target_include_directories(${project_dir}
        #     PRIVATE
        #     ${PROJECT_SOURCE_DIR}/deps/bml-dsp/vendor/fftw/include
        #     "C:\\Program Files (x86)\\liblsl\\include"
        #     ${PROJECT_SOURCE_DIR}/deps/bml-dsp/include)

        

    endif ()
endforeach ()

# Comment the line below if you want automatic cmake regneration enabled
set(CMAKE_SUPPRESS_REGENERATION true)
