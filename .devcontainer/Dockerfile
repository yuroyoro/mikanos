# See here for image contents: https://github.com/sarisia/mikanos-docker
ARG VARIANT=latest
FROM ghcr.io/sarisia/mikanos:${VARIANT}

# [Optional] Uncomment this section to install additional OS packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
      && apt-get -y install --no-install-recommends tig vim

# setup diff-highlight
RUN sudo cp /usr/share/doc/git/contrib/diff-highlight/diff-highlight /usr/local/bin/ \
      && sudo chmod +x /usr/local/bin/diff-highlight

ARG USERNAME=vscode

# switch to unprivileged
USER ${USERNAME}
WORKDIR /home/${USERNAME}

# clone original mikanos into $HOME
RUN git clone https://github.com/uchan-nos/mikanos.git

# create a symlink to MikanLoaderPkg in my repository
RUN cd /home/${USERNAME}/edk2 && ln -s /workspaces/mikanos/MikanLoaderPkg ./

# create a symlink to my Conf/
RUN cd /home/${USERNAME}/edk2 && rm -rf ./Conf && ln -s /workspaces/mikanos/edk2/Conf ./

ENV PATH $PATH:/home/${USERNAME}/osbook/devenv/

# switch back to root
USER root

