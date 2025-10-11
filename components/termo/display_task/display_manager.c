#include "display_manager.h"
#include "termo_common.h"
#include <string.h>

static char const* const TAG = "display_manager";

static sh1107_err_t sh1107_bus_transmit_data(void* user,
                                             uint8_t const* data,
                                             size_t data_size)
{
    display_config_t* config = (display_config_t*)user;

    HAL_GPIO_WritePin(config->sh1107_slave_select_gpio,
                      config->sh1107_slave_select_pin,
                      GPIO_PIN_RESET);
    HAL_StatusTypeDef err =
        HAL_SPI_Transmit(config->sh1107_spi_bus, data, data_size, 100);
    HAL_GPIO_WritePin(config->sh1107_slave_select_gpio,
                      config->sh1107_slave_select_pin,
                      GPIO_PIN_SET);

    return err == HAL_OK ? SH1107_ERR_OK : SH1107_ERR_FAIL;
}

static sh1107_err_t sh1107_bus_initialize(void* user)
{
    return SH1107_ERR_OK;
}

static sh1107_err_t sh1107_bus_deinitialize(void* user)
{
    return SH1107_ERR_OK;
}

static sh1107_err_t sh1107_gpio_initialize(void* user)
{
    display_config_t* config = (display_config_t*)user;

    HAL_GPIO_WritePin(config->sh1107_slave_select_gpio,
                      config->sh1107_slave_select_pin,
                      GPIO_PIN_RESET);

    return SH1107_ERR_OK;
}

static sh1107_err_t sh1107_gpio_deinitialize(void* user)
{
    return SH1107_ERR_OK;
}

static sh1107_err_t sh1107_gpio_write(void* user, uint32_t pin, bool state)
{
    display_config_t* config = (display_config_t*)user;

    HAL_GPIO_WritePin(config->sh1107_control_gpio, pin, (GPIO_PinState)state);

    return SH1107_ERR_OK;
}

static sh1107_err_t sh1107_initialize_chip(sh1107_t* sh1107)
{
    sh1107_gpio_write(sh1107->interface.bus_user, sh1107->config.reset_pin, 0);
    HAL_Delay(100);
    sh1107_gpio_write(sh1107->interface.bus_user, sh1107->config.reset_pin, 1);
    HAL_Delay(100);

    uint8_t cmd = (0xAE); // Display OFF
    sh1107_bus_transmit_data(sh1107->interface.bus_user, &cmd, 1);
    cmd = (0xD5); // Set Display Clock Divide Ratio
    sh1107_bus_transmit_data(sh1107->interface.bus_user, &cmd, 1);
    cmd = (0x80);
    sh1107_bus_transmit_data(sh1107->interface.bus_user, &cmd, 1);
    cmd = (0xA8); // Set Multiplex Ratio
    sh1107_bus_transmit_data(sh1107->interface.bus_user, &cmd, 1);
    cmd = (0x7F);
    sh1107_bus_transmit_data(sh1107->interface.bus_user, &cmd, 1);
    cmd = (0xD3); // Display Offset
    sh1107_bus_transmit_data(sh1107->interface.bus_user, &cmd, 1);
    cmd = (0x00);
    sh1107_bus_transmit_data(sh1107->interface.bus_user, &cmd, 1);
    cmd = (0x40); // Display Start Line
    sh1107_bus_transmit_data(sh1107->interface.bus_user, &cmd, 1);
    cmd = (0x8D); // Charge Pump
    sh1107_bus_transmit_data(sh1107->interface.bus_user, &cmd, 1);
    cmd = (0x14);
    sh1107_bus_transmit_data(sh1107->interface.bus_user, &cmd, 1);
    cmd = (0xAF); // Display // ON
    sh1107_bus_transmit_data(sh1107->interface.bus_user, &cmd, 1);

    return SH1107_ERR_OK;
}

static inline bool display_manager_send_system_event(
    system_event_t const* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueSend(termo_queue_manager_get(TERMO_QUEUE_TYPE_SYSTEM),
                      event,
                      pdMS_TO_TICKS(10)) == pdPASS;
}

static inline bool display_manager_receive_display_notify(
    display_notify_t* notify)
{
    TERMO_ASSERT(notify != NULL);

    return xTaskNotifyWait(0x00,
                           DISPLAY_NOTIFY_ALL,
                           (uint32_t*)notify,
                           pdMS_TO_TICKS(10)) == pdPASS;
}

static inline bool display_manager_has_display_event(void)
{
    return uxQueueMessagesWaiting(
               termo_queue_manager_get(TERMO_QUEUE_TYPE_DISPLAY)) > 0UL;
}

static inline bool display_manager_receive_display_event(display_event_t* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueReceive(termo_queue_manager_get(TERMO_QUEUE_TYPE_DISPLAY),
                         event,
                         pdMS_TO_TICKS(10)) == pdPASS;
}

static termo_err_t display_manager_notify_handler(display_manager_t* manager,
                                                  display_notify_t notify)
{
    TERMO_ASSERT(manager != NULL);

    return TERMO_ERR_OK;
}

