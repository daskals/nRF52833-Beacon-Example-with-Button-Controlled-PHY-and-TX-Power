/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_uart_over_ble_main main.c
 * @{
 * @ingroup  ble_sdk_app_nus_eval
 * @brief    UART over BLE application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service.
 * This application uses the @ref srvlib_conn_params module.
 */


#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "app_timer.h"
// #include "ble_nus.h"
// #include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "nrf_pwr_mgmt.h"

// Added Axtra
#include "bsp.h"
#include "nrf_soc.h"
#include "nrf_sdm.h"

//#include "SEGGER_RTT.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"

// Do NRF_LOG_ENABLED 1  in config file
#define APP_BLE_CONN_CFG_TAG            1       

//NRF_LOG_ENABLED 1

// #define SUCCESS_LED_PIN LED_1 // Use LED_1 from pca10100.h (P0.13 on the nRF52833 DK)                           /**< A tag identifying the SoftDevice BLE configuration. */

#define NON_CONNECTABLE_ADV_INTERVAL    MSEC_TO_UNITS(2000, UNIT_0_625_MS)  /**< The advertising interval for non-connectable advertisement (100 ms). This value can vary between 100ms to 10.24s). */

#define APP_BEACON_INFO_LENGTH          0x10                              /**< Total length of information advertised by the Beacon. */
#define APP_COMPANY_IDENTIFIER          0x0059                             /**< Company identifier for Nordic Semiconductor ASA. as per www.bluetooth.org. */

static ble_gap_adv_params_t m_adv_params;                                  /**< Parameters to be passed to the stack when starting advertising. */
static uint8_t              m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET; /**< Advertising handle used to identify an advertising set. */
static uint8_t              m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];  /**< Buffer for storing an encoded advertising set. */


// Variables for PHY and transmission power
static uint8_t current_phy = BLE_GAP_PHY_CODED; // BLE_GAP_PHY_CODED or  BLE_GAP_PHY_1MBPS
static int8_t current_tx_power = 4;            //  4 dBm or 4 dBm


/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = NULL,
        .len    = 0

    }
};

static uint8_t m_beacon_info[APP_BEACON_INFO_LENGTH];                    /**< Information advertised by the Beacon. */

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    uint8_t       flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

    ble_advdata_manuf_data_t manuf_specific_data;

    manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;

    // Increment the last byte of the beacon info
    m_beacon_info[APP_BEACON_INFO_LENGTH - 1]++;

    manuf_specific_data.data.p_data = (uint8_t *)m_beacon_info;
    manuf_specific_data.data.size   = APP_BEACON_INFO_LENGTH;

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type             = BLE_ADVDATA_NO_NAME;
    advdata.flags                 = flags;
    advdata.p_manuf_specific_data = &manuf_specific_data;

    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));

    if (current_phy == BLE_GAP_PHY_1MBPS)
    {
        m_adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    }
    else if (current_phy == BLE_GAP_PHY_CODED)
    {
        m_adv_params.properties.type = BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    }

    m_adv_params.p_peer_addr     = NULL;    // Undirected advertisement.
    m_adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval        = NON_CONNECTABLE_ADV_INTERVAL;
    m_adv_params.duration        = 0; // Never time out.
    m_adv_params.primary_phy     = current_phy;
    m_adv_params.secondary_phy   = current_phy;

    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
    APP_ERROR_CHECK(err_code);

    // Set the transmission power to the current value after configuring the advertising set
    err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, m_adv_handle, current_tx_power);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for updating the advertising payload.
 *
 * @details Increments the last byte of the beacon info, wraps it to 0 if it overflows, and updates the advertising data.
 */
static void update_advertising_payload(void)
{
    uint32_t err_code;

    // Increment the last byte of the beacon info and wrap to 0 if it overflows
    m_beacon_info[APP_BEACON_INFO_LENGTH - 1]++;
    if (m_beacon_info[APP_BEACON_INFO_LENGTH - 1] == 0)
    {
        NRF_LOG_INFO("Beacon Info last byte wrapped to 0");
    }

    // Print the updated beacon info as a byte stream
    NRF_LOG_INFO("Updated Beacon Info: ");
    for (int i = 0; i < APP_BEACON_INFO_LENGTH; i++)
    {
        NRF_LOG_RAW_INFO("%02X", m_beacon_info[i]);
    }
    NRF_LOG_RAW_INFO("\r\n");

    // Update the manufacturer-specific data
    ble_advdata_manuf_data_t manuf_specific_data;
    manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;
    manuf_specific_data.data.p_data = (uint8_t *)m_beacon_info;
    manuf_specific_data.data.size = APP_BEACON_INFO_LENGTH;

    // Build and encode the updated advertising data
    ble_advdata_t advdata;
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type = BLE_ADVDATA_NO_NAME;
    advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
    advdata.p_manuf_specific_data = &manuf_specific_data;

    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    // Update the advertising set with the new data
    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, NULL);
    APP_ERROR_CHECK(err_code);

}



