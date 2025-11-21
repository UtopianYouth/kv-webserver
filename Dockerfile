# ===== 构建阶段 =====
FROM ubuntu:20.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y g++ make && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY include/ ./include/
COPY src/ ./src/
COPY Makefile ./

RUN make

# ===== 运行阶段 =====
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y libstdc++6 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /build/bin/kv-webserver ./bin/
COPY frontend/ ./frontend/
COPY resources/ ./resources/

EXPOSE 8080

CMD ["./bin/kv-webserver", "8080"]
