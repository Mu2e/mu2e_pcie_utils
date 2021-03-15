#!/bin/bash

if ! [ -d trace-git ]; then
  git clone https://cdcvs.fnal.gov/projects/trace-git
fi

cat >Makefile <<EOF

EXTRA_SYMBOLS=KBUILD_EXTRA_SYMBOLS=$PWD/trace-git/src_module/Module.symvers
TRACE_INC=$PWD/trace-git/include

all: 
	make -C trace-git/src_module/
	make -C mu2e_driver/ CC=/usr/bin/gcc EXTRA_SYMBOLS=\${EXTRA_SYMBOLS} TRACE_INC=\${TRACE_INC}
	make -C mcs/basic_driver/ CC=/usr/bin/gcc EXTRA_SYMBOLS=\${EXTRA_SYMBOLS} TRACE_INC=\${TRACE_INC}

clean:
	make -C trace-git/src_module/ clean
	make -C mu2e_driver/ clean
	make -C mcs/basic_driver/ clean

EOF
