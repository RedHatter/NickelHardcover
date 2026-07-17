set dotenv-load

export VERSION := `git describe --tags --long --dirty 2>/dev/null`

[group('helpers')]
default:
  @{{ just_executable() }} --list --justfile {{ justfile() }}

# Build and push the toolchain docker image to docker hub
[group('build')]
build-tc:
  docker buildx build . --tag strayrose/nickeltc --push

# Render modified svgs to png
[group('build')]
[working-directory('hook/res')]
build-res:
  #!/usr/bin/env sh
  for svg in *.svg; do
    png="${svg%.svg}.png"
    if [ ! -e "$png" -o "$svg" -nt "$png" ]; then
      echo "$png"
      resvg "$svg" "$png"
    fi
  done

# Build the nickel QT plugin
[group('build')]
[working-directory('hook')]
build-hook:
  make

# Build the rust CLI
[group('build')]
[working-directory('cli')]
build-cli:
  cargo build --release --target=arm-unknown-linux-gnueabihf

[group('build')]
build: build-res build-hook build-cli

# Package files into installable KoboRoot.tgz
[group('package')]
package:
  #!/usr/bin/env sh
  mkdir KoboRoot
  cd KoboRoot
  mkdir -p usr/local/Kobo/imageformats/ mnt/onboard/.adds/NickelHardcover
  cp ../hook/libhardcover.so                                                usr/local/Kobo/imageformats/
  cp ../cli/target/arm-unknown-linux-gnueabihf/release/nickel-hardcover-cli mnt/onboard/.adds/NickelHardcover/cli
  cp ../hook/res/config_example.ini                                         mnt/onboard/.adds/NickelHardcover
  tar -vczf ../KoboRoot.tgz . | grep '[^/]$'
  cd ..
  rm -r KoboRoot

# Build KoboRoot.tgz within NickelTC docker container
[group('package')]
build-package:
  docker run --volume="$PWD:$PWD" --workdir="$PWD" -u $(id --user):$(id --group) --rm strayrose/nickeltc just build package

# Copy KoboRoot.tgz onto kobo device
[group('package')]
copy-package: build-package
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

[group('helpers')]
clean:
  cd hook && make clean
  cd cli && cargo clean
  rm -fv KoboRoot.tgz
  for f in hook/res/*.png; do git ls-files --error-unmatch $f > /dev/null 2>&1 || rm -v $f; done

# Run `logread` over ssh
[group('helpers')]
logs:
  #!/usr/bin/env expect
  spawn ssh $::env(KOBO_SERVER)
  expect "password: "
  send "$::env(KOBO_PASSWORD)\r"
  expect -ex {[root@kobo ~]#}
  send "logread -f\r"
  interact
