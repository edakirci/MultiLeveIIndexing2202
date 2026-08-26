FROM gcc:latest

WORKDIR /app

RUN apt-get update && apt-get install -y \
    libcjson-dev \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

CMD ["/bin/bash"]