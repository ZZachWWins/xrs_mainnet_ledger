#include "menu.h"
#include "../globals.h"

void ui_menu_main(void) {
#ifdef HAVE_NBGL
    extern void nbgl_menu_main(void);
    nbgl_menu_main();
#elif defined(HAVE_BAGL)
    extern void bagl_menu_main(void);
    bagl_menu_main();
#endif
}
