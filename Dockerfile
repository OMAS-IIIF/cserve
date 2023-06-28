
ARG OMAS_BASE=lrosenth/omas-base:2.0.0
ARG UBUNTU_BASE=ubuntu:22.04

FROM $OMAS_BASE as builder

ARG UID=1000
RUN useradd -m -u ${UID} -s /bin/bash builder
USER builder


