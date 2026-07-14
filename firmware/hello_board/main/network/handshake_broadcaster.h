#pragma once

/*
 * FreeRTOS task entry point for periodic UDP discovery broadcasts.
 * The task sends an obfuscated handshake packet every five seconds.
 */
void handshake_broadcaster_task(void *pvParameters);
