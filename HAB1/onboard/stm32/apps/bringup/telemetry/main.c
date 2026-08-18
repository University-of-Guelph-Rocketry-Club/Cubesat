/* Standalone telemetry bring-up application. */

#include "../../../../../shared/telemetry/telemetry_record.h"

int main(void)
{
    hab_telemetry_record_t record = {0};
    record.packet_version = HAB_TELEMETRY_PACKET_VERSION;
    (void)record;

    for (;;) {
        /* TODO: encode and transmit only validated fields. */
    }
}
