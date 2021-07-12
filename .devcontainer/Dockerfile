# See here for image contents: https://github.com/sarisia/mikanos-docker
ARG VARIANT=latest
FROM ghcr.io/sarisia/mikanos:${VARIANT}

# [Optional] Uncomment this section to install additional OS packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
      && apt-get -y install --no-install-recommends tig

# setup diff-highlight
RUN sudo cp /usr/share/doc/git/contrib/diff-highlight/diff-highlight /usr/local/bin/ \
      && sudo chmod +x /usr/local/bin/diff-highlight

ARG USERNAME=vscode

# switch to unprivileged
USER ${USERNAME}
WORKDIR /home/${USERNAME}

# clone original mikanos into $HOME
RUN git clone https://github.com/uchan-nos/mikanos.git

# switch back to root
USER root
