FROM ubuntu:16.04

RUN apt-get update && apt-get install -y \
    gcc-4.8 \
    g++-4.8 \
    g++ \
    cmake \
    git \
    libboost-context-dev \
    libboost-dev \
    default-jdk \
    doxygen \
    transfig \ 
    python3

RUN git clone git://scm.gforge.inria.fr/simgrid/simgrid.git simgrid;

WORKDIR simgrid

RUN git checkout tags/v3_11_1; \ 
    cmake -DMAKE_INSTALL_PREFIX=/opt/simgrid .; \
    make ;\
    make install