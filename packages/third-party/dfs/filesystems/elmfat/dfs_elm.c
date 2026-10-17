/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2008-02-22     QiuYi        The first version.
 * 2011-10-08     Bernard      fixed the block size in statfs.
 * 2011-11-23     Bernard      fixed the rename issue.
 * 2012-07-26     aozima       implement ff_memalloc and ff_memfree.
 * 2012-12-19     Bernard      fixed the O_APPEND and lseek issue.
 * 2013-03-01     aozima       fixed the stat(st_mtime) issue.
 * 2014-01-26     Bernard      Check the sector size before mount.
 * 2017-02-13     Hichard      Update Fatfs version to 0.12b, support exFAT.
 * 2017-04-11     Bernard      fix the st_blksize issue.
 * 2017-05-26     Urey         fix f_mount error when mount more fats
 */

#include <dfs_bare.h>

#include "ffconf.h"
#include "ff.h"
#include <string.h>
#include <sys/time.h>

/* ELM FatFs provide a DIR struct */
#define HAVE_DIR_STRUCTURE

#include <dfs_fs.h>
#include <dfs_file.h>
#include <diskio.h>
#include <dfs_elm.h>

#include "boot_param.h"

static struct elm_dev_info disk[FF_VOLUMES] = {0};

static int elm_result_to_dfs(FRESULT result)
{
    int status = RT_EOK;

    switch (result)
    {
    case FR_OK:
        break;

    case FR_NO_FILE:
    case FR_NO_PATH:
    case FR_NO_FILESYSTEM:
        status = -ENOENT;
        break;

    case FR_INVALID_NAME:
        status = -EINVAL;
        break;

    case FR_EXIST:
    case FR_INVALID_OBJECT:
        status = -EEXIST;
        break;

    case FR_DISK_ERR:
    case FR_NOT_READY:
    case FR_INT_ERR:
        status = -EIO;
        break;

    case FR_WRITE_PROTECTED:
    case FR_DENIED:
        status = -EROFS;
        break;

    case FR_MKFS_ABORTED:
        status = -EINVAL;
        break;

    default:
        status = -1;
        break;
    }

    return status;
}

/* results:
 *  -1, no space to install fatfs driver
 *  >= 0, there is an space to install fatfs driver
 */
static int get_disk(rt_device_t id)
{
    int index;

    for (index = 0; index < FF_VOLUMES; index ++)
    {
        if (disk[index].dev_name == id)
            return index;
    }

    return -1;
}

int dfs_elm_mount(struct dfs_filesystem *fs, unsigned long rwflag, const void *data)
{
    FATFS *fat;
    FRESULT result;
    int index;
    char logic_nbr[3] = {'0',':', 0};

    /* get an empty position */
    index = get_disk(RT_NULL);
    if (index == -1) {
        pr_err("Failed to get empty disk position.\n");
        return -ENOENT;
    }
    logic_nbr[0] = '0' + index;

    /* save device */
    disk[index].dev_name = fs->dev_id;
    disk[index].dev_type = (long)data;

    fat = (FATFS *)rt_malloc_align(sizeof(FATFS), CACHE_LINE_SIZE);
    if (fat == RT_NULL)
    {
        disk[index].dev_name = RT_NULL;
        pr_err("Failed to allocate memory.\n");
        return -ENOMEM;
    }

    /* mount fatfs, always 0 logic driver */
    result = f_mount(fat, (const TCHAR *)logic_nbr, 1);
    if (result == FR_OK)
    {
#if FF_FS_MINIMIZE <= 1
        char drive[8];
        DIR *dir;

        rt_snprintf(drive, sizeof(drive), "%d:/", index);
        dir = (DIR *)rt_malloc(sizeof(DIR));
        if (dir == RT_NULL)
        {
            f_mount(RT_NULL, (const TCHAR *)logic_nbr, 1);
            disk[index].dev_name = RT_NULL;
            rt_free_align(fat);
            pr_err("Failed to allocate memory for %s\n", drive);
            return -ENOMEM;
        }

        /* open the root directory to test whether the fatfs is valid */
        result = f_opendir(dir, drive);
        rt_free(dir);
        if (result != FR_OK) {
            pr_err("Try to open the fatfs failure.\n");;
            goto __err;
        }
#endif

        /* mount succeed! */
        fs->data = fat;
        return 0;
    }

#if FF_FS_MINIMIZE <= 1
__err:
#endif
    f_mount(RT_NULL, (const TCHAR *)logic_nbr, 1);
    disk[index].dev_name = RT_NULL;
    rt_free_align(fat);
    return elm_result_to_dfs(result);
}

