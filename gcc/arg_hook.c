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
#include <fstream>
#include <sys/time.h>

#define CREATE_TABLE "CREATE TABLE IF NOT EXISTS t_commands(\
                        id INTEGER PRIMARY KEY AUTOINCREMENT, \
                        runtime_uuid TEXT, \
                        timestamp INTEGER, \
                        pwd TEXT, \
                        proj_root TEXT, \
                        output TEXT, \
                        cmdline TEXT, \
                        arg_idx INTEGER, \
                        json JSON);"

char output_resolved_path[PATH_MAX] = {0};
char *proj_root;
char *pwd_path;
char runtime_uuid[UUID4_LEN] = {0};
char *arg_string = NULL;
uint64_t runtime_timestamp;
bool OPT_o_found = false;
bool arg_hook_failed = false;

int arg_hook_sqlite_busy_handler(void *data, int n_retries){
    int sleep = (rand() % 10) * 1000;
    usleep(sleep);
    return 1;
}


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

int insert_decoded_option(sqlite3 *db, struct cl_decoded_option *option, unsigned int _option_idx, char **sqlite_fail_msg){
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
                '%s', \n\
                '%s', \n\
                '%s',\n\
                '%s',\n\
                %d,\n\
                '%s');",
            runtime_uuid,
            runtime_timestamp,
            pwd_path,
            proj_root,
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
    setenv("GCC_RUNTIME_UUID", runtime_uuid, 1);
#ifdef ARG_HOOK_DEBUG
    std::cerr << insert_cmd << std::endl;
#endif
    int rc = sqlite3_exec(db, insert_cmd, NULL, NULL, sqlite_fail_msg);
    free(insert_cmd);
    return rc;
}

void insert_decoded_options(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options, int argc, char **argv){
    sqlite3 *db;
    char *sqlite_fail_msg;

#ifdef ARG_HOOK_DEBUG
    std::cerr << "UUID: " << runtime_uuid << std::endl;
#endif

    // get pwd_path
    pwd_path = getcwd(NULL, 0);
    // get proj_root
    proj_root = getenv("PROJ_ROOT");

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
    uint64_t arg_string_length = 0x400;
    arg_string = (char *)xmalloc(arg_string_length);
    memset(arg_string, 0, arg_string_length);
    for(int _arg_idx; _arg_idx < argc; _arg_idx++){
        if(strlen(arg_string) + strlen(argv[_arg_idx]) + 0x100 >= arg_string_length){
            arg_string_length = (strlen(arg_string) + strlen(argv[_arg_idx]) + 0x100) * 2;
            arg_string = (char *)xrealloc(arg_string, arg_string_length);
        }
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
        goto SQLITE_OP_FAIL;
    }

    sqlite3_busy_handler(db, arg_hook_sqlite_busy_handler, NULL);

    // Create table
    rc = sqlite3_exec(db, CREATE_TABLE, NULL, NULL, &sqlite_fail_msg);
    if(rc != SQLITE_OK) {
        goto SQLITE_OP_FAIL;
    }

    // begin transaction
    rc = sqlite3_exec(db, "begin;", NULL, NULL, &sqlite_fail_msg);
    if(rc != SQLITE_OK) {
        goto SQLITE_OP_FAIL;
    }

    // insert each row
    for(unsigned int _option_idx = 0; _option_idx < decoded_options_count; _option_idx ++){
        rc = insert_decoded_option(db, &decoded_options[_option_idx], _option_idx, &sqlite_fail_msg);
        if (rc != SQLITE_OK) {
            sqlite3_exec(db, "rollback;", NULL, NULL, NULL);
            goto SQLITE_OP_FAIL;
        }
    }
    sqlite3_exec(db, "commit;", NULL, NULL, NULL);
    sqlite3_close(db);
    return;

SQLITE_OP_FAIL:
    // std::fstream logFile;
    // char logfile_path[256] = {0};
    // sprintf(logfile_path, "/tmp/arg_hook_log/%s.log", runtime_uuid);
    // logFile.open(logfile_path, std::ios::out | std::ios::app);
    // logFile << sqlite_fail_msg << std::endl;
    // logFile.close();
    arg_hook_failed = true;
    return;
}

