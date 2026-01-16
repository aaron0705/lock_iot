#include <WiFi.h>
#include <HardwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <vector>              // FIX: dùng vector<String>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "mbedtls/aes.h"
#include <base64.h> // Dùng để giải mã chuỗi Base64 trước khi Decrypt
using std::vector;

StaticJsonDocument<200> ins_json;
ins_json["off"] = static_cast<uint8_t>(0); 
ins_json["open"] = static_cast<uint8_t>(1); 
ins_json["busy"] = static_cast<uint8_t>(2); 
ins_json["ready"] = static_cast<uint8_t>(3); 
ins_json["opened"] = static_cast<uint8_t>(4);
ins_json["locked"] = static_cast<uint8_t>(5); 
ins_json["success"] = static_cast<uint8_t>(6); 
ins_json["fail"] = static_cast<uint8_t>(7);
ins_json["ping"] = static_cast<uint8_t>(8);

// LCD 16x2 SDA 21 SCL 22
LiquidCrystal_I2C lcd(0x27, 16, 2);

// UART2 cho STM32
HardwareSerial MySerial(2);

//Wifi
WiFiManager wm;

// MQTT
WiFiClient espClient;
PubSubClient client(espClient);
bool on_message = false;
JsonDocument doc_input;

// FIX: Dùng vector<String> để đệm dữ liệu khi chưa gửi được
vector<String> offline_buffer;

// Broker / Topic
const char* broker = "192.168.0.109";
const int   port = 1883;
const char* id = "1";
const char* user = "esp32-lock";
const char* passwd = "esp32@lock";
const char* location = "";
const char* type = "lock";
const char* topic = "iot/smarthome" + location + "/" + type + "/" + (char)id;

JsonDocument rx_msg;          // FIX: thay thế msg/mqtt_msg rời rạc

// Khóa và iv cho aes
String key = "1234567890123456";                // 16 byte (AES-128)
String iv  = "1234567890123456";                // 16 byte

// Gửi lệnh và đọc phản hồi từ STM32 (có timeout)
uint8_t send_stm32_command(const uint8_t &command, uint32_t timeout_ms = 100) 
{
  MySerial.write(command);                    // FIX: dùng print cho String
  MySerial.setTimeout(timeout_ms);
  uint8_t data = MySerial.read();
  return data;
}

// FIX: Callback đúng chữ ký PubSubClient
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Nhan tin nhan ma hoa: ");
  
  // 1. Chuyển payload thô thành String
  String encryptedPayload = "";
  for (int i = 0; i < length; i++) {
    encryptedPayload += (char)payload[i];
  }
  Serial.println(encryptedPayload);

  // 2. GIẢI MÃ AES
  String jsonDecrypted = decryptMessage(encryptedPayload);
  
  Serial.print("Sau khi giai ma: ");
  Serial.println(jsonDecrypted); // Kết quả sẽ là: {"cmd":"TURN_ON",...}

  if (jsonDecrypted == "") {
    Serial.println("Loi giai ma! Key sai hoac du lieu hong.");
    return;
  }
  // Lưu ý: Bây giờ ta deserialize từ chuỗi 'jsonDecrypted' chứ không phải 'payload' nữa
  DeserializationError error = deserializeJson(doc_input, jsonDecrypted);

  if (error) {
    Serial.print("Loi JSON sau khi giai ma: ");
    Serial.println(error.c_str());
    return;
  }

  on_message = true;
}

// FIX: Hàm kết nối lại MQTT và subscribe sau khi connect
void reconnectMQTT() 
{
  while (!client.connected()) 
  {
    Serial.print("[MQTT] Connecting...");
    if (client.connect(id, user, passwd)) 
    {
      client.subscribe(topic, 1);   // FIX: subscribe ở đây
    } 
    else 
    {
      Serial.printf("failed, rc=%d. Retry in 2s.\n", client.state());
      delay(2000);
    }
  }
}

// Publish an toàn: đệm nếu chưa gửi được
void safePublish(const char* topic, const String& payload) 
{
  if (WiFi.status() == WL_CONNECTED && client.connected()) 
  {
    if (!client.publish(topic, payload.c_str(), false)) 
    {
      // publish thất bại -> đệm
      offline_buffer.push_back(payload);
    }
  } 
  else 
  {
    // chưa sẵn sàng -> đệm
    offline_buffer.push_back(payload);
  }
}

// Đẩy dữ liệu đệm khi online
void flushOfflineBuffer() 
{
  if (WiFi.status() != WL_CONNECTED || !client.connected()) return;
  for (size_t i = 0; i < offline_buffer.size(); ) 
  {
    String entry = offline_buffer[i];
    bool ok = client.publish(topic, offline_buffer[i], false);
    if (ok) 
    {
      offline_buffer.erase(offline_buffer.begin() + i); // FIX: xóa đúng phần tử
      continue; // không tăng i
    }
    }
    i++; // nếu chưa gửi được thì để lại
  }
}

