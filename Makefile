# Version of the base Docker image
OMAS_BASE := lrosenth/omas-base:2.0.0
UBUNTU_BASE := ubuntu:22.04

.PHONY: compile
compile:
	docker run \
		--rm \
		--user=$(shell id -u) \
		-v ${PWD}:/cserve $(OMAS_BASE) /bin/sh -c "mkdir -p /cserve/build-linux && cd /cserve/build-linux && cmake .. && make"

.PHONY: compile-dbg
compile-dbg:
	docker run \
		--rm \
		--user=$(shell id -u) \
		-v ${PWD}:/cserve $(OMAS_BASE) /bin/sh -c "mkdir -p /cserve/build-linux && cd /cserve/build-linux && cmake -DMAKE_DEBUG:BOOL=ON .. && make"

.PHONY: test
test:
	docker run \
		--rm \
		--user=$(shell id -u) \
		-v ${PWD}:/cserve $(OMAS_BASE) /bin/sh -c "mkdir -p /cserve/build-linux && cd /cserve/build-linux && cmake .. && make test"

.PHONY: test-dbg
test-dbg:
	docker run \
		--rm \
		--user=$(shell id -u) \
		-v ${PWD}:/cserve $(OMAS_BASE) /bin/sh -c "mkdir -p /cserve/build-linux && cd /cserve/build-linux && cmake -DMAKE_DEBUG:BOOL=ON .. && make test"
