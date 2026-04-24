
#include "main.h"
#include "bsp_w25_lfs.h"
#include "lfs.h"
#include "modbus_tcp_server_database.h"
#include "board_flash_system.h"

extern lfs_t lfs_W25Q;
lfs_t *board_file_system = NULL;

#define MODBUS_REGS_FILE "modbus_regs.bin"
bool modbus_reg_valid = false;

fs_err_t fs_mount_medium(void)
{

    int err = lfs_mount(&lfs_W25Q, &cfg_W25Q);
    if (err)
    {
        err = lfs_format(&lfs_W25Q, &cfg_W25Q);
        if (err)
        {
            return FS_ERR_FORMAT;
        }
        err = lfs_mount(&lfs_W25Q, &cfg_W25Q);
        if (err)
        {
            return FS_ERR_MOUNT;
        }
    }
    board_file_system = &lfs_W25Q;
    return FS_OK;
}

fs_err_t fs_load_modbus_reg(void)
{
    if (board_file_system == NULL)
    {
        return FS_ERR_NOT_MOUNTED;
    }

    lfs_file_t file;
    int err = lfs_file_open(board_file_system, &file, MODBUS_REGS_FILE, LFS_O_RDONLY);
    if (err != FS_OK)
    {
        return err;
    }

    err = lfs_file_read(board_file_system, &file, coil_regs_database, sizeof(coil_regs_database));
    if (err != FS_OK)
    {
        lfs_file_close(board_file_system, &file);
        return err;
    }

    err = lfs_file_read(board_file_system, &file, holding_regs_database, sizeof(holding_regs_database));
    if (err != FS_OK)
    {
        lfs_file_close(board_file_system, &file);
        return err;
    }

    err = lfs_file_close(board_file_system, &file);
    if (err != FS_OK)
    {
        return err;
    }

    return FS_OK;
}

fs_err_t fs_write_modbus_reg(lfs_file_t *file)
{
    int err;
    err = lfs_file_write(board_file_system, file, coil_regs_database, sizeof(coil_regs_database));
    if (err != 0)
    {
        return FS_ERR_WRITE;
    }

    err = lfs_file_write(board_file_system, file, holding_regs_database, sizeof(holding_regs_database));
    if (err != 0)
    {
        return FS_ERR_WRITE;
    }
    return FS_OK;
}

fs_err_t fs_save_modbus_reg(void)
{
    if (board_file_system == NULL)
    {
        return FS_ERR_NOT_MOUNTED;
    }
    lfs_file_t file;
    int err = lfs_file_open(board_file_system, &file, MODBUS_REGS_FILE, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err != 0)
    {
        return FS_ERR_OPEN;
    }
    int w1 = lfs_file_write(board_file_system, &file, coil_regs_database, sizeof(coil_regs_database));
    int w2 = lfs_file_write(board_file_system, &file, holding_regs_database, sizeof(holding_regs_database));
    int cerr = lfs_file_close(board_file_system, &file);
    if (w1 < 0 || w2 < 0 || cerr < 0)
    {
        return (w1 < 0 || w2 < 0) ? FS_ERR_WRITE : FS_ERR_CLOSE;
    }
    return FS_OK;
}

fs_err_t fs_init_modbus_reg(void)
{

    fs_err_t err = fs_load_modbus_reg();
    if (err == LFS_ERR_CORRUPT)
    {
        mb_default_reg(REG_TYPE_ALL);
        LOG_ERR("FS", "modbus registers file corrupted.\n");

        lfs_file_t new_file;
        err = lfs_file_open(board_file_system, &new_file, MODBUS_REGS_FILE, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
        if (err != 0)
        {
            LOG_ERR("FS", "Failed to create new modbus registers file.\n");
            return FS_ERR_OPEN;
        }

        err = fs_write_modbus_reg(&new_file);
        if (err != 0)
        {
            LOG_ERR("FS", "[%d]Failed to write default modbus registers to file.\n", err);
        }

        err = lfs_file_close(board_file_system, &new_file);
        if (err != 0)
        {
            LOG_ERR("FS", "[%d]Failed to close modbus registers file after writing default values.\n", err);
        }

        return FS_OK;
    }
    else if (err != FS_OK)
    {
        mb_default_reg(REG_TYPE_ALL);
        err = fs_save_modbus_reg();
        if(err != FS_OK)
        {
            LOG_ERR("FS", "Failed to save default modbus registers to file.\n");
        }else{
            LOG_ERR("FS", "[%d]Load modbus registers from file failed!!!\n", err);
        }
    }else{
        LOG_INFO("FS", "Load modbus registers from file ok\n");
    }
    return err;
}

fs_err_t fs_recreate_modbus_reg(void)
{
    lfs_file_t new_file;
    int err = lfs_file_open(board_file_system, &new_file, MODBUS_REGS_FILE, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err != 0)
    {
        return FS_ERR_OPEN;
    }

    err = lfs_file_write(board_file_system, &new_file, coil_regs_database, sizeof(coil_regs_database));
    if (err != 0)
    {
        lfs_file_close(board_file_system, &new_file);
        return FS_ERR_WRITE_COIL;
    }

    err = lfs_file_write(board_file_system, &new_file, holding_regs_database, sizeof(holding_regs_database));
    if (err != 0)
    {
        lfs_file_close(board_file_system, &new_file);
        return FS_ERR_WRITE_HOLDING;
    }

    err = lfs_file_close(board_file_system, &new_file);
    if (err != 0)
    {
        return FS_ERR_CLOSE;
    }

    return FS_OK;
}
