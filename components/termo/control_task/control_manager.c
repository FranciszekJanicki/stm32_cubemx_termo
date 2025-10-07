#include "control_manager.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "termo_common.h"
#include <string.h>

static char const* const TAG = "control_manager";

static mcp9808_err_t mcp9808_bus_initialize(void* user)
{
    control_config_t* config = user;

    return HAL_I2C_IsDeviceReady(config->mcp9808_i2c_bus,
                                 config->mcp9808_i2c_address,
                                 10,
                                 100) == HAL_OK
               ? MCP9808_ERR_OK
               : MCP9808_ERR_FAIL;
}

static mcp9808_err_t mcp9808_bus_write_data(void* user,
                                            uint8_t address,
                                            uint8_t const* data,
                                            size_t data_size)
{
    control_config_t* config = user;

    return HAL_I2C_Mem_Write(config->mcp9808_i2c_bus,
                             config->mcp9808_i2c_address,
                             address,
                             I2C_MEMADD_SIZE_8BIT,
                             data,
                             data_size,
                             100) == HAL_OK
               ? MCP9808_ERR_OK
               : MCP9808_ERR_FAIL;
}

static mcp9808_err_t mcp9808_bus_read_data(void* user,
                                           uint8_t address,
                                           uint8_t* data,
                                           size_t data_size)
{
    control_config_t* config = user;

    return HAL_I2C_Mem_Read(config->mcp9808_i2c_bus,
                            config->mcp9808_i2c_address,
                            address,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            data_size,
                            100) == HAL_OK
               ? MCP9808_ERR_OK
               : MCP9808_ERR_FAIL;
}

static mcp9808_err_t mcp9808_initialize_chip(mcp9808_t const* mcp9808)
{
    mcp9808_err_t err = MCP9808_ERR_OK;

    mcp9808_manufacturer_id_reg_t man_id = {0};
    mcp9808_device_id_reg_t dev_id = {0};

    err |= mcp9808_get_manufacturer_id_reg(mcp9808, &man_id);
    err |= mcp9808_get_device_id_reg(mcp9808, &dev_id);

    if (err != MCP9808_ERR_OK)
        return err;

    if (man_id.manufacturer_id != 0x0054 || (dev_id.device_id & 0xFF) != 0x04) {
        return MCP9808_ERR_FAIL;
    }

    mcp9808_config_reg_t cfg = {0};
    err |= mcp9808_get_config_reg(mcp9808, &cfg);

    cfg.t_hyst = 0;
    cfg.shdn = 0;
    cfg.crit_lock = 0;
    cfg.win_lock = 0;
    cfg.int_clear = 0;
    cfg.alert_stat = 0;
    cfg.alert_cnt = 0;
    cfg.alert_sel = 0;
    cfg.alert_pol = 0;
    cfg.aler_mod = 0;

    err |= mcp9808_set_config_reg(mcp9808, &cfg);

    mcp9808_resolution_reg_t res = {.resolution = 0x03};
    err |= mcp9808_set_resolution_reg(mcp9808, &res);

    mcp9808_t_upper_reg_t upper = {.t_upper = 30 << 4};
    mcp9808_t_lower_reg_t lower = {.t_lower = 20 << 4};
    mcp9808_t_crit_reg_t crit = {.t_crit = 40 << 4};

    err |= mcp9808_set_t_upper_reg(mcp9808, &upper);
    err |= mcp9808_set_t_lower_reg(mcp9808, &lower);
    err |= mcp9808_set_t_crit_reg(mcp9808, &crit);

    return err;
}

float32_t mcp9808_resolution_to_scale(mcp9808_resolution_t);

static inline bool control_manager_start_delta_timer(control_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    return HAL_TIM_Base_Start_IT(manager->config.delta_timer) == HAL_OK;
}

static inline bool control_manager_stop_delta_timer(control_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    return HAL_TIM_Base_Stop_IT(manager->config.delta_timer) == HAL_OK;
}

static inline bool control_manager_receive_control_notify(
    control_notify_t* notify)
{
    TERMO_ASSERT(notify != NULL);

    return xTaskNotifyWait(0x00,
                           CONTROL_NOTIFY_ALL,
                           (uint32_t*)notify,
                           pdMS_TO_TICKS(10)) == pdPASS;
}

static inline bool control_manager_has_control_event(void)
{
    return uxQueueMessagesWaiting(
               termo_queue_manager_get(TERMO_QUEUE_TYPE_CONTROL)) > 0UL;
}

static inline bool control_manager_receive_control_event(control_event_t* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueReceive(termo_queue_manager_get(TERMO_QUEUE_TYPE_CONTROL),
                         event,
                         pdMS_TO_TICKS(10)) == pdPASS;
}

