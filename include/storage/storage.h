/**
 * @file storage.h
 * @author Angel Talero (angelgotalero@outlook.com)
 * @brief Read/Write from flash
 * @version 0.1
 * @date 2025-02-17
 *
 * @copyright Copyright (c) 2025
 */

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>

/**
 * @brief Key for accessing objects in storage
 */
typedef uint16_t storage_key_t;

/**
 * @brief Label for the partition
 */
#define NVS_PARTITION storage_partition

/**
 * @brief Handle to the flash device
 */
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)

/**
 * @brief Partition offset in device flash memory
 */
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)

/**
 * @brief Number of sectors to use
 * @note Having 3 sectors would require more flash memory
 */
#define SECTOR_COUNT 2U

/**
 * @brief Initialize the storage subsystem
 * @note Must be called by every module that requires it to wait for the storage subsystem to be ready
 *
 * @param timeout Timeout for the storage module to become available
 * @return int -ERRNO code or 0 on success
 */
int storage_init(k_timeout_t timeout);

/**
 * @brief Read an object from storage given its key
 * 
 * @param key Key associated to the object
 * @param data Pointer where data will be stored
 * @param size Length of the expected data
 * @param timeout Max time to wait for other threads to release the fs
 * @return int -ERRNO or 0 on success
 */
int storage_read(storage_key_t key, void *data, size_t size, k_timeout_t timeout);

/**
 * @brief Write an object to storage with an associated key for later retrieval
 * 
 * @param key Key associated to the object
 * @param data Pointer to data to be stored
 * @param size Size ot the data in the pointer
 * @param timeout Max time to wait for other threads to release the fs
 * @return int -ERRNO or 0 on success
 */
int storage_write(storage_key_t key, const void *data, size_t size, k_timeout_t timeout);

#endif // __STORAGE_H__