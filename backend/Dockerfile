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

# 只复制后端可执行文件（不再包含前端资源）
COPY --from=builder /build/bin/kv-webserver ./bin/

EXPOSE 8080

CMD ["./bin/kv-webserver", "8080"]
