FROM alpine:3.22.4 AS deps

RUN apk add --no-cache \
    build-base \
    git \
    ninja \
    openssl-dev \
    util-linux-dev \
    zlib-dev \
    python3 \
    py3-pip \
    && pip3 install cmake --break-system-packages

WORKDIR /src
COPY CMakeLists.txt .
COPY cmake/ cmake/
RUN mkdir -p src && echo "" > src/CMakeLists.txt
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build


FROM deps AS builder
COPY src/ src/
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build


FROM alpine:3.22.4

RUN apk add --no-cache \
    libstdc++ \
    openssl \
    util-linux \
    zlib

WORKDIR /app
COPY --from=builder /src/build/src/Backend .

EXPOSE 8080
CMD ["/app/Backend"]
