#include "termo_manager.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "termo_common.h"
#include <string.h>

static char const* const TAG = "termo_manager";

static mcp9808_err_t mcp9808_bus_initialize(void* user)
{
    termo_config_t* config = user;

    return HAL_I2C_IsDeviceReady(config->mcp9808_i2c_bus,
                                 config->mcp9808_i2c_address << 1U,
                                 10,
                                 100) == HAL_OK
               ? MCP9808_ERR_OK
               : MCP9808_ERR_FAIL;
}

static mcp9808_err_t mcp9808_bus_deinitialize(void* user)
{
    return MCP9808_ERR_OK;
}

static mcp9808_err_t mcp9808_bus_write_data(void* user,
                                            uint8_t address,
                                            uint8_t const* data,
                                            size_t data_size)
{
    termo_config_t* config = user;

    return HAL_I2C_Mem_Write(config->mcp9808_i2c_bus,
                             config->mcp9808_i2c_address << 1U,
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
    termo_config_t* config = user;

    return HAL_I2C_Mem_Read(config->mcp9808_i2c_bus,
                            config->mcp9808_i2c_address << 1U,
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
    mcp9808_manufacturer_id_reg_t man_id = {0};
    mcp9808_device_id_reg_t dev_id = {0};

    mcp9808_get_manufacturer_id_reg(mcp9808, &man_id);
    mcp9808_get_device_id_reg(mcp9808, &dev_id);

    if (man_id.manufacturer_id != 0x0054 || (dev_id.device_id & 0xFF) != 0x04) {
        return MCP9808_ERR_FAIL;
    }

    mcp9808_config_reg_t cfg = {0};
    mcp9808_get_config_reg(mcp9808, &cfg);

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

    mcp9808_set_config_reg(mcp9808, &cfg);

    mcp9808_resolution_reg_t res = {.resolution = 0x03};
    mcp9808_set_resolution_reg(mcp9808, &res);

    mcp9808_t_upper_reg_t upper = {.t_upper = 30 << 4};
    mcp9808_t_lower_reg_t lower = {.t_lower = 20 << 4};
    mcp9808_t_crit_reg_t crit = {.t_crit = 40 << 4};

    mcp9808_set_t_upper_reg(mcp9808, &upper);
    mcp9808_set_t_lower_reg(mcp9808, &lower);
    mcp9808_set_t_crit_reg(mcp9808, &crit);

    return MCP9808_ERR_OK;
}

float32_t mcp9808_resolution_to_scale(mcp9808_resolution_t);

static inline bool termo_manager_start_pwm_timer(termo_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    return HAL_TIM_PWM_Start(manager->config.pwm_timer,
                             manager->config.pwm_channel) == HAL_OK;
}

static inline bool termo_manager_stop_pwm_timer(termo_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    return HAL_TIM_PWM_Stop(manager->config.pwm_timer,
                            manager->config.pwm_channel) == HAL_OK;
}

static inline bool termo_manager_set_pwm_timer_compare(termo_manager_t* manager,
                                                       uint32_t compare)
{
    TERMO_ASSERT(manager != NULL);

    __HAL_TIM_SET_COMPARE(manager->config.pwm_timer,
                          manager->config.pwm_channel,
                          compare & 0xFFFFU);
    return true;
}

static inline bool termo_manager_start_delta_timer(termo_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    return HAL_TIM_Base_Start_IT(manager->config.delta_timer) == HAL_OK;
}

static inline bool termo_manager_stop_delta_timer(termo_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    return HAL_TIM_Base_Stop_IT(manager->config.delta_timer) == HAL_OK;
}

static inline bool termo_manager_send_system_event(system_event_t const* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueSend(termo_queue_manager_get(TERMO_QUEUE_TYPE_SYSTEM),
                      event,
                      pdMS_TO_TICKS(10)) == pdPASS;
}

static inline bool termo_manager_receive_termo_notify(termo_notify_t* notify)
{
    TERMO_ASSERT(notify != NULL);

    return xTaskNotifyWait(0x00,
                           TERMO_NOTIFY_ALL,
                           (uint32_t*)notify,
                           pdMS_TO_TICKS(10)) == pdPASS;
}

static inline bool termo_manager_has_termo_event(void)
{
    return uxQueueMessagesWaiting(
               termo_queue_manager_get(TERMO_QUEUE_TYPE_TERMO)) > 0UL;
}

static inline bool termo_manager_receive_termo_event(termo_event_t* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueReceive(termo_queue_manager_get(TERMO_QUEUE_TYPE_TERMO),
                         event,
                         pdMS_TO_TICKS(10)) == pdPASS;
}

static termo_err_t termo_manager_notify_delta_timer_handler(
    termo_manager_t* manager)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);

    float termo_temperature;
    float error_temperature = manager->reference - manager->measurement;
    if (pid_regulator_get_sat_control(&manager->pid,
                                      error_temperature,
                                      manager->delta_time,
                                      &termo_temperature) !=
        PID_REGULATOR_ERR_OK) {
        TERMO_LOG(TAG, "Failed pid_regulator_get_sat_control!");
        return TERMO_ERR_FAIL;
    }

    uint32_t compare =
        (uint32_t)(termo_temperature *
                   (float)manager->config.pwm_timer->Init.Period /
                   mcp9808_resolution_to_scale(0x03));

    if (compare > manager->config.pwm_timer->Init.Period) {
        compare = manager->config.pwm_timer->Init.Period;
    } else if (compare < 0) {
        compare = 0;
    }

    if (!termo_manager_set_pwm_timer_compare(manager, compare)) {
        return TERMO_ERR_FAIL;
    }

    TERMO_LOG(TAG,
              "Ref: %.2fC, Meas: %.2fC, Err: %.2fC, Ctrl: %.2fC, Comp: %lu",
              manager->reference,
              manager->measurement,
              error_temperature,
              termo_temperature,
              compare);

    return TERMO_ERR_OK;
}

