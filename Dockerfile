FROM --platform=linux/amd64 ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

ENV TZ=Europe/Stockholm
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && apt-get install -y \
    gcc \
    g++ \
    make \
    gdb \
    build-essential \
    file \
    valgrind \
    strace \
    ltrace \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

ENV CFLAGS="-m64 -march=x86-64"
ENV CXXFLAGS="-m64 -march=x86-64"

RUN echo "* soft nofile 65536" >> /etc/security/limits.conf && \
    echo "* hard nofile 65536" >> /etc/security/limits.conf

RUN echo "alias ll='ls -l'" >> /root/.bashrc && \
    echo "alias la='ls -la'" >> /root/.bashrc
