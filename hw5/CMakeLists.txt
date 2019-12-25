find_package(OpenCL REQUIRED)
add_executable(histogram histogram.cpp)
target_link_libraries(histogram OpenCL::OpenCL)