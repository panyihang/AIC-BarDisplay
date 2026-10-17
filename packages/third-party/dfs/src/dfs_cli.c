/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * matteo <duanmt@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <console.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_errno.h>

#ifdef AIC_CONSOLE_BARE_DRV
#include <dfs_bare.h>
#include <dfs.h>
#include <dfs_file.h>
#include <dfs_private.h>

static int cmd_ls(int argc, char **argv)
{
    extern void ls(const char *pathname);

    if (argc == 1)
    {
#ifdef DFS_USING_WORKDIR
        ls(working_directory);
#else
        ls("/");
#endif
    }
    else
    {
        ls(argv[1]);
    }

    return 0;
}
CONSOLE_CMD(ls, cmd_ls,  "List information about the FILEs.");

static int cmd_cp(int argc, char **argv)
{
    void copy(const char *src, const char *dst);

    if (argc != 3)
    {
        rt_kprintf("Usage: cp SOURCE DEST\n");
        rt_kprintf("Copy SOURCE to DEST.\n");
    }
    else
    {
        copy(argv[1], argv[2]);
    }

    return 0;
}
CONSOLE_CMD(cp, cmd_cp, "Copy SOURCE to DEST.");

extern void rm(const char *filename);
static int cmd_rm(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("Usage: rm file\n");
        rt_kprintf("Remove file.\n");
        return -1;
    }
    rm(argv[1]);
    return 0;
}
CONSOLE_CMD(rm, cmd_rm, "rm file.");

static int cmd_mkdir(int argc, char **argv)
{
    if (argc == 1)
    {
        rt_kprintf("Usage: mkdir [OPTION] DIRECTORY\n");
        rt_kprintf("Create the DIRECTORY, if they do not already exist.\n");
    }
    else
    {
        mkdir(argv[1], 0);
    }

    return 0;
}
CONSOLE_CMD(mkdir, cmd_mkdir, "Create the DIRECTORY.");

extern int chdir(const char *path);
static int cmd_chdir(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("Usage: cd DIRECTORY\n");
        rt_kprintf("Change the current working directory to DIRECTORY.\n");
        return -1;
    }

    chdir(argv[1]);

    return 0;
}
CONSOLE_CMD(cd, cmd_chdir, "Change DIRECTORY.");

static int cmd_echo(int argc, char **argv)
{
    if (argc == 2)
    {
        rt_kprintf("%s\n", argv[1]);
    }
    else if (argc == 3)
    {
        int fd;

        fd = open(argv[2], O_RDWR | O_APPEND | O_CREAT, 0);
        if (fd >= 0)
        {
            write(fd, argv[1], strlen(argv[1]));
            close(fd);
        }
        else
        {
            rt_kprintf("open file:%s failed!\n", argv[2]);
        }
    }
    else
    {
        rt_kprintf("Usage: echo \"string\" / + [filename]\n");
    }

    return 0;
}
CONSOLE_CMD(echo, cmd_echo, "echo string to file");

static int cmd_cat(int argc, char **argv)
{
    int index;
    extern void cat(const char *filename);

    if (argc == 1)
    {
        rt_kprintf("Usage: cat / + [FILE]...\n");
        rt_kprintf("Concatenate FILE(s)\n");
        return 0;
    }

    for (index = 1; index < argc; index ++)
    {
        cat(argv[index]);
    }

    return 0;
}
CONSOLE_CMD(cat, cmd_cat, "Concatenate FILE(s)");

extern struct dfs_filesystem filesystem_table[];
static int cmd_mount(int argc, char **argv)
{
    if (argc == 1)
    {
        struct dfs_filesystem *iter;

        /* display the mount history */
        rt_kprintf("filesystem  device  mountpoint\n");
        rt_kprintf("----------  ------  ----------\n");
        for (iter = &filesystem_table[0];
                iter < &filesystem_table[DFS_FILESYSTEMS_MAX]; iter++)
        {
            if ((iter != NULL) && (iter->path != NULL))
            {
                rt_kprintf("%-10s  %-6s  %-s\n",
                           iter->ops->name, (char *)iter->dev_id, iter->path);
            }
        }
        return 0;
    }
    else if (argc == 4)
    {
        char *device = argv[1];
        char *path = argv[2];
        char *fstype = argv[3];

        /* mount a filesystem to the specified directory */
        rt_kprintf("mount device %s(%s) onto %s ... ", device, fstype, path);
        if (dfs_mount(device, path, fstype, 0, 0) == 0)
        {
            rt_kprintf("succeed!\n");
            return 0;
        }
        else
        {
            rt_kprintf("failed!\n");
            return -1;
        }
    }
    else
    {
        rt_kprintf("Usage: mount <device> <mountpoint> <fstype>.\n");
        return -1;
    }
}
CONSOLE_CMD(mount, cmd_mount, "mount <device> <mountpoint> <fstype>");

/* unmount the filesystem from the specified mountpoint */
static int cmd_umount(int argc, char **argv)
{
    char *path = argv[1];

    if (argc != 2)
    {
        rt_kprintf("Usage: unmount <mountpoint>.\n");
        return -1;
    }

    rt_kprintf("unmount %s ... ", path);
    if (dfs_unmount(path) < 0)
    {
        rt_kprintf("failed!\n");
        return -1;
    }
    else
    {
        rt_kprintf("succeed!\n");
        return 0;
    }
}
CONSOLE_CMD(umount, cmd_umount, "Unmount device from file system");
#endif // defined(AIC_CONSOLE_BARE_DRV)
