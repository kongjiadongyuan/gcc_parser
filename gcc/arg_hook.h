#ifndef ARG_HOOK_H
#define ARG_HOOK_H

#define ARG_HOOK_PATH_SIZE 255
#define ORIG_GCC "/usr/bin/gcc"
#define ORIG_GPP "/usr/bin/g++"
#define ORIG_CPP "/usr/bin/c++"
#define ORIG_CPP_O "/usr/bin/cpp"

extern void print_cl_decoded_option(struct cl_decoded_option *opt);

extern void argv_dump(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options);

extern void observe_decoded_options(unsigned int count, struct cl_decoded_option *options);

extern void call_orig(unsigned int count, struct cl_decoded_option *options);

extern bool endswith(char *str1, char *str2);

extern void call_orig_tmp(int argc, char **argv);

extern void arg_hook_main(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options, int argc, char **argv);

#endif