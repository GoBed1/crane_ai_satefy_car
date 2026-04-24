#ifndef MODBUS_TCP_SERVER_FLASH_H
#define MODBUS_TCP_SERVER_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// fs_err_t uses non-overlapping, non-negative codes to avoid clashing with
// littlefs's negative error codes (lfs_error).
typedef enum {
	FS_OK = 0,
	FS_ERR_NOT_MOUNTED   = 1,
	FS_ERR_MOUNT         = 2,
	FS_ERR_FORMAT        = 3,
	FS_ERR_OPEN          = 4,
	FS_ERR_READ_COIL     = 5,
    FS_ERR_READ_HOLDING  = 6,
	FS_ERR_WRITE_COIL    = 7,
	FS_ERR_WRITE_HOLDING = 8,
	FS_ERR_CLOSE         = 9,
	FS_ERR_CORRUPT       = 10,
	FS_ERR_MB_DEFAULT    = 11,
	FS_ERR_WRITE         = 12
} fs_err_t;

fs_err_t fs_mount_medium(void);
fs_err_t fs_save_modbus_reg(void);
fs_err_t fs_load_modbus_reg(void);
fs_err_t fs_init_modbus_reg(void);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_TCP_SERVER_FLASH_H */ 
