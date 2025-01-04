cmake_minimum_required(VERSION 3.10)
project(choraleGPT-preprocessing)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Find required packages
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TINYXML2 REQUIRED tinyxml2)

# Add include directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Create executable
add_executable(inputXml 
    src/inputXml.cpp
    src/Arguments.cpp
    src/Chorale.cpp
    src/Part.cpp
    src/XmlUtils.cpp
)

# Link libraries
target_link_libraries(inputXml 
    PRIVATE
    CURL::libcurl
    ${TINYXML2_LIBRARIES}
)
