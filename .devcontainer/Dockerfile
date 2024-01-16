FROM mcr.microsoft.com/vscode/devcontainers/base:ubuntu

# Install Node.js
ARG NODE_MAJOR=20
ENV NODE_MAJOR=$NODE_MAJOR
RUN curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg
RUN echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_$NODE_MAJOR.x nodistro main" | tee /etc/apt/sources.list.d/nodesource.list

# Install GitHub CLI Key and Repo
RUN curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg \
    && chmod go+r /usr/share/keyrings/githubcli-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | tee /etc/apt/sources.list.d/github-cli.list > /dev/null

# Install Main Tools
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -qqy install \
        bc \
        sudo \
        build-essential \
        software-properties-common \
        ca-certificates \
        clang \
        curl \
        dnsutils \
        gcc \
        git \
        python3 \
        python3-dev \
        python3-venv \
        python3-distutils \
        python3-pip \
        python3-setuptools \
        python2.7 libpython2.7-dev flex bison gperf cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 libncurses5 \ 
        srecord \
        udev \
        xz-utils \
        usbutils \
        iputils-ping \
        iputils-arping \
        nmap \
        bash-completion \
        meson \
        gnupg \
        nodejs \
        gh \
    && apt-get autoremove -y \
    && apt-get clean autoclean -y
    # && rm -rf /var/lib/apt/lists/*  

RUN add-apt-repository ppa:openjdk-r/ppa -y \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -qqy install openjdk-21-jdk    
    # && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/tio/tio.git \
    && cd tio \
    && meson setup build \
    && meson compile -C build \
    && meson install -C build \
    && cd .. \
    && rm -rf tio

ENV PYTHONPATH "${PYTHONPATH}:/usr/bin/python3"
RUN ln -s /usr/bin/python3 /usr/bin/python

RUN curl -fLo /etc/udev/rules.d/99-platformio-udev.rules --create-dirs https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules \
    && curl -fLo /etc/udev/rules.d/60-openocd.rules --create-dirs https://raw.githubusercontent.com/raspberrypi/openocd/rp2040/contrib/60-openocd.rules \
    && /lib/systemd/systemd-udevd --daemon \
    && udevadm control --reload-rules

ARG USERNAME=vscode
ENV USERNAME=$USERNAME
RUN usermod -a -G dialout,plugdev ${USERNAME}
RUN mkdir -p /home/${USERNAME}/.vscode-server/extensions \
    && chown -R ${USERNAME}:${USERNAME} /home/${USERNAME} \
    && chmod -R 777 /home/${USERNAME}
USER $USERNAME

RUN curl -fsSL --create-dirs -o ~/get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py \
    && python3 ~/get-platformio.py

RUN mkdir -p ~/.local/bin \
    && ln -s ~/.platformio/penv/bin/platformio ~/.local/bin/platformio \
    && ln -s ~/.platformio/penv/bin/pio ~/.local/bin/pio \
    && ln -s ~/.platformio/penv/bin/piodebuggdb ~/.local/bin/piodebuggdb

RUN echo "add-auto-load-safe-path /workspace/.gdbinit" > ~/.gdbinit    

WORKDIR /workspace

ENTRYPOINT ["platformio"] 