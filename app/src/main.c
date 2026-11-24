#include <stdint.h>
#include <string.h>

#include "os.h"
#include "ux.h"
#include "io.h"

#include "globals.h"
#include "constants.h"
#include "sw.h"
#include "apdu/dispatcher.h"
#include "ui/display.h"
#include "ui/menu.h"

global_ctx_t G_context;
settings_t N_settings_real;

#ifdef HAVE_BAGL
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;
#endif

static void app_main(void) {
    io_init();

    ui_menu_main();

    for (;;) {
        BEGIN_TRY {
            TRY {
                io_recv_command();

                command_t cmd;
                memset(&cmd, 0, sizeof(cmd));

                if (G_io_apdu_buffer[0] == 0) {
                    continue;
                }

                cmd.cla  = G_io_apdu_buffer[0];
                cmd.ins  = G_io_apdu_buffer[1];
                cmd.p1   = G_io_apdu_buffer[2];
                cmd.p2   = G_io_apdu_buffer[3];
                cmd.lc   = G_io_apdu_buffer[4];
                cmd.data = G_io_apdu_buffer + 5;

                if (5 + cmd.lc > io_get_length()) {
                    io_send_sw(SW_WRONG_DATA_LENGTH);
                    continue;
                }

                apdu_dispatcher(&cmd);
            }
            CATCH(EXCEPTION_IO_RESET) {
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                io_send_sw(SW_INTERNAL_ERROR);
            }
            FINALLY {
            }
        }
        END_TRY;
    }
}

void coin_main(void) {
    for (;;) {
        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

#ifdef TARGET_NANOX
                G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif
                USB_power(0);
                USB_power(1);

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, NULL);
#endif

                app_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
                continue;
            }
            CATCH_ALL {
                break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

    os_sched_exit(-1);
}

__attribute__((section(".boot")))
int main(void) {
    __asm volatile("cpsie i");

    os_boot();

    BEGIN_TRY {
        TRY {
            coin_main();
        }
        CATCH_OTHER(e) {
            (void) e;
        }
        FINALLY {
        }
    }
    END_TRY;

    return 0;
}