String AES_Encrypt(String text, String key, String iv) 
{
  // 1. Tính toán Padding (PKCS7)
  // AES làm việc theo khối 16 bytes. Nếu chuỗi không chia hết cho 16, ta phải bù vào.
  int inputLen = text.length();
  int remainder = inputLen % 16;
  int padding = 16 - remainder;
  int paddedLen = inputLen + padding;

  // Tạo buffer đầu vào đã được padding
  unsigned char input[paddedLen];
  // Copy dữ liệu gốc vào
  memcpy(input, text.c_str(), inputLen);
  // Điền các byte còn thiếu bằng giá trị của số byte thiếu (Quy tắc PKCS7)
  for (int i = inputLen; i < paddedLen; i++) {
    input[i] = (unsigned char)padding;
  }

  // 2. Chuẩn bị buffer đầu ra
  unsigned char output[paddedLen];

  // 3. Sao chép IV (Quan trọng)
  // Hàm mbedtls_aes_crypt_cbc sẽ thay đổi giá trị IV sau khi chạy
  // Nên ta cần một bản sao để không làm hỏng biến IV gốc
  unsigned char iv_copy[16];
  memcpy(iv_copy, iv.c_str(), 16);

  // 4. Cấu hình mbedTLS AES
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  // Key 128 bit = 16 bytes * 8
  mbedtls_aes_setkey_enc(&aes, key.c_str(), 128);

  // 5. Thực hiện mã hóa (Mode CBC)
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, paddedLen, iv_copy, input, output);

  // Giải phóng bộ nhớ AES context
  mbedtls_aes_free(&aes);

  // 6. Chuyển sang chuỗi HEX (Logic cũ của bạn)
  String encryptedHex = "";
  for (int i = 0; i < paddedLen; i++) {
    if (output[i] < 16) encryptedHex += "0";
    encryptedHex += String(output[i], HEX);
  }

  return encryptedHex;
}

// --- HÀM GIẢI MÃ AES-128 ECB ---
String decryptMessage(String encryptedBase64) 
{
  // 1. Giải mã Base64 trước (Vì dữ liệu mã hóa là binary, không truyền text được)
  String encryptedHex = base64::decode(encryptedBase64);
  
  // Chuẩn bị vùng nhớ
  int len = encryptedHex.length();
  if (len % 16 != 0) return ""; // Lỗi: Độ dài không chia hết cho block 16
  
  char * input = (char*) encryptedHex.c_str();
  unsigned char output[len + 1]; // Buffer chứa kết quả
  
  // 2. Cấu hình AES
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_dec(&aes, (const unsigned char*) key, 128);

  // 3. Giải mã từng khối 16 byte (Mode ECB)
  for (int i = 0; i < len; i += 16) {
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)(input + i), output + i);
  }
  mbedtls_aes_free(&aes);

  // 4. Xử lý Padding (PKCS7) để cắt bỏ các ký tự thừa cuối chuỗi
  // Khi mã hóa AES, nếu chuỗi không đủ 16 byte, nó tự thêm padding.
  // Khi giải mã xong, ta phải cắt nó đi.
  int pad = output[len - 1];
  if (pad > 0 && pad <= 16) {
      output[len - pad] = 0; // Kết thúc chuỗi sớm hơn
  } else {
      output[len] = 0; // An toàn
  }

  return String((char*)output);
}

void handle_MQTT_msg(JsonDocument doc_history)
{
  if (doc_history["command"] == ins_json["open"])   // thực hiện lệnh open
  {
    // Ping STM32
    // nhận về tín hiệu ngoại của stm32
    doc_history["status"] = send_stm32_command(ins_json["ping"]);
    if (doc_history["status"] == ins_json["ready"]) 
    {
      // Mở khóa
      // nhận về tín nội
      doc_history["response"] = send_stm32_command(ins_json["open"]);
      // mã hóa dư liệu gửi cho server
      lcd.print("Openning");
    } 
    else if (doc_history["status"] == ins_json["busy"])
    {
      doc_history["response"] = "FAILED";
      lcd.print("STM32 busy");
    }
    lcd.clear();
    lcd.setCursor(0, 0);

    if (doc_history["status"] == ins_json["busy"]) lcd.print("Openning");
    else lcd.print("Failed, try again");

    String plain_text;
    serializeJson(doc_history, plain_text);
    String cipher = AES_Encrypt(plain_text, key, iv);

    // Gửi callback/log về server
    safePublish(topic, cipher);
  }
  else if (doc_history["command"] == ins_json["off"])
  {
    // chưa nghĩ ra
  }
}

void physical_task(void *pvParameters)
{
  while (1)
  {
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void network_task(void *pvParameters)
{
  while (1)
  {
    // Giữ kết nối MQTT
    if (!client.connected()) 
    {
      reconnectMQTT();
    }
    client.loop();

    // Xử lý lệnh nhận được
    if (on_message) 
    {
      handle_MQTT_msg(doc_input);
      doc_input.clear(); // reset sau khi xử lý
    }

    // Đẩy dữ liệu đệm (nếu có)
    if (!offline_buffer.empty())
    {
      flushOfflineBuffer();
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void setup() 
{
  // UART debug + UART2 cho STM32
  Serial.begin(115200);
  MySerial.begin(115200, SERIAL_8N1, 16, 17); // RX=16, TX=17

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("WiFi: Connecting");

  while (!wm.autoConnect("MyESP32_AP"));
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi: Connected");

  // MQTT
  client.setServer(broker, port);
  client.setCallback(mqtt_callback);    // FIX: đúng hàm callback
  reconnectMQTT();                     // FIX: connect + subscribe

  xTaskCreatePinnedToCore(
    network_task,   // function
    "NetTask",      // name
    8192,           // stack size
    NULL,           // parameter
    1,              // priority
    NULL,           // task create 
    0               // core
  );

  // Tạo Task Button ở Core 1 (Ưu tiên cao hơn chút)
  xTaskCreatePinnedToCore(
    physical_task, 
    "PhyTask", 
    2048, 
    NULL, 
    2, 
    NULL, 
    1
  );
}

void loop()  
{
  
}
