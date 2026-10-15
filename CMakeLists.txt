cmake_minimum_required(VERSION 3.24)
project(lfx_event_bus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LFX_EVENT_BUS_BUILD_TESTS "Build the unit tests" ON)
option(LFX_EVENT_BUS_BUILD_SANDBOX "Build the sandbox example" ON)

# Evita il warning di deprecazione di CMake 4.x sulle librerie esterne (Moodycamel/GTest)
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

# --- DIPENDENZE INTERNE (Moodycamel) ---
include(FetchContent)
FetchContent_Declare(
        readerwriterqueue
        URL https://github.com/cameron314/readerwriterqueue/archive/refs/tags/v1.0.6.zip
)
FetchContent_MakeAvailable(readerwriterqueue)

# --- LIBRERIA PRINCIPALE ---
add_library(lfx_event_bus STATIC)

target_sources(lfx_event_bus PRIVATE
        src/core/eb_broker.cpp
)

target_include_directories(lfx_event_bus
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${readerwriterqueue_SOURCE_DIR} # Moodycamel è totalmente PRIVATO!
)

if (ANDROID)
    find_library(log-lib log)
    target_link_libraries(lfx_event_bus PRIVATE ${log-lib})
endif()

# --- UNIT TESTS (GoogleTest) ---
if(LFX_EVENT_BUS_BUILD_TESTS)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(lfx_event_bus_tests
            src/tests/test_broker.cpp
    )

    target_include_directories(lfx_event_bus_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${readerwriterqueue_SOURCE_DIR}
    )

    target_link_libraries(lfx_event_bus_tests
            PRIVATE
            lfx_event_bus
            gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(lfx_event_bus_tests)
endif()

# --- SANDBOX ---
if(LFX_EVENT_BUS_BUILD_SANDBOX)
    add_executable(lfx_event_bus_sandbox example/main.cpp)
    target_include_directories(lfx_event_bus_sandbox PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(lfx_event_bus_sandbox PRIVATE lfx_event_bus)
endif()

# --- INSTALLAZIONE & PACKAGING (CPack) ---
include(GNUInstallDirs)

install(TARGETS lfx_event_bus
        EXPORT lfx_event_bus-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/lfx_event_bus
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT lfx_event_bus-targets
        FILE lfx_event_bus-targets.cmake
        NAMESPACE lfx_event_bus::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lfx_event_bus
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
        "cmake/lfx_event_bus-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/lfx_event_bus-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/lfx_event_bus"
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/lfx_event_bus-config.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/lfx_event_bus"
)

set(CPACK_GENERATOR "ZIP")

if (ANDROID)
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-android-arm64")
elseif (WIN32)
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-windows-x64")
endif()

include(CPack)