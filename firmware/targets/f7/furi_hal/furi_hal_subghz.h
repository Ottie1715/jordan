/**
 * @file furi_hal_subghz.h
 * SubGhz HAL API
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <toolbox/level_duration.h>
#include <furi_hal_gpio.h>
#include <furi_hal_spi_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Low level buffer dimensions and guard times */
#define API_HAL_SUBGHZ_ASYNC_TX_BUFFER_FULL (256)
#define API_HAL_SUBGHZ_ASYNC_TX_BUFFER_HALF (API_HAL_SUBGHZ_ASYNC_TX_BUFFER_FULL / 2)
#define API_HAL_SUBGHZ_ASYNC_TX_GUARD_TIME 999

/** Radio Presets */
typedef enum {
    FuriHalSubGhzPresetIDLE, /**< default configuration */
    FuriHalSubGhzPresetOok270Async, /**< OOK, bandwidth 270kHz, asynchronous */
    FuriHalSubGhzPresetOok650Async, /**< OOK, bandwidth 650kHz, asynchronous */
    FuriHalSubGhzPreset2FSKDev238Async, /**< FM, deviation 2.380371 kHz, asynchronous */
    FuriHalSubGhzPreset2FSKDev476Async, /**< FM, deviation 47.60742 kHz, asynchronous */
    FuriHalSubGhzPresetMSK99_97KbAsync, /**< MSK, deviation 47.60742 kHz, 99.97Kb/s, asynchronous */
    FuriHalSubGhzPresetGFSK9_99KbAsync, /**< GFSK, deviation 19.042969 kHz, 9.996Kb/s, asynchronous */
    FuriHalSubGhzPresetCustom, /**Custom Preset*/
} FuriHalSubGhzPreset;

/** Switchable Radio Paths */
typedef enum {
    FuriHalSubGhzPathIsolate, /**< Isolate Radio from antenna */
    FuriHalSubGhzPath433, /**< Center Frequency: 433MHz. Path 1: SW1RF1-SW2RF2, LCLCL */
    FuriHalSubGhzPath315, /**< Center Frequency: 315MHz. Path 2: SW1RF2-SW2RF1, LCLCLCL */
    FuriHalSubGhzPath868, /**< Center Frequency: 868MHz. Path 3: SW1RF3-SW2RF3, LCLC */
} FuriHalSubGhzPath;

/** SubGhz state */
typedef enum {
    SubGhzStateInit, /**< Init pending */

    SubGhzStateIdle, /**< Idle, energy save mode */

    SubGhzStateAsyncRx, /**< Async RX started */

    SubGhzStateAsyncTx, /**< Async TX started, DMA and timer is on */
    SubGhzStateAsyncTxLast, /**< Async TX continue, DMA completed and timer got last value to go */
    SubGhzStateAsyncTxEnd, /**< Async TX complete, cleanup needed */

} SubGhzState;

/** SubGhz regulation, receive transmission on the current frequency for the
 * region */
typedef enum {
    SubGhzRegulationOnlyRx, /**only Rx*/
    SubGhzRegulationTxRx, /**TxRx*/
} SubGhzRegulation;

/** SubGhz radio types */
typedef enum {
    SubGhzRadioInternal,
    SubGhzRadioExternal,
} SubGhzRadioType;

/** Structure for accessing SubGhz settings*/
typedef struct {
    volatile SubGhzState state;
    volatile SubGhzRegulation regulation;
    volatile FuriHalSubGhzPreset preset;
    const GpioPin* async_mirror_pin;
    SubGhzRadioType radio_type;
    FuriHalSpiBusHandle* spi_bus_handle;
    const GpioPin* cc1101_g0_pin;
    uint8_t rolling_counter_mult;
    bool ext_module_power_disabled : 1;
    bool timestamp_file_names : 1;
} FuriHalSubGhz;

extern volatile FuriHalSubGhz furi_hal_subghz;

/* Mirror RX/TX async modulation signal to specified pin
 *
 * @warning    Configures pin to output mode. Make sure it is not connected
 *             directly to power or ground.
 *
 * @param[in]  pin   pointer to the gpio pin structure or NULL to disable
 */
void furi_hal_subghz_set_async_mirror_pin(const GpioPin* pin);

/** Initialize and switch to power save mode Used by internal API-HAL
 * initialization routine Can be used to reinitialize device to safe state and
 * send it to sleep
 */
void furi_hal_subghz_init();

/** Initialize and switch to power save mode Used by internal API-HAL
 * initialization routine Can be used to reinitialize device to safe state and
 * send it to sleep
 * @return     true if initialisation is successfully
 */
bool furi_hal_subghz_init_check(void);

/** Send device to sleep mode
 */
void furi_hal_subghz_sleep();

/** Dump info to stdout
 */
void furi_hal_subghz_dump_state();

/** Load registers from preset by preset name
 *
 * @param      preset  to load
 */
void furi_hal_subghz_load_preset(FuriHalSubGhzPreset preset);

/** Load custom registers from preset
 *
 * @param      preset_data   registers to load
 */
void furi_hal_subghz_load_custom_preset(uint8_t* preset_data);

/** Load registers
 *
 * @param      data  Registers data
 */
void furi_hal_subghz_load_registers(uint8_t* data);

/** Load PATABLE
 *
 * @param      data  8 uint8_t values
 */
void furi_hal_subghz_load_patable(const uint8_t data[8]);

/** Write packet to FIFO
 *
 * @param      data  bytes array
 * @param      size  size
 */
void furi_hal_subghz_write_packet(const uint8_t* data, uint8_t size);

/** Check if receive pipe is not empty
 *
 * @return     true if not empty
 */
