/*
 * ESP32 Maximum Power Consumption Sketch
 * 
 * This sketch is designed to consume as much power as possible by:
 * - Running WiFi in AP and Station mode simultaneously
 * - Broadcasting WiFi beacons and scanning continuously
 * - Running Bluetooth Classic and BLE simultaneously
 * - Utilizing all available peripherals
 * - Running CPU-intensive tasks
 * - Disabling all power saving features
 * 
 * WARNING: This sketch will generate significant heat and consume maximum power!
 * Use proper cooling and power supply (5V/2A+ recommended)
 */

#include "WiFi.h"
#include "BluetoothSerial.h"
#include "BluetoothA2DPSink.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_device.h"
#include "esp_sleep.h"
#include "esp_pm.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "soc/rtc.h"
#include "esp_clk.h"

// Power consumption optimization settings
#define CPU_FREQ_MHZ 240
#define WIFI_POWER_SAVE WIFI_PS_NONE
#define BT_POWER_LEVEL ESP_PWR_LVL_P9

// Task handles
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t bluetoothTaskHandle = NULL;
TaskHandle_t cpuIntensiveTaskHandle = NULL;
TaskHandle_t peripheralTaskHandle = NULL;
TaskHandle_t beaconTaskHandle = NULL;

// Bluetooth objects
BluetoothSerial SerialBT;
BluetoothA2DPSink a2dp_sink;

// WiFi credentials for station mode
const char* ssid = "ESP32_POWER_DRAIN";
const char* password = "12345678901234567890123456789012"; // 32 char password

// Beacon data
uint8_t beaconPacket[109] = {
  0x80, 0x00, 0x00, 0x00, // Frame Control
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination MAC
  0x24, 0x0a, 0xc4, 0x00, 0x00, 0x00, // Source MAC
  0x24, 0x0a, 0xc4, 0x00, 0x00, 0x00, // BSSID
  0x00, 0x00, // Sequence number
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
  0x64, 0x00, // Beacon interval
  0x31, 0x04, // Capability info
  0x00, 0x06, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, // SSID
  0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // Supported rates
  0x03, 0x01, 0x01, // Channel
  0x05, 0x04, 0x01, 0x02, 0x00, 0x00, // Traffic indication map
  0x07, 0x06, 0x55, 0x53, 0x20, 0x01, 0x0b, 0x1e, // Country info
  0x2a, 0x01, 0x00, // Power constraint
  0x32, 0x04, 0x30, 0x48, 0x60, 0x6c, // Extended supported rates
  0x2d, 0x1a, 0xad, 0x01, 0x17, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // HT capabilities
  0x3d, 0x16, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // HT information
  0x4a, 0x0e, 0x14, 0x00, 0x0a, 0x00, 0x2c, 0x01, 0xc8, 0x00, 0x14, 0x00, 0x05, 0x00, 0x19, 0x00, // Overlapping BSS scan parameters
  0x7f, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Extended capabilities
  0xdd, 0x18, 0x00, 0x50, 0xf2, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Vendor specific
};

// Global variables for power consumption
volatile uint32_t cpuIntensiveCounter = 0;
volatile bool runPowerDrain = true;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Maximum Power Consumption Sketch Starting...");
  
  // Disable all power saving features
  disablePowerSaving();
  
  // Set CPU to maximum frequency
  setCpuFrequencyMhz(CPU_FREQ_MHZ);
  
  // Initialize all peripherals
  initializePeripherals();
  
  // Start all power-consuming tasks
  startPowerConsumingTasks();
  
  Serial.println("All power-consuming tasks started!");
  Serial.println("WARNING: This will consume maximum power and generate heat!");
}

void loop() {
  // Main loop - keep CPU busy
  for (int i = 0; i < 1000; i++) {
    // CPU-intensive calculations
    volatile float result = 0;
    for (int j = 0; j < 100; j++) {
      result += sin(i * j) * cos(i + j) * tan(i - j);
    }
    cpuIntensiveCounter++;
  }
  
  // Print status every 10 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 10000) {
    lastPrint = millis();
    Serial.printf("CPU Counter: %lu, Free Heap: %d, CPU Freq: %d MHz\n", 
                  cpuIntensiveCounter, ESP.getFreeHeap(), getCpuFrequencyMhz());
  }
  
  delay(1); // Minimal delay to prevent watchdog
}

