#include "FileControl.h"

void* choice(void *arg);
void download(Arg args, messege msg);
int loadmd(Arg args, char *mdfile, int left_size, char (*md)[70], int num);
int recvfile(Arg args, char *outfile, int left_size);
void redownload(Arg args, messege msg);
void combinefile(char (*md)[70], char *filename, int num);
void display_file(char *ip);

