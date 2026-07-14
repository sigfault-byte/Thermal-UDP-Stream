#include "camera/mlx90640_runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "camera/MLX90640_API.h"

/*
 * This mutex protects runtime I2C access to the MLX90640.
 *
 * It is separate from stream_control's mutex:
 *
 *   - stream_control protects tiny software state such as START/STOP,
 *     destination IP, quantization mode, and requested refresh rate.
 *
 *   - camera_mutex protects the physical camera bus transaction.
 *
 * Keeping them separate avoids holding the stream-control lock while waiting
 * for slow I2C reads.
 */
static SemaphoreHandle_t camera_mutex;

void mlx90640_runtime_init(void)
{
    camera_mutex = xSemaphoreCreateMutex();
    configASSERT(camera_mutex != NULL);
}

int mlx90640_runtime_get_frame_data(
    uint8_t slave_addr,
    uint16_t *frame_data
)
{
    int result;

    /*
     * MLX90640_GetFrameData performs several I2C reads/writes internally:
     * wait status, clear status, read pixels, read aux data, read control.
     *
     * Hold the mutex for the whole helper call so a refresh-rate command
     * cannot write the control register halfway through a frame read.
     */
    xSemaphoreTake(camera_mutex, portMAX_DELAY);
    result = MLX90640_GetFrameData(slave_addr, frame_data);
    xSemaphoreGive(camera_mutex);

    return result;
}

int mlx90640_runtime_set_refresh_rate(
    uint8_t slave_addr,
    uint8_t refresh_rate_code
)
{
    int result;

    /*
     * Refresh-rate writes touch the same control register that frame reads copy
     * into frameData[832]. Serialize this with frame reads so the camera state
     * changes between frames, not in the middle of one.
     */
    xSemaphoreTake(camera_mutex, portMAX_DELAY);
    result = MLX90640_SetRefreshRate(slave_addr, refresh_rate_code);
    xSemaphoreGive(camera_mutex);

    return result;
}