static termo_err_t display_manager_event_start_handler(
    display_manager_t* manager,
    display_event_payload_start_t const* start)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(start != NULL);

    if (manager->is_running) {
        return TERMO_ERR_ALREADY_RUNNING;
    }

    if (!display_manager_send_system_event(
            &(system_event_t){.type = SYSTEM_EVENT_TYPE_STARTED,
                              .origin = SYSTEM_EVENT_ORIGIN_DISPLAY})) {
        return TERMO_ERR_FAIL;
    }

    manager->is_running = true;

    return TERMO_ERR_OK;
}

static termo_err_t display_manager_event_stop_handler(
    display_manager_t* manager,
    display_event_payload_stop_t const* stop)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(stop != NULL);

    if (!manager->is_running) {
        return TERMO_ERR_NOT_RUNNING;
    }

    if (!display_manager_send_system_event(
            &(system_event_t){.type = SYSTEM_EVENT_TYPE_STOPPED,
                              .origin = SYSTEM_EVENT_ORIGIN_DISPLAY})) {
        return TERMO_ERR_FAIL;
    }

    manager->is_running = false;

    return TERMO_ERR_OK;
}

static termo_err_t display_manager_event_reference_handler(
    display_manager_t* manager,
    display_event_payload_reference_t const* reference)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(reference != NULL);

    sh1107_draw_string_formatted(
        &manager->sh1107,
        0,
        0,
        "Reference temperature: %.2f C, sampling time: %.2f s",
        reference->temperature,
        reference->sampling_time);

    return TERMO_ERR_OK;
}

static termo_err_t display_manager_event_measure_handler(
    display_manager_t* manager,
    display_event_payload_measure_t const* measure)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(measure != NULL);

    sh1107_draw_string_formatted(
        &manager->sh1107,
        0,
        10,
        "Measure temperature: %.2f C, humidity: %.2f %%, pressure: %.2f hPa",
        measure->temperature,
        measure->humidity,
        measure->pressure);

    return TERMO_ERR_OK;
}

static termo_err_t display_manager_event_handler(display_manager_t* manager,
                                                 display_event_t const* event)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(event != NULL);

    switch (event->type) {
        case DISPLAY_EVENT_TYPE_START: {
            return display_manager_event_start_handler(manager,
                                                       &event->payload.start);
        }
        case DISPLAY_EVENT_TYPE_STOP: {
            return display_manager_event_stop_handler(manager,
                                                      &event->payload.stop);
        }
        case DISPLAY_EVENT_TYPE_REFERENCE: {
            return display_manager_event_reference_handler(
                manager,
                &event->payload.reference);
        }
        default: {
            return TERMO_ERR_UNKNOWN_EVENT;
        }
    }
}

termo_err_t display_manager_process(display_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    display_notify_t notify;
    if (display_manager_receive_display_notify(&notify)) {
        TERMO_RET_ON_ERR(display_manager_notify_handler(manager, notify));
    }

    display_event_t event;
    while (display_manager_has_display_event()) {
        if (display_manager_receive_display_event(&event)) {
            TERMO_RET_ON_ERR(display_manager_event_handler(manager, &event));
        }
    }

    return TERMO_ERR_OK;
}

termo_err_t display_manager_initialize(display_manager_t* manager,
                                       display_config_t const* config)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(config != NULL);

    manager->is_running = false;
    manager->config = *config;

    memset(manager->sh1107_frame_buffer,
           0,
           sizeof(manager->sh1107_frame_buffer));

    sh1107_initialize(
        &manager->sh1107,
        &(sh1107_config_t){.font_buffer = (uint8_t*)font5x7,
                           .font_chars = FONT5X7_CHARS,
                           .font_height = FONT5X7_HEIGHT,
                           .font_width = FONT5X7_WIDTH,
                           .control_pin = config->sh1107_control_pin,
                           .reset_pin = config->sh1107_reset_pin,
                           .frame_buffer = manager->sh1107_frame_buffer,
                           .frame_width = SH1107_SCREEN_WIDTH,
                           .frame_height = SH1107_SCREEN_HEIGHT},
        &(sh1107_interface_t){.bus_user = &manager->config,
                              .bus_initialize = sh1107_bus_initialize,
                              .bus_deinitialize = sh1107_bus_deinitialize,
                              .bus_transmit = sh1107_bus_transmit_data,
                              .gpio_user = &manager->config,
                              .gpio_initialize = sh1107_gpio_initialize,
                              .gpio_deinitialize = sh1107_gpio_deinitialize,
                              .gpio_write = sh1107_gpio_write});
    sh1107_initialize_chip(&manager->sh1107);
    sh1107_draw_string(&manager->sh1107, 0, 0, "DUPA ZBITA");
    sh1107_draw_string(&manager->sh1107, 30, 30, "DUPA CIPA");
    sh1107_display_frame_buffer(&manager->sh1107);

    if (!display_manager_send_system_event(
            &(system_event_t){.type = SYSTEM_EVENT_TYPE_READY,
                              .origin = SYSTEM_EVENT_ORIGIN_DISPLAY})) {
        return TERMO_ERR_FAIL;
    }

    return TERMO_ERR_OK;
}
