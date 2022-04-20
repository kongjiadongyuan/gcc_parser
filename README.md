# gcc_parser

This branch (based on gcc-9.3.0) has been tested with ubuntu:20.04, as gcc-9.3.0 is default compiler of ubuntu:20.04.

This project aims at parsing incoming arguments just like GCC.
The critical point of this project is to get the argument vector and the meaning of each argument, providing the ability to change at runtime.

~~The project cannot be compiled correctly yet, but "xgcc" can be generated.~~

Now the entire project can be compiled normally.

# Requirements
GNU gcc needs to generate options.h at runtime, and this project needs to generate rev_options.h from options.h, so python3 and some package is neccessary.

```bash
sudo apt install python3-pip uuid-dev
pip3 install robotpy-cppheaderparser
```

Make sure **python3** (not python or python2) in your path

# Use
```bash
# after downloading
tar -zxvf gcc_parser.tar.gz
export PATH=/path/to/gcc_parser/bin:$PATH

# compile your project
COMPILE_COMMANDS_DB=/path/to/target/db/${proj_name}.db PROJ_ROOT=/path/where/compilation/starts make -j${nproc}
# Once COMPILE_COMMANDS_DB specified, gcc_parser will create a sqlite3 database
# Or gcc_parser may act like a gcc without any modification.

# PROJ_ROOT is optional.
```

# Build

```bash
# clone
cd ${WORK_DIR}
git clone https://github.com/kongjiadongyuan/gcc_parser.git

# download dependencies
cd gcc_parser
contrib/download_prerequisites

# start build
mkdir -p ${WORK_DIR}/build 
mkdir -p ${WORK_DIR}/install
cd ${WORK_DIR}/build
${WORK_DIR}/gcc_parser/configure --prefix=${WORK_DIR}/install -enable-language=c,c++ --disable-multilib --disable-werror --disable-bootstrap

# only build xgcc
make maybe-all-gcc
make maybe-install-gcc

# or build the entire project
make
make install

# have a try
${WORK_DIR}/install/bin/gcc test.c -o test
```

# MEMO
## How to add a object file used in xgcc
```makefile
# ${WORK_DIR}/gcc_parser/gcc/Makefile.in

# Object files for gcc many-languages driver.
GCC_OBJS = gcc.o gcc-main.o ggc-none.o arg_hook.o
```
arghook.o is what we need.

## DON'T print anything
Neither stdout nor stderr, otherwise something bad will happen.

[Here's the file where the main logic resides](gcc/arg_hook.c)
