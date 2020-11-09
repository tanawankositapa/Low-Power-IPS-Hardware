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

#include <string.h>
#include "board.h"
#include "nordic_common.h"
#include "nrf.h"
#include "utils.h"
#include "ble_hci.h"
#include "ble_advertising.h"
#include "ble_db_discovery.h"
#include "ble_conn_params.h"
#include "nrf_sdh_soc.h"
#include "nrf_fstorage.h"
#include "nrf_ble_gatt.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "peer_manager.h"
#include "custom_board.h"
#include "nrf_drv_twi.h"

Ble_scanRsp_t m_bls_scan_rsp[10];
uint8_t rspNum = 0;

#define APP_BLE_CONN_CFG_TAG        1                                   /**< A tag identifying the SoftDevice BLE configuration. */
#define SCAN_DURATION_WITELIST      3000                                /**< Duration of the scanning in units of 10 milliseconds. */
#define DEV_NAME_LEN                ((BLE_GAP_ADV_SET_DATA_SIZE_MAX + 1) - \
                                    AD_DATA_OFFSET)                     /**< Determines the device name length. */

NRF_BLE_SCAN_DEF(m_scan);                                           /**< Scanning module instance. */

#define MAX_ADDRESS_COUNT 100

static const ble_uuid128_t m_target_beacon = {0xFD,0xA5,0x06,0x93,0xA4,0xE2,0x4F,0xB1,0xAF,0xCF,0xC6,0xEB,0x07,0x64,0x78,0x25};
uint8_t uuid128[16];
uint8_t address_list[MAX_ADDRESS_COUNT][BLE_GAP_ADDR_LEN] = {0};
int rssi_list[MAX_ADDRESS_COUNT] ={0};
int address_list_length = 0;

lora_cfg_t		g_lora_cfg;																													/**< lorab board config param.*/
bool loraconfigupdataflg = false;


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
 int set_handler(int argc, char* argv[], lora_cfg_t* cfg_info)
{
    
    NRF_LOG_INFO("%s=%s", argv[0], argv[1]);
    
    if (strcmp(argv[0], "dev_eui") == 0) {
        StrToHex((char *)cfg_info->dev_eui, argv[1], 8);
    }
    else if (strcmp(argv[0], "app_eui") == 0) {
        StrToHex((char *)cfg_info->app_eui, argv[1], 8);
    }
    else if (strcmp(argv[0], "app_key") == 0) {
        StrToHex((char *)cfg_info->app_key, argv[1], 16);
    }
    else if (strcmp(argv[0], "dev_addr") == 0) {
        cfg_info->dev_addr = strtoul(argv[1], NULL, 16);
    }
    else if (strcmp(argv[0], "nwkskey") == 0) {
        StrToHex((char *)cfg_info->nwkskey, argv[1], 16);
    }
    else if (strcmp(argv[0], "appskey") == 0) {
        StrToHex((char *)cfg_info->appskey, argv[1], 16);
    }
    else
    {
        return -1;  
    }
    return 0;
}

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

void add_scan_rsp_list(const uint8_t* name, const uint8_t* mac, int8_t rssi)
{
    uint8_t num = rspNum;
    uint8_t nullmac[6] = {0};
    uint8_t i;
    
    if (num == 0) {
        memcpy(m_bls_scan_rsp[0].bleMac, mac, 6);
        strcpy((char*)m_bls_scan_rsp[0].bleName, (char*)name);
        m_bls_scan_rsp[0].bleRssi = rssi;
        rspNum = 1;
	//printf("add scan_rsp to list 0\r\n");
    }
    else if (num < 10) {
        for(i=0; i<num; i++){
            if(0 == memcmp(m_bls_scan_rsp[i].bleMac, mac, 6))
                return;
            
            if (0 == memcmp(m_bls_scan_rsp[i].bleMac, nullmac, 6))
            {
                memcpy(m_bls_scan_rsp[i].bleMac, mac, 6);
                strcpy((char*)m_bls_scan_rsp[i].bleName, (char*)name);
                m_bls_scan_rsp[i].bleRssi = rssi;
                rspNum++;
                return;
                //printf("add scan_rsp to list %d\r\n",i);
            }
        }
    }
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
}

