# Define Docker image and container names
DOCKER_IMAGE_NAME = gcc-x86-64-dev
DOCKER_CONTAINER_NAME = gcc-dev-env

# Define directory paths
SRC_DIR = $(PWD)/src
SCRIPTS_DIR = $(PWD)/scripts

# Default target: build and run
.PHONY: all
all: build_docker run_docker

# Build the Docker image for x86_64 platform
.PHONY: build_docker
build_docker:
	@echo "Building Docker image for x86_64 platform..."
	docker build --platform=linux/amd64 -t $(DOCKER_IMAGE_NAME) .

# Run the container with x86_64 environment
# - Checks if container exists and removes it
# - Mounts current directory to /app
# - Sets working directory to /app
# - Runs in interactive mode with TTY
.PHONY: run_docker
run_docker:
	@echo "Starting x86_64 development environment..."
	@if [ "$$(docker ps -aq -f name=^/${DOCKER_CONTAINER_NAME}$$)" ]; then \
		echo "Stopping existing container..."; \
		docker stop ${DOCKER_CONTAINER_NAME} >/dev/null; \
		docker rm ${DOCKER_CONTAINER_NAME} >/dev/null; \
	fi
	@echo "Starting new x86_64 container..."
	@docker run -it \
		--platform linux/amd64 \
		--name ${DOCKER_CONTAINER_NAME} \
		--privileged \
		--cap-add=SYS_PTRACE \
		--cap-add=SYS_ADMIN \
		--security-opt seccomp=unconfined \
		--security-opt apparmor=unconfined \
		--memory 16g \
		--memory-swap 32g \
		--ulimit nofile=65536:65536 \
		--ulimit stack=-1:-1 \
		-v $$(pwd):/app \
		-w /app \
		${DOCKER_IMAGE_NAME} \
		/bin/bash

# Stop and remove the container if it exists
.PHONY: stop_docker
stop_docker:
	@echo "Stopping container..."
	@docker stop $(DOCKER_CONTAINER_NAME) 2>/dev/null || true
	@docker rm $(DOCKER_CONTAINER_NAME) 2>/dev/null || true

# Clean up everything - stops container and removes image
.PHONY: clean_docker
clean_docker: stop
	@echo "Cleaning up..."
	@docker rmi $(DOCKER_IMAGE_NAME) 2>/dev/null || true

# Rebuild everything from scratch
.PHONY: rebuild_docker
rebuild_docker: clean_docker build_docker run_docker

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CC = clang
    CFLAGS = -g -O3 -I./include
else
    CC = gcc
    CFLAGS = -g -O3 -I./include -Wno-int-to-void-pointer-cast
endif

.PHONY: compile
compile:
	@echo "Compiling with $(CC)..."
	$(CC) $(CFLAGS) ./src/*.c -o ./bin/cfeeny