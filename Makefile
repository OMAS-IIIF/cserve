
.PHONY: compile
compile:
	docker run --rm -v ${PWD}:/cserve lrosenth/omas-base:1.0.0 /bin/sh -c "mkdir -p /cserve/build-linux && cd /cserve/build-linux && cmake .. && make"

.PHONY: test
test:
	docker run --rm -v ${PWD}:/cserve lrosenth/omas-base:1.0.0 /bin/sh -c "mkdir -p /cserve/build-linux && cd /cserve/build-linux && cmake .. && make test"