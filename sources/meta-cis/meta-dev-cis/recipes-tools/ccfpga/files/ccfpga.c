#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#include <sys/mman.h>

#define UIO_ROOT "/sys/class/uio"
#define FPGA_TAP_PATH "/tmp/fpgaTap.txt"


#define UIO_HANDLE_ void *
#define UIO_OPEN_FAILED ((UIO_HANDLE_)-1)

#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif


typedef unsigned int U32;


struct uio_handle {
    char *dev;
    int fd;
    void *maddr;
    size_t msize;
    char *mname;
};

const char *get_dev_name(char (*dev)[], const char *name)
{
    DIR *dir;
    FILE *fdd;
    struct dirent *dir_e;
    char name_path[255];
    char name_buf[64];
    struct uio_handle *hdl = NULL;
    
    dir = opendir(UIO_ROOT);
    if(dir == NULL) {
        printf("failed to open directory %s.\n", UIO_ROOT);
        return NULL;
    }

    dir_e = readdir(dir);
    if(dir_e == NULL) {
        printf("failed to read directory %s.\n", UIO_ROOT);
        closedir(dir);
        return NULL;
    }

	while ((dir_e = readdir(dir))) {
		memset(name_path, 0, sizeof(name_path));
		memset(name_buf, 0, sizeof(name_buf));
    
        if (dir_e) {
            if (strstr(dir_e->d_name, ".")) {
                continue;
            }
            sprintf(name_path, UIO_ROOT "/%s/name", dir_e->d_name);

            fdd = fopen(name_path, "r");
            if (fdd == NULL) {
                closedir(dir);
                printf("fopen failed with error\n", errno);
                return NULL;
            }
            
            if(fscanf(fdd, "%s\n", name_buf) == EOF) {
                printf("fscanf failed with error: %d\n", errno);
            }
            
            if (!strcmp(name, name_buf)) {
                sprintf(*dev, "%s", dir_e->d_name);
                //printf("%s\n", dir_e->d_name);
                //printf("%s\n", name_buf);
                closedir(dir);
                fclose(fdd);
                return *dev;
            }
            
            fclose(fdd);
        }
    }
    
    closedir(dir);
    
    return NULL;
}

const char *get_map_property(char (*prop_out)[], const char *prop, const char *dev)
{
    FILE *fp;
    char path[255] = {0};

    snprintf(path, sizeof(path), UIO_ROOT "/%s/maps/map0/%s", dev, prop);
    fp = fopen(path, "r");
    if (fp == NULL) {
        return NULL;
    }
    
    fgets(*prop_out, 32, fp);
    fclose(fp);

    return *prop_out;
}

struct uio_handle *uio_open(const char *name)
{
    char dev[16] = {0};
    char dev_path[32] = {0};
    char map_name[32] = {0};
    int fd = 0;
    struct uio_handle *hdl = NULL;

    if (get_dev_name(&dev, name) == NULL)  {
        printf("uio_open: failed to open \"%s\" \n", name);
        goto open_error;
    }

    sprintf(dev_path, "/dev/%s", dev);

    fd = open(dev_path, O_RDWR | O_SYNC);
    if (fd == -1)  {
        goto open_error;
    }

    hdl = malloc(sizeof(struct uio_handle));
    if (hdl == NULL)  {
        printf("uio_open: out of memory");
        goto open_error;
    }

    hdl->fd = fd;
    hdl->dev = strdup(dev);

    if(get_map_property(&map_name, "name", dev) == NULL) {
        printf("uio_open: failed to get map name");
        goto open_error;
    }

    hdl->maddr = NULL;
    hdl->msize = 0;
    hdl->mname = strdup(map_name);

    return hdl;

open_error:

    if(hdl != NULL) {
        if (hdl->dev) {
            free(hdl->dev);
        }
        if (hdl->mname) {
            free(hdl->mname);
        }
        free(hdl);
    }

    if(fd != -1) {
        close(fd);
    }
    
    return NULL;
}

void *uio_mmap(struct uio_handle *hdl)
{
    uint32_t base;
    uint32_t size;
    int page_size;
    int page_offset;
    size_t map_size;
    intptr_t map_addr;
    char tmp[32];

    if (!hdl || hdl == UIO_OPEN_FAILED) {
        errno = EINVAL;
        return NULL;
    }

    if (get_map_property(&tmp, "addr", hdl->dev) == NULL) {
        errno = EINVAL;
        printf("get_map_property addr Error!\n");
        return NULL;
    }

    base = strtoul(tmp, NULL, 16);
    //printf("base = 0x%x\n", base);

    if (get_map_property(&tmp, "size", hdl->dev) == NULL) {
        errno = EINVAL;
        printf("get_map_property addr size!\n");
        return NULL;
    }

    size = strtoul(tmp, NULL, 16);
    //printf("size = 0x%x\n", size);    

    page_size = sysconf(_SC_PAGESIZE);
    page_offset = base % page_size;

    //printf("page_size = 0x%x\n", page_size);
    //printf("page_offset = 0x%x\n", page_offset);

    if((size + page_offset) > (uint32_t)(UINT32_MAX - page_size)) {
        printf("Error! size = 0x%x, page_offset = 0x%x, page_size = 0x%x\n", size, page_offset, page_size);
        return NULL;
    }

    if((size ) > (uint32_t)(UINT32_MAX - page_offset)) {
        printf("Error! size = 0x%x, page_offset = 0x%x\n", size, page_offset);
        return NULL;
    }
    /* Calculate actual size spanning over the whole required page range */
    map_size = (page_offset + size + page_size - 1) & ~(page_size - 1);
    map_addr = base - page_offset;

    //printf("map_size = 0x%x\n", map_size);
    //printf("map_addr = 0x%x\n", map_addr);

    map_addr = (uintptr_t)mmap((void *)map_addr, map_size, PROT_READ |
                               PROT_WRITE, MAP_SHARED, hdl->fd, 0);
    if(map_addr > (intptr_t)(UINT32_MAX - page_offset)) {
        munmap((void *)map_addr, map_size);
        printf("Error! map_addr = 0x%x, page_offset = 0x%x\n", map_addr, page_offset);
        return NULL;
    }

    if (NULL == (void *)map_addr) {      
        printf("Failed mmap\n");
        return NULL;
    }

    hdl->maddr = (void *)map_addr;
    hdl->msize = map_size;
    return (void *)(map_addr + page_offset);
}