int dfs_elm_unmount(struct dfs_filesystem *fs)
{
    FATFS *fat;
    FRESULT result;
    int  index;
    char logic_nbr[3] = {'0',':', 0};

    fat = (FATFS *)fs->data;

    RT_ASSERT(fat != RT_NULL);

    /* find the device index and then umount it */
    index = get_disk(fs->dev_id);
    if (index == -1) /* not found */
        return -ENOENT;

    logic_nbr[0] = '0' + index;
    result = f_mount(RT_NULL, logic_nbr, (BYTE)0);
    if (result != FR_OK)
        return elm_result_to_dfs(result);

    fs->data = RT_NULL;
    disk[index].dev_name = RT_NULL;
    rt_free_align(fat);

    return RT_EOK;
}

#if !FF_FS_READONLY && FF_USE_MKFS
int dfs_elm_mkfs(rt_device_t dev_id)
{
#define FSM_STATUS_INIT            0
#define FSM_STATUS_USE_TEMP_DRIVER 1
    FATFS *fat = RT_NULL;
    BYTE *work;
    int flag;
    FRESULT result;
    int index;
    char logic_nbr[3] = {'0',':', 0};
    MKFS_PARM opt;

    work = rt_malloc(FF_MAX_SS);
    if(RT_NULL == work) {
        return -ENOMEM;
    }

    if (dev_id == RT_NULL)
    {
        rt_free(work); /* release memory */
        return -EINVAL;
    }

    /* if the device is already mounted, then just do mkfs to the drv,
     * while if it is not mounted yet, then find an empty drive to do mkfs
     */

    flag = FSM_STATUS_INIT;
    index = get_disk(dev_id);
    if (index == -1)
    {
        /* not found the device id */
        index = get_disk(RT_NULL);
        if (index == -1)
        {
            /* no space to store an temp driver */
            rt_kprintf("sorry, there is no space to do mkfs! \n");
            rt_free(work); /* release memory */
            return -ENOSPC;
        }
        else
        {
            fat = (FATFS *)rt_malloc_align(sizeof(FATFS), CACHE_LINE_SIZE);
            if (fat == RT_NULL)
            {
                rt_free(work); /* release memory */
                return -ENOMEM;
            }

            flag = FSM_STATUS_USE_TEMP_DRIVER;

            disk[index].dev_name = dev_id;

            /* just fill the FatFs[vol] in ff.c, or mkfs will failded!
             * consider this condition: you just umount the elm fat,
             * then the space in FatFs[index] is released, and now do mkfs
             * on the disk, you will get a failure. so we need f_mount here,
             * just fill the FatFS[index] in elm fatfs to make mkfs work.
             */
            logic_nbr[0] = '0' + index;
            f_mount(fat, logic_nbr, (BYTE)index);
        }
    }
    else
    {
        logic_nbr[0] = '0' + index;
    }

    /* [IN] Logical drive number */
    /* [IN] Format options */
    /* [-]  Working buffer */
    /* [IN] Size of working buffer */
    rt_memset(&opt, 0, sizeof(opt));
    opt.fmt = FM_ANY|FM_SFD;
    result = f_mkfs(logic_nbr, &opt, work, FF_MAX_SS);
    rt_free(work); work = RT_NULL;

    /* check flag status, we need clear the temp driver stored in disk[] */
    if (flag == FSM_STATUS_USE_TEMP_DRIVER)
    {
        rt_free_align(fat);
        f_mount(RT_NULL, logic_nbr, (BYTE)index);
        disk[index].dev_name = RT_NULL;
    }

    if (result != FR_OK)
    {
        rt_kprintf("format error, result=%d\n", result);
        return elm_result_to_dfs(result);
    }

    return RT_EOK;
}
#endif

