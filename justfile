set dotenv-load

[group('helpers')]
default:
  @{{ just_executable() }} --list --justfile {{ justfile() }}

# Render modified svgs to png
[group('build')]
[working-directory('hook/res')]
build-res:
  #!/usr/bin/env sh
  for file in *.svg; do
    output="${file%.svg}.png"
    if [ ! -e "$output" -o "$file" -nt "$output" ]; then
      echo "$output"
      rsvg-convert "$file" --output "$output"
    fi
  done

# Prepare the toolchain docker image
[group('build')]
build-tc:
  docker build .forgejo/nickeltc --build-arg UID=$(id --user) --build-arg GID=$(id --group) --tag strayrose/nickeltc

# Cross compile for Kobo
[group('build')]
build: build-res
  docker run --volume="$PWD:$PWD" --workdir="$PWD" --rm strayrose/nickeltc sh -c \
    "export VERSION=$(git describe --tags --long --dirty 2>/dev/null) && \
    make --directory=hook && cd cli && \
    cargo build --release --target=arm-unknown-linux-gnueabihf"

# Package files into installable KoboRoot.tgz
[group('package')]
package: build
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

# Copy KoboRoot.tgz onto kobo device
[group('package')]
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
  rm -fv KoboRoot.tgz
  for f in hook/res/*.png; do git ls-files --error-unmatch $f > /dev/null 2>&1 || rm -v $f; done

# Run `logread` over ssh
logs:
  #!/usr/bin/env expect
  spawn ssh $::env(KOBO_SERVER)
  expect "password: "
  send "$::env(KOBO_PASSWORD)\r"
  expect -ex {[root@kobo ~]#}
  send "logread -f\r"
  interact
