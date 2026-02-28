cmake_minimum_required(VERSION 3.20)
project(GodotPPServer)

# 1. Compile ONLY the main server file
add_executable(GodotPPServer
    src/main.cpp
)

# 2. Point to the QNS headers so main.cpp knows what the DLL functions look like
target_include_directories(GodotPPServer PRIVATE "${CMAKE_SOURCE_DIR}/src/qns/include")

set(SNL_LIB_PATH "${CMAKE_BINARY_DIR}/_deps/snl-build/snl.lib")

# 3. Link the QNSCore Library. 
# CMake automatically routes this to build/src/qns/Debug/QNSCore.lib or Release!
target_link_libraries(GodotPPServer PRIVATE
        "${SNL_LIB_PATH}"
        EnTT::EnTT
        QNSCore   
)

# 4. Automatically copy the DLL next to the Server .exe so it can run!
# $<TARGET_FILE:QNSCore> grabs the .dll
# $<TARGET_FILE_DIR:GodotPPServer> is the folder where the server .exe was just built
add_custom_command(TARGET GodotPPServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:QNSCore>
    $<TARGET_FILE_DIR:GodotPPServer>
    COMMENT "Copying QNSCore dynamic library to server output directory..."
)