static termo_err_t termo_manager_notify_temp_ready_handler(
    termo_manager_t* manager)
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

    system_event_t event = {
        .origin = SYSTEM_EVENT_ORIGIN_TERMO,
        .type = SYSTEM_EVENT_TYPE_TERMO_MEASURE,
        .payload.termo_measure = {.temperature = measurement,
                                  .humidity = 0.0F,
                                  .pressure = 0.0F}};
    if (!termo_manager_send_system_event(&event)) {
        return TERMO_ERR_FAIL;
    }

    return TERMO_ERR_OK;
}

static termo_err_t termo_manager_notify_handler(termo_manager_t* manager,
                                                termo_notify_t notify)
{
    TERMO_ASSERT(manager != NULL);

    if ((notify & TERMO_NOTIFY_TEMP_READY) == TERMO_NOTIFY_TEMP_READY) {
        TERMO_RET_ON_ERR(termo_manager_notify_temp_ready_handler(manager));
    }
    if ((notify & TERMO_NOTIFY_DELTA_TIMER) == TERMO_NOTIFY_DELTA_TIMER) {
        TERMO_RET_ON_ERR(termo_manager_notify_delta_timer_handler(manager));
    }

    return TERMO_ERR_OK;
}

static termo_err_t termo_manager_event_start_handler(
    termo_manager_t* manager,
    termo_event_payload_start_t const* start)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(start != NULL);

    if (manager->is_running) {
        return TERMO_ERR_ALREADY_RUNNING;
    }

    if (!termo_manager_start_delta_timer(manager)) {
        return TERMO_ERR_FAIL;
    }

    if (!termo_manager_start_pwm_timer(manager)) {
        return TERMO_ERR_FAIL;
    }

    system_event_t event = {.origin = SYSTEM_EVENT_ORIGIN_TERMO,
                            .type = SYSTEM_EVENT_TYPE_TERMO_STARTED,
                            .payload.termo_started = {}};
    if (!termo_manager_send_system_event(&event)) {
        return TERMO_ERR_FAIL;
    }

    manager->is_running = true;

    return TERMO_ERR_OK;
}

