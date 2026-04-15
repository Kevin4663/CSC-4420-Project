#include <stdio.h>
#include "simfstypes.h"

/* File system operations */
void printfs(char *);
void initfs(char *);

/* Internal functions */
FILE *openfs(char *filename, char *mode);
void closefs(FILE *fp);

/* File operation helpers (return 0 on success, 1 on error).
 * `write_file` reads `count` bytes from stdin and writes them at `offset`.
 * `read_file` writes up to `count` bytes from `offset` to stdout.
 */
int create_file(char *fsname, char *fname);
int delete_file(char *fsname, char *fname);
int write_file(char *fsname, char *fname, int offset, int count);
int read_file(char *fsname, char *fname, int offset, int count);
