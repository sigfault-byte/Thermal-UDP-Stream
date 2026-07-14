#pragma once

/*
 * FreeRTOS task entry point for the TCP command server.
 * The task listens on port 5006 and logs received bytes until the command
 * packet protocol is implemented.
 */
void tcp_command_server_task(void *pvParameters);