bool starts_with(const char *str, const char *prefix)
{
    if (!str || !prefix)
        return 0;
   if(strncmp(str, prefix, strlen(prefix)) == 0)
        return 1;
   return 0;
}

void hijack_debug_options(unsigned int *decoded_options_count_ref, struct cl_decoded_option **decoded_options_ref){
    // change debug options to -gdwarf-4
    unsigned int target_vector_count = 0;
    struct cl_decoded_option *target_vector = XNEWVEC(struct cl_decoded_option, *decoded_options_count_ref + 1);
    struct cl_decoded_option *decoded_options = *decoded_options_ref;

    // filter other debug options
    for (unsigned int _option_index=0; _option_index < *decoded_options_count_ref; _option_index ++){
        if (starts_with(rev_opt_code[decoded_options[_option_index].opt_index], "OPT_g")){
            continue;
        }
        memcpy(&target_vector[target_vector_count], &decoded_options[_option_index], sizeof(struct cl_decoded_option));
        target_vector_count += 1;
    }

    // add -gdwarf-4
    target_vector[target_vector_count].opt_index = OPT_gdwarf_;
    target_vector[target_vector_count].warn_message = NULL;
    target_vector[target_vector_count].arg = "4";
    target_vector[target_vector_count].orig_option_with_args_text = "-gdwarf-4";
    target_vector[target_vector_count].canonical_option_num_elements = 1;
    target_vector[target_vector_count].canonical_option[0] = "-gdwarf-4";
    target_vector[target_vector_count].canonical_option[1] = NULL;
    target_vector[target_vector_count].canonical_option[2] = NULL;
    target_vector[target_vector_count].canonical_option[3] = NULL;
    target_vector[target_vector_count].value = 4;
    target_vector[target_vector_count].errors = 0;
    target_vector_count += 1;

    // replace
    free(*decoded_options_ref);
    *decoded_options_count_ref = target_vector_count;
    *decoded_options_ref = target_vector;
}

void hijack_optimization(unsigned int *decoded_options_count_ref, struct cl_decoded_option **decoded_options_ref)
{
    // get optimization level from environment
    char *target_opt_level = getenv("GCC_PARSER_HIJACK_OPTIMIZATION");
    if ((!strcmp(target_opt_level, "fast")) ||
        (!strcmp(target_opt_level, "s")) ||
        (!strcmp(target_opt_level, "g")) ||
        (!strcmp(target_opt_level, "0")) ||
        (!strcmp(target_opt_level, "1")) ||
        (!strcmp(target_opt_level, "2")) ||
        (!strcmp(target_opt_level, "3")))
    {
        // change optimization depending on environment
        unsigned int target_vector_count = 0;
        struct cl_decoded_option *target_vector = XNEWVEC(struct cl_decoded_option, *decoded_options_count_ref + 1);
        struct cl_decoded_option *decoded_options = *decoded_options_ref;

        // filter other optimization options
        for (unsigned int _option_index = 0; _option_index < *decoded_options_count_ref; _option_index++)
        {
            if (starts_with(rev_opt_code[decoded_options[_option_index].opt_index], "OPT_O"))
            {
                continue;
            }
            memcpy(&target_vector[target_vector_count], &decoded_options[_option_index], sizeof(struct cl_decoded_option));
            target_vector_count += 1;
        }
        if (!strcmp(target_opt_level, "fast")){
            target_vector[target_vector_count].opt_index = OPT_Ofast;
            target_vector[target_vector_count].arg = NULL;
        }
        else if(!strcmp(target_opt_level, "s")){
            target_vector[target_vector_count].opt_index = OPT_Os;
            target_vector[target_vector_count].arg = NULL;
        }
        else if(!strcmp(target_opt_level, "g")){
            target_vector[target_vector_count].opt_index = OPT_Og;
            target_vector[target_vector_count].arg = NULL;
        }
        else{
            target_vector[target_vector_count].opt_index = OPT_O;
            target_vector[target_vector_count].arg = target_opt_level;
        }
        // target_vector[target_vector_count].opt_index
        target_vector[target_vector_count].warn_message = NULL;
        // target_vector[target_vector_count].arg
        char *tmp_arg_str = (char *)xmalloc(strlen("-O") + strlen(target_opt_level) + 1);
        sprintf(tmp_arg_str, "-O%s", target_opt_level);
        target_vector[target_vector_count].orig_option_with_args_text = tmp_arg_str;
        target_vector[target_vector_count].canonical_option_num_elements = 1;
        target_vector[target_vector_count].canonical_option[0] = tmp_arg_str;
        target_vector[target_vector_count].canonical_option[1] = NULL;
        target_vector[target_vector_count].canonical_option[2] = NULL;
        target_vector[target_vector_count].canonical_option[3] = NULL;
        target_vector[target_vector_count].value = 1;
        target_vector[target_vector_count].errors = 4;

        target_vector_count += 1;
        // replace
        *decoded_options_count_ref = target_vector_count;
        *decoded_options_ref = target_vector;
    }
    else{
        // do nothing
        return;
    }
}

