#include "stream_control.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "utils/quantization.h"

#define STREAM_CONTROL_STARTED_BIT BIT0

static const char *TAG = "stream_control";

/*
 * A mutex is a small lock around shared data.
 *
 * In this firmware, two FreeRTOS tasks care about the same two variables:
 *
 *   - TCP command task:
 *       writes is_running and destination_addr when START/STOP arrives.
 *       writes quantization_mode when SET_QUANTIZATION arrives.
 *
 *   - UDP sender task:
 *       reads is_running and destination_addr before sending frames.
 *       reads quantization_mode before encoding pixels.
 *
 * Without a mutex, one task could read these variables while the other task is
 * halfway through changing them. That is the multithreading problem here.
 *
 * Rule for this file:
 *
 *   xSemaphoreTake(state_mutex, portMAX_DELAY);
 *       touch shared state
 *   xSemaphoreGive(state_mutex);
 *
 * Only one task can be between Take and Give at a time.
 */
static SemaphoreHandle_t state_mutex;

/*
 * An event group is different from the mutex.
 *
 * The mutex protects data.
 * The event bit is a wake-up signal.
 *
 * The UDP sender waits on STREAM_CONTROL_STARTED_BIT while streaming is stopped.
 * START sets the bit, which wakes the UDP sender.
 * STOP clears the bit, so the next wait will sleep again.
 */
static EventGroupHandle_t state_events;

/*
 * Shared state protected by state_mutex.
 *
 * is_running:
 *   false -> do not read camera frames, do not send UDP thermal packets.
 *   true  -> read camera frames and send them to destination_addr.
 *
 * destination_addr:
 *   The viewer IP copied from the TCP peer that sent START.
 *   It uses the same byte order as sockaddr_in.sin_addr.s_addr.
 *
 * quantization_mode:
 *   The range used to compress temperatures into one byte.
 *   Default is mode 1, the original 10-45 C range.
 */
static bool is_running;
static uint32_t destination_addr;
static uint8_t quantization_mode;

void stream_control_init(void)
{
    /*
     * Create the mutex before any task can call start/stop/read helpers.
     * configASSERT stops the firmware loudly if allocation fails.
     */
    state_mutex = xSemaphoreCreateMutex();
    configASSERT(state_mutex != NULL);

    /*
     * Create the event group used to wake the UDP sender on START.
     */
    state_events = xEventGroupCreate();
    configASSERT(state_events != NULL);

    /*
     * Boot starts with the camera stream stopped and no viewer destination.
     */
    is_running = false;
    destination_addr = 0;
    quantization_mode = QUANTIZATION_MODE_10_45;

    /*
     * Make sure the START bit is clear, so stream_control_wait_started()
     * blocks until the first real START command arrives.
     */
    xEventGroupClearBits(
        state_events,
        STREAM_CONTROL_STARTED_BIT
    );
}

void stream_control_start(uint32_t destination_addr_s_addr)
{
    /*
     * Take the mutex before writing shared state.
     * If the UDP sender is reading this state right now, this call waits until
     * the UDP sender gives the mutex back.
     */
    xSemaphoreTake(state_mutex, portMAX_DELAY);

    /*
     * These two assignments should be seen as one state update:
     * "streaming is enabled, and this is where frames should go".
     */
    destination_addr = destination_addr_s_addr;
    is_running = true;

    /*
     * Give the mutex back quickly. Do not log or do socket work while holding
     * the mutex; keep the protected section small.
     */
    xSemaphoreGive(state_mutex);

    /*
     * Wake the UDP sender if it is blocked waiting for the first START.
     */
    xEventGroupSetBits(
        state_events,
        STREAM_CONTROL_STARTED_BIT
    );

    ESP_LOGI(TAG, "Thermal stream enabled");
}

void stream_control_stop(void)
{
    /*
     * Take the mutex before changing is_running so the UDP sender cannot read
     * a half-updated state.
     */
    xSemaphoreTake(state_mutex, portMAX_DELAY);

    /*
     * The destination IP is left in memory, but ignored while is_running=false.
     * A later START will overwrite it with the newest TCP peer IP.
     */
    is_running = false;

    /*
     * Release the shared state before touching the event bit.
     */
    xSemaphoreGive(state_mutex);

    /*
     * Clear the bit so the UDP sender returns to sleeping once it notices STOP.
     */
    xEventGroupClearBits(
        state_events,
        STREAM_CONTROL_STARTED_BIT
    );

    ESP_LOGI(TAG, "Thermal stream disabled");
}

void stream_control_wait_started(void)
{
    /*
     * This is the sleeping point for the UDP sender.
     *
     * pdFALSE: do not automatically clear the bit when this wait returns.
     * pdTRUE:  wait until all requested bits are set; here there is only one.
     * portMAX_DELAY: wait forever until START arrives.
     */
    xEventGroupWaitBits(
        state_events,
        STREAM_CONTROL_STARTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );
}

bool stream_control_is_running(void)
{
    bool running;

    /*
     * Copy is_running while holding the mutex.
     * The returned bool is a snapshot; the caller must check again later if it
     * needs fresh state.
     */
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    running = is_running;
    xSemaphoreGive(state_mutex);

    return running;
}

bool stream_control_get_destination(uint32_t *destination_addr_s_addr)
{
    bool running;

    /*
     * Copy both pieces of shared state under one mutex lock.
     * This prevents a mixed result such as:
     *   old destination IP + new running state
     */
    xSemaphoreTake(state_mutex, portMAX_DELAY);

    running = is_running;

    /*
     * Only provide a destination while streaming is active.
     * When stopped, the caller should not send frames anywhere.
     */
    if (running) {
        *destination_addr_s_addr = destination_addr;
    }

    /*
     * Release the mutex before returning to the caller.
     */
    xSemaphoreGive(state_mutex);

    return running;
}

void stream_control_set_quantization_mode(uint8_t mode)
{
    /*
     * The caller validates the mode before this function is called.
     * This module only stores the selected shared value.
     */
    xSemaphoreTake(state_mutex, portMAX_DELAY);

    quantization_mode = mode;

    xSemaphoreGive(state_mutex);

    ESP_LOGI(TAG, "Quantization mode set to %u", mode);
}

uint8_t stream_control_get_quantization_mode(void)
{
    uint8_t mode;

    /*
     * Return a snapshot of the current mode.
     * If TCP changes the mode right after this function returns, the next frame
     * will see the new mode.
     */
    xSemaphoreTake(state_mutex, portMAX_DELAY);
    mode = quantization_mode;
    xSemaphoreGive(state_mutex);

    return mode;
}
