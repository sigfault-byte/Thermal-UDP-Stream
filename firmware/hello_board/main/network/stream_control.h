#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Shared START/STOP state for the thermal stream.
 *
 * Why this file exists:
 *
 *   - The TCP command task receives START/STOP packets from the Qt viewer.
 *   - The UDP sender task reads camera frames and sends them to the viewer.
 *
 * Those are two different FreeRTOS tasks, so they can run at different moments.
 * This file is the one small shared "control panel" between them.
 */
void stream_control_init(void);

/*
 * Enable streaming and store the UDP destination IP address.
 *
 * Called by the TCP command task when it receives START.
 * The UDP sender will wake up and send frames to this address on port 5005.
 *
 * destination_addr_s_addr is the sin_addr.s_addr value from a sockaddr_in.
 */
void stream_control_start(uint32_t destination_addr_s_addr);

/*
 * Disable streaming. Discovery and TCP command handling continue running.
 *
 * Called by the TCP command task when it receives STOP.
 * The UDP sender will notice this and go back to sleep.
 */
void stream_control_stop(void);

/*
 * Block the caller until streaming is enabled by START.
 *
 * Called by the UDP sender task. This is what prevents camera reads while the
 * viewer has not asked for frames yet.
 */
void stream_control_wait_started(void);

/*
 * Return true while START is active and STOP has not disabled streaming.
 *
 * The UDP sender checks this between frame reads so STOP can interrupt the
 * stream cleanly.
 */
bool stream_control_is_running(void);

/*
 * Copy the current UDP destination IP address.
 *
 * The function copies the address while holding the mutex, then releases the
 * mutex before returning. Callers should not touch the module's global state
 * directly.
 *
 * Returns false when streaming is not currently enabled.
 */
bool stream_control_get_destination(uint32_t *destination_addr_s_addr);