#if !FF_FS_READONLY
int dfs_elm_statfs(struct dfs_filesystem *fs, struct statfs *buf)
{
    FATFS *f;
    FRESULT res;
    char driver[8];
    DWORD fre_clust, fre_sect, tot_sect;

    RT_ASSERT(fs != RT_NULL);
    RT_ASSERT(buf != RT_NULL);

    f = (FATFS *)fs->data;

    rt_snprintf(driver, sizeof(driver), "%d:", f->pdrv);
    res = f_getfree(driver, &fre_clust, &f);
    if (res)
        return elm_result_to_dfs(res);

    /* Get total sectors and free sectors */
    tot_sect = (f->n_fatent - 2) * f->csize;
    fre_sect = fre_clust * f->csize;

    buf->f_bfree = fre_sect;
    buf->f_blocks = tot_sect;
#if FF_MAX_SS != 512
    buf->f_bsize = f->ssize;
#else
    buf->f_bsize = 512;
#endif

    return 0;
}
#endif

int dfs_elm_open(struct dfs_fd *file)
{
    FIL *fd;
    BYTE mode;
    FRESULT result;
    char *drivers_fn;

#if (FF_VOLUMES > 1)
    int vol;
    struct dfs_filesystem *fs = (struct dfs_filesystem *)file->data;
    extern int elm_get_vol(FATFS * fat);

    if (fs == NULL)
        return -ENOENT;

    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
        return -ENOENT;
    drivers_fn = (char *)rt_malloc(256);
    if (drivers_fn == RT_NULL)
        return -ENOMEM;

    rt_snprintf(drivers_fn, 256, "%d:%s", vol, file->path);
#else
    drivers_fn = file->path;
#endif

    if (file->flags & O_DIRECTORY)
    {
#if FF_FS_MINIMIZE <= 1
        DIR *dir;

#if !FF_FS_READONLY
        if (file->flags & O_CREAT)
        {
            result = f_mkdir(drivers_fn);
            if (result != FR_OK)
            {
#if FF_VOLUMES > 1
                rt_free(drivers_fn);
#endif
                return elm_result_to_dfs(result);
            }
        }
#endif

        /* open directory */
        dir = (DIR *)rt_malloc(sizeof(DIR));
        if (dir == RT_NULL)
        {
#if FF_VOLUMES > 1
            rt_free(drivers_fn);
#endif
            return -ENOMEM;
        }

        result = f_opendir(dir, drivers_fn);
#if FF_VOLUMES > 1
        rt_free(drivers_fn);
#endif
        if (result != FR_OK)
        {
            rt_free(dir);
            return elm_result_to_dfs(result);
        }

        file->data = dir;
        return RT_EOK;
#endif
    }
    else
    {
        mode = FA_READ;

        if (file->flags & O_WRONLY)
            mode |= FA_WRITE;
        if ((file->flags & O_ACCMODE) & O_RDWR)
            mode |= FA_WRITE;
        /* Opens the file, if it is existing. If not, a new file is created. */
        if (file->flags & O_CREAT)
            mode |= FA_OPEN_ALWAYS;
        /* Creates a new file. If the file is existing, it is truncated and overwritten. */
        if (file->flags & O_TRUNC)
            mode |= FA_CREATE_ALWAYS;
        /* Creates a new file. The function fails if the file is already existing. */
        if (file->flags & O_EXCL)
            mode |= FA_CREATE_NEW;

        /* allocate a fd */
        fd = (FIL *)rt_malloc_align(sizeof(FIL), CACHE_LINE_SIZE);
        if (fd == RT_NULL)
        {
#if FF_VOLUMES > 1
            rt_free(drivers_fn);
#endif
            return -ENOMEM;
        }
        memset(fd, 0, sizeof(FIL));

        result = f_open(fd, drivers_fn, mode);
#if FF_VOLUMES > 1
        rt_free(drivers_fn);
#endif
        if (result == FR_OK)
        {
            file->pos  = fd->fptr;
            file->size = f_size(fd);
            file->data = fd;

            if (file->flags & O_APPEND)
            {
                /* seek to the end of file */
                f_lseek(fd, f_size(fd));
                file->pos = fd->fptr;
            }
        }
        else
        {
            /* open failed, return */
            rt_free_align(fd);
            return elm_result_to_dfs(result);
        }
    }

    return RT_EOK;
}

