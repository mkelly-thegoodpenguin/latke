cmake_minimum_required (VERSION 3.7)
project (latke)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(OpenCL REQUIRED)
include_directories(${OPENCL_INCLUDE_DIR} src)

# Install directories
include(GNUInstallDirs)


# Defines the source code for the library
add_library(latke STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/UtilOCL.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/KernelOCL.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IArch.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ArchAMD.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/latke.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/DeviceManagerOCL.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DualBufferOCL.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DualImageOCL.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/IDualMemOCL.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/QueueOCL.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RunInfoOCL.h	

	${CMAKE_CURRENT_SOURCE_DIR}/src/DeviceOCL.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/DeviceOCL.h	
	${CMAKE_CURRENT_SOURCE_DIR}/src/DeviceManagerOCL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DualBufferOCL.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/DualImageOCL.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/QueueOCL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KernelOCL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/UtilOCL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RunInfoOCL.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform.cl
)

 configure_file(
 ${CMAKE_CURRENT_SOURCE_DIR}/latke_config.h.cmake.in
 ${CMAKE_CURRENT_SOURCE_DIR}/src/latke_config.h
 @ONLY
 )

# Gather list of all cl files
file(GLOB CLFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cl 
                  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cl 
                  ${CMAKE_CURRENT_SOURCE_DIR}/src/latke_config.h )

foreach(CLFile ${CLFiles})
  add_custom_command(TARGET latke PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                         copy_if_different  ${CLFile}  $<TARGET_FILE_DIR:latke> )
endforeach()

add_executable(latke_test latke_test.cpp)
target_link_libraries(latke_test latke ${OPENCL_LIBRARY})

