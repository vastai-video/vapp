#ifndef  TEST_GET_OPT_H
#define  TEST_GET_OPT_H
#ifdef _WIN32
extern char* optarg;
extern int optind, opterr, optopt;

int getopt(int argc, char* const argv[],
    const char* optstring);
int getopt_long(int argc, char* const argv[],
    const char* optstring,
    const struct option* longopts, int* longindex);

#define no_argument 0
#define required_argument 1
#define optional_argument 2

struct option {
    const char* name;
    int has_arg;
    int* flag;
    int val;
};
#endif
#endif