int dfs_elm_close(struct dfs_fd *file)
{
    FRESULT result;

    result = FR_OK;
    if (file->type == FT_DIRECTORY)
    {
        DIR *dir;

        dir = (DIR *)(file->data);
        RT_ASSERT(dir != RT_NULL);

        /* release memory */
        rt_free(dir);
    }
    else if (file->type == FT_REGULAR)
    {
        FIL *fd;
        fd = (FIL *)(file->data);
        RT_ASSERT(fd != RT_NULL);

        if (file->fs && file->fs->data)
        {
            /* close file if fs is not unmount */
            result = f_close(fd);
        }
        if (result == FR_OK)
        {
            /* release memory */
            rt_free_align(fd);
        }
    }

    return elm_result_to_dfs(result);
}

int dfs_elm_ioctl(struct dfs_fd *file, int cmd, void *args)
{
    switch (cmd)
    {
    case RT_FIOFTRUNCATE:
        {
            FIL *fd;
            FSIZE_t fptr, length;
            FRESULT result = FR_OK;
            fd = (FIL *)(file->data);
            RT_ASSERT(fd != RT_NULL);

            /* save file read/write point */
            fptr = fd->fptr;
            length = *(off_t*)args;
            if (length <= fd->obj.objsize)
            {
#if !FF_FS_READONLY
                fd->fptr = length;
                result = f_truncate(fd);
#endif
            }
            else
            {
                result = f_lseek(fd, length);
            }
            /* restore file read/write point */
            fd->fptr = fptr;
            return elm_result_to_dfs(result);
        }
    }
    return -ENOSYS;
}

int dfs_elm_read(struct dfs_fd *file, void *buf, size_t len)
{
    FIL *fd;
    FRESULT result;
    UINT byte_read;

    if (file->type == FT_DIRECTORY)
    {
        return -EISDIR;
    }

    fd = (FIL *)(file->data);
    RT_ASSERT(fd != RT_NULL);

    result = f_read(fd, buf, len, &byte_read);
    /* update position */
    file->pos  = fd->fptr;
    if (result == FR_OK)
        return byte_read;

    return elm_result_to_dfs(result);
}

#if !FF_FS_READONLY
int dfs_elm_write(struct dfs_fd *file, const void *buf, size_t len)
{
    FIL *fd;
    FRESULT result;
    UINT byte_write;

    if (file->type == FT_DIRECTORY)
    {
        return -EISDIR;
    }

    fd = (FIL *)(file->data);
    RT_ASSERT(fd != RT_NULL);

    result = f_write(fd, buf, len, &byte_write);
    /* update position and file size */
    file->pos  = fd->fptr;
    file->size = f_size(fd);
    if (result == FR_OK)
        return byte_write;

    return elm_result_to_dfs(result);
}

int dfs_elm_flush(struct dfs_fd *file)
{
    FIL *fd;
    FRESULT result;

    fd = (FIL *)(file->data);
    RT_ASSERT(fd != RT_NULL);

    result = f_sync(fd);
    return elm_result_to_dfs(result);
}
#endif

