/*
 * Copyright (c) 2020-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __CONSOLE_H_
#define __CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#define CONSOLE_OK        (0)
#define CONSOLE_ERROR     (-1)
#define CONSOLE_QUIT      (-2)
#define CONSOLE_ERROR_ARG (-3)

#ifdef AIC_CONSOLE_USING_AUTH
#ifndef AIC_CONSOLE_PASSWORD_MAX
#define AIC_CONSOLE_PASSWORD_MAX 32
#endif
#ifndef AIC_CONSOLE_PASSWORD_MIN
#define AIC_CONSOLE_PASSWORD_MIN 3
#endif
#ifndef AIC_CONSOLE_DEFAULT_PASSWORD
#define AIC_CONSOLE_DEFAULT_PASSWORD "bootloader"
#endif
#endif

#define __console_init \
    __attribute__((used)) __attribute__((section(".tinyspl.console.cmd")))

struct console_init_cmd {
    const char *cmdname;
    int (*proc)(int argc, char **argv);
    uint8_t mode;
    bool initialized;
    const char *help;
};

#define CONSOLE_CMD(_cmd, _proc, _help)                                      \
    __console_init struct console_init_cmd console_init_cmd_##_cmd = {       \
        .cmdname = #_cmd, .proc = _proc, .initialized = false, .help = _help \
    };

struct console_cmd {
    char *cmdname;
    int (*proc)(int argc, char **argv);
    char *help;
    struct console_cmd *next;
};

void console_init(void);
int console_loop(void);
void console_set_usrname(const char *usrname);
int console_run_cmd(const char *cmdstr);
struct console_cmd *console_find_cmd_by_name(const char *name);
struct console_cmd *console_register_cmd(const char *cmd,
                                         int (*proc)(int, char **),
                                         const char *help);
int console_unregister_cmd(const char *cmd);
int console_set_bootcmd(const char *cmdstr);
int console_get_ctrlc(void);
int console_confirm_yesno(void);

#ifdef __cplusplus
}
#endif

#endif
