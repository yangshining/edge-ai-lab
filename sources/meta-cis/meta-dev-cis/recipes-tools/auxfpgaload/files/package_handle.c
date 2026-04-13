#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <mtd/mtd-user.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include "package_handle.h"
#include "aux_fpga_load.h"

#if GPIO_ENABLE
#include <signal.h>
#include <gpiod.h>
#endif


#define cpu_to_le32(x)      (x)
#define le32_to_cpu(x)      (x)
#define tole(x) cpu_to_le32(x)
#define DO_CRC(x) crc = tab[(crc ^ (x)) & 255] ^ (crc >> 8)

#define EXE_NAME 0
#define OPTION 1
#define SRC_NAME 2
#define DST_NAME 3
#define BLOCK_NR_OR_DEV_NAME 4
#define OVERLAP_SZ_OR_GPIO 5
#define FLASHCP_SIZE    (10 * 1024)
#define CST 8

#define DEFAULT_SRC_NAME "/lib/firmware/aux-fpga.bin"
#define DEFAULT_DST_NAME "/tmp/aux_fpga.bin"
#define DEFAULT_DEV_NAME "/dev/mtd0"
#define DEFAULT_GPIODEV_NAME "/dev/gpiochip1"
#define SYS_MTD "/sys/class/mtd"
#define MAX_TRY_TIME 40

#ifdef PROD_APPLE
#define DEFAULT_GPIO_NR 11
#define DEFAULT_GPIO_SRST_NR 25
#define DEFAULT_GPIO_POR_RST_NR 25
#define DEFAULT_GPIO_STAT_NR 12
#endif

#ifdef PROD_HU
#define DEFAULT_GPIO_NR 80
#define DEFAULT_GPIO_SRST_NR 78
#define DEFAULT_GPIO_POR_RST_NR 83
#define DEFAULT_GPIO_STAT_NR 79
#endif

#ifndef DEFAULT_GPIO_NR
#define DEFAULT_GPIO_NR 80
#endif

#ifndef DEFAULT_GPIO_SRST_NR
#define DEFAULT_GPIO_SRST_NR 78
#endif

#ifndef DEFAULT_GPIO_POR_RST_NR
#define DEFAULT_GPIO_POR_RST_NR 83
#endif

#ifndef DEFAULT_GPIO_STAT_NR
#define DEFAULT_GPIO_STAT_NR 79
#endif

#define DEFAULT_BLOCK_NR 2
#define DEFAULT_OVERLAP_SZ 1024
#define FISRT_STR(x, y) (!strncmp(x, y, strlen(x)))

typedef struct package_header PACK_HEADER;
typedef struct block_info BLK_INFO;

struct package_pad default_pad =
{
    MAGIC_NUM_PAD,                           // 魔数
    NORMAL_FLAG,                             // 标志
    {
        "2025-01-15-15",                     // 日期
        "DAS_HU_AUX_FPGA_PACKAGE",           // 名称
        "0.0.0"                              // 版本号
    },
#if CRC_ENABLE
    0,                                       // crc 0
    0,                                       // file size 0
#endif
    DEFAULE_PAD_SIZE,                        // 32K bytes
    {0}                                      // 保留字段初始化为0
};

#if CRC_ENABLE
static const uint32_t crc32_table[256] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
};

static uint32_t calc_crc32(uint32_t val, const void *ss, size_t len)
{
    const unsigned char *s = ss;

    while (len-- != 0)
        val = crc32_table[(val ^ *s++) & 0xff] ^ (val >> 8);
    return val;
}
#endif //end of CRC_ENABLE

static int mtd_get_num(struct mtd_info *info)
{
    DIR *sysfs_mtd;
    struct dirent *dirent;

    memset(info, 0, sizeof(struct mtd_info));
    info->sysfs_supported = 1;

    /*
     * We have to scan the MTD sysfs directory to identify how many MTD
     * devices are present.
     */
    sysfs_mtd = opendir(SYS_MTD);
    if (!sysfs_mtd) {
        ERROR("cannot open \"%s\"", SYS_MTD);
        return -1;
    }

    info->lowest_mtd_num = INT_MAX;
    while (1) {
        int mtd_num, ret;
        char tmp_buf[256];

        errno = 0;
        dirent = readdir(sysfs_mtd);
        if (!dirent)
            break;

        if (strlen(dirent->d_name) >= 255) {
            ERROR("invalid entry in %s: \"%s\"",
                   SYS_MTD, dirent->d_name);
            errno = EINVAL;
            goto out_close;
        }

        ret = sscanf(dirent->d_name, MTD_NAME_PATT"%s",
                 &mtd_num, tmp_buf);
        if (ret == 1) {
            info->mtd_dev_cnt += 1;
            if (mtd_num > info->highest_mtd_num)
                info->highest_mtd_num = mtd_num;
            if (mtd_num < info->lowest_mtd_num)
                info->lowest_mtd_num = mtd_num;
        }
    }

    if (!dirent && errno) {
        ERROR("readdir failed on \"%s\"", SYS_MTD);
        goto out_close;
    }

    if (closedir(sysfs_mtd)) {
        ERROR("closedir failed on \"%s\"", SYS_MTD);
        return -1;
    }

    if (info->lowest_mtd_num == INT_MAX)
        info->lowest_mtd_num = 0;

    return 0;

out_close:
    closedir(sysfs_mtd);
    return -1;
}

