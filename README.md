# gcc_parser

This branch (based on gcc-9.3.0) has been tested with ubuntu:20.04, as gcc-9.3.0 is default compiler of ubuntu:20.04.
This branch (9.3.0) is currently the main maintained branch where features will be implemented and tested first.

This project aims at parsing incoming arguments just like GCC.
The critical point of this project is to get the argument vector and the meaning of each argument, providing the ability to change at runtime.

~~The project cannot be compiled correctly yet, but "xgcc" can be generated.~~

Now the entire project can be compiled normally.

# Dependencies
GNU gcc needs to generate options.h at runtime, and this project needs to generate rev_options.h from options.h, so python3 and some package is neccessary.

```bash
sudo apt install python3-pip uuid-dev
pip3 install robotpy-cppheaderparser
```

Make sure **python3** (not python or python2) in your path

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

# maybe only build xgcc
# make maybe-all-gcc
# make maybe-install-gcc

# build the entire project
make
make install

# have a try
${WORK_DIR}/install/bin/gcc test.c -o test
```


# Use
gcc_parser changes compilation behavior based on environment variables
The currently supported environment variables are listed below

|environment variable|value|meaning|
|---|---|---|
|COMPILE_COMMANDS_DB|path|path to compile_commands.sqlite3, controls information recording behavior, if not set, gcc_parser will not save runtime information|
|PROJ_ROOT|path|path to the target project's root path, will be recorded in the database|
|ARCHIVE|path|path to save backup files, if not set, gcc_parser will not create backup files. ld_hook also depends on this environment variable|
|GCC_PARSER_HIJACK_DWARF4|1|if set, gcc_parser will cover any debug options like "-g*", and force "-gdwarf-4"|
|GCC_PARSER_HIJACK_OPTIMIZATION|0,1,2,3,fast,g,s|if set, gcc_parser will force optimization level to which the value corresponding to|

# MEMO
Here are some development records.
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
