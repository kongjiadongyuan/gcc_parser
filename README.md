# gcc arg parser

This project aims at parsing incoming arguments just like GCC.
The critical point of this project is to get the argument vector and the meaning of each argument, providing the ability to change at runtime.

The project cannot be compiled correctly yet, but "xgcc" can be generated.

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
# only compile xgcc
make maybe-all-gcc
# only install gcc
make maybe-install-gcc

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