static int find_target_mtd_region(struct mtd_region* region)
{
    struct mtd_info info;
    struct mtd_info_user *info_user = NULL;
    int *inval_info_user = NULL;
    int mtd_index = 0, inval_mtd_index = 0;
    unsigned char device[32];
    int dev_fd = -1;
    int i, region_index;
    int start_mtd = 0, end_mtd = 0, start_mtd_tmp = 0, end_mtd_tmp = 0;
    size_t mtds_size = 0, mtds_size_tmp = 0;

    if (!region) {
        ERROR("invalid region ptr!");
        return -1;
    }

    memset(region, 0, sizeof(struct mtd_region));

    if (mtd_get_num(&info) != 0) {
        ERROR("mtd_get_num failed, error: %d!", errno);
    }

    info_user = malloc(sizeof(struct mtd_info_user) * info.mtd_dev_cnt);
    if (!info_user) {
        ERROR("apply for info_user space failed!");
        goto error_out0;
    }
    memset(info_user, 0, sizeof(struct mtd_info_user) * info.mtd_dev_cnt);

    inval_info_user = malloc(sizeof(int) * info.mtd_dev_cnt);
    if (!inval_info_user) {
        ERROR("apply for inval_info_user space failed!");
        goto error_out0;
    }
    memset(inval_info_user, 0, sizeof(int) * info.mtd_dev_cnt);

    for (i = info.lowest_mtd_num; i <= info.highest_mtd_num; i++) {
        mtd_index = i - info.lowest_mtd_num;
        snprintf(device, sizeof(device), "/dev/mtd%d", i);
        /* get some info about the flash device */
        dev_fd = open (device, O_RDONLY);
        if (dev_fd < 0 || ioctl (dev_fd, MEMGETINFO, info_user + mtd_index) < 0) {
            ERROR("This doesn't seem to be a valid MTD flash device, fd:%d!", dev_fd);
            goto error_out0;
        }
        close(dev_fd);
        dev_fd = -1;
        if (!(info_user[mtd_index].flags & MTD_WRITEABLE)) {
            inval_info_user[inval_mtd_index++] = i;
        }
    }

    if (inval_mtd_index == 0) {
        region->index_start = info.lowest_mtd_num;
        region->index_end = info.highest_mtd_num;
        for (i = region->index_start; i <= region->index_end; i++) {
            region->size += info_user[i - info.lowest_mtd_num].size;
        }
        goto normal_out;
    }

    if (inval_info_user[0] != info.lowest_mtd_num) {
        start_mtd = info.lowest_mtd_num;
        end_mtd = inval_info_user[0] - 1;
        for (i = start_mtd; i <= end_mtd; i++) {
            mtds_size += info_user[i - info.lowest_mtd_num].size;
        }
    }

    for (region_index = 0; region_index < inval_mtd_index; region_index++) {
        start_mtd_tmp = inval_info_user[region_index] + 1;
        if (region_index == inval_mtd_index - 1) {
                end_mtd_tmp = info.highest_mtd_num;
        } else {
            end_mtd_tmp = inval_info_user[region_index + 1] - 1;
        }

        if (end_mtd_tmp < start_mtd_tmp) {
            continue;
        }

        mtds_size_tmp = 0;
        for (i = start_mtd_tmp; i <= end_mtd_tmp; i++) {
            mtds_size_tmp += info_user[i - info.lowest_mtd_num].size;
        }

        if (mtds_size_tmp > mtds_size) {
            start_mtd = start_mtd_tmp;
            end_mtd = end_mtd_tmp;
            mtds_size = mtds_size_tmp;
        }
    }

    if (!mtds_size) {
        ERROR("No valid mtd region!");
        goto error_out0;
    }

    region->index_start = start_mtd;
    region->index_end = end_mtd;
    region->size = mtds_size;

normal_out:
    VERBOSE("Find mtd region from mtd%d to mtd%d, total size %ld bytes.", region->index_start, region->index_end, region->size);
    if(info_user) free(info_user);
    if(inval_info_user) free(inval_info_user);
    return 0;

error_out0:
    ERROR("%s: Find mtd region failed!", __func__);
    if(info_user) free(info_user);
    if(inval_info_user) free(inval_info_user);
    if(dev_fd > 0) close(dev_fd);
    return -1;
}

static void print_pad(struct package_pad *pad)
{
    if (!pad) {
        ERROR("Invalid pad ptr.");
        return;
    }

    VERBOSE("Pad magic num: 0x%x", pad->magic_num);
    NOTICE_NO_PREFIX("Pad flag: %d (0-normal, 1-update, 2-force)", pad->flag);
    NOTICE_NO_PREFIX("Pad date: %s", pad->version.date);
    NOTICE_NO_PREFIX("Pad name: %s", pad->version.name);
    NOTICE_NO_PREFIX("Pad version: %s", pad->version.version_num);
    VERBOSE("Pad size: %ld bytes", pad->pad_size);
    return;
}

static int generate_pad_header(struct package_pad *pad)
{
    int fil_fd = -1;
    uint8_t *buf = 0;
    time_t now;
    struct tm *tm_info;

    int ret;

    if (!pad) {
        ERROR("Invalid pad ptr.");
        return -1;
    }
    memset(pad, 0, sizeof(struct package_pad));
    fil_fd = open (VERSION_FILE, O_RDONLY);
    if (fil_fd < 0) {
        ERROR("While trying to get the file status of %s.", VERSION_FILE);
        goto error_out0;
    }

    buf = malloc(VERSION_SIZE);
    if (!buf) {
        ERROR("%s: Failed when apply for buf space!", __func__);
        goto error_out0;
    }

    if (read(fil_fd, buf, VERSION_SIZE) < 0) {
        ERROR("Read %s failed!", VERSION_FILE);
        goto error_out0;
    }

    ret = sscanf(buf, VERSION_PATT, pad->version.date, pad->version.name, pad->version.version_num, &pad->flag);
    if (ret < 3) {
        ERROR("Extract %s data failed!", VERSION_FILE);
        goto error_out0;
    }

    now = time(NULL);
    tm_info = gmtime(&now);
    tm_info->tm_hour += CST;
    strftime(pad->version.date, sizeof(pad->version.date), "%Y-%m-%d-%H", tm_info);

    if (ret != 4) {
        pad->flag = NORMAL_FLAG;
    }

    pad->magic_num = MAGIC_NUM_PAD;
    pad->pad_size = DEFAULE_PAD_SIZE;
    INFO("Generate pad success!");
    print_pad(pad);
    return 0;

error_out0:
    if(fil_fd < 0) close(fil_fd);
    if(buf) free(buf);
    WARN("Get config version data failed, use default value!");
    memcpy(pad, &default_pad, sizeof(struct package_pad));
    print_pad(pad);
    return -2;
}

static void printProgBar(int len, int totalnum, int prognum, struct timespec start)
{
    static int bnum;
    while(prognum&&bnum--) {
        putchar('\b');
    }
    putchar('[');
    int num = len;
    int curnum=prognum*len/totalnum;
    while(num) {
        num--;
        if(curnum) {
            curnum--;
            putchar('#');
            continue;
        }
        putchar(' ');
    }
    putchar(']');
    struct timespec curtime;
    clock_gettime(CLOCK_MONOTONIC, &curtime);
    int infolen = printf(" %.2f%% %.1lfs", prognum*100.0/totalnum, (curtime.tv_sec-start.tv_sec+(curtime.tv_nsec-start.tv_nsec)/1000000000.0));
    bnum=len+2+infolen;
    if (totalnum == prognum) {
        putchar('\n');
    }
    fflush(stdout);
}

static int read_file(const char *file, unsigned char *buf, size_t size, off_t offset)
{
    int fd;

    fd = open(file, O_RDONLY);
    if (fd < 0) {
        ERROR("Open file %s failed!", file);
        return -1;
    }

    if (offset > 0) {
        if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
            ERROR("Seek file %s pos %ld failed!", file, offset);
            close(fd);
            return -1;
        }
    }

    if (read(fd, buf, size) != size) {
        ERROR("get file %s size %ld data failed!", file, size);
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

#if CRC_ENABLE
static uint32_t calc_file_crc(uint32_t val, const char *file, size_t size, off_t offset)
{
    unsigned char buf[KILO_BYTE];
    size_t buf_size, read_size;
    off_t read_offset;

    buf_size = sizeof(buf);
    read_offset = offset;
    while (size) {
        read_size = buf_size < size ? buf_size : size;
        if (read_file(file, buf, read_size, read_offset) == -1) {
            ERROR("get file %s size %ld data from %ld failed!", file, read_size, read_offset);
            return UINT32_MAX;
        }
        val = calc_crc32(val, buf, read_size);
        size -= read_size;
        read_offset += read_size;
    }

    VERBOSE("Calculate file %s CRC = 0x%x.", file, val);
    return val;
}
#endif

static int get_pad(const char *filename, struct package_pad *pad, off_t start)
{
    size_t size;

    if (!filename) {
        ERROR("Invalid filename!");
        return -1;
    }

    if (!pad) {
        ERROR("Invalid pad ptr!");
        return -1;
    }

#if 0
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        ERROR("Open file %s failed!", filename);
        return -1;
    }

    size = sizeof(struct package_pad);
    if (lseek (fd, start, SEEK_SET) < 0) {
        ERROR("Seek file %s pos %ld failed!", filename, start);
        goto error_out;
    }

    if (size != read(fd, pad, size)) {
        ERROR("get file %s size %ld data failed!", filename, size);
        goto error_out;
    }
#else
    size = sizeof(struct package_pad);
    if (read_file(filename, (unsigned char *)pad, size, start) < 0) {
        ERROR("get file %s size %ld data failed!", filename, size);
        return -1;
    }
#endif

    /*make every str end with '\0'*/
    pad->version.date[sizeof(pad->version.date) - 1] = '\0';
    pad->version.name[sizeof(pad->version.name) - 1] = '\0';
    pad->version.version_num[sizeof(pad->version.version_num) - 1] = '\0';
    return 0;
}

