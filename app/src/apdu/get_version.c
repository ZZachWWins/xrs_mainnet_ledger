#include "get_version.h"
#include "../constants.h"
#include "../sw.h"
#include "io.h"

int handler_get_version(void) {
    static const uint8_t version[] = {
        1,  // major
        0,  // minor
        0,  // patch
    };
    return io_send_response_pointer(version, sizeof(version), SW_OK);
}
