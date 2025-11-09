#!/bin/bash
# Run Ironwail with stub libraries for headless testing
export LD_LIBRARY_PATH="$(dirname "$0")/lib_stubs:$(dirname "$0")/nng_lib:$LD_LIBRARY_PATH"
exec "$(dirname "$0")/ironwail" "$@"