static termo_err_t termo_manager_event_stop_handler(
    termo_manager_t* manager,
    termo_event_payload_stop_t const* stop)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(stop != NULL);

    if (!manager->is_running) {
        return TERMO_ERR_NOT_RUNNING;
    }

    if (!termo_manager_stop_delta_timer(manager)) {
        return TERMO_ERR_FAIL;
    }

    if (!termo_manager_stop_pwm_timer(manager)) {
        return TERMO_ERR_FAIL;
    }

    system_event_t event = {.origin = SYSTEM_EVENT_ORIGIN_TERMO,
                            .type = SYSTEM_EVENT_TYPE_TERMO_STOPPED,
                            .payload.termo_stopped = {}};
    if (!termo_manager_send_system_event(&event)) {
        return TERMO_ERR_FAIL;
    }

    manager->is_running = false;

    return TERMO_ERR_OK;
}

static termo_err_t termo_manager_event_reference_handler(
    termo_manager_t* manager,
    termo_event_payload_reference_t const* reference)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(reference != NULL);

    manager->delta_time = reference->sampling_period;
    manager->reference = reference->temperature;

    return TERMO_ERR_OK;
}

static termo_err_t termo_manager_event_handler(termo_manager_t* manager,
                                               termo_event_t const* event)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(event != NULL);

    switch (event->type) {
        case TERMO_EVENT_TYPE_START: {
            return termo_manager_event_start_handler(manager,
                                                     &event->payload.start);
        }
        case TERMO_EVENT_TYPE_STOP: {
            return termo_manager_event_stop_handler(manager,
                                                    &event->payload.stop);
        }
        case TERMO_EVENT_TYPE_REFERENCE: {
            return termo_manager_event_reference_handler(
                manager,
                &event->payload.reference);
        }
        default: {
            return TERMO_ERR_UNKNOWN_EVENT;
        }
    }
}

termo_err_t termo_manager_process(termo_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    termo_notify_t notify;
    if (termo_manager_receive_termo_notify(&notify)) {
        TERMO_RET_ON_ERR(termo_manager_notify_handler(manager, notify));
    }

    termo_event_t event;
    while (termo_manager_has_termo_event()) {
        if (termo_manager_receive_termo_event(&event)) {
            TERMO_RET_ON_ERR(termo_manager_event_handler(manager, &event));
        }
    }

    xTaskNotify(termo_task_manager_get(TERMO_TASK_TYPE_TERMO),
                TERMO_NOTIFY_TEMP_READY,
                eSetBits);

    return TERMO_ERR_OK;
}

termo_err_t termo_manager_initialize(termo_manager_t* manager,
                                     termo_config_t const* config,
                                     termo_params_t const* params)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(config != NULL);
    TERMO_ASSERT(params != NULL);

    manager->is_running = false;
    manager->delta_time = 0.001F;
    manager->reference = 30.0F;
    manager->measurement = 0.0F;
    manager->config = *config;

    if (mcp9808_initialize(
            &manager->mcp9808,
            &(mcp9808_config_t){.scale = mcp9808_resolution_to_scale(0x03)},
            &(mcp9808_interface_t){
                .bus_user = &manager->config,
                .bus_initialize = mcp9808_bus_initialize,
                .bus_deinitialize = mcp9808_bus_deinitialize,
                .bus_read_data = mcp9808_bus_read_data,
                .bus_write_data = mcp9808_bus_write_data,
            }) != MCP9808_ERR_OK) {
        TERMO_LOG(TAG, "Failed mcp9808_initialize!");
    }

    if (mcp9808_initialize_chip(&manager->mcp9808) != MCP9808_ERR_OK) {
        TERMO_LOG(TAG, "Failed mcp9808_initialize_chip!");
    }

    if (pid_regulator_initialize(
            &manager->pid,
            &(pid_regulator_config_t){.prop_gain = params->kp,
                                      .int_gain = params->ki,
                                      .dot_gain = params->kd,
                                      .min_control = params->min_temp,
                                      .max_control = params->max_temp,
                                      .sat_gain = params->kc,
                                      .dead_error = 0.0F}) !=
        PID_REGULATOR_ERR_OK) {
        TERMO_LOG(TAG, "Failed pid_regulator_initialize!");
    }

    system_event_t event = {.origin = SYSTEM_EVENT_ORIGIN_TERMO,
                            .type = SYSTEM_EVENT_TYPE_TERMO_READY,
                            .payload.termo_ready = {}};
    if (!termo_manager_send_system_event(&event)) {
        return TERMO_ERR_FAIL;
    }

    return TERMO_ERR_OK;
}
