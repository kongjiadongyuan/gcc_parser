#ifndef ARG_HOOK_H
#define ARG_HOOK_H

#define ARG_HOOK_PATH_SIZE 255

extern void observe_decoded_options(unsigned int count, struct cl_decoded_option *options);

extern void arg_hook_main(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options);

#endif