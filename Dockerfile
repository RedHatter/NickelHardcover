FROM ghcr.io/pgaskin/nickeltc:1.0-9779f94.19

ENV RUSTUP_HOME=/usr/local/rustup \
    CARGO_HOME=/usr/local/cargo \
    PATH=/usr/local/cargo/bin:$PATH

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --no-modify-path --profile minimal --target arm-unknown-linux-gnueabihf && \
    cargo install resvg just && \
    chmod -R a+w $RUSTUP_HOME $CARGO_HOME