int dfs_elm_lseek(struct dfs_fd *file, off_t offset)
{
    FRESULT result = FR_OK;
    if (file->type == FT_REGULAR)
    {
        FIL *fd;

        /* regular file type */
        fd = (FIL *)(file->data);
        RT_ASSERT(fd != RT_NULL);

        result = f_lseek(fd, offset);
        if (result == FR_OK)
        {
            /* return current position */
            file->pos = fd->fptr;
            return fd->fptr;
        }
    }
    else if (file->type == FT_DIRECTORY)
    {
#if FF_FS_MINIMIZE <= 1
        /* which is a directory */
        DIR *dir;

        dir = (DIR *)(file->data);
        RT_ASSERT(dir != RT_NULL);

        result = f_seekdir(dir, offset / sizeof(struct dirent));
        if (result == FR_OK)
        {
            /* update file position */
            file->pos = offset;
            return file->pos;
        }
#endif
    }

    return elm_result_to_dfs(result);
}

#if FF_FS_MINIMIZE <= 1
int dfs_elm_getdents(struct dfs_fd *file, struct dirent *dirp, uint32_t count)
{
    DIR *dir;
    FILINFO fno;
    FRESULT result;
    rt_uint32_t index;
    struct dirent *d;

    dir = (DIR *)(file->data);
    RT_ASSERT(dir != RT_NULL);

    /* make integer count */
    count = (count / sizeof(struct dirent)) * sizeof(struct dirent);
    if (count == 0)
        return -EINVAL;

    index = 0;
    while (1)
    {
        char *fn;

        d = dirp + index;

        result = f_readdir(dir, &fno);
        if (result != FR_OK || fno.fname[0] == 0)
            break;

#if FF_USE_LFN
        fn = *fno.fname ? fno.fname : fno.altname;
#else
        fn = fno.fname;
#endif

        d->d_type = DT_UNKNOWN;
        if (fno.fattrib & AM_DIR)
            d->d_type = DT_DIR;
        else
            d->d_type = DT_REG;

        d->d_namlen = (rt_uint8_t)rt_strlen(fn);
        d->d_reclen = (rt_uint16_t)sizeof(struct dirent);
        rt_strncpy(d->d_name, fn, DFS_PATH_MAX);

        index ++;
        if (index * sizeof(struct dirent) >= count)
            break;
    }

    if (index == 0)
        return elm_result_to_dfs(result);

    file->pos += index * sizeof(struct dirent);

    return index * sizeof(struct dirent);
}
#endif

#if !FF_FS_READONLY
int dfs_elm_unlink(struct dfs_filesystem *fs, const char *path)
{
    FRESULT result;

#if FF_VOLUMES > 1
    int vol;
    char *drivers_fn;
    extern int elm_get_vol(FATFS * fat);

    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
        return -ENOENT;
    drivers_fn = (char *)rt_malloc(256);
    if (drivers_fn == RT_NULL)
        return -ENOMEM;

    rt_snprintf(drivers_fn, 256, "%d:%s", vol, path);
#else
    const char *drivers_fn;
    drivers_fn = path;
#endif

    result = f_unlink(drivers_fn);
#if FF_VOLUMES > 1
    rt_free(drivers_fn);
#endif
    return elm_result_to_dfs(result);
}

int dfs_elm_rename(struct dfs_filesystem *fs, const char *oldpath, const char *newpath)
{
    FRESULT result;

#if FF_VOLUMES > 1
    char *drivers_oldfn;
    const char *drivers_newfn;
    int vol;
    extern int elm_get_vol(FATFS * fat);

    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
        return -ENOENT;

    drivers_oldfn = (char *)rt_malloc(256);
    if (drivers_oldfn == RT_NULL)
        return -ENOMEM;
    drivers_newfn = newpath;

    rt_snprintf(drivers_oldfn, 256, "%d:%s", vol, oldpath);
#else
    const char *drivers_oldfn, *drivers_newfn;

    drivers_oldfn = oldpath;
    drivers_newfn = newpath;
#endif

    result = f_rename(drivers_oldfn, drivers_newfn);
#if FF_VOLUMES > 1
    rt_free(drivers_oldfn);
#endif
    return elm_result_to_dfs(result);
}
#endif

