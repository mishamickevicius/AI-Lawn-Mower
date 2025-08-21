#include "esp_log.h"
#include "gpio_test.h"
#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"



#define PWM_FREQ 1000000
#define PERIOD_TICKS_COUNT 20000
#define PWM_OUTPUT_GPIO_PIN 16

// static vars for MCPWM
static mcpwm_timer_handle_t timer = NULL;
static mcpwm_timer_config_t timer_config = {
    .group_id = 0,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = PWM_FREQ,
    .period_ticks = PERIOD_TICKS_COUNT,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
};
static mcpwm_oper_handle_t oper_handle = NULL;
static mcpwm_operator_config_t oper_config = {
    .group_id = 0,
};
static mcpwm_cmpr_handle_t cmpr_handle = NULL;
static mcpwm_comparator_config_t cmpr_config = {
    .flags.update_cmp_on_tez = true // TEZ === Timer equals zero
};
static mcpwm_gen_handle_t generator_handle = NULL;
static mcpwm_generator_config_t generator_config = {
    .gen_gpio_num = PWM_OUTPUT_GPIO_PIN,
};


// Function to init the pwm module
esp_err_t init_pwm_module()
{
    // Create timer 
    // Timer allows the operator to calculate the pulses
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    // Create operator
    ESP_ERROR_CHECK(mcpwm_new_operator(&oper_config, &oper_handle));
    // Connect the operator and timer 
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper_handle, timer));

    // Create Comparator
    // Comparators determine the duty cycle of the signal
    // Operator uses this to determine when to state change
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper_handle, &cmpr_config, &cmpr_handle));

    // Create Generator
    // This is what actually generates(pulls high or low) based on timer and comparators logic
    ESP_ERROR_CHECK(mcpwm_new_generator(oper_handle, &generator_config, &generator_handle));

    // Enable timer
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    // Start timer
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP)); // Start timer until stop signal

    // Set Comparator compare value
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(cmpr_handle, 1500));

    // Set generator actions(this tells the generator what to do on events)
    mcpwm_gen_timer_event_action_t gen_timer_event_action_config = {
        .direction = MCPWM_TIMER_DIRECTION_UP,
        .event = MCPWM_TIMER_EVENT_EMPTY,
        .action = MCPWM_GEN_ACTION_HIGH,
    };
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_handle, gen_timer_event_action_config));
    mcpwm_gen_compare_event_action_t gen_compare_event_action_config = {
        .direction = MCPWM_TIMER_DIRECTION_UP,
        .comparator = cmpr_handle,
        .action = MCPWM_GEN_ACTION_LOW,
    };
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_handle, gen_compare_event_action_config));

    return ESP_OK;
}  