# QNS (Quic Network System) - builds both:
#  - QNS (GDExtension DLL) for Godot
#  - QNSCore (pure C++ static lib) for a dedicated server

cmake_minimum_required(VERSION 3.20)
project(QNS)

# -------------------------------------------------------------------------------------------------
# Sources
# -------------------------------------------------------------------------------------------------
set(QNS_SRC
    src/quic_network_system.cpp
    src/register_types.cpp
)

set(QNS_CORE_SRC
    src/quic_network_system.cpp
)

# -------------------------------------------------------------------------------------------------
# Rust backend
# -------------------------------------------------------------------------------------------------
if(WIN32)
    set(RUST_LIB_NAME "game_sockets.lib")
else()
    set(RUST_LIB_NAME "libgame_sockets.a")
endif()

set(RUST_LIB_PATH_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/rust_backend/target/debug/${RUST_LIB_NAME}")
set(RUST_LIB_PATH_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/rust_backend/target/release/${RUST_LIB_NAME}")
set(RUST_LIB_PATH $<IF:$<CONFIG:Release>,${RUST_LIB_PATH_RELEASE},${RUST_LIB_PATH_DEBUG}>)

add_custom_target(RustGameSockets ALL
    COMMAND cargo build $<$<CONFIG:Release>:--release>
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rust_backend"
    COMMENT "Building Rust backend for ${CMAKE_SYSTEM_NAME}..."
)

# -------------------------------------------------------------------------------------------------
# QNSCore (pure C++ core for dedicated server)
# -------------------------------------------------------------------------------------------------
add_library(QNSCore SHARED ${QNS_CORE_SRC})
set_target_properties(QNSCore PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_dependencies(QNSCore RustGameSockets)
target_include_directories(QNSCore PUBLIC include)
target_link_libraries(QNSCore PRIVATE "${RUST_LIB_PATH}")
if(WIN32)
    target_link_libraries(QNSCore PRIVATE Userenv.lib Ntdll.lib ws2_32.lib)
endif()

# -------------------------------------------------------------------------------------------------
# QNS (Godot GDExtension)
# -------------------------------------------------------------------------------------------------
add_library(QNS SHARED ${QNS_SRC})
add_dependencies(QNS RustGameSockets)

target_compile_definitions(QNS PRIVATE QNS_WITH_GODOT)
target_include_directories(QNS PUBLIC include)
target_link_libraries(QNS PRIVATE godot::cpp "${RUST_LIB_PATH}")
if(WIN32)
    target_link_libraries(QNS PRIVATE Userenv.lib Ntdll.lib ws2_32.lib)
endif()

set_target_properties(QNS PROPERTIES PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")

# -------------------------------------------------------------------------------------------------
# Automation: Recreate Addon on Build
# -------------------------------------------------------------------------------------------------
set(QNS_ADDON_DIR "${CMAKE_SOURCE_DIR}/game/addons/qns")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/qns.gdextension.in"
    "${QNS_ADDON_DIR}/qns.gdextension"
    COPYONLY
)

add_custom_command(TARGET QNS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${QNS_ADDON_DIR}/bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:QNS>"
            "${QNS_ADDON_DIR}/bin/$<TARGET_FILE_NAME:QNS>"
    COMMENT "QNS Addon successfully packaged for ${CMAKE_SYSTEM_NAME}"
)
