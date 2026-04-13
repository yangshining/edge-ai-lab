#ifndef __AFLD_H__
#define __AFLD_H__

enum {
    UPGRADE_EXEC_INVALID,       /* Do upgarade, no valid firmware exist */
    UPGRADE_EXEC_UPDATE,        /* Do upgarade, flag = 1 */
    UPGRADE_EXEC_FORCE,         /* Do upgarade, flag = 2 */
    UPGRADE_SKIP_NORMAL,        /* Skip upgrade, upgrade flag = 0, and firmware exist */
    UPGRADE_SKIP_UPDATE,        /* Skip upgrade, upgrade flag = 1, and firmware to be upgraded is not newer*/
    UPGRADE_FAILED = -1         /* Failed */
};

enum {
    AUX_FPGA_INVALID = 0,           /* AUX FPGA invalid */
    AUX_FPGA_VALID = 1,             /* AUX FPGA valid */
    AUX_FPGA_CHECK_FAILED = -1     /* AUX FPGA check failed */
};
#if GPIO_ENABLE
int update_aux_fpga(void);
int is_firmware_valid(void);
#endif
#endif /* __AFLD_H__ */