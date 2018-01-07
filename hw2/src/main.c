#include "debug.h"
#include "utf.h"
#include "wrappers.h"
#include <stdlib.h>

// const char* STR_UTF8  = "UTF8";
// const char* STR_UTF16LE = "UTF16LE";
// const char* STR_UTF16BE  = "UTF16BE";

int
main(int argc, char *argv[])
{
  int infile, outfile, in_flags, out_flags;
  parse_args(argc, argv);
  check_bom();
  print_state();
  in_flags = O_RDONLY;
  out_flags = O_WRONLY | O_CREAT;
  infile = Open(program_state->in_file, in_flags);
  outfile = Open(program_state->out_file, out_flags);
  if(compare_inode(get_inode(infile), get_inode(outfile)) == 1){
    printf("Error: infile and outfile are the same \n");
    free(program_state);
    exit(EXIT_FAILURE);
  }
  lseek(SEEK_SET, program_state->bom_length, infile); /* Discard BOM */
  get_encoding_function()(infile, outfile);
  if(program_state != NULL) {
    //close((int)program_state);
    close(infile);
    close(outfile);
    free(program_state);
  }
  //free((void*) (infile));
  return EXIT_SUCCESS;
}