#if FF_FS_MINIMIZE == 0
int dfs_elm_stat(struct dfs_filesystem *fs, const char *path, struct stat *st)
{
    FILINFO file_info;
    FRESULT result;

#if FF_VOLUMES > 1
    int vol;
    char *drivers_fn;
    extern int elm_get_vol(FATFS * fat);

    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
        return -ENOENT;
    drivers_fn = (char *)rt_malloc(256);
    if (drivers_fn == RT_NULL)
        return -ENOMEM;

    rt_snprintf(drivers_fn, 256, "%d:%s", vol, path);
#else
    const char *drivers_fn;
    drivers_fn = path;
#endif

    result = f_stat(drivers_fn, &file_info);
#if FF_VOLUMES > 1
    rt_free(drivers_fn);
#endif
    if (result == FR_OK)
    {
        /* convert to dfs stat structure */
        st->st_dev = 0;

        st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH |
                      S_IWUSR | S_IWGRP | S_IWOTH;
        if (file_info.fattrib & AM_DIR)
        {
            st->st_mode &= ~S_IFREG;
            st->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
        }
        if (file_info.fattrib & AM_RDO)
            st->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);

        st->st_size  = file_info.fsize;
#if 0
        /* get st_mtime. */
        {
            struct tm tm_file;
            int year, mon, day, hour, min, sec;
            WORD tmp;

            tmp = file_info.fdate;
            day = tmp & 0x1F;           /* bit[4:0] Day(1..31) */
            tmp >>= 5;
            mon = tmp & 0x0F;           /* bit[8:5] Month(1..12) */
            tmp >>= 4;
            year = (tmp & 0x7F) + 1980; /* bit[15:9] Year origin from 1980(0..127) */

            tmp = file_info.ftime;
            sec = (tmp & 0x1F) * 2;     /* bit[4:0] Second/2(0..29) */
            tmp >>= 5;
            min = tmp & 0x3F;           /* bit[10:5] Minute(0..59) */
            tmp >>= 6;
            hour = tmp & 0x1F;          /* bit[15:11] Hour(0..23) */

            rt_memset(&tm_file, 0, sizeof(tm_file));
            tm_file.tm_year = year - 1900; /* Years since 1900 */
            tm_file.tm_mon  = mon - 1;     /* Months *since* january: 0-11 */
            tm_file.tm_mday = day;         /* Day of the month: 1-31 */
            tm_file.tm_hour = hour;        /* Hours since midnight: 0-23 */
            tm_file.tm_min  = min;         /* Minutes: 0-59 */
            tm_file.tm_sec  = sec;         /* Seconds: 0-59 */

            st->st_mtime = timegm(&tm_file);
        } /* get st_mtime. */
#endif
    }

    return elm_result_to_dfs(result);
}
#endif

static const struct dfs_file_ops dfs_elm_fops =
{
    .open = dfs_elm_open,
    .close = dfs_elm_close,
    .ioctl = dfs_elm_ioctl,
    .read = dfs_elm_read,
#if !FF_FS_READONLY
    .write = dfs_elm_write,
    .flush = dfs_elm_flush,
#endif
    .lseek = dfs_elm_lseek,
#if FF_FS_MINIMIZE <= 1
    .getdents = dfs_elm_getdents,
#endif
    .poll = RT_NULL, /* poll interface */
};

static const struct dfs_filesystem_ops dfs_elm =
{
    "elm",
    DFS_FS_FLAG_DEFAULT,
    &dfs_elm_fops,

    .mount = dfs_elm_mount,
    .unmount = dfs_elm_unmount,
#if !FF_FS_READONLY && FF_USE_MKFS
    .mkfs = dfs_elm_mkfs,
#endif
#if !FF_FS_READONLY
    .statfs = dfs_elm_statfs,
#endif

#if !FF_FS_READONLY
    .unlink = dfs_elm_unlink,
#endif
#if FF_FS_MINIMIZE == 0
    .stat = dfs_elm_stat,
#endif
#if !FF_FS_READONLY
    .rename = dfs_elm_rename,
#endif
};

int elm_init(void)
{
    /* register fatfs file system */
    dfs_register(&dfs_elm);
    return 0;
}
INIT_COMPONENT_EXPORT(elm_init);

