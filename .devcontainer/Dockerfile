FROM "ubuntu:22.04"

# First, get curl and gnupg so we can get the custom deb sources
# for nodejs and yarn
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends curl gnupg apt-transport-https ca-certificates

# get golang
RUN curl -sL https://go.dev/dl/go1.19.1.linux-amd64.tar.gz -o go1.19.1.linux-amd64.tar.gz
RUN tar -C /usr/local -xzf go1.19.1.linux-amd64.tar.gz
RUN rm go1.19.1.linux-amd64.tar.gz

# From https://github.com/microsoft/vscode-dev-containers/blob/main/container-templates/dockerfile/.devcontainer/Dockerfile
# Create a non-admin user for vscode
# using the library-scripts from the microsoft example repo
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID
ARG INSTALL_ZSH=''
ARG UPGRADE_PACKAGES=''

COPY library-scripts/*.sh library-scripts/*.env /tmp/library-scripts/
RUN cp /tmp/library-scripts/etc_skel_profile.sh /etc/skel/.profile

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && /bin/bash /tmp/library-scripts/common-debian.sh "${INSTALL_ZSH}" "${USERNAME}" "${USER_UID}" "${USER_GID}" "${UPGRADE_PACKAGES}" "true" "true"

RUN DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends \
         build-essential \
         git \
         gdb \
         libpcre3-dev \
         libkrb5-dev \
         libssl-dev \
         gcc-mingw-w64 \
         g++-mingw-w64 \
         clang \
         cmake \
         lzma-dev \
         libxml2-dev

RUN DEBIAN_FRONTEND=noninteractive apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Allow ownership access for repos mounted at /workspaces/go-libsass
# this is used to allow for rev-parse when we bind-mount
# a volume, because of the permissions Docker uses when
# mounting a bind volume.
#
# See https://github.blog/2022-04-12-git-security-vulnerability-announced/
RUN git config --system --add safe.directory /workspaces/go-libsass

# expose dev ports
EXPOSE 3000
EXPOSE 3001
