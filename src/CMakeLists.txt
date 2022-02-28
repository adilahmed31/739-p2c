# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(project3)

include_directories(../include)

add_executable(client client.cc cache_store.cc)

target_link_libraries(client 
	Threads::Threads
       	gRPC::grpc++
	p3protolib)
add_dependencies(client p3protolib)
set_target_properties(client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

add_executable(server server.cc)

target_link_libraries(server 
	Threads::Threads
       	gRPC::grpc++
	p3protolib)
add_dependencies(server p3protolib)
set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)