static int mtd_erase(const char *device)
{
    int dev_fd = -1;
    struct mtd_info_user mtd;
    struct erase_info_user erase;
    int blocks, i;

    /* get some info about the flash device */
    dev_fd = open (device, O_SYNC|O_RDWR);
    if (dev_fd < 0 || ioctl (dev_fd, MEMGETINFO, &mtd) < 0) {
        ERROR("This doesn't seem to be a valid MTD flash device FD[%d]!", dev_fd);
        goto error_out0;
    }

    blocks = mtd.size / mtd.erasesize;
    erase.length = mtd.erasesize;
    erase.start = 0;

    for (i = 1; i <= blocks; i++) {
        if (ioctl (dev_fd,MEMERASE, &erase) < 0) {
            ERROR("While erasing blocks from 0x%.8x-0x%.8x on %s.",
                    (unsigned int) erase.start,(unsigned int) (erase.start + erase.length), device);
            goto error_out0;
        }
        erase.start += mtd.erasesize;
    }

    close(dev_fd);
    return 0;
error_out0:
    if (dev_fd > 0) close(dev_fd);
    return -1;
}

static int mtd_write(const char *device, const char *filename, int flags) 
{
    int i;
    int dev_fd = -1,fil_fd = -1;
    ssize_t result;
    size_t size,written;
    struct mtd_info_user mtd;
    struct erase_info_user erase;
    struct stat filestat;
    unsigned char src[FLASHCP_SIZE], dest[FLASHCP_SIZE];

    /* get some info about the flash device */
    dev_fd = open (device, O_SYNC|O_RDWR);
    if (dev_fd < 0 || ioctl (dev_fd, MEMGETINFO, &mtd) < 0) {
        ERROR("This doesn't seem to be a valid MTD flash device FD[%d]!", dev_fd);
        goto error_out0;
    }

    /* get some info about the file we want to copy */
    fil_fd = open (filename, O_RDONLY);
    if (fil_fd < 0 || fstat (fil_fd, &filestat) < 0) {
        ERROR("While trying to get the file status of %s.", filename);
        goto error_out0;
    }

    /* does it fit into the device/partition? */
    if (filestat.st_size > mtd.size) {
        ERROR("%s won't fit into %s!", filename, device);
        goto error_out0;
    }

    /*
    * erase enough blocks so that we can write the file *
    */
    erase.start = 0;

    if (flags == 0) {
        erase.length = mtd.size;
    }
    else {
        erase.length = (filestat.st_size + mtd.erasesize - 1) / mtd.erasesize;
        erase.length *= mtd.erasesize;
    }
    /* if not, erase the whole chunk in one shot */
    int blocks = erase.length / mtd.erasesize;
    erase.length = mtd.erasesize;
    for (i = 1; i <= blocks; i++) {
        if (ioctl (dev_fd,MEMERASE,&erase) < 0) {
            ERROR("While erasing blocks from 0x%.8x-0x%.8x on %s.",
                    (unsigned int) erase.start,(unsigned int) (erase.start + erase.length),device);
            goto error_out0;
        }
        erase.start += mtd.erasesize;
    }

    /*
    * write the entire file to flash *
    */
    size = filestat.st_size;
    i = FLASHCP_SIZE;
    written = 0;
    while (size) {
        if (size < FLASHCP_SIZE) i = size;

        /* read from filename */
        if(i != read (fil_fd,src,i)) {
            ERROR("fil_fd While reading data from %s.",filename);
            goto error_out0;
        }

        /* write to device */
        result = write(dev_fd,src,i);
        if (i != result) {
            if (result < 0) {
                ERROR("While writing data to 0x%.8lx-0x%.8lx on %s.",
                        written,written + i,device);
                goto error_out0;
            }
            ERROR("Short write count returned while writing to x%.8zx-0x%.8zx on %s: %zu/%llu bytes written to flash",
                    written,written + i,device,written + result,(unsigned long long)filestat.st_size);
            goto error_out0;
        }

        written += i;
        size -= i;
    }

    /*
    * verify that flash == file data *
    */
    if(lseek (fil_fd,0L,SEEK_SET) < 0) {
        ERROR("fil_fd While seeking to start of %s.",filename);
        goto error_out0;
    }

    if(lseek (dev_fd,0L,SEEK_SET) < 0) {
        ERROR("dev_fd While seeking to start of %s.",device);
        goto error_out0;
    }

    size = filestat.st_size;
    i = FLASHCP_SIZE;
    written = 0;
    while (size) {
        if (size < FLASHCP_SIZE) i = size;

        if(i != read(fil_fd,src,i)) {
            ERROR("fil_fd While reading data from %s.",filename);
            goto error_out0;
        }

        /* read from device */
        if(i != read(dev_fd,dest,i)) {
            ERROR("dev_fd While reading data from %s.",device);
            goto error_out0;
        }

        /* compare bufers */
        if (memcmp(src,dest,i)) {
            ERROR("File does not seem to match flash data. First mismatch at 0x%.8zx-0x%.8zx",
                    written,written + i);
            goto error_out0;
        }

        written += i;
        size -= i;
    }

    if (dev_fd > 0) close(dev_fd);
    if (fil_fd > 0) close(fil_fd);
    return 0;
error_out0:
    if (dev_fd > 0) close(dev_fd);
    if (fil_fd > 0) close(fil_fd);
    return -1;
}

int get_env_gpio(unsigned long *gpio_nr, const char* env_name)
{
    unsigned long tmp_gpio;
    char *env_str;
    char *endptr;

    env_str = getenv(env_name);
    if (!env_str) {
        VERBOSE("Get env %s failed!", env_name);
        return -1;
    }
    VERBOSE("Get env %s = %s", env_name, env_str);

    errno = 0;
    tmp_gpio = strtoul(env_str, &endptr, 0);
    VERBOSE("errno=%d", errno);
    if (errno == ERANGE) {
        VERBOSE("%s number out of range!", env_name);
        return -1;
    }
    if (env_str == endptr) {
        VERBOSE("No digits were found in %s!", env_name);
        return -1;
    }

    VERBOSE("Get env gpio num = %ld", tmp_gpio);
    *gpio_nr = tmp_gpio;
    return 0;
}

