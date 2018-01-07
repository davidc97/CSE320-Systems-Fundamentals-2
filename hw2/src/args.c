#include "debug.h"
#include "utf.h"
#include "wrappers.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* STR_UTF8  = "UTF8";
const char* STR_UTF16LE = "UTF16LE";
const char* STR_UTF16BE  = "UTF16BE";

//int opterr;
//int optopt;
//int optind;
//char *optarg;

state_t *program_state;

void
parse_args(int argc, char *argv[])
{
  int i;
  char option;
  //char *joined_argv;

  // joined_argv = join_string_array(argc, argv);
  // info("argc: %d argv: %s", argc, joined_argv);
  // free(joined_argv);
  program_state = Malloc(sizeof(state_t));
  program_state->in_file = NULL;
  program_state->out_file = NULL;
  for (i = 0; optind < argc; ++i) {
    debug("%d opterr: %d", i, opterr);
    debug("%d optind: %d", i, optind);
    debug("%d optopt: %d", i, optopt);
    debug("%d argv[optind]: %s", i, argv[optind]);
    while ((option = getopt(argc, argv, "+e:i:h")) != -1) {
      switch (option) {
        case 'e': {
          info("Encoding Argument: %s", optarg);
          program_state->encoding_to = determine_format(optarg);
            // USAGE(argv[0]);
           // print_state();
            break;
            // exit(0);
            //goto errorcase;
        }
        case 'h': {
          free(program_state);
          USAGE(argv[0]);
          exit(EXIT_SUCCESS);
        }
        case '?': {
            fprintf(stderr, KRED "-%c is not a supported argument\n" KNRM,
                    optopt);
            free(program_state);
            exit(EXIT_FAILURE);
        // case 'errorcase':
        //   USAGE(argv[0]);
        //   exit(0);
        }
        default: {
          //free(program_state);
        }
      }
    }
    //elsif
    if(argv[optind] != NULL)
    {
      if (program_state->in_file == NULL || (strcmp(program_state->in_file, "bin/utf") == 0)) {
        program_state->in_file = argv[optind];
      }
      elsif(program_state->out_file == NULL)
      {
        program_state->out_file = argv[optind];
      }
      optind++;
    }
  }
  //free(joined_argv);
}

int get_inode(int file){
  int ret;
  struct stat file_stat;
  ret = fstat (file, &file_stat);
  if(ret < 0){
    exit(EXIT_FAILURE);
  }
  return file_stat.st_ino;
}

int compare_inode(int inode1, int inode2){
  if(inode1 != inode2){
    return -1;
  }
  return 1;
}

format_t
determine_format(char *argument)
{
  if (strcmp(argument, STR_UTF16LE) == 0)
    return UTF16LE;
  if (strcmp(argument, STR_UTF16BE) == 0)
    return UTF16BE;
  if (strcmp(argument, STR_UTF8) == 0)
    return UTF8;
  return 0;
}

char*
bom_to_string(format_t bom){
  switch(bom){
    case UTF8: return (char*)STR_UTF8;
    case UTF16BE: return (char*)STR_UTF16BE;
    case UTF16LE: return (char*)STR_UTF16LE;
  }
  return "UNKNOWN";
}

char*
join_string_array(int count, char *array[])
{
  char* ret;
  int len = 0;
  int str_len = 0;
  int cur_str_len = 0;

 // str_len = array_size(count, array);
  for(int i = 0; i < count; i++){
    str_len = str_len + strlen(array[i]);
  }
 // str_len++;

  ret = malloc(str_len);

  for (int i = 0; i < count; i++) {
    cur_str_len = strlen(array[i]);
    memecpy(ret + len, array[i], cur_str_len);
    len += cur_str_len;
    memecpy(ret + len, " ", 1);
    len += 1;
  }
  return ret;
}

int
array_size(int count, char *array[])
{
  int i, sum = 1; /* NULL terminator */
  for (i = 0; i < count; ++i) {
    sum += strlen(array[i]);
    ++sum; /* For the spaces */
  }
  return sum+1;
}

void
print_state()
{
//errorcase:
  if (program_state == NULL) {
    error("program_state is %p", (void*)program_state);
    exit(EXIT_FAILURE);
  }
  info("program_state {\n"
         "  format_t encoding_to = 0x%X;\n"
         "  format_t encoding_from = 0x%X;\n"
         "  char *in_file = '%s';\n"
         "  char *out_file = '%s';\n"
         "};\n",
         program_state->encoding_to, program_state->encoding_from,
         program_state->in_file, program_state->out_file);
}