void disablePowerSaving() {
  // Disable sleep modes
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  
  // Set power management to maximum performance
  esp_pm_config_esp32_t pm_config = {
    .max_freq_mhz = CPU_FREQ_MHZ,
    .min_freq_mhz = CPU_FREQ_MHZ,
    .light_sleep_enable = false
  };
  esp_pm_configure(&pm_config);
  
  // Disable WiFi power saving
  WiFi.setSleep(false);
  
  // Set Bluetooth power level to maximum
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  bt_cfg.power_level = BT_POWER_LEVEL;
  
  Serial.println("Power saving features disabled");
}

void setCpuFrequencyMhz(uint32_t cpu_freq_mhz) {
  rtc_cpu_freq_config_t conf;
  rtc_clk_cpu_freq_get_config(&conf);
  conf.freq_mhz = cpu_freq_mhz;
  rtc_clk_cpu_freq_set_config(&conf);
  Serial.printf("CPU frequency set to %d MHz\n", cpu_freq_mhz);
}

void initializePeripherals() {
  // Initialize ADC
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
  
  // Initialize DAC
  dac_output_enable(DAC_CHANNEL_1);
  dac_output_enable(DAC_CHANNEL_2);
  
  // Initialize PWM
  ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .duty_resolution = LEDC_TIMER_13_BIT,
    .freq_hz = 5000,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&ledc_timer);
  
  // Initialize I2C
  i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 21,
    .scl_io_num = 22,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 400000
  };
  i2c_param_config(I2C_NUM_0, &conf);
  i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
  
  // Initialize SPI
  spi_bus_config_t buscfg = {
    .miso_io_num = 19,
    .mosi_io_num = 23,
    .sclk_io_num = 18,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4096
  };
  spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
  
  Serial.println("All peripherals initialized");
}

void startPowerConsumingTasks() {
  // Create high-priority tasks
  xTaskCreatePinnedToCore(wifiPowerDrainTask, "WiFiDrain", 8192, NULL, 5, &wifiTaskHandle, 0);
  xTaskCreatePinnedToCore(bluetoothPowerDrainTask, "BTDrain", 8192, NULL, 5, &bluetoothTaskHandle, 1);
  xTaskCreatePinnedToCore(cpuIntensiveTask, "CPUIntensive", 4096, NULL, 4, &cpuIntensiveTaskHandle, 0);
  xTaskCreatePinnedToCore(peripheralIntensiveTask, "PeripheralDrain", 4096, NULL, 3, &peripheralTaskHandle, 1);
  xTaskCreatePinnedToCore(beaconBroadcastTask, "BeaconBroadcast", 4096, NULL, 6, &beaconTaskHandle, 0);
}

void wifiPowerDrainTask(void *parameter) {
  // Start WiFi in both AP and Station mode
  WiFi.mode(WIFI_AP_STA);
  
  // Configure AP
  WiFi.softAP(ssid, password, 1, 0, 8); // 8 max connections
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  // Try to connect to WiFi (will fail but consume power)
  WiFi.begin("NONEXISTENT_NETWORK", "password");
  
  while (runPowerDrain) {
    // Continuous WiFi scanning
    int n = WiFi.scanNetworks();
    
    // Continuous connection attempts
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin("FAKE_NETWORK_1", "password");
      delay(100);
      WiFi.begin("FAKE_NETWORK_2", "password");
      delay(100);
      WiFi.begin("FAKE_NETWORK_3", "password");
    }
    
    // Generate WiFi traffic
    WiFi.softAPdisconnect(false);
    delay(10);
    WiFi.softAP(ssid, password, 1, 0, 8);
    
    delay(50); // Short delay to prevent watchdog
  }
}

