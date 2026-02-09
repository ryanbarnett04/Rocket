# CMakeList.txt : CMake project for Rocket-Clang, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Rocket-Clang "Rocket-Clang.cpp" "Rocket-Clang.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Rocket-Clang PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
