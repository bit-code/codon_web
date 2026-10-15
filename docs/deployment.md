# CodonWeb Production Deployment Guide

> **Enterprise-Grade Production Deployment, Containerization, and Kernel Optimization**

Because CodonWeb compiles directly into a standalone native machine-code binary (via LLVM), deploying it is fundamentally different from traditional Python applications. There is **no Python runtime, no virtual environment, and no WSGI/ASGI server (e.g. Gunicorn/Uvicorn)** required on the production host.

---

## 1. Building the Release Binary

Deployments use an Ahead-of-Time (AOT) compiled binary with aggressive LLVM optimizations:

```bash
# 1. Compile native C SIMD acceleration libraries (FastJson, picohttpparser, libpq)
make native

# 2. Run schema code generation if using AOT schemas
make codegen

# 3. Build standalone native release binary
CODON_PATH=. codon build -release \
  --mcpu=native \
  --inline-threshold=1000 \
  --linker-flags 'src/native/libcfastjson.a -lpq' \
  -o codon_app main.codon

# 4. Verify binary is standalone
ls -lh codon_app
file codon_app
```

The resulting `codon_app` is a self-contained, statically linked Linux ELF executable.

---

## 2. Multi-Worker & CPU Affinity Architecture

CodonWeb includes native multi-worker management with kernel-level socket load balancing:

```bash
# Launch with 8 workers on port 8000
./codon_app --workers=8 --host=0.0.0.0 --port=8000
```

### Key Production Characteristics:
1. **`SO_REUSEPORT` Kernel Balancing**: The Linux kernel distributes incoming TCP connection requests across worker listening sockets without userspace lock contention.
2. **CPU Affinity**: When `--workers=N` is set, workers automatically pin themselves to distinct physical CPU cores (`pthread_setaffinity_np`), maximizing CPU L1/L2 cache hits.

---

## 3. Linux Kernel & System Tuning

For servers serving 50,000+ requests per second, configure system limits to avoid TCP socket exhaustion.

### 3.1. File Descriptor Limits (`/etc/security/limits.conf`)
Append the following lines to `/etc/security/limits.conf`:
```ini
* soft nofile 1048576
* hard nofile 1048576
root soft nofile 1048576
root hard nofile 1048576
```

### 3.2. Kernel TCP Tuning (`/etc/sysctl.d/99-codonweb.conf`)
Create `/etc/sysctl.d/99-codonweb.conf` with:
```ini
# Maximum socket listen backlog
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 32768

# Enable rapid TCP TIME_WAIT socket recycling
net.ipv4.tcp_tw_reuse = 1

# Expand ephemeral port range
net.ipv4.ip_local_port_range = 1024 65535

# Socket read/write memory buffers
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216
net.ipv4.tcp_rmem = 4096 87380 16777216
net.ipv4.tcp_wmem = 4096 65536 16777216

# Increase system file handle table
fs.file-max = 2097152
```
Apply settings with:
```bash
sudo sysctl --system
```

---

## 4. Systemd Service Configuration

Create a systemd unit file at `/etc/systemd/system/codonweb.service`:

```ini
[Unit]
Description=CodonWeb High-Performance Web Application
After=network.target postgresql.service redis.service
Wants=network-online.target

[Service]
Type=simple
User=www-data
Group=www-data
WorkingDirectory=/opt/codonweb
ExecStart=/opt/codonweb/codon_app --workers=8 --port=8000 --host=127.0.0.1
Restart=always
RestartSec=3s

# Resource Limits
LimitNOFILE=1048576
LimitNPROC=65535

# Security Sandboxing
ProtectSystem=full
ProtectHome=true
NoNewPrivileges=true
PrivateTmp=true

[Install]
WantedBy=multi-user.target
```

Enable and start the service:
```bash
sudo systemctl daemon-reload
sudo systemctl enable codonweb
sudo systemctl start codonweb
sudo systemctl status codonweb
```

---

## 5. Nginx Reverse Proxy Configuration

Nginx handles TLS termination, compression, and edge caching while forwarding traffic to CodonWeb:

```nginx
upstream codonweb_upstream {
    server 127.0.0.1:8000;
    keepalive 256;
}

server {
    listen 80;
    server_name api.example.com;
    return 301 https://$host$request_uri;
}

server {
    listen 443 ssl http2;
    server_name api.example.com;

    ssl_certificate /etc/letsencrypt/live/api.example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/api.example.com/privkey.pem;

    # Performance buffers
    client_max_body_size 50M;

    location / {
        proxy_pass http://codonweb_upstream;
        proxy_http_version 1.1;

        # WebSocket support
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";

        # Forwarded headers
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        # Disable buffering for Server-Sent Events (SSE) and Chunked streams
        proxy_buffering off;
        proxy_cache off;
        proxy_read_timeout 300s;
    }
}
```

---

## 6. Docker & Containerization

### Multi-Stage `Dockerfile`
To produce an ultra-compact production container image:

```dockerfile
# ==========================================
# Stage 1: Build Environment
# ==========================================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    curl \
    git \
    libpq-dev \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Install Codon compiler
RUN curl -fsSL https://exaloop.io/install.sh | bash
ENV PATH="/root/.codon/bin:${PATH}"

WORKDIR /build
COPY . .

# Build native C extensions and application
RUN make native
RUN make codegen
RUN codon build -release --linker-flags 'src/native/libcfastjson.a -lpq' -o codon_app examples/fastapi_style_app.codon

# ==========================================
# Stage 2: Minimal Runtime Image
# ==========================================
FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y \
    ca-certificates \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build/codon_app /app/codon_app

EXPOSE 8000
ENTRYPOINT ["/app/codon_app", "--host=0.0.0.0", "--port=8000"]
```

Build and run:
```bash
docker build -t codonweb-app:latest .
docker run -d --name codonweb -p 8000:8000 --restart=always codonweb-app:latest
```

---

## 7. Health Checks & Production Monitoring

CodonWeb integrates with monitoring agents through standard endpoints:

```python
@app.get("/healthz", summary="Liveness & Readiness Probe")
def healthz(req: Request) -> Response:
    return Response.json('{"status": "healthy", "uptime_sec": 12480}')
```