void uio_close(struct uio_handle *hdl)
{
    if (hdl && hdl != UIO_OPEN_FAILED) {
        size_t map_size = hdl->msize;
        uintptr_t map_addr = (uintptr_t)hdl->maddr;

        munmap((void *)map_addr, map_size);
        hdl->maddr = NULL;
        hdl->msize = 0;

        if (hdl->dev) {
            free(hdl->dev);
        }
        if (hdl->mname) {
            free(hdl->mname);
        }
        
        close(hdl->fd);
        free(hdl);
    }
}

void fpgaDump(uint32_t startReg, uint32_t endReg, volatile U32 *baseAddressU32)
{
    uint32_t value = 0;
    uint32_t reg;
    
    for (reg = startReg; reg <= endReg; reg++) {
        value = baseAddressU32[reg];
        printf("reg[0x%x] = 0x%x\n", reg, value);
    }
}

int fpgaTap(U32 tapReg, int tapCnt, volatile U32 *baseAddressU32)
{
    uint32_t value = 0;
    int i;
    FILE* fp;

    fp = fopen(FPGA_TAP_PATH, "w+");
    if (fp == NULL) {
       printf("Open %s error!\n", FPGA_TAP_PATH);
       return -1;
    }
    
    for (i = 0; i < tapCnt; i++) {
        value = baseAddressU32[tapReg];
        if (fprintf(fp, "0x%x\n", value) < 0) {
            printf("Write Cnt %d Value Error!\n", i, value);
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);

    return 0;
}

#define starts_with(x, y) (!strncmp(x, y, strlen(x)))

#define EXE_NAME 0
#define UIO_NAME 1
#define OPTION 2
#define ADDR 3
#define VALUE 4


int main(int argc, char **argv)
{
    void *uio_mem_handle;
    void *uio_mem_base;
    volatile U32* baseAddressU32;
    U32 regAddr;
    U32 value = 0;
    int ret;
    U32 startReg, endReg, tapReg;
    int tapCnt;

    if (argc < 4) {
        printf("Usage: %s [name] [r/w/dump/tap] [addr] {value/endaddr/conut}\n", argv[EXE_NAME]);
        return 0;
    }
    
    uio_mem_handle = (void *) uio_open(argv[UIO_NAME]);
    if (uio_mem_handle == (UIO_HANDLE_) - 1) { 
        printf("uio_open failed\n");
        return -1;
    }

    uio_mem_base = uio_mmap(uio_mem_handle);
    if (uio_mem_base == NULL) {
        printf("uio_mmap failed\n");
        uio_close(uio_mem_handle);
        return -1;
    }
    
    baseAddressU32 = (U32*)uio_mem_base;

    if (starts_with("r", argv[OPTION])) {
        regAddr = strtoul(argv[ADDR], NULL, 16);
                      
        value = baseAddressU32[regAddr];
        printf("reg[0x%x] = 0x%x\n", regAddr, value);
        
    } else if (starts_with("w", argv[OPTION])) {
        regAddr = strtoul(argv[ADDR], NULL, 16);
        value = strtoul(argv[VALUE], NULL, 16);
                      
        baseAddressU32[regAddr] = value;
        printf("write reg[0x%x] = 0x%x OK\n", regAddr, value);
        
    } else if (starts_with("dump", argv[OPTION])) {
        startReg = strtoul(argv[ADDR], NULL, 16);
        endReg = strtoul(argv[VALUE], NULL, 16);
        
        fpgaDump(startReg, endReg, baseAddressU32);
    } else if (starts_with("tap", argv[OPTION])) {
        tapReg = strtoul(argv[ADDR], NULL, 16);
        tapCnt = strtoul(argv[VALUE], NULL, 10);

        fpgaTap(tapReg, tapCnt, baseAddressU32);
    } else {
        printf("Usage: %s [name] [r/w/dump/tap] [addr] {value/endaddr/conut}\n", argv[EXE_NAME]);
    }

    uio_close(uio_mem_handle);

    return 0;
}
