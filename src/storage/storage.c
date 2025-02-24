#include "storage/storage.h"

#include <stdbool.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/fs/nvs.h>

LOG_MODULE_REGISTER(storage, CONFIG_STORAGE_MODULE_LOG_LEVEL);

/**
 * @brief NVS File System
 */
static struct nvs_fs fs;

/**
 * @brief Initialization flag
 */
static volatile bool fs_init = false;

/**
 * @brief Mutex for the filesystem handle
 */
K_MUTEX_DEFINE(fs_mtx);

inline int storage_init(k_timeout_t timeout)
{
    int err = 0;

    // Wait for the lock
    if ((err = k_mutex_lock(&fs_mtx, timeout)) < 0)
        return err;

    // Filesystem already initialized
    if (fs_init)
        return 0;

    // Get the device and check if its valid
    fs.flash_device = NVS_PARTITION_DEVICE;
    if (!device_is_ready(fs.flash_device))
        return -ENOENT;

    // Set partition offset
    fs.offset = NVS_PARTITION_OFFSET;

    // Get flash information
    struct flash_pages_info flash_info;
    if ((err = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &flash_info)) < 0)
        return err;

    // Show debug information
    LOG_DBG("Flash {.idx = %d, .size = %d, .offset = %ld}",
            flash_info.index,
            flash_info.size,
            flash_info.start_offset);

    // Set FS properties with flash info
    // Divide the FS size into the number of sectors
    fs.sector_size = (uint16_t)flash_info.size;
    // Set the number of sectors
    fs.sector_count = (uint16_t)SECTOR_COUNT;

    // Show debug information
    LOG_DBG("Filesystem {.ss = %d, .sc = %d, .offset = %ld}",
            fs.sector_size,
            fs.sector_count,
            fs.offset);

    // Mount the file system
    if ((err = nvs_mount(&fs)) < 0)
    {
        if ((err = nvs_clear(&fs)) < 0)
        {
            LOG_ERR("Cannot clear filesystem data: %d", err);
            return err;
        }

        if ((err = nvs_mount(&fs)) < 0)
        {
            LOG_ERR("Filesystem mount failure: %d", err);
            return err;
        }
    }

    // File system initialized
    fs_init = true;

    // Unlock the mutex
    if ((err = k_mutex_unlock(&fs_mtx)) < 0)
        return err;

    LOG_INF("storage_init: fs mounted!");
    return err;
}

inline int storage_read(storage_key_t key, void *data, size_t size, k_timeout_t timeout)
{
    int err;

    // Wait for the lock
    if ((err = k_mutex_lock(&fs_mtx, timeout)) < 0)
        return err;

    // Read data
    size_t bytes = nvs_read(&fs, (uint16_t)key, data, size);
    if (bytes < 0)
        err = (int)bytes;

    LOG_DBG("Read (%d)/(%d) bytes", bytes, size);

    // Unlock the mutex
    if ((err = k_mutex_unlock(&fs_mtx)) < 0)
        return err;

    // Check if less bytes were written
    if (bytes < size)
    {
        LOG_ERR("storage_read: Read less data than expected");
        return -EIO;
    }

    return err;
}

inline int storage_write(storage_key_t key, const void *data, size_t size, k_timeout_t timeout)
{
    int err;

    // Check if there is enought free space
    if (size > nvs_calc_free_space(&fs))
        return -ENOSPC;

    // Wait for the lock
    if ((err = k_mutex_lock(&fs_mtx, timeout)) < 0)
        return err;

    // Write data
    size_t bytes = nvs_write(&fs, (uint16_t)key, data, size);
    if (bytes < 0)
        err = (int)bytes;

    LOG_DBG("Wrote (%d)/(%d) bytes", bytes, size);

    // Unlock the mutex
    if ((err = k_mutex_unlock(&fs_mtx)) < 0)
        return err;

    if (bytes == 0)
        LOG_DBG("Data already stored, skipped");

    // Check if less bytes were written
    if (bytes < size && bytes != 0)
    {
        LOG_ERR("storage_write: Wrote less data than expected");
        return -EIO;
    }

    return err;
}
