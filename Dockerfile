FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    qt6-base-dev \
    qt6-declarative-dev \
    libqt6shadertools6-dev \
    libgl1-mesa-dev \
    libsqlite3-dev \
    libssl-dev \
    ca-certificates \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /build_dir
COPY . .

RUN mkdir build && cd build && \
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release .. && \
    ninja ChatServer UnitTests

RUN cd build && ./tests/UnitTests

FROM ubuntu:24.04

RUN sed -i 's/archive.ubuntu.com/mirror.yandex.ru/g' /etc/apt/sources.list.d/ubuntu.sources && \
    apt-get update && apt-get install -y --no-install-recommends \
    libqt6core6t64 \
    libqt6network6t64 \
    libsqlite3-0 \
    libssl3 \
    ca-certificates \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m appuser
USER appuser
WORKDIR /home/appuser

COPY --from=builder /build_dir/build/server/ChatServer .

EXPOSE 8080

ENTRYPOINT ["./ChatServer"]
CMD ["--port", "8080"]