void print_name(const ble_gap_evt_adv_report_t* p_adv_report) {
    uint16_t offset = 0;
    char name[DEV_NAME_LEN] = { 0 };
    char test[80];
    
    //uint16_t length = ble_advdata_search(p_adv_report->data.p_data, p_adv_report->data.len, &offset, BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE);
   /* if (length == 0) {
        // Look for the short local name if it was not found as complete.
        length = ble_advdata_search(p_adv_report->data.p_data, p_adv_report->data.len, &offset, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME);
    }
    
    if (length != 0) {
        memcpy(name, &p_adv_report->data.p_data[offset], length);
        NRF_LOG_INFO("name: %s", nrf_log_push(name));
    }*/
    //memcpy(name, &p_adv_report->data.p_data[offset], length);
    sprintf(test,"%s",p_adv_report->data.p_data[9]);
    NRF_LOG_INFO("%s", nrf_log_push(test));
}

void print_manufacturer_data(const ble_gap_evt_adv_report_t* p_adv_report) {
    uint16_t offset = 0;
    uint16_t length = ble_advdata_search(p_adv_report->data.p_data, p_adv_report->data.len, &offset, BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA);
    
    if (length != 0) {
        char data_string[1024] = { 0 };
        char* pos = data_string;
        for (int i = 0; i < length && i < 512; i++) {
            sprintf(pos, "%02x", p_adv_report->data.p_data[offset+i]);
            pos += 2;
        }
        
        NRF_LOG_INFO("manufacturer data: %s", nrf_log_push(data_string));
    }
}

void rssi_list_add(const uint32_t rssi){
      NRF_LOG_INFO("%d",rssi);
      rssi_list[address_list_length] = rssi;
}

void add_uuid_data(const ble_gap_evt_adv_report_t* p_adv_report){
    memcpy(uuid128, &p_adv_report->data.p_data[9], 16); //Copy UUID
    if(memcmp(uuid128, m_target_beacon.uuid128, 16) == 0){ //if uuid match.
      address_list_add(p_adv_report->peer_addr.addr);
      rssi_list_add(p_adv_report->rssi);
      print_address(p_adv_report);
      address_list_length++;
    }
}

void clear_data(){
  memset(address_list,0,sizeof(address_list));
  memset(rssi_list,0,sizeof(rssi_list));
}

void nRF_lora_init()
{
    /* load lora configuration*/
    u_fs_init();
    memset((uint8_t*)&g_lora_cfg,0,sizeof(g_lora_cfg));
    u_fs_read_lora_cfg(&g_lora_cfg);
    u_fs_check_lora_cfg(&g_lora_cfg);
    lora_init();
    printf("LoRa init success.\r\n");
}
void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
//  ้ถ้า scan แล้วไม่เจอในเวลาที่กำหนด ให้ timeout แล้ว scan ใหม่
    if (p_scan_evt->scan_evt_id == NRF_BLE_SCAN_EVT_SCAN_TIMEOUT) {
        NRF_LOG_INFO("Scan timed out.");
        clear_data();
        scan_start();
        return;
    }

    if (address_list_contains(p_scan_evt->params.filter_match.p_adv_report->peer_addr.addr) != false)
        return;
    
    add_uuid_data(p_scan_evt->params.filter_match.p_adv_report);
    add_scan_rsp_list(p_scan_evt->params.filter_match.p_adv_report->data.p_data[9],p_scan_evt->params.filter_match.p_adv_report->peer_addr.addr,p_scan_evt->params.filter_match.p_adv_report->rssi);
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
}


int main(void)
{
    ret_code_t err_code;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    ble_stack_init();
    scan_init();
    battery_voltage_init();
    nRF_lora_init();

    // Start execution.
    NRF_LOG_INFO("Start scan");
    scan_start();

    // Enter main loop.
    for (;;)
    {
        lora_process();

        NRF_LOG_FLUSH();
        if (!NRF_LOG_PROCESS())
        {
            power_manage();
        }
        if(loraconfigupdataflg)
        {
            u_fs_write_lora_cfg(&g_lora_cfg);
            loraconfigupdataflg = false;
        }

       
    }
}