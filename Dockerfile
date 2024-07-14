FROM ubuntu:22.04

LABEL org.opencontainers.image.authors="Andrey Sigorskikh"

RUN apt-get update && apt-get --yes --no-install-recommends install \
    bzip2 \
    g++ \
    make \
    wget \
    zlib1g \
    zlib1g-dev

# install boost
ARG BOOST_VER="1.81.0"
ARG BOOST_ROOT="/usr/local"
ENV LD_LIBRARY_PATH="/usr/local/lib"
RUN BOOST_VER_MOD=$(echo $BOOST_VER | tr . _) && \
    wget --no-check-certificate https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VER}/source/boost_${BOOST_VER_MOD}.tar.bz2 && \
    tar -xf boost_${BOOST_VER_MOD}.tar.bz2 && \
    cd boost_${BOOST_VER_MOD} && \
    ./bootstrap.sh --prefix=${BOOST_ROOT} && \
    ./b2 install --with-iostreams --with-program_options --build-dir=/tmp/build-boost && \
    cd .. &&  rm -rf boost_${BOOST_VER_MOD}.tar.bz2 boost_${BOOST_VER_MOD} /tmp/build-boost

# install fastq-dupaway
WORKDIR /tmp/fastq-dupaway
COPY src/ src/
COPY Makefile .
RUN make && \
    mv fastq-dupaway /usr/local/bin && \
    cd .. && rm -rf fastq-dupaway

WORKDIR /run
ENTRYPOINT [ "fastq-dupaway" ]
