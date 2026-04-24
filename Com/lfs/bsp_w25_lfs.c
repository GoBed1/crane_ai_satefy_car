#include "bsp_w25_lfs.h"
#include "bsp_qspi_w25q128.h"

#define LFS_W25Q_BASE        (0x000000)                 // 如需保留前区，可改为 8*1024*1024
#define LFS_W25Q_SIZE        (16*1024*1024)             // 16MB
#define LFS_W25Q_BLOCK_SIZE  (4096)                     // 4KB 扇区
#define LFS_W25Q_BLOCK_COUNT (LFS_W25Q_SIZE / LFS_W25Q_BLOCK_SIZE)

int w25q_lfs_read(const struct lfs_config* c, lfs_block_t block,
                         lfs_off_t off, void* buffer, lfs_size_t size) {
    uint32_t addr = LFS_W25Q_BASE + block * c->block_size + off;
    QSPI_W25Qx_Read_Buffer((uint8_t*)buffer, addr, size);
    return 0;
}

int w25q_lfs_prog(const struct lfs_config* c, lfs_block_t block,
                         lfs_off_t off, const void* buffer, lfs_size_t size) {
    // LittleFS 会保证对齐到 prog_size 且不跨 block；我们设置 prog_size=256，单次不跨页
    uint32_t addr = LFS_W25Q_BASE + block * c->block_size + off;
    return QSPI_W25Qx_Write_Buffer((uint8_t*)buffer, addr, size) ? 0 : LFS_ERR_IO;
}

int w25q_lfs_erase(const struct lfs_config* c, lfs_block_t block) {
    uint32_t addr = LFS_W25Q_BASE + block * c->block_size;
    QSPI_W25Qx_EraseSector(addr);
    return 0;
}

int w25q_lfs_sync(const struct lfs_config* c) {
    return 0;
}

// 导出给全局使用
lfs_t lfs_W25Q;
lfs_file_t file_W25Q;

const struct lfs_config cfg_W25Q = {
    .read  = w25q_lfs_read,
    .prog  = w25q_lfs_prog,
    .erase = w25q_lfs_erase,
    .sync  = w25q_lfs_sync,

    .read_size      = 16,       // 读粒度，常用 16/32/64
    .prog_size      = 256,      // 页编程粒度，W25Q 要求 256B
    .block_size     = 4096,     // 扇区 4KB
    .block_count    = LFS_W25Q_BLOCK_COUNT,  // 或者只用一部分
    .block_cycles   = 100,      // 擦写均衡参数
    .cache_size     = 256,      // ≥ prog_size
    .lookahead_size = 32,       // 32/64 均可
};