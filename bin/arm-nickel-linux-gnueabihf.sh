#!/bin/bash
exec /usr/bin/docker run --volume="$PWD:$PWD" --user="$(id --user):$(id --group)" --workdir="$PWD" --env=HOME --entrypoint="$(basename "${BASH_SOURCE[0]}")" --rm -it ghcr.io/pgaskin/nickeltc:1 "$@"
