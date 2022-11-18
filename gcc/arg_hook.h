#ifndef ARG_HOOK_H
#define ARG_HOOK_H

#define ARG_HOOK_PATH_SIZE 255
#define CMDLINE_MAX_SIZE 65535
#define EXE_MAX_SIZE 65535

struct arg_hook_proc_info_t {
    pid_t pid;
    char cmdline[CMDLINE_MAX_SIZE];
    char exe[EXE_MAX_SIZE];
};

extern void arg_hook_main(unsigned int *decoded_options_count, struct cl_decoded_option **decoded_options, int argc, char **argv);
extern void arg_hook_end(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options);

#endif