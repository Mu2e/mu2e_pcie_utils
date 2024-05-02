#!/bin/bash

# If we're running from the /bin directory, don't rebuild...
if [[ ! -f mu2e.ko  && "x$1" == "x" ]]; then
  source ./build.sh
fi

./copyToMu2e.sh
