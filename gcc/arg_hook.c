#define _GNU_SOURCE

#define ARG_HOOK_DEBUG

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
#include "sqlite3.h"
#include "rev_options.h"
// char *rev_opt_code[] = {...}
#include "incbin.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <unistd.h>
#include <limits.h>
#include <string_view>
#include <iostream>
#include <uuid/uuid.h>
#include <sys/time.h>

#define CREATE_TABLE "CREATE TABLE IF NOT EXISTS t_commands(\
                        id INTEGER PRIMARY KEY AUTOINCREMENT, \
                        runtime_uuid TEXT, \
                        timestamp INTEGER, \
                        output TEXT, \
                        cmdline TEXT, \
                        arg_idx INTEGER, \
                        opt_idx INTEGER, \
                        opt_name TEXT, \
                        warn_message TEXT, \
                        arg TEXT, \
                        orig_option_with_args_text TEXT, \
                        canonical_option_0 TEXT, \
                        canonical_option_1 TEXT, \
                        canonical_option_2 TEXT, \
                        canonical_option_3 TEXT, \
                        canonical_option_num_elements INTEGER, \
                        value TEXT, \
                        errors TEXT)"


char output_resolved_path[PATH_MAX] = {0};
char runtime_uuid[UUID_STR_LEN] = {0};
char arg_string[0x1000] = {0};
uint64_t runtime_timestamp;
bool OPT_o_found = false;


void print_cl_decoded_option(struct cl_decoded_option *opt){
    fprintf(stderr, "{\n");
    fprintf(stderr, "\topt->opt_index:                       %ld\n", opt->opt_index);
    fprintf(stderr, "\topt->opt_index MACRO:                 %s\n", rev_opt_code[opt->opt_index]);
    fprintf(stderr, "\topt->warn_message:                    %s\n", opt->warn_message);
    fprintf(stderr, "\topt->arg:                             %s\n", opt->arg);
    fprintf(stderr, "\topt->orig_option_with_args_text:      %s\n", opt->orig_option_with_args_text);
    fprintf(stderr, "\topt->canonical_option[0]:             %s\n", opt->canonical_option[0]);
    fprintf(stderr, "\topt->canonical_option[1]:             %s\n", opt->canonical_option[1]);
    fprintf(stderr, "\topt->canonical_option[2]:             %s\n", opt->canonical_option[2]);
    fprintf(stderr, "\topt->canonical_option[3]:             %s\n", opt->canonical_option[3]);
    fprintf(stderr, "\topt->canonical_option_num_elements:   %ld\n", opt->canonical_option_num_elements);
    fprintf(stderr, "\topt->value:                           %ld\n", opt->value);
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
    for(unsigned int idx = 0; idx < count; idx ++){
        fprintf(stderr, "decoded_options[%d]: ", idx);
        print_cl_decoded_option(&options[idx]);
    }
}

void insert_decoded_option(sqlite3 *db, struct cl_decoded_option *option){
    ;
}

void insert_decoded_options(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options, int argc, char **argv){
    sqlite3 *db;


    // Generate UUID
    uuid_t bin_uuid;
    uuid_generate_random(bin_uuid);
    uuid_unparse(bin_uuid, runtime_uuid);

    
#ifdef ARG_HOOK_DEBUG
    std::cerr << "UUID: " << runtime_uuid << std::endl;
#endif

    // Find realpath of the output file.
    for(unsigned int _option_idx = 0; _option_idx < decoded_options_count; _option_idx ++){
        if(decoded_options[_option_idx].opt_index == OPT_o){
            realpath(decoded_options[_option_idx].canonical_option[1], output_resolved_path);
            OPT_o_found = true;
            break;
        }
    }
    if(OPT_o_found){
#ifdef ARG_HOOK_DEBUG
        std::cerr << "output_resolved_path: " << output_resolved_path << std::endl;
#endif
    }

    // Get timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    runtime_timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#ifdef ARG_HOOK_DEBUG
    std::cerr << "runtime_timestamp: " << runtime_timestamp << std::endl;
#endif

    // Restore arg_string
    char arg_string[0x1000] = {0};
    for(int _arg_idx; _arg_idx < argc; _arg_idx++){
        strcat(arg_string, argv[_arg_idx]);
        if(_arg_idx < argc - 1)
            strcat(arg_string, " ");
    }
#ifdef ARG_HOOK_DEBUG
    std::cerr << "arg_string: " << arg_string << std::endl;
#endif

    // Insert into database

    // Get Database Path and Open
    char *dbpath = getenv("COMPILE_COMMANDS_DB");
    if(!dbpath){
#ifdef ARG_HOOK_DEBUG
        std::cerr << "COMPILE_COMMANDS_DB env not set" << std::endl;
#endif
        return;
    }
    int rc = sqlite3_open(dbpath, &db);
    if (rc != SQLITE_OK) {
#ifdef ARG_HOOK_DEBUG
        std::cerr << "database open failed" << std::endl;
#endif
        return;
    }

    

    for(unsigned int _option_idx = 0; _option_idx < decoded_options_count; _option_idx ++){
        insert_decoded_option(db, &decoded_options[_option_idx]);
    }
    sqlite3_finalize(db);
}

void arg_hook_main(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options, int argc, char **argv){
    // observe_decoded_options(decoded_options_count, decoded_options);
    insert_decoded_options(decoded_options_count, decoded_options, argc, argv);
}