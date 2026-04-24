#ifndef BSP_W25_LFS_H
#define BSP_W25_LFS_H

#include "lfs.h"

int w25q_lfs_read(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, void* buffer, lfs_size_t size);

int w25q_lfs_prog(const struct lfs_config* c, lfs_block_t block,
    lfs_off_t off, const void* buffer, lfs_size_t size);

int w25q_lfs_erase(const struct lfs_config* c, lfs_block_t block);

int w25q_lfs_sync(const struct lfs_config* c);

extern lfs_t lfs_W25Q;
extern lfs_file_t file_W25Q;
extern const struct lfs_config cfg_W25Q;

#endif
