/**
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
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
/**
 * scans for ble advertisements
 * based on the ble_central/ble_app_hrs_c example
 */
#include <stdint.h>
#include <stdio.h>
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_ble_scan.h"
#include "ble_advdata.h"
//#include "ble_bas.h"
#include "Battery Level/battery_voltage.h"
#include <sys/time.h>
#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "bsp.h"
#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#define MAX_TEST_DATA_BYTES     (15U)                /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256                         /**< UART RX buffer size. */
#define UART_HWFC APP_UART_FLOW_CONTROL_DISABLED

#define APP_BLE_CONN_CFG_TAG        1                                   /**< A tag identifying the SoftDevice BLE configuration. */
#define SCAN_DURATION_WITELIST      3000                                /**< Duration of the scanning in units of 10 milliseconds. */
#define DEV_NAME_LEN                ((BLE_GAP_ADV_SET_DATA_SIZE_MAX + 1) - \
                                    AD_DATA_OFFSET)                     /**< Determines the device name length. */

NRF_BLE_SCAN_DEF(m_scan);                                           /**< Scanning module instance. */

#define MAX_ADDRESS_COUNT 100

static const ble_uuid128_t m_target_beacon = {0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89};
uint8_t uuid128[16];
uint8_t address_list[MAX_ADDRESS_COUNT][BLE_GAP_ADDR_LEN] = {0};
int rssi_list[MAX_ADDRESS_COUNT] ={0};
uint8_t order_address[MAX_ADDRESS_COUNT] = {0};
int globalRssi = 0;
int address_list_length = 0;

///////////////////////////////////////////////////////////////////included SPI section
#include "sdk_config.h"
#include "nrf_drv_spis.h"
#include "nrf_drv_spi.h"
#define SPI_INSTANCE  0 /**< SPI instance index. */
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */
#define TEST_STRING "Nordic"
static uint8_t       m_tx_buf[] = TEST_STRING;           /**< TX buffer. */
static uint8_t       m_rx_buf[sizeof(TEST_STRING) + 1];    /**< RX buffer. */
static const uint8_t m_length = sizeof(m_tx_buf);        /**< Transfer length. */

#define SPIS_INSTANCE 1 /**< SPIS instance index. */
static const nrf_drv_spis_t spis = NRF_DRV_SPIS_INSTANCE(SPIS_INSTANCE);/**< SPIS instance. */
static volatile bool spis_xfer_done; /**< Flag used to indicate that SPIS instance completed the transfer. */
///////////////////////////////////////////////////////////////////


/**< Scan parameters requested for scanning and connection. */
static ble_gap_scan_params_t const m_scan_param =
{
    .active        = 0x01,
    .interval      = NRF_BLE_SCAN_SCAN_INTERVAL,
    .window        = NRF_BLE_SCAN_SCAN_WINDOW,
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL, //BLE_GAP_SCAN_FP_WHITELIST,
    .timeout       = SCAN_DURATION_WITELIST,
    .scan_phys     = BLE_GAP_PHY_1MBPS,
};

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

/**@brief Function to start scanning.
 */
static void scan_start(void)
{
    NRF_LOG_INFO("Starting scan.");
    APP_ERROR_CHECK(nrf_ble_scan_start(&m_scan));
}

bool address_list_contains(const uint8_t address[]) {
    for (int i = 0; i < address_list_length; i++) {
        if (address_list[i][0] == address[0]
            && address_list[i][1] == address[1]
            && address_list[i][2] == address[2]
            && address_list[i][3] == address[3]
            && address_list[i][4] == address[4]
            && address_list[i][5] == address[5]) {
            
            return true;
        }
    }
    
    return false;
}

bool address_list_add(const uint8_t address[]) {
    if (address_list_length < MAX_ADDRESS_COUNT) {
        memcpy(address_list[address_list_length], address, BLE_GAP_ADDR_LEN);
//        if(address_list[address_list_length][5] == 0xC3 && address_list[address_list_length][4] == 0xD7 && address_list[address_list_length][3] == 0x59 && address_list[address_list_length][2] == 0xEF && address_list[address_list_length][1] == 0x38 && address_list[address_list_length][0] == 0x2E ){
          if(address_list[address_list_length][5] == 0xE8 && address_list[address_list_length][4] == 0xBA && address_list[address_list_length][3] == 0x41 && address_list[address_list_length][2] == 0x56 && address_list[address_list_length][1] == 0x8D && address_list[address_list_length][0] == 0x9E ){
          order_address[0] = 0xB1;
        }else if(address_list[address_list_length][5] == 0xF3 && address_list[address_list_length][4] == 0x45 && address_list[address_list_length][3] == 0x89 && address_list[address_list_length][2] == 0xEF && address_list[address_list_length][1] == 0x19 && address_list[address_list_length][0] == 0x1E ){
          order_address[1] = 0xB2;
        }else if(address_list[address_list_length][5] == 0xC3 && address_list[address_list_length][4] == 0xD7 && address_list[address_list_length][3] == 0x59 && address_list[address_list_length][2] == 0xEF && address_list[address_list_length][1] == 0x38 && address_list[address_list_length][0] == 0x2E ){
          order_address[2] = 0xB3;
        }else if(address_list[address_list_length][5] == 0xFD && address_list[address_list_length][4] == 0xCC && address_list[address_list_length][3] == 0x03 && address_list[address_list_length][2] == 0x3B && address_list[address_list_length][1] == 0xFB && address_list[address_list_length][0] == 0x50){
          order_address[3] = 0xB4;
        }
        //sendbeacon_data();
    }
}

