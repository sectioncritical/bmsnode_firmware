
all: build

.PHONY: help
help:
	@echo ""
	@echo "BMSNode Docker Image Maintenance"
	@echo "--------------------------------"
	@echo "login - login to gitlab registry (asks for gitlab password)"
	@echo "build - build the bmsnode docker image"
	@echo "push  - push the bmsnode docker image to gitlab"
	@echo "run   - run container for local testing"
	@echo ""

.PHONY: build
build:
	docker build -t registry.gitlab.com/kroesche/bmsnode .

.PHONY: login
login:
	docker login -u kroesche registry.gitlab.com

.PHONY: push
push:
	docker push registry.gitlab.com/kroesche/bmsnode

.PHONY: containers
containers:
	docker container list --all

.PHONY: images
images:
	docker image list --all

.PHONY: run
run:
	cd ../..; docker run -it -v $$(pwd):/mount registry.gitlab.com/kroesche/bmsnode /bin/bash
