#ifndef ARG_HOOK_H
#define ARG_HOOK_H

#define ARG_HOOK_PATH_SIZE 255

extern void arg_hook_main(unsigned int *decoded_options_count, struct cl_decoded_option **decoded_options, int argc, char **argv);

#endif