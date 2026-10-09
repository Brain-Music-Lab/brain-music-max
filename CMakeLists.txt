cmake_minimum_required(VERSION 3.25)

cmake_policy(SET CMP0079 NEW)
set (CMAKE_CXX_STANDARD 17)

project(Min-DevKit CXX)

if (NOT DEFINED CMAKE_BUILD_TYPE)
  message(WARNING "CMAKE_BUILD_TYPE not set. Defaulting to 'Release'.")
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-w)
add_subdirectory(deps/bml-dsp)
add_subdirectory(deps/liblsl)

enable_testing()

string(REGEX REPLACE "(.*)/" "" THIS_PACKAGE_NAME "${CMAKE_CURRENT_SOURCE_DIR}")

# Misc setup and subroutines
include(${CMAKE_CURRENT_SOURCE_DIR}/source/min-api/script/min-package.cmake)

# Add the Lib, if it exists
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/min-lib/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/min-lib SYSTEM)
endif ()

find_package(Threads REQUIRED)

# Generate a project for every folder in the "source/projects" folder
SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/source/projects)
foreach (project_dir ${PROJECT_DIRS})
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir}/CMakeLists.txt")
        message("Generating: ${project_dir}")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir})

        if (WIN32)
            if (CMAKE_BUILD_TYPE STREQUAL Debug)
                target_compile_options(${project_dir} PUBLIC /MTd)
            else ()
                target_compile_options(${project_dir} PUBLIC /MT)
            endif()
        endif()

        # target_include_directories
        target_link_libraries(${project_dir}
            PRIVATE LSL::lsl
            PRIVATE bml-dsp
        )

        target_link_libraries(${project_dir}_test
            PRIVATE LSL::lsl
            PRIVATE bml-dsp
        )

    endif ()
endforeach ()

# Comment the line below if you want automatic cmake regneration enabled
set(CMAKE_SUPPRESS_REGENERATION true)

# Copy DLL to necessary directories. Potentially not the most elegant solution, but it works
if (WIN32)
    file(COPY ${FFTW_DLL} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    file(COPY ${FFTW_DLL} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/externals)
endif()