/* Disk Device Interface for ELM FatFs  */
#include "diskio.h"

#if defined(RAM_DISK_ENABLE)
#include "ram_disk/ram_disk.h"
#endif

#if defined(AIC_USB_HOST_EHCI_DRV)
#include "usb_disk/usb_disk.h"
#endif

#if defined(AIC_SDMC_DRV)
#include "sdmc_disk/sdmc_disk.h"
#endif

#if defined(AIC_SPINAND_DRV) && !defined(AIC_BOOTLOADER)
#include "spinand_disk/spinand_disk.h"
#endif

#if defined(AIC_SPINOR_DRV) && !defined(AIC_BOOTLOADER)
#include "spinor_disk/spinor_disk.h"
#endif

/* Initialize a Drive */
DSTATUS disk_initialize(BYTE pdrv)
{
    DSTATUS stat = STA_NOINIT;
    long dev_type = disk[pdrv].dev_type;
    const char *device_name = disk[pdrv].dev_name;
    void *handle = NULL;

    (void)handle;
    switch (dev_type) {
#if defined(RAM_DISK_ENABLE)
        case DTL(DEVICE_TYPE_RAM_DISK):
            stat = ram_disk_initialize(pdrv);
            return stat;
#endif
#if defined(AIC_USB_HOST_EHCI_DRV)
        case DTL(DEVICE_TYPE_USB_DISK):
            stat = usb_disk_initialize(pdrv);
            return stat;
#endif
#if defined(AIC_SDMC_DRV)
        case DTL(DEVICE_TYPE_SDMC_DISK):
            handle = sdmc_disk_initialize(device_name);
            stat = 0;
            break;
#endif
#if defined(AIC_SPINAND_DRV) && !defined(AIC_BOOTLOADER)
        case DTL(DEVICE_TYPE_SPINAND_DISK):
            handle = spinand_disk_initialize(device_name);
            stat = 0;
            break;
#endif
#if defined(AIC_SPINOR_DRV) && !defined(AIC_BOOTLOADER)
        case DTL(DEVICE_TYPE_SPINOR_DISK):
            handle = spinor_disk_initialize(device_name);
            stat = 0;
            break;
#endif
        default:
            break;
    }

    disk[pdrv].priv = handle;
    return stat;
}

/* Return Disk Status */
DSTATUS disk_status(BYTE pdrv)
{
    return 0;
}

/* Read Sector(s) */
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    DRESULT res = RES_PARERR;
    long dev_type = disk[pdrv].dev_type;
    void *handle = disk[pdrv].priv;

    switch (dev_type) {
#if defined(RAM_DISK_ENABLE)
        case DTL(DEVICE_TYPE_RAM_DISK):
            res = ram_disk_read(pdrv, buff, sector, count);
            return res;
#endif
#if defined(AIC_USB_HOST_EHCI_DRV)
        case DTL(DEVICE_TYPE_USB_DISK):
            res = usb_disk_read(pdrv, buff, sector, count);
            return res;
#endif
#if defined(AIC_SDMC_DRV)
        case DTL(DEVICE_TYPE_SDMC_DISK):
            res = sdmc_disk_read(handle, buff, sector, count);
            return res;
#endif
#if defined(AIC_SPINAND_DRV) && !defined(AIC_BOOTLOADER)
        case DTL(DEVICE_TYPE_SPINAND_DISK):
            res = spinand_disk_read(handle, buff, sector, count);
            return res;
#endif
#if defined(AIC_SPINOR_DRV) && !defined(AIC_BOOTLOADER)
        case DTL(DEVICE_TYPE_SPINOR_DISK):
            res = spinor_disk_read(handle, buff, sector, count);
            return res;
#endif
        default:
            break;
    }

    return res;
}

