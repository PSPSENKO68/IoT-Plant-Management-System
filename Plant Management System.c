#define BLYNK_TEMPLATE_ID "TMPL6nj2qFYdJ"
#define BLYNK_TEMPLATE_NAME "Plant Management System"
#define BLYNK_AUTH_TOKEN "yKPxVws94UjHcbQCbttf0mRnix02U_FV"

#define MOISTURE_SENSOR_PIN 34 // Chân cảm biến độ ẩm đất (analog)
#define RELAY_PIN 5            // Chân điều khiển relay
#define DRY_THRESHOLD 2000     // Ngưỡng khô, điều chỉnh theo cảm biến của bạn

#define LIGHT_SENSOR_PIN 35    // Chân cảm biến ánh sáng (analog)
#define LED_PIN 2              // Chân điều khiển LED
#define DARK_THRESHOLD 2000    // Ngưỡng tối, điều chỉnh theo nhu cầu

#define DHTPIN 14  // Chân mà DHT11 được kết nối (ví dụ GPIO 14)
#define DHTTYPE DHT11


#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

char ssid[] = "LCK";           // Thay tên WiFi của bạn
char pass[] = "12345678";      // Thay mật khẩu WiFi của bạn

int wateringMode = 2;          // 0: Auto, 1: Scheduled, 2: Manual
int wateringSwitch = 0;        // 0: Off, 1: On
int lightMode = 2;             // 0: Auto, 1: Scheduled, 2: Manual
int lightSwitch = 0;           // 0: Off, 1: On

double soilMoisture = 0;
double lightValue = 0;

unsigned long currentTime;
unsigned long lastCheckTime = 0;           // Biến lưu thời gian lần kiểm tra gần nhất
const unsigned long checkInterval = 1000;  // Khoảng thời gian kiểm tra (1 giây)


unsigned long totalLightOnTime = 10001;     // Tổng thời gian đèn đã sáng
const unsigned long printInterval = 1 * 60 * 1000; // 15 phút
unsigned long lightOnStartTime = 0;     // Thời điểm bắt đầu bật đèn
unsigned long lastPrintTime = 0;        // Thời điểm lần in ra gần nhất

DHT dht(DHTPIN, DHTTYPE);

// Cập nhật chế độ tưới từ Blynk
BLYNK_WRITE(V3) {
  wateringMode = param.asInt();
}

// Cập nhật chế độ đèn từ Blynk
BLYNK_WRITE(V4) {
  lightMode = param.asInt();
}

// Cập nhật công tắc tưới từ Blynk
BLYNK_WRITE(V5) {
  wateringSwitch = param.asInt();
}

// Cập nhật công tắc đèn từ Blynk
BLYNK_WRITE(V6) {
  lightSwitch = param.asInt();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Đang kết nối Wi-Fi...");
  }
  Serial.println("Connected to WiFi");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Blynk.virtualWrite(V0, 1); // Báo trạng thái kết nối

  dht.begin();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

}

void loop() {
  Blynk.run();

  currentTime = millis();
  if (currentTime - lastCheckTime >= checkInterval) {
    lastCheckTime = currentTime;
    checkWateringMode();
    //checkLightMode();
    //huminity_temperature();
  }
  

  //Blynk.virtualWrite(V1, soilMoisture); // Gửi độ ẩm đất lên Blynk V1
  //Blynk.virtualWrite(V2, lightValue);   // Gửi ánh sáng lên Blynk V2
}

void huminity_temperature(){
  // Đọc dữ liệu từ DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // Kiểm tra nếu đọc thành công (tránh NAN)
  if (!isnan(humidity) && !isnan(temperature)) {
    //Blynk.virtualWrite(V7, temperature);  // Giả sử V7 là datastream cho nhiệt độ
    //Blynk.virtualWrite(V8, humidity);     // Giả sử V8 là datastream cho độ ẩm
    Serial.print("Nhiệt độ: ");
    Serial.print(temperature);
    Serial.print(" °C, Độ ẩm: ");
    Serial.print(humidity);
    Serial.println(" %");
  }

}

// Đọc độ ẩm đất
double readSoilMoisture() {
  return analogRead(MOISTURE_SENSOR_PIN);
}

// Đọc giá trị ánh sáng
double readLightValue() {
  return analogRead(LIGHT_SENSOR_PIN);
}

// Điều khiển relay
void setRelay(bool state) {
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
}

// Điều khiển đèn LED
void setLED(bool state) {
  digitalWrite(LED_PIN, state ? HIGH : LOW);
}

// Kiểm tra và thực hiện tưới nước
void checkWateringMode() {
  soilMoisture = readSoilMoisture();
  Serial.print("Độ ẩm đất: ");
  Serial.println(soilMoisture);

  if (wateringMode == 2 && wateringSwitch == 1) {
    wateringSwitch = 0;
    setRelay(true);
    delay(5000); // Tưới nước trong 5 giây
    setRelay(false);
    Blynk.virtualWrite(V5, 0); // Tắt nút sau khi tưới
    Blynk.logEvent("03");
    Serial.println("Tưới nước thủ công");
  } 
  else if (wateringMode == 0 && soilMoisture > DRY_THRESHOLD) {
    Blynk.logEvent("01");
    setRelay(true);
    delay(5000);
    setRelay(false);
    Blynk.logEvent("Da_tuoi_nuoc");
    Serial.println("Tưới nước tự động");
  } 
  else if (wateringMode == 1) {
    setRelay(true);
    delay(5000);
    setRelay(false);
    Blynk.logEvent("Da_tuoi_nuoc");
    Serial.println("Tưới nước theo lịch");
  }
}

// Kiểm tra và điều khiển đèn
void checkLightMode() {
  lightValue = readLightValue();
  Serial.print("Độ sáng: ");
  Serial.println(lightValue);


  if(lightMode == 2){
    int ledState = digitalRead(LED_PIN);
    if(lightSwitch == 1 && lightSwitch != ledState){
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Bật đèn thủ công.");
      lightOnStartTime = millis(); // Lưu thời điểm bật đèn
    }else if(lightSwitch == 0 && lightSwitch != ledState){
      digitalWrite(LED_PIN, LOW);
      totalLightOnTime += millis() - lightOnStartTime;
      Serial.println("Tắt đèn thủ công.");
      totalLightOnTime = 10001;
    }
    if (currentTime - lastPrintTime >= printInterval) {
      lastPrintTime = currentTime;
      totalLightOnTime += currentTime - lightOnStartTime; // Cộng dồn thời gian đang sáng
      
      Serial.print("Tổng thời gian đèn đã sáng: ");
      
      Serial.print(totalLightOnTime / 60000); // Đổi ra phút
      Serial.println(" phút");
      
      // Cập nhật thời điểm bắt đầu lại cho chu kỳ tiếp theo
      lightOnStartTime = currentTime;
    }
  }
  else if (lightMode == 0 && lightValue > DARK_THRESHOLD) {
    setLED(true);
    Blynk.logEvent("02");
    Serial.println("Đèn tự động bật");
  } 
  else if (lightMode == 0 && lightValue <= DARK_THRESHOLD) {
    setLED(false);
    Serial.println("Đèn tự động tắt");
  }
  else if (lightMode == 1) {
    setLED(true);
    delay(5000);
    setLED(false);
    Blynk.logEvent("Den_Da_Bat");
    Serial.println("Đèn theo lịch bật");
  } 
  
}