/**@brief Function to update LED states based on PHY and transmission power. */
static void update_led_states(void)
{
    // LED 3: PHY indication
    if (current_phy == BLE_GAP_PHY_CODED)
    {
        bsp_board_led_on(2); // LED 2 ON
    }
    else if  (current_phy == BLE_GAP_PHY_1MBPS)
    {
        bsp_board_led_off(2); // LED 2 OFF

    }

    // LED 3: Transmission power 4 dBm
    if (current_tx_power == 4)
    {
        bsp_board_led_on(3); // LED 3 ON
    }
    else if (current_tx_power == 8)
    {
        bsp_board_led_off(3); // LED 2 OFF
    }

}

/**@brief Function for starting advertising with updated PHY and power. */
static void advertising_start(void)
{
    uint32_t err_code;

    // Stop advertising if it is already running
    err_code = sd_ble_gap_adv_stop(m_adv_handle);
    if (err_code != NRF_ERROR_INVALID_STATE) // Ignore if advertising is not running
    {
        APP_ERROR_CHECK(err_code);
    }

    // Update the advertising payload
    update_advertising_payload();

    // Set PHY and transmission power
    if (current_phy == BLE_GAP_PHY_1MBPS)
    {
        m_adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    }
    else if (current_phy == BLE_GAP_PHY_CODED)
    {
        m_adv_params.properties.type = BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    }
    m_adv_params.primary_phy = current_phy;
    m_adv_params.secondary_phy = current_phy;
    
    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, m_adv_handle, current_tx_power);
    APP_ERROR_CHECK(err_code);

    // Start advertising
    err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
    APP_ERROR_CHECK(err_code);

    // Update LED states
    update_led_states();
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing LEDs. */
static void leds_init(void)
{
    ret_code_t err_code = bsp_init(BSP_INIT_LEDS, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing timers. */
static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}



/**@brief Function for initializing logging. */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
    NRF_LOG_INFO("Logging initialized\r\n");
}

/**@brief Function to handle button presses. */
static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    if (button_action == APP_BUTTON_PUSH) // Button pressed
    {
        if (pin_no == BSP_BUTTON_0) // Button 1 pressed
        {
            // Toggle PHY between BLE_GAP_PHY_CODED and BLE_GAP_PHY_1MBPS
            if (current_phy == BLE_GAP_PHY_CODED)
            {
                current_phy = BLE_GAP_PHY_1MBPS;
                NRF_LOG_INFO("Button 1 pressed: PHY set to BLE_GAP_PHY_1MBPS");
            }
            else
            {
                current_phy = BLE_GAP_PHY_CODED;
                NRF_LOG_INFO("Button 1 pressed: PHY set to BLE_GAP_PHY_CODED");
            }
        }
        else if (pin_no == BSP_BUTTON_1) // Button 2 pressed
        {
            // Toggle TX power between 4 dBm and 8 dBm
            if (current_tx_power == 4)
            {
                current_tx_power = 8;
                NRF_LOG_INFO("Button 2 pressed: TX power set to 8 dBm");
            }
            else
            {
                current_tx_power = 4;
                NRF_LOG_INFO("Button 2 pressed: TX power set to 4 dBm");
            }
        }

        // Update advertising and LED states
        advertising_start();
    }
}

/**@brief Function for initializing buttons. */
static void buttons_init(void)
{
    ret_code_t err_code;

    static app_button_cfg_t buttons[] =
    {
        {BSP_BUTTON_0, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_PULLUP, button_event_handler}, // Button 1
        {BSP_BUTTON_1, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_PULLUP, button_event_handler}  // Button 2
    };

    err_code = app_button_init(buttons, ARRAY_SIZE(buttons), APP_TIMER_TICKS(50));
    APP_ERROR_CHECK(err_code);

    err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);
}

/**@brief Application main function. */
int main(void)
{
    // Enable the DC/DC converter for power efficiency
    NRF_POWER->DCDCEN = 1;
    // Initialize.
    log_init();
    timers_init();
    leds_init();
    power_management_init();
    ble_stack_init();
    advertising_init();

    // Initialize buttons
    buttons_init();

    // Start execution.
    NRF_LOG_INFO("Beacon example started.");

    // Start advertising in a loop to update the payload
    while (true)
    {
        advertising_start();
        nrf_delay_ms(2000); // Wait for 100 ms before updating the payload

        // Flush logs to ensure they are printed
        NRF_LOG_FLUSH();
    }
}
/**
 * @}
 */
