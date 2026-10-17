/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2005-02-22     Bernard      The first version.
 */

#ifndef __DFS_FS_H__
#define __DFS_FS_H__

#include <dfs.h>
#include <sys/types.h>
#include <sys/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def DFS_MOUNT_PRIO_EARLY
 * @brief Mount priority for early system initialization (INIT_ENV_EXPORT)
 *
 * File systems with this priority (0) will be mounted during early
 * environment initialization stage.
 */
#define DFS_MOUNT_PRIO_EARLY   0

/**
 * @def DFS_MOUNT_PRIO_LATE
 * @brief Mount priority for late application initialization (INIT_LATE_APP_EXPORT)
 *
 * File systems with this priority (1) will be mounted during late
 * application initialization stage.
 */
#define DFS_MOUNT_PRIO_LATE    1

/**
 * @brief Mount flag constant for read-only filesystem mounting
 * 
 * This macro defines the flag value used to indicate that a filesystem
 * should be mounted in read-only mode, preventing any write operations.
 */
#define MS_RDONLY      1

/* Pre-declaration */
struct dfs_filesystem;
struct dfs_fd;

/* File system operations */
struct dfs_filesystem_ops
{
    char *name;
    uint32_t flags;      /* flags for file system operations */

    /* operations for file */
    const struct dfs_file_ops *fops;

    /* mount and unmount file system */
    int (*mount)    (struct dfs_filesystem *fs, unsigned long rwflag, const void *data);
    int (*unmount)  (struct dfs_filesystem *fs);

    /* make a file system */
    int (*mkfs)     (rt_device_t devid);
    int (*statfs)   (struct dfs_filesystem *fs, struct statfs *buf);

    int (*unlink)   (struct dfs_filesystem *fs, const char *pathname);
    int (*stat)     (struct dfs_filesystem *fs, const char *filename, struct stat *buf);
    int (*rename)   (struct dfs_filesystem *fs, const char *oldpath, const char *newpath);
};

/* Mounted file system */
struct dfs_filesystem
{
    rt_device_t dev_id;     /* Attached device */

    char *path;             /* File system mount point */
    const struct dfs_filesystem_ops *ops; /* Operations for file system type */

    void *data;             /* Specific file system data */
};

/* file system partition table */
struct dfs_partition
{
    uint8_t type;        /* file system type */
    char name[PARTITION_NAME_MAX]; /* partition name */
    off_t  offset;       /* partition start offset */
    size_t size;         /* partition size */
    rt_sem_t lock;
};

/* mount table */
struct dfs_mount_tbl
{
    const char   *device_name;
    const char   *path;
    const char   *filesystemtype;
    unsigned long rwflag;
    const void   *data;
    unsigned int  prio;
};

int dfs_register(const struct dfs_filesystem_ops *ops);
struct dfs_filesystem *dfs_filesystem_lookup(const char *path);
const char *dfs_filesystem_get_mounted_path(struct rt_device *device);

int dfs_filesystem_get_partition(struct dfs_partition *part,
                                 uint8_t         *buf,
                                 uint32_t        pindex);

int dfs_mount(const char *device_name,
              const char *path,
              const char *filesystemtype,
              unsigned long rwflag,
              const void *data);
int dfs_unmount(const char *specialfile);

int dfs_mkfs(const char *fs_name, const char *device_name);
int dfs_statfs(const char *path, struct statfs *buffer);
int dfs_mount_device(rt_device_t dev);
int dfs_unmount_device(rt_device_t dev);

#ifdef __cplusplus
}
#endif

#endif
