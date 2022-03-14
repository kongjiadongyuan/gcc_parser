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
#include <iostream>

#define HOOK_GCC_PATH "/home/kjdy/gcc/install/bin/gcc"
#define HOOK_GPP_PATH "/home/kjdy/gcc/install/bin/g++"

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

bool endswith(char *str1, char *str2){
    if(!str1 || !str2){
        return false;
    }
    size_t l1 = strlen(str1);
    size_t l2 = strlen(str2);
    if(l2 > l1){
        return false;
    }
    else{
        if(strncmp(str1 + l1 - l2, str2, l2)){
            return false;
        }
        else{
            return true;
        }
    }
}

bool startswith(char *s, char *pattern){
    if (!s || !pattern){
        return false;
    }
    size_t l1 = strlen(s);
    size_t l2 = strlen(pattern);
    if(l2 > l1){
        return false;
    }
    else{
        if(strncmp(s, pattern, l2)){
            return false;
        }
        else{
            return true;
        }
    }
}

char *find_executable_path(char *exe){
    char *path = getenv("PATH");
    while(true){
        char *eptr = strchr(path, ':');
        int length;
        if(!eptr){
            length = strlen(path);
        } else {
            length = eptr - path;
        }
        char *file = (char *)xmalloc(length + strlen(exe) + 0x10);
        memset(file, 0, length + strlen(exe) + 0x10);
        strncpy(file, path, length);
        strcat(file, exe);
        // char *canonical = get_canonical_path(file);
        free(file);
    }
    free(path);
}

void inline_execute_with_args(int argc, char **argv)
{
    INCBIN(GccCompiler, HOOK_GCC_PATH);
    INCBIN(GppCompiler, HOOK_GPP_PATH);
    unsigned char* compiler_ptr = NULL;
    unsigned int compiler_length = 0;
    char *name = NULL;
    if(endswith(argv[0], "gcc")){
        argv[0] = HOOK_GCC_PATH;
        compiler_ptr = (unsigned char *)gGccCompilerData;
        name = (char *)"gcc";
        compiler_length = gGccCompilerSize;
    }
    else if(endswith(argv[0], "g++")){
        argv[0] = HOOK_GPP_PATH;
        compiler_ptr = (unsigned char *)gGppCompilerData;
        name = (char *)"g++";
        compiler_length = gGppCompilerSize;
    }
    else{
        xexit(0);
    }
    int exec_fd = memfd_create(name, MFD_CLOEXEC);
    if(exec_fd < 0){
        perror("memfd");
        xexit(0);
    }
    write(exec_fd, compiler_ptr, compiler_length);
    char exec_path[256];
    memset(exec_path, 0, sizeof(exec_path));
    sprintf(exec_path, "/proc/%d/fd/%d", getpid(), exec_fd);
    execvp(exec_path, argv);
}

void execute_with_args(int argc, char **argv){
       
}

void execute_with_decoded_options(unsigned int count, struct cl_decoded_option *options){
    return;
}

void arg_hook_main(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options, int argc, char **argv){
    char pwd[ARG_HOOK_PATH_SIZE];
    if(!getcwd(pwd, ARG_HOOK_PATH_SIZE)){
        exit(-1);
    }
    execute_with_args(argc, argv);
    // std::cout << "test" << std::endl;
}