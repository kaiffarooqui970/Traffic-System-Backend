cmake_minimum_required(VERSION 3.15)
project(TrafficBackend)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(APPLE)
    # Homebrew-installed ASIO / libpqxx / libpq (macOS dev setup)
    include_directories(/opt/homebrew/include)
    include_directories(/opt/homebrew/opt/libpqxx/include)
    include_directories(/opt/homebrew/opt/libpq/include)
    link_directories(/opt/homebrew/opt/libpqxx/lib)
    link_directories(/opt/homebrew/opt/libpq/lib)
endif()

add_executable(server main.cpp)

if(APPLE)
    target_link_libraries(server "-framework Foundation" "-framework Security" pqxx pq Threads::Threads)
else()
    # Linux (e.g. the Docker image): ASIO/libpqxx/libpq installed via apt
    # into standard system include/lib paths, no extra -I/-L needed.
    target_link_libraries(server pqxx pq Threads::Threads)
endif()

# ---------------------------------------------------------------------------
# Unit tests (tests/unit/cpp) - pure logic only, no DB/HTTP dependency.
# Built as a separate target so `cmake --build build --target traffic_logic_tests`
# (or `ctest`) can run without Postgres, ASIO, or libpqxx.
#
# Guarded by EXISTS because the Docker build context for this image is just
# src/crow-backend/ (see Dockerfile) - tests/ isn't copied in, and the
# production image only ever needs the `server` target anyway.
# ---------------------------------------------------------------------------
set(UNIT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/unit/cpp)

if(EXISTS ${UNIT_TEST_DIR}/test_traffic_logic.cpp)
    enable_testing()

    add_executable(traffic_logic_tests ${UNIT_TEST_DIR}/test_traffic_logic.cpp)
    target_include_directories(traffic_logic_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${UNIT_TEST_DIR}
    )

    include(CTest)
    add_test(NAME traffic_logic_tests COMMAND traffic_logic_tests)
endif()
