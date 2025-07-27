FROM ubuntu:22.04

LABEL org.opencontainers.image.authors="Andrey Sigorskikh"

RUN apt-get update && apt-get --yes --no-install-recommends install \
    bzip2 \
    g++ \
    make \
    wget \
    zlib1g \
    zlib1g-dev

# define env variables
ARG BOOST_VER="1.81.0"
ARG BOOST_VER_MOD="1_81_0"
ARG BOOST_ROOT="/usr/local"
ARG BOOST_CHECKSUM="71feeed900fbccca04a3b4f2f84a7c217186f28a940ed8b7ed4725986baf99fa"
ENV LD_LIBRARY_PATH="/usr/local/lib"

WORKDIR /tmp

# download boost
ADD --checksum=sha256:${BOOST_CHECKSUM} https://archives.boost.io/release/${BOOST_VER}/source/boost_${BOOST_VER_MOD}.tar.bz2 /tmp/boost_${BOOST_VER_MOD}.tar.bz2

# install boost
RUN tar -xf boost_${BOOST_VER_MOD}.tar.bz2 && \
    cd boost_${BOOST_VER_MOD} && \
    ./bootstrap.sh --prefix=${BOOST_ROOT} && \
    ./b2 install --with-iostreams --with-program_options --build-dir=/tmp/build-boost && \
    cd .. &&  \
    rm -rf boost_${BOOST_VER_MOD}.tar.bz2 boost_${BOOST_VER_MOD} /tmp/build-boost

# install fastq-dupaway
WORKDIR /tmp/fastq-dupaway
COPY src/ src/
COPY Makefile .
RUN make && \
    mv fastq-dupaway /usr/local/bin && \
    cd .. && rm -rf fastq-dupaway

WORKDIR /run
ENTRYPOINT [ "fastq-dupaway" ]
