#ifndef ARG_HOOK_H
#define ARG_HOOK_H

#define ARG_HOOK_PATH_SIZE 255

extern void print_cl_decoded_option(struct cl_decoded_option *opt);

extern void argv_dump(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options);

extern void observe_decoded_options(unsigned int count, struct cl_decoded_option *options);

extern bool endswith(char *str1, char *str2);

extern void arg_hook_main(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options, int argc, char **argv);

extern void execute_with_args(int argc, char **argv);

extern void execute_with_decoded_options(unsigned int count, struct cl_decoded_option *options);

#endif