FROM ghcr.io/pgaskin/nickeltc:1.0-9779f94.19
ARG UID=0
ARG GID=0
RUN if [ $UID -ne 0 ]; then groupadd --gid $GID user && adduser --uid $UID --gid $GID user && mkdir /rustup /cargo && chown user:user /rustup /cargo; fi
USER $UID
ENV RUSTUP_HOME=/rustup CARGO_HOME=/cargo
RUN mkdir -p /rustup /cargo && curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --target arm-unknown-linux-gnueabihf
ENV PATH=$CARGO_HOME/bin:$PATH