int mtd_write_continuously(const struct mtd_region *aux_region, const char *filename, int flags)
{
    int i, mtd_index, start_mtd, end_mtd;
    off_t start_pos = 0;
    int dev_fd = -1,fil_fd = -1;
    ssize_t result;
    size_t size, written, write_sz, file_size_written = 0;
    struct mtd_info_user mtd;
    struct erase_info_user erase;
    struct stat filestat;
    struct timespec start;
    uint8_t write_done_flag = 0;
    unsigned char src[FLASHCP_SIZE], dest[FLASHCP_SIZE], device[32];

    start_mtd = aux_region->index_start;
    end_mtd = aux_region->index_end;
    /* get some info about the file we want to copy */
    fil_fd = open (filename, O_RDONLY);
    if (fstat(fil_fd,&filestat) < 0) {
        ERROR("While trying to get the file status of %s.",filename);
        goto error_out0;
    }

    if (filestat.st_size > aux_region->size) {
        ERROR("No enough space for aux fpga package:\nFile size: %ld bytes\nMtd valid size:%ld bytes.", filestat.st_size, aux_region->size);
        goto error_out0;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);
    printf("Writing files:");
    for (mtd_index = start_mtd; mtd_index <= end_mtd; mtd_index++)
    {
        snprintf(device, sizeof(device), "/dev/mtd%d", mtd_index);
        /* get some info about the flash device */
        dev_fd = open (device, O_SYNC | O_RDWR);
        if (ioctl (dev_fd, MEMGETINFO, &mtd) < 0) {
            ERROR("This doesn't seem to be a valid MTD flash device!");
            goto error_out0;
        }

        /* does it fit into the device/partition? */
        write_sz = ((filestat.st_size - start_pos) < mtd.size) ? (filestat.st_size - start_pos) : mtd.size;

        if (!write_done_flag && mtd_index == start_mtd) {
            VERBOSE("Skip first mtd %s, write is when others write done", device);
            start_pos += write_sz;
            continue;
        }

        VERBOSE("%s file size is %ld bytes, write %ld bytes in %s!", filename, filestat.st_size, write_sz, device);
        if(lseek (fil_fd, start_pos, SEEK_SET) < 0) {
            ERROR("fil_fd While seeking to start of %s.",filename);
            goto error_out0;
        }

        /*
        * erase enough blocks so that we can write the file *
        */
        erase.start = 0;

        if (flags == 0) {
            erase.length = mtd.size;
        }
        else {
            erase.length = (write_sz + mtd.erasesize - 1) / mtd.erasesize;
            erase.length *= mtd.erasesize;
        }
        /* if not, erase the whole chunk in one shot */
        int blocks = erase.length / mtd.erasesize;
        erase.length = mtd.erasesize;
        for (i = 1; i <= blocks; i++) {
            if (ioctl (dev_fd,MEMERASE,&erase) < 0) {
                ERROR("While erasing blocks from 0x%.8x-0x%.8x on %s.",
                        (unsigned int) erase.start,(unsigned int) (erase.start + erase.length),device);
                goto error_out0;
            }
            erase.start += mtd.erasesize;
        }

        /*
        * write the entire file to flash *
        */
        size = write_sz;
        i = FLASHCP_SIZE;
        written = 0;
        while (size) {
            if (size < FLASHCP_SIZE) i = size;

            /* read from filename */
            if(i != read (fil_fd,src,i)) {
                ERROR("fil_fd While reading data from %s.",filename);
                goto error_out0;
            }

            /* write to device */
            result = write(dev_fd,src,i);
            if (i != result) {
                if (result < 0) {
                    ERROR("While writing data to 0x%.8lx-0x%.8lx on %s.",
                            written,written + i,device);
                    goto error_out0;
                }
                ERROR("Short write count returned while writing to x%.8zx-0x%.8zx on %s: %zu/%llu bytes written to flash",
                        written,written + i,device,written + result,(unsigned long long)write_sz);
                goto error_out0;
            }

            written += i;
            size -= i;
        }

        /*
        * verify that flash == file data *
        */
        if(lseek (fil_fd, start_pos, SEEK_SET) < 0) {
            ERROR("fil_fd While seeking to start of %s.",filename);
            goto error_out0;
        }

        if(lseek (dev_fd, 0L, SEEK_SET) < 0) {
            ERROR("dev_fd While seeking to start of %s.",device);
            goto error_out0;
        }

        size = write_sz;
        i = FLASHCP_SIZE;
        written = 0;
        while (size) {
            if (size < FLASHCP_SIZE) i = size;

            if(i != read(fil_fd,src,i)) {
                ERROR("fil_fd While reading data from %s.",filename);
                goto error_out0;
            }

            /* read from device */
            if(i != read(dev_fd,dest,i)) {
                ERROR("dev_fd While reading data from %s.",device);
                goto error_out0;
            }

            /* compare bufers */
            if (memcmp(src,dest,i)) {
                ERROR("File does not seem to match flash data. First mismatch at 0x%.8zx-0x%.8zx",
                        written,written + i);
                goto error_out0;
            }

            written += i;
            size -= i;
            printProgBar(50, filestat.st_size, file_size_written + written, start);
        }
        if (dev_fd > 0) close(dev_fd);

        start_pos += write_sz;
        file_size_written += write_sz;

        if (start_pos == filestat.st_size) {
            VERBOSE("Write file other part done! Total %ld bytes, write first part!", file_size_written);
            start_pos = 0;
            write_done_flag = 1;
            mtd_index = start_mtd;
            mtd_index--;
            continue;
        }

        if (file_size_written == filestat.st_size) {
            INFO("Write file done! Total %ld bytes.", file_size_written);
            break;
        }
    }

    if (file_size_written != filestat.st_size) {
        ERROR("%s file size is %ld bytes, %ld bytes written, %ld bytes not written!", filename, filestat.st_size, start_pos, filestat.st_size - start_pos);
        goto error_out0;
    }

    if (fil_fd > 0) close(fil_fd);
    return 0;
error_out0:
    if (dev_fd > 0) close(dev_fd);
    if (fil_fd > 0) close(fil_fd);
    return -1;
}

int compare_version(const char *ver1, const char *ver2) {
    int num1, num2;
    char *v1 = strdup(ver1);
    char *v2 = strdup(ver2);
    char *savep1 = NULL;
    char *savep2 = NULL;

    char *token1 = strtok_r(v1, ".", &savep1);
    char *token2 = strtok_r(v2, ".", &savep2);

    while (token1 != NULL || token2 != NULL) {
        num1 = token1 ? atoi(token1) : 0;
        num2 = token2 ? atoi(token2) : 0;
        if (num1 > num2) {
            free(v1);
            free(v2);
            return 1;
        } else if (num1 < num2) {
            free(v1);
            free(v2);
            return -1;
        }

        token1 = strtok_r(NULL, ".", &savep1);
        token2 = strtok_r(NULL, ".", &savep2);
    }

    free(v1);
    free(v2);
    return 0;
}

