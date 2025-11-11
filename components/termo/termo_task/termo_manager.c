#include "termo_manager.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "termo_common.h"
#include <string.h>

static char const* const TAG = "termo_manager";

static inline bool frequency_to_prescaler_and_period(uint32_t frequency_hz,
                                                     uint32_t clock_hz,
                                                     uint32_t max_prescaler,
                                                     uint32_t max_period,
                                                     uint32_t* prescaler,
                                                     uint32_t* period)
{
    if (frequency_hz == 0U || !prescaler || !period) {
        return false;
    }

    uint32_t temp_prescaler = 0U;
    uint32_t temp_period = clock_hz / frequency_hz;

    while (temp_period > max_period && temp_prescaler < max_prescaler) {
        temp_prescaler++;
        temp_period = clock_hz / ((temp_prescaler + 1U) * frequency_hz);
    }
    if (temp_period > max_period) {
        temp_period = max_period;
        temp_prescaler = (clock_hz / (temp_period * frequency_hz)) - 1U;
    }
    if (temp_prescaler > max_prescaler) {
        temp_prescaler = max_prescaler;
    }

    *prescaler = temp_prescaler;
    *period = temp_period;

    return true;
}

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

    return HAL_TIM_PWM_Start_IT(manager->config.pwm_timer,
                                manager->config.pwm_channel) == HAL_OK;
}

static inline bool termo_manager_stop_pwm_timer(termo_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    return HAL_TIM_PWM_Stop_IT(manager->config.pwm_timer,
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

static inline bool termo_manager_start_update_timer(termo_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    return HAL_TIM_Base_Start_IT(manager->config.update_timer) == HAL_OK;
}

static inline bool termo_manager_stop_update_timer(termo_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    return HAL_TIM_Base_Stop_IT(manager->config.update_timer) == HAL_OK;
}

static inline bool termo_manager_set_update_timer_period(
    termo_manager_t* manager,
    float32_t update_time)
{
    TERMO_ASSERT(manager != NULL);

    if (update_time > 1.0F || update_time < 0.1F) {
        return TERMO_ERR_FAIL;
    }

    uint32_t clock_hz = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
        clock_hz *= 2;
    }

    uint32_t frequency = (uint32_t)(1.0F / update_time);

    uint32_t period;
    uint32_t prescaler;
    bool result = frequency_to_prescaler_and_period(frequency,
                                                    clock_hz,
                                                    0xFFFFU,
                                                    0xFFFFU,
                                                    &prescaler,
                                                    &period);

    if (result && period < 0xFFFFU && prescaler < 0xFFFFU) {
        uint32_t compare = (uint32_t)((float32_t)period / 2.0F);

        __HAL_TIM_DISABLE(manager->config.update_timer);
        __HAL_TIM_SET_COUNTER(manager->config.update_timer, 0U);
        __HAL_TIM_SET_PRESCALER(manager->config.update_timer, prescaler);
        __HAL_TIM_SET_AUTORELOAD(manager->config.update_timer, period);
        __HAL_TIM_ENABLE(manager->config.update_timer);

        TERMO_LOG(TAG,
                  "clock: %u, frequency: %u, period: %u, prescaler: %u",
                  clock_hz,
                  frequency,
                  period,
                  prescaler);

        return true;
    }

    return false;
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

static inline uint32_t termo_manager_control_temperature_to_compare(
    termo_manager_t* manager,
    float32_t control_temperature)
{
    TERMO_ASSERT(manager != NULL);

    float32_t compare =
        (control_temperature - manager->params.min_temp) *
            (float32_t)(manager->params.max_compare -
                        manager->params.min_compare) /
            (manager->params.max_temp - manager->params.min_temp) +
        (float32_t)manager->params.max_compare;

    if (compare < manager->params.min_compare) {
        compare = manager->params.min_compare;
    }
    if (compare > manager->params.max_compare) {
        compare = manager->params.max_compare;
    }

    return (uint32_t)compare;
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

    float32_t control_temperature = 0.0F;
    float32_t error_temperature = manager->reference - manager->measurement;
    if (pid_regulator_get_sat_control(&manager->pid,
                                      error_temperature,
                                      manager->params.delta_time,
                                      &control_temperature) !=
        PID_REGULATOR_ERR_OK) {
        termo_manager_set_pwm_timer_compare(manager, 0U);
        termo_manager_stop_pwm_timer(manager);
        manager->has_fault = true;

        return TERMO_ERR_FAIL;
    } else {
        if (manager->has_fault) {
            termo_manager_start_pwm_timer(manager);
            manager->has_fault = false;
        }
    }

    uint32_t compare =
        termo_manager_control_temperature_to_compare(manager,
                                                     control_temperature);

    if (!termo_manager_set_pwm_timer_compare(manager, compare)) {
        termo_manager_set_pwm_timer_compare(manager, 0U);
        termo_manager_stop_pwm_timer(manager);
        manager->has_fault = true;

        return TERMO_ERR_FAIL;
    } else {
        if (manager->has_fault) {
            termo_manager_start_pwm_timer(manager);
            manager->has_fault = false;
        }
    }

    TERMO_LOG(TAG,
              "Ref: %.2fC, Meas: %.2fC, Err: %.2fC, Ctrl: %.2fC, Comp: %lu",
              manager->reference,
              manager->measurement,
              error_temperature,
              control_temperature,
              compare);

    return TERMO_ERR_OK;
}

static termo_err_t termo_manager_notify_update_timer_handler(
    termo_manager_t* manager)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);

    float32_t measurement = 0.0F;
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

    if ((notify & TERMO_NOTIFY_UPDATE_TIMER) == TERMO_NOTIFY_UPDATE_TIMER) {
        TERMO_RET_ON_ERR(termo_manager_notify_update_timer_handler(manager));
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

    if (!termo_manager_start_update_timer(manager)) {
        return TERMO_ERR_FAIL;
    }

    if (!termo_manager_set_pwm_timer_compare(manager, 0U)) {
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

    if (!termo_manager_stop_update_timer(manager)) {
        return TERMO_ERR_FAIL;
    }

    if (!termo_manager_set_pwm_timer_compare(manager, 0U)) {
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

    if (manager->update_time != reference->update_time) {
        if (!termo_manager_set_update_timer_period(manager,
                                                   reference->update_time)) {
            return TERMO_ERR_FAIL;
        }
    }

    manager->update_time = reference->update_time;
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
    manager->has_fault = false;

    manager->update_time = 0.0F;
    manager->reference = 0.0F;
    manager->measurement = 0.0F;

    manager->config = *config;
    manager->params = *params;

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
