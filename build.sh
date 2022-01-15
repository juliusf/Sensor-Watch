#!/bin/bash

docker build  -t gcc-arm-embedded .
docker run --rm \
        -v "`pwd`:/build" \
        -w "/build" \
        --device /dev/fuse \
        --cap-add SYS_ADMIN \
        -t gcc-arm-embedded \
        /bin/bash -c "cd $1 && make -j `nproc`"

