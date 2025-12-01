FROM ubuntu:24.04 as builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    nlohmann-json3-dev

COPY . /app
WORKDIR /app
RUN cmake -B build -S . && \
    cmake --build build --config Release

FROM ubuntu:24.04

# Только минимальные рантайм-зависимости
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=builder /app/build/rtchat_server/rtchat_server /usr/local/bin/app

CMD ["app"]
