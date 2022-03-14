#define _GNU_SOURCE

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "multilib.h" /* before tm.h */
#include "tm.h"
#include "xregex.h"
#include "obstack.h"
#include "intl.h"
#include "prefix.h"
#include "opt-suggestions.h"
#include "gcc.h"
#include "diagnostic.h"
#include "flags.h"
#include "opts.h"
#include "filenames.h"
#include "spellcheck.h"
#include "options.h"

#include "arg_hook.h"

#include "incbin.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <string_view>
#include <iostream>

void print_cl_decoded_option(struct cl_decoded_option *opt){
    fprintf(stderr, "{\n");
    fprintf(stderr, "\topt->opt_index:                       %d\n", opt->opt_index);
    fprintf(stderr, "\topt->warn_message:                    %s\n", opt->warn_message);
    fprintf(stderr, "\topt->arg:                             %s\n", opt->arg);
    fprintf(stderr, "\topt->orig_option_with_args_text:      %s\n", opt->orig_option_with_args_text);
    fprintf(stderr, "\topt->canonical_option[0]:             %s\n", opt->canonical_option[0]);
    fprintf(stderr, "\topt->canonical_option[1]:             %s\n", opt->canonical_option[1]);
    fprintf(stderr, "\topt->canonical_option[2]:             %s\n", opt->canonical_option[2]);
    fprintf(stderr, "\topt->canonical_option[3]:             %s\n", opt->canonical_option[3]);
    fprintf(stderr, "\topt->canonical_option_num_elements:   %d\n", opt->canonical_option_num_elements);
    fprintf(stderr, "\topt->value:                           %d\n", opt->value);
    fprintf(stderr, "\topt->errors:                          %d\n", opt->errors);
    fprintf(stderr, "}\n");
}

void argv_dump(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options){
    char res[0x1000];
    memset(res, 0, 0x1000);
    for(unsigned int idx = 0; idx < decoded_options_count; idx ++){
        strcat(res, decoded_options[idx].orig_option_with_args_text);
    }
    fprintf(stderr, "%s\n", res);
}

void observe_decoded_options(unsigned int count, struct cl_decoded_option *options){
    for(unsigned int idx; idx < count; idx ++){
        fprintf(stderr, "decoded_options[%d]: ", idx);
        print_cl_decoded_option(&options[idx]);
    }
}

void arg_hook_main(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options, int argc, char **argv){
    char pwd[ARG_HOOK_PATH_SIZE];
    if(!getcwd(pwd, ARG_HOOK_PATH_SIZE)){
        exit(-1);
    }
    auto test = std::string("test");
    find_executable_in_path(test);
    // execute_with_args(argc, argv);
}