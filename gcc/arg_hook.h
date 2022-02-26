#ifndef KJDY_H
#define KJDY_H
#include "opts.h"
#include "options.h"
#include <stdio.h>
#include <unistd.h>

#define ARG_HOOK_PATH_SIZE 255

void print_cl_decoded_option(struct cl_decoded_option *opt){
    printf("{\n");
    printf("\topt->opt_index:                       %d\n", opt->opt_index);
    printf("\topt->warn_message:                    %s\n", opt->warn_message);
    printf("\topt->arg:                             %s\n", opt->arg);
    printf("\topt->orig_option_with_args_text:      %s\n", opt->arg);
    printf("\topt->canonical_option[0]:             %s\n", opt->canonical_option[0]);
    printf("\topt->canonical_option[1]:             %s\n", opt->canonical_option[1]);
    printf("\topt->canonical_option[2]:             %s\n", opt->canonical_option[2]);
    printf("\topt->canonical_option[3]:             %s\n", opt->canonical_option[3]);
    printf("\topt->canonical_option_num_elements:   %d\n", opt->canonical_option_num_elements);
    printf("\topt->value:                           %ll\n", opt->value);
    printf("\topt->errors:                          %d\n", opt->errors);
    printf("}\n");
}



void arg_hook_main(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options){
    char pwd[ARG_HOOK_PATH_SIZE];
    if(!getcwd(pwd, ARG_HOOK_PATH_SIZE)){
        exit(-1);
    }
    printf("directory: getcwd");
}

#endif