/* Write Sector(s) */
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    DRESULT res = RES_PARERR;
    long dev_type = disk[pdrv].dev_type;
    void *handle = disk[pdrv].priv;

    switch (dev_type) {
#if defined(RAM_DISK_ENABLE)
        case DTL(DEVICE_TYPE_RAM_DISK):
            res = ram_disk_write(pdrv, buff, sector, count);
            return res;
#endif
#if defined(AIC_USB_HOST_EHCI_DRV)
        case DTL(DEVICE_TYPE_USB_DISK):
            res = usb_disk_write(pdrv, buff, sector, count);
            return res;
#endif
#if defined(AIC_SDMC_DRV)
        case DTL(DEVICE_TYPE_SDMC_DISK):
            res = sdmc_disk_write(handle, buff, sector, count);
            return res;
#endif
#if defined(AIC_SPINAND_DRV) && !defined(AIC_BOOTLOADER)
        case DTL(DEVICE_TYPE_SPINAND_DISK):
            res = spinand_disk_write(handle, buff, sector, count);
            return res;
#endif
#if defined(AIC_SPINOR_DRV) && !defined(AIC_BOOTLOADER)
        case DTL(DEVICE_TYPE_SPINOR_DISK):
            res = spinor_disk_write(handle, buff, sector, count);
            return res;
#endif
        default:
            break;
    }

    return res;
}

/* Miscellaneous Functions */
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    DRESULT res = RES_PARERR;
    long dev_type = disk[pdrv].dev_type;
    void *handle = disk[pdrv].priv;

    switch (cmd) {
        case GET_DEVICE_TYPE:
            *(uint32_t *)buff = dev_type;
            return RES_OK;
            break;
        default:
            break;
    }

    switch (dev_type) {
#if defined(RAM_DISK_ENABLE)
        case DTL(DEVICE_TYPE_RAM_DISK):
            res = ram_disk_ioctl(pdrv, cmd, buff);
            return res;
#endif
#if defined(AIC_USB_HOST_EHCI_DRV)
        case DTL(DEVICE_TYPE_USB_DISK):
            res = usb_disk_ioctl(pdrv, cmd, buff);
            return res;
#endif
#if defined(AIC_SDMC_DRV)
        case DTL(DEVICE_TYPE_SDMC_DISK):
            res = sdmc_disk_ioctl(handle, cmd, buff);
            return res;
#endif
#if defined(AIC_SPINAND_DRV) && !defined(AIC_BOOTLOADER)
        case DTL(DEVICE_TYPE_SPINAND_DISK):
            res = spinand_disk_ioctl(handle, cmd, buff);
            return res;
#endif
#if defined(AIC_SPINOR_DRV) && !defined(AIC_BOOTLOADER)
        case DTL(DEVICE_TYPE_SPINOR_DISK):
            res = spinor_disk_ioctl(handle, cmd, buff);
            return res;
#endif
        default:
            break;
    }

    return res;
}

DWORD get_fattime(void)
{
    return (DWORD)aic_timer_get_us();
}

#if FF_FS_REENTRANT
int ff_cre_syncobj(BYTE drv, FF_SYNC_t *m)
{
    char name[8];
    aicos_mutex_t mutex;

    rt_snprintf(name, sizeof(name), "fat%d", drv);
    mutex = aicos_mutex_create();
    if (mutex != RT_NULL)
    {
        *m = mutex;
        return RT_TRUE;
    }

    return RT_FALSE;
}

int ff_del_syncobj(FF_SYNC_t m)
{
    if (m != RT_NULL)
        aicos_mutex_give(m);

    return RT_TRUE;
}

int ff_req_grant(FF_SYNC_t m)
{
    if (aicos_mutex_take(m, FF_FS_TIMEOUT) == RT_EOK)
        return RT_TRUE;

    return RT_FALSE;
}

void ff_rel_grant(FF_SYNC_t m)
{
    aicos_mutex_give(m);
}

#endif

/* Memory functions */
#if FF_USE_LFN == 3
/* Allocate memory block */
void *ff_memalloc(UINT size)
{
    return rt_malloc(size);
}

/* Free memory block */
void ff_memfree(void *mem)
{
    rt_free(mem);
}
#endif /* FF_USE_LFN == 3 */

