cmake_minimum_required(VERSION 3.15)

project(QuadBlendDrive VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add local JUCE installation
add_subdirectory(JUCE)

# Add subdirectory containing plugin source
add_subdirectory(Source)
