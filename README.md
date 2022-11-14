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
|GCC_ARCHIVE|path|path to save backup files, if not set, gcc_parser will not create backup files|
|GCC_PARSER_HIJACK_DWARF4|1|if set, gcc_parser will cover any debug options like "-g*", and force "-gdwarf-4"|
|GCC_PARSER_HIJACK_OPTIMIZATION|0,1,2,3,fast,g,s|if set, gcc_parser will force optimization level to which the value corresponding to|

## 数据库格式

### 约定
1. 以下所有的路径都必须满足如下条件之一：
    1. 本身是绝对路径；
    2. pwd+路径字符串是绝对路径；
2. COMPILE_COMMANDS_DB环境变量指定数据库的路径，**必须是绝对路径**；如果没有指定，hook函数应当直接返回，对外表现为没有修改过的编译器（链接器）；
3. PROJ_ROOT变量不指定视为空字符串；

### gcc_parser
1. 表：t_commands
2. 字段如下：
    1. id: 自增主键
    2. runtime_uuid: 每一次运行都不同
    3. timestamp: 以毫秒为单位的时间戳
    4. pwd: 当前命令执行的路径（getcwd()的返回值）
    5. proj_root: 由环境变量PROJ_ROOT传入的值
    6. output: -o指定的输出文件的路径
    7. cmdline: 执行的完整命令行
    8. arg_idx: 当前decoded_args结构体在整条命令中的顺序
    9. json: 其他要存储的内容，由json格式存入
3. traced：当gcc被hook时，在环境变量中添加GCC_RUNTIME_UUID，其值与runtime_uuid相等，由ld_hook读取

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
