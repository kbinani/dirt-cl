project(dirt-cl)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (EXISTS "$ENV{CUDA_PATH}")
  list(APPEND dirt_cl_include_directories $ENV{CUDA_PATH}/include)
  list(APPEND dirt_cl_link_directories $ENV{CUDA_PATH}/lib/x64)
endif()
list(APPEND dirt_cl_link_libraries OpenCL)

add_executable(dirt-cl src/main.cpp src/main.cl)

target_link_libraries(dirt-cl ${dirt_cl_link_libraries})
target_include_directories(dirt-cl PRIVATE ${dirt_cl_include_directories})
target_link_directories(dirt-cl PRIVATE ${dirt_cl_link_directories})