/* return 1 is need, return 0 is not need, return -1 is error*/
int is_need_update(const char* dst, const char* src, const struct mtd_region *aux_region)
{
    struct package_pad dst_pad;
    struct package_pad src_pad;
#if CRC_ENABLE
    int mtd_index = 0;
    struct mtd_info_user mtd;
    unsigned char device[32];
    int dev_fd = -1, crc_flag = 0;
    size_t read_size, calc_size;
    off_t read_offset;
    uint32_t crc = 0;
#endif

    if (get_pad(dst, &dst_pad, 0)) {
        ERROR("Get pad from file %s failed!", dst);
        return UPGRADE_FAILED;
    }

    if (get_pad(src, &src_pad, 0)) {
        ERROR("Get pad from file %s failed!", src);
        return UPGRADE_FAILED;
    }

    NOTICE_NO_PREFIX("--------------- Target Aux-FPGA Info -----------------");
    print_pad(&src_pad);
    NOTICE_NO_PREFIX("--------------- Current Aux-FPGA Info ----------------");
    print_pad(&dst_pad);
    NOTICE_NO_PREFIX("---------------------- Info End ----------------------");
    if (src_pad.magic_num != MAGIC_NUM_PAD) {
        ERROR("Target file %s pad num error 0x%x!", src, src_pad.magic_num);
        return UPGRADE_FAILED;
    }

    if (dst_pad.magic_num != MAGIC_NUM_PAD) {
        INFO("Current version invalid, need update!");
        return UPGRADE_EXEC_INVALID;
    }

    if (src_pad.flag == UPDATE_FLAG && compare_version(src_pad.version.version_num, dst_pad.version.version_num) > 0)
    {
        INFO("Target version is newer, need update for update-flag!");
        return UPGRADE_EXEC_UPDATE;
    }

    if (src_pad.flag == FORCE_FLAG)
    {
        INFO("Update anyway for force-flag!");
        return UPGRADE_EXEC_FORCE;
    }

#if CRC_ENABLE
    calc_size = dst_pad.file_size;
    if (!calc_size) {
        INFO("No file size param, need update!");
        return UPGRADE_EXEC_INVALID;
    } else if (calc_size > aux_region->size) {
        INFO("File size invalid, need update!");
        VERBOSE("File size = %ld, mtd size = %ld.", calc_size, aux_region->size);
        return UPGRADE_EXEC_INVALID;
    }

    VERBOSE("Pad CRC = [0x%x].", dst_pad.crc);

    for (mtd_index = aux_region->index_start; mtd_index <= aux_region->index_end; mtd_index++) {
        snprintf(device, sizeof(device), "/dev/mtd%d", mtd_index);
        /* get some info about the flash device */
        dev_fd = open (device, O_RDONLY);
        if (dev_fd < 0 || ioctl (dev_fd, MEMGETINFO, &mtd) < 0) {
            ERROR("This doesn't seem to be a valid MTD flash device, dev: %s!", device);
            if(dev_fd > 0) close(dev_fd);
            return UPGRADE_FAILED;
        }

        close(dev_fd);

        read_offset = (mtd_index == aux_region->index_start) ? dst_pad.pad_size : 0;
        read_size = mtd.size - read_offset;
        read_size = read_size < calc_size ? read_size : calc_size;
        crc = calc_file_crc(crc, device, read_size, read_offset);

        calc_size -= read_size;
        if (!calc_size) {
            VERBOSE("Calculate CRC done! CRC[0x%x].", crc);
            crc_flag = 1;
            break;
        }
    }

    if (!crc_flag || crc != dst_pad.crc) {
        VERBOSE("Exist package CRC error, need update! Calculated CRC[0x%x], Pad CRC[0x%x], CRC flag[%d].", crc, dst_pad.crc, crc_flag);
        INFO("Check failed, need update!");
        return UPGRADE_EXEC_INVALID;
    }
#endif

    if (src_pad.flag == NORMAL_FLAG) {
        INFO("Current version exists, no need update for normal-flag!");
        return UPGRADE_SKIP_NORMAL;
    }

    if (src_pad.flag == UPDATE_FLAG) {
        INFO("Current version is not obsolete, no need update for update-flag!");
        return UPGRADE_SKIP_UPDATE;
    }

    /* should never be here if pad valid */
    ERROR("Pad flag[%d] invalid!", src_pad.flag);
    return UPGRADE_FAILED;
}

