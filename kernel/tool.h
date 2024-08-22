#include <string>
#include <vector>
int mgets(char *buf, int size, int fd);
int writemsg(int fd, void *msg, int left_byte);
int readmsg(int fd, void *msg, int left_byte);
void xto_char(unsigned char *xmd, char *smd, int len);
int check(char *clientmd, unsigned char *servermd);
int find_nnull(std::vector<std::string> *l);
void printT(int i, int sum);