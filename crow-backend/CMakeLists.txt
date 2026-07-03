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
