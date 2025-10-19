/*
 * ESP32 Maximum Power Consumption Sketch
 *
 * This sketch is designed to consume as much power as possible by:
 * - Running WiFi in AP and Station mode simultaneously
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
#include "esp32/clk.h"

// Power consumption optimization settings
#define CPU_FREQ_MHZ 240

// Task handles
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t bluetoothTaskHandle = NULL;
TaskHandle_t cpuIntensiveTaskHandle = NULL;
TaskHandle_t peripheralTaskHandle = NULL;

// Bluetooth objects
BluetoothSerial SerialBT;
BluetoothA2DPSink a2dp_sink;

// WiFi credentials for station mode
const char* ssid = "ESP32_POWER_DRAIN";
const char* password = "12345678901234567890123456789012"; // 32 char password

// Global variables for power consumption
volatile uint32_t cpuIntensiveCounter = 0;
volatile bool runPowerDrain = true;

// BLE advertising parameters (moved to top for proper scoping)
static esp_ble_adv_params_t adv_params = {
  .adv_int_min = 0x20,
  .adv_int_max = 0x40,
  .adv_type = ADV_TYPE_IND,
  .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .channel_map = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

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
                  (unsigned long)cpuIntensiveCounter, ESP.getFreeHeap(), getCpuFrequencyMhz());
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

  Serial.println("Power saving features disabled");
}

void initializePeripherals() {
  // Initialize ADC
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);

  // Initialize DAC
  dac_output_enable(DAC_CHANNEL_1);
  dac_output_enable(DAC_CHANNEL_2);

  // Initialize PWM using member-by-member assignment
  ledc_timer_config_t ledc_timer = {};
  ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_timer.timer_num = LEDC_TIMER_0;
  ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
  ledc_timer.freq_hz = 5000;
  ledc_timer.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&ledc_timer);

  // Initialize I2C using member-by-member assignment
  i2c_config_t conf = {};
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = 21;
  conf.scl_io_num = 22;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 400000;
  i2c_param_config(I2C_NUM_0, &conf);
  i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

  // Initialize SPI using member-by-member assignment
  spi_bus_config_t buscfg = {};
  buscfg.miso_io_num = 19;
  buscfg.mosi_io_num = 23;
  buscfg.sclk_io_num = 18;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = 4096;
  spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);

  Serial.println("All peripherals initialized");
}

void startPowerConsumingTasks() {
  // Create high-priority tasks
  xTaskCreatePinnedToCore(wifiPowerDrainTask, "WiFiDrain", 8192, NULL, 5, &wifiTaskHandle, 0);
  xTaskCreatePinnedToCore(bluetoothPowerDrainTask, "BTDrain", 8192, NULL, 5, &bluetoothTaskHandle, 1);
  xTaskCreatePinnedToCore(cpuIntensiveTask, "CPUIntensive", 4096, NULL, 4, &cpuIntensiveTaskHandle, 0);
  xTaskCreatePinnedToCore(peripheralIntensiveTask, "PeripheralDrain", 4096, NULL, 3, &peripheralTaskHandle, 1);
}

void wifiPowerDrainTask(void *parameter) {
  Serial.println("Starting WiFi power drain task...");
  
  // Start as Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32_POWER_DRAIN_AP", "12345678901234567890123456789012");
  
  // Try to connect as station (will fail but consume power)
  WiFi.begin(ssid, password);
  
  while (runPowerDrain) {
    // Continuous WiFi scanning
    int n = WiFi.scanNetworks();
    
    // Continuous connection attempts
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password);
    }
    
    // Continuous AP operations
    WiFi.softAP("ESP32_POWER_DRAIN_AP", "12345678901234567890123456789012");
    
    // Small delay to prevent watchdog
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void bluetoothPowerDrainTask(void *parameter) {
  Serial.println("Starting Bluetooth power drain task...");
  
  // Initialize Bluetooth Classic
  if (SerialBT.begin("ESP32_POWER_DRAIN_BT")) {
    Serial.println("Bluetooth Classic started");
  }
  
  // Initialize A2DP Sink
  a2dp_sink.start("ESP32_POWER_DRAIN_A2DP");
  
  // Initialize BLE
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BTDM);
  
  esp_bluedroid_init();
  esp_bluedroid_enable();
  
  // Start BLE advertising
  esp_ble_gap_config_adv_data((uint8_t*)"\x02\x01\x06", 3);
  esp_ble_gap_start_advertising(&adv_params);
  
  while (runPowerDrain) {
    // Continuous Bluetooth operations
    if (SerialBT.available()) {
      SerialBT.read();
    }
    
    // Continuous BLE advertising restart
    esp_ble_gap_start_advertising(&adv_params);
    
    // Small delay to prevent watchdog
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void cpuIntensiveTask(void *parameter) {
  Serial.println("Starting CPU intensive task...");
  
  while (runPowerDrain) {
    // Matrix multiplication for CPU load
    volatile float matrix[16][16];
    volatile float result[16][16];
    
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) {
        matrix[i][j] = (float)(i * j + cpuIntensiveCounter);
      }
    }
    
    // Perform matrix multiplication
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) {
        result[i][j] = 0;
        for (int k = 0; k < 16; k++) {
          result[i][j] += matrix[i][k] * matrix[k][j];
        }
      }
    }
    
    // Trigonometric calculations
    volatile float trig_result = 0;
    for (int i = 0; i < 1000; i++) {
      trig_result += sin(i * 0.001) * cos(i * 0.002) * tan(i * 0.003);
    }
    
    cpuIntensiveCounter++;
    
    // Small delay to prevent watchdog
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void peripheralIntensiveTask(void *parameter) {
  Serial.println("Starting peripheral intensive task...");
  
  // PWM channel configuration
  ledc_channel_config_t ledc_channel = {};
  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_0;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num = 2;
  ledc_channel.duty = 0;
  ledc_channel.hpoint = 0;
  ledc_channel_config(&ledc_channel);
  
  // SPI device configuration
  spi_device_interface_config_t devcfg = {};
  devcfg.clock_speed_hz = 1000000;
  devcfg.mode = 0;
  devcfg.spics_io_num = 5;
  devcfg.queue_size = 7;
  
  spi_device_handle_t handle;
  spi_bus_add_device(HSPI_HOST, &devcfg, &handle);
  
  while (runPowerDrain) {
    // ADC readings
    int adc_value = adc1_get_raw(ADC1_CHANNEL_0);
    
    // DAC output
    dac_output_voltage(DAC_CHANNEL_1, (adc_value >> 4) & 0xFF);
    dac_output_voltage(DAC_CHANNEL_2, (adc_value >> 8) & 0xFF);
    
    // PWM operations
    for (int duty = 0; duty < 8192; duty += 100) {
      ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
    
    // I2C operations
    uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
    i2c_master_write_to_device(I2C_NUM_0, 0x48, data, 4, 1000 / portTICK_PERIOD_MS);
    
    // SPI operations
    spi_transaction_t t = {};
    t.length = 32;
    t.tx_buffer = &adc_value;
    t.rx_buffer = NULL;
    spi_device_transmit(handle, &t);
    
    // Small delay to prevent watchdog
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