void bluetoothPowerDrainTask(void *parameter) {
  // Initialize Bluetooth Classic
  SerialBT.begin("ESP32_PowerDrain_BT");
  
  // Initialize Bluetooth A2DP
  a2dp_sink.start("ESP32_PowerDrain_A2DP");
  
  // Initialize BLE
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BTDM);
  
  while (runPowerDrain) {
    // Continuous Bluetooth operations
    if (SerialBT.hasClient()) {
      SerialBT.println("Power drain data: " + String(millis()));
    }
    
    // BLE advertising
    esp_ble_adv_data_t adv_data = {
      .set_scan_rsp = false,
      .include_name = true,
      .include_txpower = true,
      .min_interval = 0x0006,
      .max_interval = 0x0010,
      .appearance = 0x00,
      .manufacturer_len = 0,
      .p_manufacturer_data = NULL,
      .service_data_len = 0,
      .p_service_data = NULL,
      .service_uuid_len = 0,
      .p_service_uuid = NULL,
      .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT)
    };
    
    esp_ble_gap_config_adv_data(&adv_data);
    esp_ble_gap_start_advertising(&adv_params);
    
    delay(100);
  }
}

void cpuIntensiveTask(void *parameter) {
  while (runPowerDrain) {
    // Matrix multiplication
    float matrixA[16][16];
    float matrixB[16][16];
    float matrixC[16][16];
    
    // Fill matrices with random data
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) {
        matrixA[i][j] = (float)random(1000) / 1000.0;
        matrixB[i][j] = (float)random(1000) / 1000.0;
      }
    }
    
    // Perform matrix multiplication
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) {
        matrixC[i][j] = 0;
        for (int k = 0; k < 16; k++) {
          matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
        }
      }
    }
    
    // FFT-like calculations
    for (int i = 0; i < 1000; i++) {
      volatile float result = 0;
      for (int j = 0; j < 100; j++) {
        result += sin(i * j * 0.01) * cos(i + j * 0.01);
      }
    }
    
    delay(1); // Minimal delay
  }
}

void peripheralIntensiveTask(void *parameter) {
  while (runPowerDrain) {
    // ADC readings
    for (int i = 0; i < 100; i++) {
      adc1_get_raw(ADC1_CHANNEL_0);
    }
    
    // DAC output
    static uint8_t dacValue = 0;
    dac_output_voltage(DAC_CHANNEL_1, dacValue);
    dac_output_voltage(DAC_CHANNEL_2, 255 - dacValue);
    dacValue++;
    
    // PWM output
    ledc_channel_config_t ledc_channel = {
      .gpio_num = 2,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = 2048, // 50% duty cycle
      .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
    
    // I2C operations
    uint8_t data[32];
    for (int i = 0; i < 32; i++) {
      data[i] = random(256);
    }
    i2c_master_write_to_device(I2C_NUM_0, 0x48, data, 32, 1000);
    
    // SPI operations
    spi_device_handle_t handle;
    spi_device_interface_config_t devcfg = {
      .clock_speed_hz = 1000000,
      .mode = 0,
      .spics_io_num = 5,
      .queue_size = 7
    };
    spi_bus_add_device(HSPI_HOST, &devcfg, &handle);
    
    spi_transaction_t t = {
      .length = 32 * 8,
      .tx_buffer = data,
      .rx_buffer = data
    };
    spi_device_transmit(handle, &t);
    spi_bus_remove_device(handle);
    
    delay(10);
  }
}

void beaconBroadcastTask(void *parameter) {
  while (runPowerDrain) {
    // Send beacon frames
    esp_wifi_80211_tx(WIFI_IF_AP, beaconPacket, sizeof(beaconPacket), true);
    
    // Modify beacon data slightly
    beaconPacket[10] = (beaconPacket[10] + 1) % 256;
    beaconPacket[11] = (beaconPacket[11] + 1) % 256;
    
    delay(100); // Send beacons every 100ms
  }
}

// BLE advertising parameters
static esp_ble_adv_params_t adv_params = {
  .adv_int_min = 0x20,
  .adv_int_max = 0x40,
  .adv_type = ADV_TYPE_IND,
  .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .channel_map = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};