void print_address(const ble_gap_evt_adv_report_t* p_adv_report) {
    NRF_LOG_INFO("addr: %02x:%02x:%02x:%02x:%02x:%02x",
       p_adv_report->peer_addr.addr[5],
       p_adv_report->peer_addr.addr[4],
       p_adv_report->peer_addr.addr[3],
       p_adv_report->peer_addr.addr[2],
       p_adv_report->peer_addr.addr[1],
       p_adv_report->peer_addr.addr[0]);
//    uart_init();
}

void rssi_list_add(const uint32_t rssi){
      printf("%d\n",rssi);
      NRF_LOG_INFO("%d",rssi);
      rssi_list[address_list_length] = rssi;
      globalRssi = rssi;
      //sendbeacon_data();
      //uart_init();
//       NRF_LOG_INFO("%d",globalRssi);
}

void add_uuid_data(const ble_gap_evt_adv_report_t* p_adv_report){
    memcpy(uuid128, &p_adv_report->data.p_data[9], 16); //Copy UUID
    if(memcmp(uuid128, m_target_beacon.uuid128, 16) == 0){ //if uuid match.
      address_list_add(p_adv_report->peer_addr.addr);
      rssi_list_add(p_adv_report->rssi);
      //print_address(p_adv_report);
      address_list_length++;
    }
    //address_list_add(p_adv_report->peer_addr.addr);
    //rssi_list_add(p_adv_report->rssi);
    //address_list_length++;
}

void clear_data(){
  memset(address_list,0,sizeof(address_list));
  memset(rssi_list,0,sizeof(rssi_list));
  memset(order_address,0,sizeof(order_address));
  address_list_length = 0;
}

static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
//    printf("Evt handler Init!");
//  ้ถ้า scan แล้วไม่เจอในเวลาที่กำหนด ให้ timeout แล้ว scan ใหม่
    if (p_scan_evt->scan_evt_id == NRF_BLE_SCAN_EVT_SCAN_TIMEOUT || address_list_length == 4) {
        NRF_LOG_INFO("Scan timed out.");
        sendbeacon_data();
        clear_data();
        scan_start(); 
        return;
    }

    if (address_list_contains(p_scan_evt->params.filter_match.p_adv_report->peer_addr.addr) != false)
        return;
    
    
    add_uuid_data(p_scan_evt->params.filter_match.p_adv_report);
}

/**@brief Function for initialization scanning and setting filters.
 */
static void scan_init(void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;
    memset(&init_scan, 0, sizeof(init_scan));

    init_scan.p_scan_param     = &m_scan_param;
    init_scan.connect_if_match = false;
    init_scan.conn_cfg_tag     = APP_BLE_CONN_CFG_TAG;

    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);
    printf("Scan Init!");
}

void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    spi_xfer_done = true;
    NRF_LOG_INFO("Transfer completed.");
    if (m_rx_buf[0] != 0)
    {
        NRF_LOG_INFO(" Received:");
        NRF_LOG_HEXDUMP_INFO(m_rx_buf, strlen((const char *)m_rx_buf));
    }
}
void spis_event_handler(nrf_drv_spis_event_t event)
{
    if (event.evt_type == NRF_DRV_SPIS_XFER_DONE)
    {
        spis_xfer_done = true;
        NRF_LOG_INFO(" Transfer completed. Received: %s",(uint32_t)m_rx_buf);
    }
}
void uart_error_handle(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}

void sendbeacon_data(){
        char buffer[512],n;
        //itoa(globalRssi, snum, 10);
        n = sprintf(buffer,"%02x,%d\n\r",order_address[0],rssi_list[0]);
        for(uint8_t j=1;j<address_list_length;j++){
        n += sprintf(buffer+n,"%02x,%d\n\r",order_address[j],rssi_list[j]);
        }//printf("%02x:%02x:%02x:%02x:%02x:%02x",address_list[0][5],address_list[0][4],address_list[0][3],address_list[0][2],address_list[0][1],address_list[0][0]);
        //printf("%d",globalRssi);
        n += sprintf(buffer+n,"\n\r");
        for(uint32_t i=0;i<strlen(buffer);i++){
        app_uart_put(buffer[i]);
        nrf_delay_ms(20);
        }
        memset(buffer,0,sizeof(buffer));
}

void uart_init()
{
  uint32_t err_code;
const app_uart_comm_params_t comm_params =
      {
          RX_PIN_NUMBER,
          TX_PIN_NUMBER,
          RTS_PIN_NUMBER,
          CTS_PIN_NUMBER,
          UART_HWFC,
          false,
          NRF_UART_BAUDRATE_115200
      };
    APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_error_handle,
                         APP_IRQ_PRIORITY_LOWEST,
                         err_code);

    APP_ERROR_CHECK(err_code);
    printf("\r\nUART example started.\r\n");

    
        uint8_t cr;
//        int globalRssi = -48;
       
//        printf("%c",snum[0]);
//        app_uart_put(snum[0]);
//        app_uart_put(snum[1]);
//        app_uart_put(snum[2]);
//          app_uart_put('-');
//        app_uart_put('2');
//        app_uart_put('8');
//while(true){
//        while (app_uart_put(snum[0]) != NRF_SUCCESS);
//        while (app_uart_put(snum[1]) != NRF_SUCCESS);
//        while (app_uart_put(snum[2]) != NRF_SUCCESS);
//        if (cr == 'q' || cr == 'Q')
//        {
//            printf(" \r\nExit!\r\n");
//
//            while (true)
//            {
//                // Do nothing.
//            }
//        }
//    }
}
int main(void)
{
    /*ret_code_t err_code;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);*/
    ble_stack_init();
    scan_init();
    battery_voltage_init();
    uart_init();
    // Start execution.
    NRF_LOG_INFO("Start scan");
    scan_start();
    
    // Enter main loop.
    for (;;)
    {
        NRF_LOG_FLUSH();

       
    }
}