#ifndef __PKHD_H__
#define __PKHD_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @mtd_dev_cnt: count of MTD devices in system
 * @lowest_mtd_num: lowest MTD device number in system
 * @highest_mtd_num: highest MTD device number in system
 * @sysfs_supported: non-zero if sysfs is supported by MTD
 */

#ifndef uint32_t
    typedef unsigned int uint32_t;
#endif

#ifndef uint16_t
    typedef unsigned short uint16_t;
#endif

#ifndef uint8_t 
    typedef unsigned char uint8_t;
#endif

#ifndef GPIO_ENABLE
#define GPIO_ENABLE 0
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#ifndef CRC_ENABLE
#define CRC_ENABLE 1
#endif

int s_debug_level = DEBUG_LEVEL;

#define DEBUG_VERB 0
#define DEBUG_INFO 1
#define DEBUG_WARN 2
#define DEBUG_NOTICE 3
#define DEBUG_ERROR 4
#define PROGRAM_NAME "Auxfpgaload"
#define MAGIC_NUM 0x43495358u /* CISX */
#define MAGIC_NUM_PAD 0x43504144u /* CPAD */
#define AUX_PRINT(Debug_type, fmt, ...) do { \
        if (Debug_type >= s_debug_level) { \
            printf(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define VERBOSE(fmt, ...)       AUX_PRINT(DEBUG_VERB, "%s Verbos: " fmt "\n", PROGRAM_NAME, ##__VA_ARGS__)
#define INFO(fmt, ...)          AUX_PRINT(DEBUG_INFO, "%s Info: " fmt "\n", PROGRAM_NAME, ##__VA_ARGS__)
#define WARN(fmt, ...)          AUX_PRINT(DEBUG_WARN, "%s Warning: " fmt "\n", PROGRAM_NAME, ##__VA_ARGS__)
#define ERROR(fmt, ...)         AUX_PRINT(DEBUG_ERROR, "%s Error: " fmt "\n", PROGRAM_NAME, ##__VA_ARGS__)
#define NOTICE(fmt, ...)        AUX_PRINT(DEBUG_NOTICE, "%s Notice: " fmt "\n", PROGRAM_NAME, ##__VA_ARGS__)

#define NOTICE_NO_PREFIX(fmt, ...)          AUX_PRINT(DEBUG_NOTICE, fmt "\n", ##__VA_ARGS__)

#define VERSION_FILE "version.txt"
#define TMP_DIR "/tmp/"
#define VERSION_SIZE 1024
#define VERSION_PATT "Date: %15s\nName: %63s\nVersion: %15s\nFlag: %d\n"
#define AUX_CMD_DISABLE_FILE "/ram2/acd"

#define MTD_NAME_PATT    "mtd%d"

#define KILO_BYTE 1024
#define MEGA_BYTE (1024 * KILO_BYTE)

#define NORMAL_FLAG 0 /* Update when no package in aux flash */
#define UPDATE_FLAG 1 /* Update when old package in aux flash */
#define FORCE_FLAG 2 /* Update anyway */
#define DEFAULE_PAD_SIZE (32 * KILO_BYTE)

#ifndef INT_MAX
#define INT_MAX ((int)(~0U>>1))
#endif

#ifndef UINT32_MAX
#define UINT32_MAX ((uint32_t)~0U)
#endif

/**
 * @mtd_dev_cnt: count of MTD devices in system
 * @lowest_mtd_num: lowest MTD device number in system
 * @highest_mtd_num: highest MTD device number in system
 * @sysfs_supported: non-zero if sysfs is supported by MTD
 */
struct mtd_info
{
    int mtd_dev_cnt;
    int lowest_mtd_num;
    int highest_mtd_num;
    unsigned int sysfs_supported:1;
};

struct mtd_region
{
    uint32_t mtd_dev_cnt;
    uint32_t index_start;
    uint32_t index_end;
    size_t size;
};

struct block_info
{
    uint32_t offset;
    uint32_t reloc_offset;
    uint32_t size;
};

struct package_header
{
    uint32_t magic_num;
    uint32_t block_nr;
    struct block_info blk_info[0];
};


struct package_version
{
    uint8_t date[16];
    uint8_t name[64];
    uint8_t version_num[16];
};

struct package_pad
{
    uint32_t magic_num;
    uint32_t flag;
    struct package_version version;
    size_t pad_size;

#if CRC_ENABLE
    uint32_t crc;
    size_t file_size;
#endif

    uint8_t reserve[32];
    uint8_t data[0];
};

#ifdef __cplusplus
}
#endif

#endif /* __PKHD_H__ */
