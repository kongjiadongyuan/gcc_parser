# gcc arg parser

This project aims at parsing incoming arguments just like GCC.
The critical point of this project is to get the argument vector and the meaning of each argument, providing the ability to change at runtime.

The project cannot be compiled correctly yet, but "xgcc" can be generated.

# Build

```bash
./configure --prefix=/usr/local/gcc --with-gmp=/usr/local/gmp --with-mpfr=/usr/local/mpfr --with-mpc=/usr/local/mpc --disable-multilib --disable-werror --disable-bootstrap
```