/**
 * @file shell_commands.c
 * @brief Shell command table for RF_IO_RS485_V3.
 *
 * Add new commands here. Each entry: { "name", handler_func, "help text" }.
 * The g_shell_commands array is referenced by cli_shell via extern.
 *
 * Built-in commands: help (handled by cli_shell_help_handler).
 * App commands: reboot, get_io, set_do, get_zigbee — extend as needed.
 */

#include "cli_shell.h"
#include "board.h"
#include "bsp_board_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "usb_rs485.h"
#include <stdio.h>
#include <stdlib.h>

/* Command handlers  */

static int cmd_reboot(ShellContext_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    cli_shell_put_line(shell, "Rebooting...");
    vTaskDelay(pdMS_TO_TICKS(100));
    bsp_restart();
    return 0;
}

static int cmd_get_io(ShellContext_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    cli_shell_printf(shell, "DI1=%d DI2=%d DI3=%d DI4=%d",
        bsp_get_input(1), bsp_get_input(2),
        bsp_get_input(3), bsp_get_input(4));
    return 0;
}

static int cmd_set_do(ShellContext_t *shell, int argc, char *argv[])
{
    if (argc != 3) {
        cli_shell_put_line(shell, "Usage: set_do <1-4> <0|1>");
        return -1;
    }
    int ch  = atoi(argv[1]);
    int val = atoi(argv[2]);
    if (ch < 1 || ch > 4) {
        cli_shell_put_line(shell, "Error: channel must be 1-4");
        return -1;
    }
    if (val) bsp_output_on(ch);
    else     bsp_output_off(ch);
    cli_shell_printf(shell, "DO%d = %d", ch, val);
    return 0;
}

static int cmd_bridge_on(ShellContext_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    usb_rs485_enable();
    cli_shell_put_line(shell, "USB-RS485 bridge ENABLED (Modbus suspended)");
    return 0;
}

static int cmd_bridge_off(ShellContext_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    usb_rs485_disable();
    cli_shell_put_line(shell, "USB-RS485 bridge DISABLED (Modbus resumed)");
    return 0;
}

static int cmd_bridge_status(ShellContext_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    cli_shell_printf(shell, "Bridge: %s", usb_rs485_is_enabled() ? "ENABLED" : "DISABLED");
    return 0;
}

/* Command table */

static const Cli_Shell_Cmd s_shell_commands[] = {
    { "help",          cli_shell_help_handler, "List all commands"                          },
    { "reboot",        cmd_reboot,             "Reboot the device"                          },
    { "get_io",        cmd_get_io,             "Read DI1-DI4 states"                        },
    { "set_do",        cmd_set_do,             "set_do <ch 1-4> <val 0|1>"                  },
    { "bridge_on",     cmd_bridge_on,          "Manually enable USB-RS485 bridge"           },
    { "bridge_off",    cmd_bridge_off,         "Manually disable USB-RS485 bridge"          },
    { "bridge_status", cmd_bridge_status,      "Show current bridge on/off state"           },
};


/* Exported symbols required by cli_shell */
const Cli_Shell_Cmd *const g_shell_commands    = s_shell_commands;
const size_t               g_num_shell_commands = sizeof(s_shell_commands) / sizeof(s_shell_commands[0]);