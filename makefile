# Define Docker image and container names
DOCKER_IMAGE_NAME = gcc-x86-64-dev
DOCKER_CONTAINER_NAME = gcc-dev-env

# Define directory paths
SRC_DIR = $(PWD)/src
SCRIPTS_DIR = $(PWD)/scripts

# Default target: build and run
.PHONY: all
all: build run

# Build the Docker image for x86_64 platform
.PHONY: build
build:
	@echo "Building Docker image for x86_64 platform..."
	docker build --platform=linux/amd64 -t $(DOCKER_IMAGE_NAME) .

# Run the container with x86_64 environment
# - Checks if container exists and removes it
# - Mounts current directory to /app
# - Sets working directory to /app
# - Runs in interactive mode with TTY
.PHONY: run
run:
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
		--memory 16g \
		--memory-swap 32g \
		--ulimit nofile=65536:65536 \
		--ulimit stack=-1:-1 \
		-v $$(pwd):/app \
		-w /app \
		${DOCKER_IMAGE_NAME} \
		/bin/bash

# Stop and remove the container if it exists
.PHONY: stop
stop:
	@echo "Stopping container..."
	@docker stop $(DOCKER_CONTAINER_NAME) 2>/dev/null || true
	@docker rm $(DOCKER_CONTAINER_NAME) 2>/dev/null || true

# Clean up everything - stops container and removes image
.PHONY: clean
clean: stop
	@echo "Cleaning up..."
	@docker rmi $(DOCKER_IMAGE_NAME) 2>/dev/null || true

# Rebuild everything from scratch
.PHONY: rebuild
rebuild: clean build run
