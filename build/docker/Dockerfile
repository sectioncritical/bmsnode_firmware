FROM ubuntu:18.04
WORKDIR /mount
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
        libpcre3-dev \
        curl \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/python python /usr/bin/python3 10
RUN git clone -b 1.90 --depth=1 https://github.com/danmar/cppcheck.git \
    && cd cppcheck \
    && make MATCHCOMPILER=yes FILESDIR=/usr/share/cppcheck HAVE_RULES=yes CXXFLAGS="-O2 -DNDEBUG -Wall -Wno-sign-compare -Wno-unused-function" install \
    && cd .. && rm -rf cppcheck
CMD ["/bin/bash"]
