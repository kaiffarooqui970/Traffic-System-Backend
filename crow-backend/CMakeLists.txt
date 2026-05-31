cmake_minimum_required(VERSION 3.15)
project(TrafficBackend)

set(CMAKE_CXX_STANDARD 17)

# 1. ASIO Networking Library
include_directories(/opt/homebrew/include)

# 2. PostgreSQL and libpqxx Libraries
include_directories(/opt/homebrew/opt/libpqxx/include)
include_directories(/opt/homebrew/opt/libpq/include)
link_directories(/opt/homebrew/opt/libpqxx/lib)
link_directories(/opt/homebrew/opt/libpq/lib)

add_executable(server main.cpp)

# 3. Link everything together (Apple Frameworks + Postgres + pqxx)
target_link_libraries(server "-framework Foundation" "-framework Security" pqxx pq)