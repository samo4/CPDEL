#pragma once

/* Start the SCPI server listening on port 5025.
   This function creates a FreeRTOS task that accepts telnet connections,
   decodes SCPI commands, and publishes them to the app_bus.
   Safe to call only once at startup. */
void scpi_server_start(void);
void scpi_server_stop(void);
