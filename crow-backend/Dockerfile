FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        libpqxx-dev \
        libpq-dev \
        libasio-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel

FROM debian:bookworm-slim

# Installing the -dev package (rather than guessing the exact runtime .so
# package name/version) keeps this reliable across Debian point releases;
# it also pulls in libpq5 as a dependency.
RUN apt-get update && apt-get install -y --no-install-recommends \
        libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/build/server ./server

EXPOSE 8080
CMD ["./server"]
