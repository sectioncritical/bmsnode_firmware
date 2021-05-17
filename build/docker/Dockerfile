# Notes about base image
#
# We need to use bullseye because we need the versions of
# doxygen, gcc-avr, and cppcheck that are included. If we use buster, then
# those utilities do not propertly support the ATtiny1614.
# They can be built separately as done in the past (see git history of this
# Dockerfile), but it is easier and faster to get the them from the distro
# repo.
#
# About image size:
#
# At the time of this writing, the image size is about 800M. A lot of that
# comes from python packages that are installed. It appears that there are
# slim python images available, for example 3.9-slim-buster, which are quite
# a bit smaller. However at this time python docker images are not available
# based on bullseye so we are back to having out of date AVR build tools.
# In the future, when there is a python slim image for bullseye, then this
# Dockerfile can be updated to use that which should result in a significantly
# smaller image.

FROM debian:bullseye-slim
WORKDIR /bmsnode

# npm/node stuff was originally here to support documentation tool smcat
# but that is not used in the build right now so dont need node

# base tools
RUN apt-get update          \
    && apt-get install -y   \
        avr-libc            \
        cppcheck            \
        curl                \
        doxygen             \
        gcc-avr             \
        git                 \
        make                \
#        nodejs              \
#        npm                 \
        pkg-config          \
        python3             \
        python3-pip         \
        python3-venv        \
        python3-wheel       \
        python3-pygments    \
        unzip zip           \
    && rm -rf /var/lib/apt/lists/*
#    && rm -rf /var/lib/apt/lists/* \
#    && update-alternatives --install /usr/bin/python python /usr/bin/python3 10 \
#    && npm install --global state-machine-cat


# we only need the include and gcc directories from the atpack

RUN mkdir -p /opt/avr \
    && curl -L -o /root/attiny.atpack \
       https://packs.download.microchip.com/Microchip.ATtiny_DFP.2.6.122.atpack \
    && unzip /root/attiny.atpack "include/*" "gcc/*" -d /opt/avr/ATtiny_DFP.2.6.122 \
    && ln -s /opt/avr/ATtiny_DFP.2.6.122 /opt/avr/ATtiny_DFP \
    && rm -f /root/attiny.atpack

CMD ["/bin/bash"]
