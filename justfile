set dotenv-load := true

[group('helpers')]
default:
  @{{just_executable()}} --list --justfile {{justfile()}}

# Cross compile SyncController for Kobo
[group('build')]
[working-directory: 'cli']
build-cli:
  docker pull ewpratten/kobo-cross-armhf:latest || podman pull ewpratten/kobo-cross-armhf:latest
  cross build --release --target arm-unknown-linux-musleabihf

# Cross compile nickel hook for Kobo
[group('build')]
[working-directory: 'hook']
build-hook:
  PATH=../bin:$PATH make

# Cross compile all code for Kobo
build: build-hook build-cli

# Package files into installable KoboRoot.tgz
[group('build')]
package: build
  #!/usr/bin/env sh
  mkdir KoboRoot
  cd KoboRoot
  mkdir -p usr/local/Kobo/imageformats/ mnt/onboard/.adds/NickelHardcover usr/share/NickelHardcover
  cp ../hook/libhardcover.so                                                 usr/local/Kobo/imageformats/
  cp ../cli/target/arm-unknown-linux-musleabihf/release/nickel-hardcover-cli mnt/onboard/.adds/NickelHardcover/cli
  cp ../res/config.ini                                                       mnt/onboard/.adds/NickelHardcover
  cp ../res/*.png                                                            usr/share/NickelHardcover
  tar -vczf ../KoboRoot.tgz . | grep '[^/]$'
  cd ..
  rm -r KoboRoot

# Copy KoboRoot.tgz onto kobo device
copy-package: package
  cp KoboRoot.tgz /media/$(whoami)/KOBOeReader/.kobo/
  sudo eject /media/$(whoami)/KOBOeReader/

# Fetch hardcover.app graphql schema
[group('helpers')]
fetch-schema:
  npx get-graphql-schema --header "authorization=$HARDCOVER_AUTHORIZATION" https://api.hardcover.app/v1/graphql > cli/src/graphql/schema.graphql

# Format all source files
[group('helpers')]
format:
  cd cli && cargo fmt
  clang-format -i hook/src/**/*.cc hook/src/*.cc
  clang-format -i hook/src/**/*.h hook/src/*.h

clean:
  cd hook && make clean
  cd cli && cargo clean
  rm KoboRoot.tgz

# Run `logread` over ssh
logs:
  #!/usr/bin/env expect
  spawn ssh $::env(KOBO_SERVER)
  expect "password: "
  send "$::env(KOBO_PASSWORD)\r"
  expect -ex {[root@kobo ~]#}
  send "logread -f\r"
  interact
