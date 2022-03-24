#define _GNU_SOURCE

// #define ARG_HOOK_DEBUG

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
#include "cJSON.h"
#include "sqlite3.h"
#include "uuid4.h"
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
#include <sys/time.h>

#define CREATE_TABLE "CREATE TABLE IF NOT EXISTS t_commands(\
                        id INTEGER PRIMARY KEY AUTOINCREMENT, \
                        runtime_uuid TEXT, \
                        timestamp INTEGER, \
                        output TEXT, \
                        cmdline TEXT, \
                        arg_idx INTEGER, \
                        decoded_argv JSON);"


char output_resolved_path[PATH_MAX] = {0};
char runtime_uuid[UUID4_LEN] = {0};
char arg_string[0x1000] = {0};
uint64_t runtime_timestamp;
bool OPT_o_found = false;
bool arg_hook_failed = false;


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

char *decoded_option_serialize(struct cl_decoded_option *option){
    cJSON *json = NULL;
    json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "opt_index", option->opt_index);
    cJSON_AddStringToObject(json, "opt_name", rev_opt_code[option->opt_index]);
    cJSON_AddStringToObject(json, "warn_message", option->warn_message);
    cJSON_AddStringToObject(json, "arg", option->arg);
    cJSON_AddStringToObject(json, "orig_option_with_args_text", option->orig_option_with_args_text);
    cJSON *canonical_option = NULL;
    canonical_option = cJSON_CreateArray();
    cJSON_AddItemToArray(canonical_option, cJSON_CreateString(option->canonical_option[0]));
    cJSON_AddItemToArray(canonical_option, cJSON_CreateString(option->canonical_option[1]));
    cJSON_AddItemToArray(canonical_option, cJSON_CreateString(option->canonical_option[2]));
    cJSON_AddItemToArray(canonical_option, cJSON_CreateString(option->canonical_option[3]));
    cJSON_AddItemToObject(json, "canonical_option", canonical_option);
    cJSON_AddNumberToObject(json, "canonical_option_num_elements", option->canonical_option_num_elements);
    cJSON_AddNumberToObject(json, "value", option->value);
    cJSON_AddNumberToObject(json, "errors", option->errors);
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    return json_str;
}

int insert_decoded_option(sqlite3 *db, struct cl_decoded_option *option, unsigned int _option_idx){
    char *insert_cmd;
    int size = 0x200;
    insert_cmd = (char *)xmalloc(size);
    memset(insert_cmd, 0, size);
    char *serialized_option = decoded_option_serialize(option);
#ifdef ARG_HOOK_DEBUG
    std::cerr << serialized_option << std::endl;
#endif
    while(true){
        int writed = snprintf(
            insert_cmd,
            size,
            "INSERT INTO t_commands VALUES(\n\
                NULL,\n\
                '%s',\n\
                %ld,\n\
                '%s',\n\
                '%s',\n\
                %d,\n\
                '%s');",
            runtime_uuid,
            runtime_timestamp,
            output_resolved_path,
            arg_string,
            _option_idx,
            serialized_option
        );
        if (writed >= size - 1){
            size = size * 2;
            insert_cmd = (char *)xrealloc(insert_cmd, size);
            memset(insert_cmd, 0, size);
        }
        else{
            free(serialized_option);
            break;
        }
    }
#ifdef ARG_HOOK_DEBUG
    std::cerr << insert_cmd << std::endl;
#endif
    int rc = sqlite3_exec(db, insert_cmd, NULL, NULL, NULL);
    free(insert_cmd);
    return rc;
}

void insert_decoded_options(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options, int argc, char **argv){
    sqlite3 *db;


    // Generate UUID
    uuid4_init();
    uuid4_generate(runtime_uuid);

    
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
    else
    {
#ifdef ARG_HOOK_DEBUG
        std::cerr << "OPT_o not found" << std::endl;
#endif
        arg_hook_failed = true;
        return;
    }

    // Get timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    runtime_timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#ifdef ARG_HOOK_DEBUG
    std::cerr << "runtime_timestamp: " << runtime_timestamp << std::endl;
#endif

    // Restore arg_string
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
        arg_hook_failed = true;
        return;
    }
    int rc = sqlite3_open(dbpath, &db);
    if (rc != SQLITE_OK) {
#ifdef ARG_HOOK_DEBUG
        std::cerr << "database open failed" << std::endl;
#endif
        arg_hook_failed = true;
        return;
    }

    // Create table
    rc = sqlite3_exec(db, CREATE_TABLE, NULL, NULL, NULL);
    if(rc != SQLITE_OK) {
#ifdef ARG_HOOK_DEBUG
        std::cerr << "create table failed" << std::endl;
#endif
        arg_hook_failed = true;
        return;
    }

    // begin transaction
    rc = sqlite3_exec(db, "begin;", NULL, NULL, NULL);
    if(rc != SQLITE_OK) {
#ifdef ARG_HOOK_DEBUG
        std::cerr << "begin failed" << std::endl;
#endif
        arg_hook_failed = true;
        return;
    }

    // insert each row
    for(unsigned int _option_idx = 0; _option_idx < decoded_options_count; _option_idx ++){
        rc = insert_decoded_option(db, &decoded_options[_option_idx], _option_idx);
        if (rc != SQLITE_OK) {
#ifdef ARG_HOOK_DEBUG
            std::cerr << "insert failed" << std::endl;
#endif
            sqlite3_exec(db, "rollback;", NULL, NULL, NULL);
            arg_hook_failed = true;
            return;
        }
    }
    sqlite3_exec(db, "commit;", NULL, NULL, NULL);
    sqlite3_close(db);
}

void arg_hook_main(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options, int argc, char **argv){
    // if COMPILE_COMMANDS_DB not set, return.
    if (!getenv("COMPILE_COMMANDS_DB")){
        return;
    }
    // Main logic start.
    insert_decoded_options(decoded_options_count, decoded_options, argc, argv);
}