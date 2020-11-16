FROM debian:buster-slim
WORKDIR /mount

# base tools
RUN apt-get update \
    && apt-get install -y \
        gcc \
        g++ \
        gcc-avr \
        avr-libc \
        make \
        pkg-config \
        python3 \
        python3-pip \
        python3-venv \
        python3-wheel \
        python3-pygments \
        git \
        curl \
        cmake \
        flex \
        bison \
        libpcre3-dev \
        nodejs \
        npm \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/python python /usr/bin/python3 10 \
    && npm install --global state-machine-cat

# build cppcheck
RUN git clone -b 1.90 --depth=1 https://github.com/danmar/cppcheck.git \
    && cd cppcheck \
    && make MATCHCOMPILER=yes FILESDIR=/usr/share/cppcheck HAVE_RULES=yes CXXFLAGS="-O2 -DNDEBUG -Wall -Wno-sign-compare -Wno-unused-function" install \
    && cd .. && rm -rf cppcheck

# build doxygen 1.8.20 from sources
RUN curl -L -o doxygen-1.8.20.src.tar.gz https://doxygen.nl/files/doxygen-1.8.20.src.tar.gz \
    && tar -zxf doxygen-1.8.20.src.tar.gz \
    && cd doxygen-1.8.20 \
    && mkdir -v build \
    && cd build \
    && cmake -G "Unix Makefiles" \
        -D CMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -Wno-dev .. \
    && make \
    && make install \
    && cd ../.. && rm -rf doxygen-1.8.20 && rm -f doxygen-1.8.20.src.tar.gz

CMD ["/bin/bash"]
