#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>

#include <string.h>

#include <libgen.h>

#include <errno.h>

#define RLOG_FNAME "/var/log/racd/rlog.log"
#define RLOG_FIELDS_NUM 6
typedef struct {
    long num;
    char *file;
    char delimiter[2];
}rlog_hdr;

void usage();
int rlog_clear(char *fname);
int get_fields(char *line, char *delimiter, char **fields);
void printall(char *line, char *delimiter, int num);
int rlog_print(rlog_hdr *hdr);

void usage()
{
    printf("Usage: rlog [-c][-h]\n" "  rlog read and clear command\n" 
    "  -c | --clear   Clear rlog\n" "  -h | --help    Help usage\n");
}

int rlog_clear(char *fname)
{
    FILE *file;
    file = fopen(fname, "w");
    if (file == NULL)
        return -errno;
    fclose(file);
    return 0;
}

int get_fields(char *line, char *delimiter, char **fields)
{
    int i = 0;
    char *begin;
    char *end;

    strtok(line, delimiter);

    while (i < RLOG_FIELDS_NUM) {
        begin = strtok(NULL, delimiter);
        if (begin == NULL) {
            return -EINVAL;
        }
        while (isspace(*begin)) begin++;

        end = begin + strlen(begin) - 1;
        while (((end > begin) && isspace(*end)) || (*end == '\n' )) {
            *end = '\0';
            end--;
        }
        fields[i] = begin;
        i++;
    }
    return 0;
}

void printall(char *line, char *delimiter, int num)
{
    char *fields[RLOG_FIELDS_NUM] = { 0, };
    char *prog = NULL;

    if (!get_fields(line, delimiter, fields)) {

        prog = strtok(fields[2], " ");
        prog = basename(prog);

        printf("%-4d%-29.29s%-10.10s%-21.21s%-6.6s%s\n",
               num, fields[0], fields[1], prog, fields[3], fields[5]);
        printf("%-50.50s\n", fields[4]);
        printf("................................................................................................\n");
    } else {
        printf("%-4d%5s%s", num, "", "Failed to get rlog!\n");
        printf("................................................................................................\n");
    }

}

int rlog_print(rlog_hdr *hdr)
{
    int num = 1;
    
    char *info = NULL;
    FILE *file;
    size_t length = 0;

    file = fopen(hdr->file, "r");
    if (file == NULL)
        return -errno;
    //dump title
    printf("%-4s%-29s%-10s%-21s%-6s%s\n", "NUM", "REASON/DESC", "CAUSE", "PROGRAM", "PID", "TIME");
    printf("................................................................................................\n");

    while (getline(&info, &length, file) != -1) {
        if (!strcmp(info, "\n") || info[0] == '\0')
            continue;
        printall( info , hdr->delimiter, num );
        num++;
    }
    fclose(file);
    return 0;
}


static struct option options[] = {
    {"help", no_argument, NULL, 'h'},
    {"clear", no_argument, NULL, 'c'},
    {NULL, 0, NULL, 0}
};

int main(int argc, char *argv[])
{
    rlog_hdr hdr;
    int ret;
    int opt;

    hdr.file = RLOG_FNAME;
    hdr.num = 0;
    strcpy(hdr.delimiter, "@");

    while ((opt = getopt_long(argc, argv, "hc", options, NULL)) 
            != -1) {
        switch (opt) {
            case 'h' :
                usage();
                return 0;
            case 'c' :
                if ((ret = rlog_clear(hdr.file))) {
                        printf("rlog: failed to clear rlog %d.\n", ret);
                        return 1;
                }
                return 0;
            default :
                return 0;
        }
    }

    if ((ret = rlog_print(&hdr))) {
        printf("rlog: failed to read rlog %d.\n", ret);
        return 1;
    }

    return 0;
}