int package_recombine(const char *filename,
                      const char *target_filename,
                      int block_nr,
                      size_t overlap_sz)
{
    struct stat filestat;
    int fd = -1, dst_fd = -1;
    size_t file_size, blk_size, read_bytes, write_bytes;
    off_t rd_offset, wr_offset, pad_offset;
    int i = 0;
    char *buf = NULL;
    char *header = NULL;
    size_t head_size = 0;
    struct package_pad pad;
    char *pad_buf = NULL;
    if (!filename)
    {
        ERROR("%s: Invalid file name!", __func__);
        goto error_out;
    }

    if (block_nr < 2)
    {
        ERROR("%s: No need do recombine for block number: %d!", __func__, block_nr);
        goto error_out;
    }

    fd = open(filename, O_RDWR, 0644);
    if (-1 == fd)
    {
        ERROR("%s: Error opening file %s.", __func__, filename);
        goto error_out;
    }

    if (fstat(fd,&filestat) != 0) {
        ERROR("%s: Failed while trying to get the file status of %s!", __func__,filename);
        goto error_fd;
    }

    dst_fd = open(target_filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (-1 == dst_fd)
    {
        ERROR("%s: Error opening file %s.", __func__, target_filename);
        goto error_fd;
    }

    file_size = filestat.st_size;
    blk_size = (file_size + block_nr - 1) / block_nr;

    buf = malloc(blk_size + overlap_sz);
    if (!buf)
    {
        ERROR("%s: Failed when apply for buf space!", __func__);
        goto error_fd;
    }

    if (generate_pad_header(&pad) == -1) {
        ERROR("%s: Failed when generate_pad_header!", __func__);
        goto error_buf;
    }

    head_size = sizeof(PACK_HEADER) + sizeof(BLK_INFO) * block_nr;
    header = malloc(head_size);
    if (!header)
    {
        ERROR("%s: Failed when apply for header space!", __func__);
        goto error_buf;
    }

    ((PACK_HEADER *)header)->magic_num = MAGIC_NUM;
    ((PACK_HEADER *)header)->block_nr = block_nr;

    pad_offset = head_size + pad.pad_size;
    
    for (i = 0; i < block_nr; i++)
    {
        rd_offset = blk_size * (block_nr - 1 - i) - overlap_sz;
        if (i == block_nr -1)
        {
            rd_offset += overlap_sz;
        }
        wr_offset = (blk_size + overlap_sz) * (block_nr - 1 - i) + pad_offset;
        if (wr_offset < rd_offset)
        {
            ERROR("%s: recombine %s block %d failed, rd_offset: %ld, wr_offset: %ld!",
                __func__, filename, (block_nr - i), rd_offset, wr_offset);
            goto error_header;
        }

        if (lseek(fd, rd_offset, SEEK_SET) == -1)
        {
            ERROR("%s: Failed when seek fd %d, rd_offset[%ld]!", __func__, fd, rd_offset);
            goto error_header;
        }

        read_bytes = read(fd, buf, blk_size + overlap_sz);
        if (read_bytes != (blk_size + overlap_sz) && i != 0)
        {
            ERROR("%s: read %s block %d failed, expect size: %ld, read size: %ld!",
                __func__, filename, (block_nr - i), (blk_size + overlap_sz), read_bytes);
            goto error_header;
        }

        if (lseek(dst_fd, wr_offset, SEEK_SET) == -1)
        {
            ERROR("%s: Failed when seek fd %d, wr_offset[%ld]!", __func__, fd, wr_offset);
            goto error_header;
        }

        write_bytes = write(dst_fd, buf, read_bytes);
        if (write_bytes != read_bytes)
        {
            ERROR("%s: write %s block %d failed, expect size: %ld, write size: %ld!",
                __func__, filename, (block_nr - i), read_bytes, write_bytes);
            goto error_header;
        }
        ((PACK_HEADER *)header)->blk_info[block_nr - 1 -i].offset = wr_offset;
        ((PACK_HEADER *)header)->blk_info[block_nr - 1 -i].reloc_offset = rd_offset;
        ((PACK_HEADER *)header)->blk_info[block_nr - 1 -i].size = write_bytes;
    }

    for (i = 0; i < head_size/sizeof(uint32_t); i++)
    {
        ((uint32_t *)&header)[i] = cpu_to_le32(((uint32_t *)&header)[i]);
    }

    lseek(dst_fd, pad.pad_size, SEEK_SET);
    write_bytes = write(dst_fd, header, head_size);
    if (write_bytes != head_size)
    {
        printf ("%s: write %s header failed, expect size: %ld, write size: %ld!\n",
            __func__, target_filename, head_size, write_bytes);
        goto error_header;
    }

    pad_buf = malloc(pad.pad_size);
    if (!pad_buf)
    {
        ERROR("%s: Failed when apply for pad_buf space!", __func__);
        goto error_header;
    }

    memset(pad_buf, 0, pad.pad_size);
    lseek(dst_fd, 0, SEEK_SET);
    write_bytes = write(dst_fd, pad_buf, pad.pad_size);
    if (write_bytes != pad.pad_size)
    {
        printf ("%s: write %s header failed, expect size: %ld, write size: %ld!\n",
            __func__, target_filename, pad.pad_size, write_bytes);
        goto error_pad_buf;
    }

    lseek(dst_fd, 0, SEEK_SET);
    write_bytes = write(dst_fd, &pad, sizeof(pad));
    if (write_bytes != sizeof(pad))
    {
        printf ("%s: write %s header failed, expect size: %ld, write size: %ld!\n",
            __func__, target_filename, sizeof(pad), write_bytes);
        goto error_pad_buf;
    }
    INFO("Generate file success: %s", target_filename);
    free(pad_buf);
    free(header);
    free(buf);
    close(fd);
    return 0;

error_pad_buf:
    free(pad_buf);
error_header:
    free(header);
error_buf:
    free(buf);
error_fd:
    if (dst_fd < 0) close(dst_fd);
    close(fd);
error_out:
    return -1;
}

int package_decombine(const char *src_filename,
                      const char *dst_filename)
{
    PACK_HEADER hd;
    int src_fd = -1, dst_fd = -1;
    size_t read_bytes, write_bytes, dst_file_size;
    off_t rd_offset, wr_offset, pad_offset;
    int i = 0;
    char *buf = NULL;
    char *header = NULL;
    char *pad_buf = NULL;
    struct package_pad aux_pad;
    /* check input params */
    if (!src_filename || !dst_filename)
    {
        ERROR("%s: Invalid file name!", __func__);
        goto error_out;
    }

    if (get_pad(src_filename, &aux_pad, 0)) {
        ERROR("%s: get pad from %s failed!", __func__, src_filename);
        goto error_out;
    }
    pad_offset = aux_pad.pad_size;

    /* open files */
    src_fd = open (src_filename, O_RDONLY, 0644);
    if (-1 == src_fd)
    {
        ERROR("%s: Error opening file %s.", __func__, src_filename);
        goto error_fd;
    }

    dst_fd = open (dst_filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (-1 == dst_fd)
    {
        ERROR("%s: Error opening file %s.", __func__, dst_filename);
        goto error_fd;
    }

    /* move position to start */
    if (lseek(src_fd, pad_offset, SEEK_SET) == -1)
    {
        ERROR("%s: Failed when seek fd %d!", __func__, src_fd);
        goto error_fd;
    }

    if (read(src_fd, &hd, sizeof(PACK_HEADER)) != sizeof(PACK_HEADER))
    {
        ERROR("%s: read fd failed %d!", __func__, src_fd);
        goto error_fd;
    }

    for (i = 0; i < sizeof(PACK_HEADER)/sizeof(uint32_t); i++)
    {
        ((uint32_t *)&hd)[i] = le32_to_cpu(((uint32_t *)&hd)[i]);
    }

    INFO("magic =0x%x, block_nr = %d!", hd.magic_num, hd.block_nr);
    if (MAGIC_NUM != hd.magic_num)
    {
        ERROR("%s: Invalid magic number, please use right package!", __func__);
        goto error_fd; 
    }

    header = malloc(sizeof(PACK_HEADER) + sizeof(BLK_INFO) * hd.block_nr);
    if (!header)
    {
        ERROR("%s: Failed when apply for header space!", __func__);
        goto error_fd;
    }

    for (i = 0; i < (sizeof(PACK_HEADER) + sizeof(BLK_INFO) * hd.block_nr)/sizeof(uint32_t); i++)
    {
        ((uint32_t *)&header)[i] = le32_to_cpu(((uint32_t *)&header)[i]);
    }

    if (lseek(src_fd, pad_offset, SEEK_SET) == -1)
    {
        ERROR("%s: Failed when seek fd %d!", __func__, src_fd);
        goto error_fd;
    }

    if (read(src_fd, header, (sizeof(PACK_HEADER) + sizeof(BLK_INFO) * hd.block_nr)) != (sizeof(PACK_HEADER) + sizeof(BLK_INFO) * hd.block_nr))
    {
        ERROR("%s: read fd failed %d!", __func__, src_fd);
        goto error_fd;
    }

    buf = malloc(((PACK_HEADER *)header)->blk_info[0].size);
    if (!buf)
    {
        ERROR("%s: Failed when apply for buf space!", __func__);
        goto error_alloc;
    }
    
    for (i = 0; i < ((PACK_HEADER *)header)->block_nr; i++)
    {
        if (lseek(src_fd, ((PACK_HEADER *)header)->blk_info[i].offset, SEEK_SET) == -1)
        {
            ERROR("%s: Failed when seek fd %d!", __func__, src_fd);
            goto error_alloc;
        }

        if (lseek(dst_fd, ((PACK_HEADER *)header)->blk_info[i].reloc_offset + pad_offset, SEEK_SET) == -1)
        {
            ERROR("%s: Failed when seek fd %d!", __func__, src_fd);
            goto error_alloc;
        }

        read_bytes = read(src_fd, buf, ((PACK_HEADER *)header)->blk_info[i].size);
        if (read_bytes != ((PACK_HEADER *)header)->blk_info[i].size)
        {
            ERROR("%s: read %s block %d failed, expect size: %d, read size: %ld!",
                __func__, src_filename, i, ((PACK_HEADER *)header)->blk_info[i].size, read_bytes);
            goto error_alloc;
        }

        write_bytes = write(dst_fd, buf, read_bytes);
        if (write_bytes != read_bytes)
        {
            ERROR("%s: write %s header failed, expect size: %ld, write size: %ld!",
                __func__, dst_filename, read_bytes, write_bytes);
            goto error_alloc;
        }
    }

#if CRC_ENABLE
    dst_file_size = lseek(dst_fd, 0, SEEK_END);
    if (dst_file_size == -1) {
        ERROR("%s: Failed when seek dst_fd %d!", __func__, src_fd);
        goto error_alloc;
    }
    aux_pad.file_size = dst_file_size - aux_pad.pad_size;
    aux_pad.crc = calc_file_crc(0, dst_filename, aux_pad.file_size, aux_pad.pad_size);
#endif

    pad_buf = malloc(aux_pad.pad_size);
    if (!pad_buf)
    {
        ERROR("%s: Failed when apply for pad_buf space!", __func__);
        goto error_alloc;
    }

    memset(pad_buf, 0, aux_pad.pad_size);
    if (lseek(dst_fd, 0, SEEK_SET) == -1) {
        ERROR("%s: Failed when seek dst_fd %d!", __func__, src_fd);
        goto error_alloc;
    }

    write_bytes = write(dst_fd, pad_buf, aux_pad.pad_size);
    if (write_bytes != aux_pad.pad_size)
    {
        ERROR("%s: write %s header failed, expect size: %ld, write size: %ld!",
            __func__, dst_filename, aux_pad.pad_size, write_bytes);
        goto error_alloc;
    }

    if (lseek(dst_fd, 0, SEEK_SET) == -1) {
        ERROR("%s: Failed when seek dst_fd %d!", __func__, src_fd);
        goto error_alloc;
    }
    write_bytes = write(dst_fd, &aux_pad, sizeof(aux_pad));
    if (write_bytes != sizeof(aux_pad))
    {
        ERROR("%s: write %s header failed, expect size: %ld, write size: %ld!",
            __func__, dst_filename, sizeof(aux_pad), write_bytes);
        goto error_alloc;
    }

    free(pad_buf);
    free(header);
    free(buf);
    close(src_fd);
    close(dst_fd);
    return 0;

error_alloc:
    if (pad_buf) free(pad_buf);
    if (header) free(header);
    if (buf) free(buf);
error_fd:
    if (src_fd != -1) close(src_fd);
    if (dst_fd != -1) close(dst_fd);
error_out:
    return -1;
}

#if GPIO_ENABLE
/* POR Reset binding with flash reset, make sure release POR reset */
int release_por_reset()
{
    char *gpio_name = DEFAULT_GPIODEV_NAME;
    unsigned long reset_gpio = DEFAULT_GPIO_POR_RST_NR;

    if (get_env_gpio(&reset_gpio, "AUX_POR_RST_GPIO") == -1) {
        WARN("Get AUX_POR_RST_GPIO num failed, use default value.");
    }

    if (gpiod_ctxless_set_value(gpio_name, reset_gpio, 1, 0, "AUX_RST_GPIO", NULL, NULL) != 0) {
        ERROR("Gpio write port[%ld] value[%d] error!", reset_gpio, 1);
        return AUX_FPGA_CHECK_FAILED;
    }

    return 0;
}

/* return 1 is valid, return 0 is not valid, return -1 is error*/
int is_firmware_valid()
{
    char *gpio_name = DEFAULT_GPIODEV_NAME;
    unsigned long reset_gpio = DEFAULT_GPIO_POR_RST_NR;
    unsigned long stat_gpio = DEFAULT_GPIO_STAT_NR;
    int try_times = 0;
    int status;

    if (get_env_gpio(&reset_gpio, "AUX_POR_RST_GPIO") == -1) {
        WARN("Get AUX_POR_RST_GPIO num failed, use default value.");
    }

    if (get_env_gpio(&stat_gpio, "AUX_STAT_GPIO") == -1) {
        WARN("Get AUX_STAT_GPIO num failed, use default value.");
    }

    /* reset aux fpga by set reset gpio 0 and 1 */
    if (gpiod_ctxless_set_value(gpio_name, reset_gpio, 0, 0, "AUX_RST_GPIO", NULL, NULL) != 0) {
        ERROR("Gpio write port[%ld] value[%d] error!", reset_gpio, 0);
        return AUX_FPGA_CHECK_FAILED;
    }

    //delay 100ms
    usleep(100000);

    if (gpiod_ctxless_set_value(gpio_name, reset_gpio, 1, 0, "AUX_RST_GPIO", NULL, NULL) != 0) {
        ERROR("Gpio write port[%ld] value[%d] error!", reset_gpio, 1);
        return AUX_FPGA_CHECK_FAILED;
    }

    /* check stat gpio */
    while (try_times++ < MAX_TRY_TIME) {
        usleep(500000);
        status = gpiod_ctxless_get_value(gpio_name, stat_gpio, 0, "AUX_STAT_GPIO");
        if (status == -1) {
            ERROR("Gpio get port[%ld] value[%d] error!", stat_gpio, status);
            return AUX_FPGA_CHECK_FAILED;
        }

        if (status == 1) {
            VERBOSE("Aux fpga valid!");
            return AUX_FPGA_VALID;
        }
    }

    WARN("Aux fpga not valid!");
    return AUX_FPGA_INVALID;
}

int update_aux_fpga()
{
    char *src_file_name = DEFAULT_SRC_NAME;
    char *dst_file_name = DEFAULT_DST_NAME;
    char *gpio_name = DEFAULT_GPIODEV_NAME;
    unsigned long gpio_nr = DEFAULT_GPIO_NR;
    int ret, upd_ret;
    char pad_dev[32];
    struct mtd_region aux_region;
    char *env_str;

    if (package_decombine(src_file_name, dst_file_name)) {
        ERROR("%s: Decombine failed!", __func__);
        return UPGRADE_FAILED;
    }

    if (get_env_gpio(&gpio_nr, "AUX_FLASH_GPIO") == -1) {
        WARN("Get GPIO num failed, use default value.");
    }
    VERBOSE("AUX flash gpio = %ld", gpio_nr);

    release_por_reset();
    void (*handler)(int) = signal(SIGINT, SIG_IGN);
    ret = gpiod_ctxless_set_value(gpio_name, gpio_nr, 1, 0, "FLASH_SWITCH", NULL, NULL);
    if (ret != 0) {
        ERROR("Gpio write port[%ld] value[%d] error : ret = %d.", gpio_nr, 1, ret);
        return UPGRADE_FAILED;
    }

    if (find_target_mtd_region(&aux_region)) {
        ERROR("Find mtd region failed!");
        goto error_out;
    }

    snprintf(pad_dev, sizeof(pad_dev), "/dev/mtd%d", aux_region.index_start);
    upd_ret = is_need_update(pad_dev, dst_file_name, &aux_region);
    if (upd_ret == UPGRADE_FAILED) {
        ERROR("Error when decide update action!");
        goto error_out;
    } else if (upd_ret >= UPGRADE_SKIP_NORMAL && upd_ret <= UPGRADE_SKIP_UPDATE) {
        INFO("No need to update!");
        goto normal_out;
    }

    ret = mtd_write_continuously(&aux_region, dst_file_name, 0);
    if (ret != 0) {
        mtd_erase(pad_dev);
        ERROR("Write file [%s] error : ret = %d.", dst_file_name, ret);
        goto error_out;
    }

normal_out:
    if (remove(dst_file_name) != 0) {
        VERBOSE("Remove dst file %s failed!", dst_file_name);
    }

    if (gpiod_ctxless_set_value(gpio_name, gpio_nr, 0, 0, "FLASH_SWITCH", NULL, NULL) != 0) {
        ERROR("Gpio write port[%ld] value[%d] error : ret = %d.", gpio_nr, 0, ret);
        return UPGRADE_FAILED;
    }

    INFO("%s execute successful!", __func__);
    signal(SIGINT, handler);
    return upd_ret;

error_out:
    if (gpiod_ctxless_set_value(gpio_name, gpio_nr, 0, 0, "FLASH_SWITCH", NULL, NULL) != 0)
        ERROR("Gpio write port[%ld] value[%d] error : ret = %d.", gpio_nr, 0, ret);
    signal(SIGINT, handler);
    return UPGRADE_FAILED;
}
#endif //End of GPIO_ENABLE

int main(int argc, char **argv)
{
    char usage_str[100];
    unsigned long block_nr = DEFAULT_BLOCK_NR;
    size_t overlap_sz = DEFAULT_OVERLAP_SZ;
    char *dst_file_name = DEFAULT_DST_NAME;
    char *dev_name = DEFAULT_DEV_NAME;
    char *gpio_name = DEFAULT_GPIODEV_NAME;
    unsigned long gpio_nr = DEFAULT_GPIO_NR;
    int ret;
    char pad_dev[32];
    struct mtd_region aux_region;
    struct package_pad aux_pad;
    char *env_str;

    sprintf(usage_str, "Usage: %s [r/d] [src_file_name ] {dst_file_name} {external option}", argv[EXE_NAME]);

#if GPIO_ENABLE
    release_por_reset();

    if (get_env_gpio(&gpio_nr, "AUX_FLASH_GPIO") == -1) {
        WARN("Get GPIO num failed, use default value.");
    }
    VERBOSE("AUX flash gpio = %ld", gpio_nr);
    if (FISRT_STR("ls-aux", argv[EXE_NAME])) {
        signal(SIGINT, SIG_IGN);
        ret = gpiod_ctxless_set_value(gpio_name, gpio_nr, 1, 0, "FLASH_SWITCH", NULL, NULL);
        if (ret != 0) {
            ERROR("Gpio write port[%ld] value[%d] error : ret = %d.", gpio_nr, 1, ret);
            return -1;
        }

        if (find_target_mtd_region(&aux_region)) {
            ERROR("Find mtd region failed!");
            goto error_out;
        }

        snprintf(pad_dev, sizeof(pad_dev), "/dev/mtd%d", aux_region.index_start);

        if (get_pad(pad_dev, &aux_pad, 0)) {
            ERROR("Get pad from file %s failed!", pad_dev);
            goto error_out;;
        }
        NOTICE_NO_PREFIX("-------------- Aux-FPGA Package Info --------------");
        print_pad(&aux_pad);
        NOTICE_NO_PREFIX("--------------------- Info End --------------------");
ls_aux_fpga_out:
        if (gpiod_ctxless_set_value(gpio_name, gpio_nr, 0, 0, "FLASH_SWITCH", NULL, NULL) != 0) {
            ERROR("Gpio write port[%ld] value[%d] error : ret = %d.", gpio_nr, 0, ret);
            return -1;
        }

        return 0;
    }

    env_str = getenv("ENABLE_AUX_CMD");
    if (!env_str || strcmp(env_str, "true") != 0) {
        ERROR("Execution denied! You don't have the right!");
        return -1;
    }

    if (FISRT_STR("clr", argv[OPTION])) {
        if (access(AUX_CMD_DISABLE_FILE, F_OK) == 0) {
            if (remove(AUX_CMD_DISABLE_FILE) == 0) {
                INFO("Clear CMD blocker successful!");
                return 0;
            } else {
                ERROR("Clear CMD blocker failed!");
                return -1;
            }
        }
        INFO("Nothing to do!");
        return 0;
    }

    if (access(AUX_CMD_DISABLE_FILE, F_OK) == -1) {
        close(open(AUX_CMD_DISABLE_FILE, O_WRONLY | O_CREAT, 0644));
    } else {
        ERROR("Execution denied! You don't have the right!");
        return -1;
    }

#endif

    if (argc < 3) {
        INFO("%s", usage_str);
        return 0;
    }

    if (FISRT_STR("r", argv[OPTION])) {
        dst_file_name = "aux_fpga_package.bin";
        if (argc > 3) {
            dst_file_name = argv[DST_NAME];
        }

        if (argc > 4) {
            block_nr = strtoul(argv[BLOCK_NR_OR_DEV_NAME], NULL, 0);
        }

        if (argc > 5) {
            overlap_sz = strtoul(argv[OVERLAP_SZ_OR_GPIO], NULL, 0);
        }

        if (package_recombine(argv[SRC_NAME], dst_file_name, block_nr, overlap_sz)) {
            ERROR("%s: Recombine failed!", argv[EXE_NAME]);
            return -1;
        }
    }

    if (FISRT_STR("d", argv[OPTION])) {
        if (argc > 3) {
            dst_file_name = argv[DST_NAME];
        }

        if (argc > 4) {
            dev_name = argv[BLOCK_NR_OR_DEV_NAME];
        }

        if (argc > 5) {
            gpio_name = argv[OVERLAP_SZ_OR_GPIO];
        }

        if (argc > 6) {
            gpio_nr = strtoul(argv[6], NULL, 0);
        }
        if (FISRT_STR("dr", argv[OPTION])) {
            dst_file_name = argv[SRC_NAME];
        } else if (package_decombine(argv[SRC_NAME], dst_file_name)) {
            ERROR("%s: Decombine failed!", argv[EXE_NAME]);
            return -1;
        }
#if GPIO_ENABLE
        signal(SIGINT, SIG_IGN);
        ret = gpiod_ctxless_set_value(gpio_name, gpio_nr, 1, 0, "FLASH_SWITCH", NULL, NULL);
        if (ret != 0) {
            ERROR("Gpio write port[%ld] value[%d] error : ret = %d.", gpio_nr, 1, ret);
            return -1;
        }
#if 0
        ret = mtd_write(dev_name, dst_file_name, 0);
        if (ret != 0)
        {
            ERROR("MTD write dev_name[%s] file[%s] error : ret = %d.", dev_name, dst_file_name, ret);
            return -1;
        }
#endif
        if (find_target_mtd_region(&aux_region)) {
            ERROR("Find mtd region failed!");
            goto error_out;
        }

        snprintf(pad_dev, sizeof(pad_dev), "/dev/mtd%d", aux_region.index_start);
        ret = is_need_update(pad_dev, dst_file_name, &aux_region);
        if (ret == UPGRADE_FAILED) {
            ERROR("Error when decide update action!");
            goto error_out;
        } else if (ret >= UPGRADE_SKIP_NORMAL && ret <= UPGRADE_SKIP_UPDATE) {
            INFO("No need to update!");
            goto normal_out;
        }

        ret = mtd_write_continuously(&aux_region, dst_file_name, 0);
        if (ret != 0) {
            mtd_erase(pad_dev);
            ERROR("Write file [%s] error : ret = %d.", argv[SRC_NAME], ret);
            goto error_out;
        }

normal_out:
        if (!FISRT_STR("dr", argv[OPTION]) && remove(dst_file_name) != 0) {
            VERBOSE("Remove dst file %s failed!", dst_file_name);
        }

        if (gpiod_ctxless_set_value(gpio_name, gpio_nr, 0, 0, "FLASH_SWITCH", NULL, NULL) != 0) {
            ERROR("Gpio write port[%ld] value[%d] error : ret = %d.", gpio_nr, 0, ret);
            return -1;
        }
#endif
    }

    INFO("%s execute successful!", argv[EXE_NAME]);
    return 0;

#if GPIO_ENABLE
error_out:
    if (gpiod_ctxless_set_value(gpio_name, gpio_nr, 0, 0, "FLASH_SWITCH", NULL, NULL) != 0)
        ERROR("Gpio write port[%ld] value[%d] error : ret = %d.", gpio_nr, 0, ret);
    return -1;
#endif
}