bool furi_hal_subghz_rx_pipe_not_empty();

/** Check if received data crc is valid
 *
 * @return     true if valid
 */
bool furi_hal_subghz_is_rx_data_crc_valid();

/** Read packet from FIFO
 *
 * @param      data  pointer
 * @param      size  size
 */
void furi_hal_subghz_read_packet(uint8_t* data, uint8_t* size);

/** Flush rx FIFO buffer
 */
void furi_hal_subghz_flush_rx();

/** Flush tx FIFO buffer
 */
void furi_hal_subghz_flush_tx();

/** Shutdown Issue SPWD command
 * @warning    registers content will be lost
 */
void furi_hal_subghz_shutdown();

/** Reset Issue reset command
 * @warning    registers content will be lost
 */
void furi_hal_subghz_reset();

/** Switch to Idle
 */
void furi_hal_subghz_idle();

/** Switch to Receive
 */
void furi_hal_subghz_rx();

/** Switch to Transmit
 *
 * @return     true if the transfer is allowed by belonging to the region
 */
bool furi_hal_subghz_tx();

/** Get RSSI value in dBm
 *
 * @return     RSSI value
 */
float furi_hal_subghz_get_rssi();

/** Get LQI
 *
 * @return     LQI value
 */
uint8_t furi_hal_subghz_get_lqi();

/** Check if frequency is in valid range
 *
 * @param      value  frequency in Hz
 *
 * @return     true if frequency is valid, otherwise false
 */
bool furi_hal_subghz_is_frequency_valid(uint32_t value);

/** Set frequency and path This function automatically selects antenna matching
 * network
 *
 * @param      value  frequency in Hz
 *
 * @return     real frequency in Hz
 */
uint32_t furi_hal_subghz_set_frequency_and_path(uint32_t value);

/** Read extend and bypass settings values into out params
 *
 * @param      extend  pointer to bool for extend
 * @param      bypass  pointer to bool for bypass
 */
void furi_hal_subghz_get_extend_settings(bool* extend, bool* bypass);

/** Set extend and bypass settings values to file
 *
 * @param      extend  bool for extend
 * @param      bypass  bool for bypass
 */
void furi_hal_subghz_set_extend_settings(bool extend, bool bypass);

/** Сheck if transmission is allowed on this frequency with your current config
 *
 * @param      value  frequency in Hz
 *
 * @return     true if allowed
 */
bool furi_hal_subghz_is_tx_allowed(uint32_t value);

/** Set frequency
 *
 * @param      value  frequency in Hz
 *
 * @return     real frequency in Hz
 */
uint32_t furi_hal_subghz_set_frequency(uint32_t value);

/** Set path
 *
 * @param      path  path to use
 */
void furi_hal_subghz_set_path(FuriHalSubGhzPath path);

/* High Level API */

/** Signal Timings Capture callback */
typedef void (*FuriHalSubGhzCaptureCallback)(bool level, uint32_t duration, void* context);

/** Enable signal timings capture Initializes GPIO and TIM2 for timings capture
 *
 * @param      callback  FuriHalSubGhzCaptureCallback
 * @param      context   callback context
 */
void furi_hal_subghz_start_async_rx(FuriHalSubGhzCaptureCallback callback, void* context);

/** Disable signal timings capture Resets GPIO and TIM2
 */
void furi_hal_subghz_stop_async_rx();

/** Async TX callback type
 * @param      context  callback context
 * @return     LevelDuration
 */
typedef LevelDuration (*FuriHalSubGhzAsyncTxCallback)(void* context);

/** Start async TX Initializes GPIO, TIM2 and DMA1 for signal output
 *
 * @param      callback  FuriHalSubGhzAsyncTxCallback
 * @param      context   callback context
 *
 * @return     true if the transfer is allowed by belonging to the region
 */
bool furi_hal_subghz_start_async_tx(FuriHalSubGhzAsyncTxCallback callback, void* context);

/** Wait for async transmission to complete
 *
 * @return     true if TX complete
 */
bool furi_hal_subghz_is_async_tx_complete();

/** Stop async transmission and cleanup resources Resets GPIO, TIM2, and DMA1
 */
void furi_hal_subghz_stop_async_tx();

/** Switching between internal and external radio
 * @param      state SubGhzRadioInternal or SubGhzRadioExternal
 * @return     true if switching is successful
 */
bool furi_hal_subghz_init_radio_type(SubGhzRadioType state);

/** Get current radio
 * @return     SubGhzRadioInternal or SubGhzRadioExternal
 */
SubGhzRadioType furi_hal_subghz_get_radio_type(void);

/** Check for a radio module
 * @return     true if check is successful
 */
bool furi_hal_subghz_check_radio(void);

/** Turn on the power of the external radio module
 * @return     true if power-up is successful
 */
bool furi_hal_subghz_enable_ext_power(void);

/** Turn off the power of the external radio module
 */
void furi_hal_subghz_disable_ext_power(void);

/** Get the current rolling protocols counter ++ value
 * @return    uint8_t current value
 */
uint8_t furi_hal_subghz_get_rolling_counter_mult(void);

/** Set the current rolling protocols counter ++ value
 * @param      mult uint8_t = 1, 2, 4, 8
 */
void furi_hal_subghz_set_rolling_counter_mult(uint8_t mult);

/** If true - disable 5v power of the external radio module
 */
void furi_hal_subghz_set_external_power_disable(bool state);

/** Get the current state of the external power disable flag
 */
bool furi_hal_subghz_get_external_power_disable(void);

/** Set what radio module we will be using
 */
void furi_hal_subghz_select_radio_type(SubGhzRadioType state);

#ifdef __cplusplus
}
#endif