termo_err_t control_manager_initialize(control_manager_t* manager,
                                       control_config_t const* config)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(config != NULL);

    memset(manager, 0, sizeof(*manager));
    memcpy(&manager->config, config, sizeof(manager->config));

    if (mcp9808_initialize(
            &manager->mcp9808,
            &(mcp9808_config_t){.scale = mcp9808_resolution_to_scale(0x03)},
            &(mcp9808_interface_t){.bus_user = &manager->config,
                                   .bus_initialize = mcp9808_bus_initialize,
                                   .bus_read_data = mcp9808_bus_read_data,
                                   .bus_write_data = mcp9808_bus_write_data}) !=
        MCP9808_ERR_OK) {
        TERMO_LOG(TAG, "Failed mcp9808_initialize!");
    }

    if (mcp9808_initialize_chip(&manager->mcp9808) != MCP9808_ERR_OK) {
        TERMO_LOG(TAG, "Failed mcp9808_initialize_chip!");
    }

    return TERMO_ERR_OK;
}

static termo_err_t control_manager_notify_delta_timer_handler(
    control_manager_t* manager)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);

    return TERMO_ERR_OK;
}

static termo_err_t control_manager_notify_temp_ready_handler(
    control_manager_t* manager)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);

    float measurement;
    if (mcp9808_get_temp_data_scaled(&manager->mcp9808, &measurement) !=
        MCP9808_ERR_OK) {
        TERMO_LOG(TAG, "Failed mcp9808_get_temp_data_scaled!");
        return TERMO_ERR_FAIL;
    }

    manager->measurement = measurement;

    return TERMO_ERR_OK;
}

static termo_err_t control_manager_notify_handler(control_manager_t* manager,
                                                  control_notify_t notify)
{
    TERMO_ASSERT(manager != NULL);

    if ((notify & CONTROL_NOTIFY_TEMP_READY) == CONTROL_NOTIFY_TEMP_READY) {
        TERMO_RET_ON_ERR(control_manager_notify_temp_ready_handler(manager));
    }
    if ((notify & CONTROL_NOTIFY_DELTA_TIMER) == CONTROL_NOTIFY_DELTA_TIMER) {
        TERMO_RET_ON_ERR(control_manager_notify_delta_timer_handler(manager));
    }

    return TERMO_ERR_UNKNOWN_NOTIFY;
}

static termo_err_t control_manager_event_start_handler(
    control_manager_t* manager,
    control_event_payload_start_t const* start)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(start != NULL);

    if (manager->is_running) {
        return TERMO_ERR_ALREADY_RUNNING;
    }

    if (!control_manager_start_delta_timer(manager)) {
        return TERMO_ERR_FAIL;
    }

    manager->is_running = true;

    return TERMO_ERR_OK;
}

static termo_err_t control_manager_event_stop_handler(
    control_manager_t* manager,
    control_event_payload_stop_t const* stop)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(stop != NULL);

    if (!manager->is_running) {
        return TERMO_ERR_NOT_RUNNING;
    }

    if (!control_manager_stop_delta_timer(manager)) {
        return TERMO_ERR_FAIL;
    }

    manager->is_running = false;

    return TERMO_ERR_OK;
}

static termo_err_t control_manager_event_reference_handler(
    control_manager_t* manager,
    control_event_payload_reference_t const* reference)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(reference != NULL);

    if (manager->reference == reference->temperature ||
        manager->delta_time == reference->sampling_period) {
        return TERMO_ERR_OK;
    }

    manager->delta_time = reference->sampling_period;
    manager->reference = reference->temperature;

    return TERMO_ERR_OK;
}

static termo_err_t control_manager_event_handler(control_manager_t* manager,
                                                 control_event_t const* event)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(event != NULL);

    switch (event->type) {
        case CONTROL_EVENT_TYPE_START: {
            return control_manager_event_start_handler(manager,
                                                       &event->payload.start);
        }
        case CONTROL_EVENT_TYPE_STOP: {
            return control_manager_event_stop_handler(manager,
                                                      &event->payload.stop);
        }
        case CONTROL_EVENT_TYPE_REFERENCE: {
            return control_manager_event_reference_handler(
                manager,
                &event->payload.reference);
        }
        default: {
            return TERMO_ERR_UNKNOWN_EVENT;
        }
    }
}

termo_err_t control_manager_process(control_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    control_notify_t notify;
    if (control_manager_receive_control_notify(&notify)) {
        TERMO_RET_ON_ERR(control_manager_notify_handler(manager, notify));
    }

    control_event_t event;
    while (control_manager_has_control_event()) {
        if (control_manager_receive_control_event(&event)) {
            TERMO_RET_ON_ERR(control_manager_event_handler(manager, &event));
        }
    }

    return TERMO_ERR_OK;
}