void archive_gcc(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options){
    char **input_files = XNEWVEC(char *, decoded_options_count);
    unsigned int input_files_count = 0;
    char **output_files = XNEWVEC(char *, decoded_options_count);
    unsigned int output_files_count = 0;
    
    char *gcc_archive_path = getenv("GCC_ARCHIVE");
    char *command = (char *)xmalloc(65535);
    snprintf(command, 65535, "mkdir -p %s/%s", gcc_archive_path, runtime_uuid);
    system(command);
    snprintf(command, 65535, "mkdir -p %s/%s/input", gcc_archive_path, runtime_uuid);
    system(command);
    snprintf(command, 65535, "mkdir -p %s/%s/output", gcc_archive_path, runtime_uuid);
    system(command);
    free(command);

    // save input files and output files
    for (unsigned int _option_index = 0; _option_index < decoded_options_count; _option_index ++){
        if (decoded_options[_option_index].opt_index == OPT_SPECIAL_input_file){
            char *command = (char *)xmalloc(65535);
            snprintf(command, 65535, "cp %s %s/%s/input", decoded_options[_option_index].arg, gcc_archive_path, runtime_uuid);
            system(command);
            free(command);
        }
        if (decoded_options[_option_index].opt_index == OPT_o){
            char *command = (char *)xmalloc(65535);
            snprintf(command, 65535, "cp %s %s/%s/output", decoded_options[_option_index].canonical_option[1], gcc_archive_path, runtime_uuid);
            system(command);
            free(command);
        }
    }
}

void arg_hook_main(unsigned int *decoded_options_count_ref, struct cl_decoded_option **decoded_options_ref, int argc, char **argv){
    // Generate UUID
    uuid4_init();
    uuid4_generate(runtime_uuid);

    // srand initialization
    srand(time(NULL));

    // debug
    if(getenv("GCC_PARSER_DEBUG")){
        observe_decoded_options(*decoded_options_count_ref, *decoded_options_ref);
    }

    // hijack debug options
    if(getenv("GCC_PARSER_HIJACK_DWARF4")){
        hijack_debug_options(decoded_options_count_ref, decoded_options_ref);
    }

    // hijack optimization
    if(getenv("GCC_PARSER_HIJACK_OPTIMIZATION")){
        hijack_optimization(decoded_options_count_ref, decoded_options_ref);
    }

    // create database
    if (getenv("COMPILE_COMMANDS_DB")){
        insert_decoded_options(*decoded_options_count_ref, *decoded_options_ref, argc, argv);
        return;
    }
}

void arg_hook_end(unsigned int decoded_options_count, struct cl_decoded_option *decoded_options){
    if(getenv("GCC_ARCHIVE")){
        archive_gcc(decoded_options_count, decoded_options);
    }
}
