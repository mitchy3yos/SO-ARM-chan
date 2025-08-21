#include <LittleFS.h>
#include <M5Unified.h>
#include <ArduinoJson.h>
#include <SCServo.h>

// --- SCServo レジスタアドレス定義 (STS315用) ---
constexpr uint8_t ADDR_MIN_POSITION_LIMIT = 9;
constexpr uint8_t ADDR_MAX_POSITION_LIMIT = 11;
constexpr uint8_t ADDR_HOMING_OFFSET = 31;

// --- 構造体定義 ---
struct ServoConfig {
  int center;
  int min;
  int max;
};
struct SequenceItem {
  int patternIndex;
  unsigned long duration;
};

// ◆◆◆ 設定ファイルから読み込む変数群 ◆◆◆
String ssid;
String password;
String openaiApiKey;
String voicevoxApiKey;
float imu_threshold = 0.01f;
float imu_scale_factor = 320.0f;
unsigned long intervals[6];
int patternOffsets[3][6][2];
const int MAX_EYE_PATTERNS = 10;
String eyePatterns[MAX_EYE_PATTERNS];
int eyePatternsCount = 0;
const int MAX_SEQUENCE_ITEMS = 20;
SequenceItem sequence[MAX_SEQUENCE_ITEMS];
int sequenceCount = 0;

// --- グローバル変数 ---
ServoConfig servoConfigs[6];
unsigned long lastUpdates[6] = {0};
bool positive[6] = {true, true, true, true, true, true};
int currentPattern = 0;
SMS_STS st;
float ax, ay, az;
float prev_ax = 0.0f, prev_ay = 0.0f, prev_az = 0.0f;
const int AVG_COUNT = 3;
float delta_ax_buffer[AVG_COUNT] = {0};
float delta_ay_buffer[AVG_COUNT] = {0};
int buffer_index = 0;
LGFX_Sprite canvas(&M5.Lcd);
LGFX_Sprite eyeSprites[MAX_EYE_PATTERNS];
int currentSequenceIndex = 0;
unsigned long lastSequenceChange = 0;
int eyeBaseX, eyeBaseY;

// --- 関数 ---
void loadConfiguration() {
  if (!LittleFS.exists("/config.json")) {
    M5.Lcd.println("/config.json not found!");
    return;
  }
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    M5.Lcd.println("Failed to open config.json");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  if (error) {
    M5.Lcd.printf("config.json ERR:%s\n", error.c_str());
    return;
  }
  ssid = doc["wifi"]["ssid"] | "default_ssid";
  password = doc["wifi"]["password"] | "";
  openaiApiKey = doc["apiKeys"]["openai"] | "";
  voicevoxApiKey = doc["apiKeys"]["voicevox"] | "";
  imu_threshold = doc["imu"]["threshold"] | 0.01f;
  imu_scale_factor = doc["imu"]["scale_factor"] | 320.0f;
  JsonArray servo_intervals = doc["servo"]["intervals_ms"];
  for (int i = 0; i < 6; ++i) intervals[i] = servo_intervals[i] | 5000;
  JsonArray servo_patterns = doc["servo"]["patterns"];
  for (int p = 0; p < 3; ++p) {
    for (int s = 0; s < 6; ++s) {
      patternOffsets[p][s][0] = servo_patterns[p][s][0] | 0;
      patternOffsets[p][s][1] = servo_patterns[p][s][1] | 0;
    }
  }
  JsonArray eye_patterns_json = doc["display"]["eye_patterns"];
  eyePatternsCount = 0;
  for (JsonVariant v : eye_patterns_json) {
    if (eyePatternsCount < MAX_EYE_PATTERNS) {
      eyePatterns[eyePatternsCount++] = v.as<String>();
    }
  }
  JsonArray eye_sequence_json = doc["display"]["eye_sequence"];
  sequenceCount = 0;
  for (JsonObject item : eye_sequence_json) {
    if (sequenceCount < MAX_SEQUENCE_ITEMS) {
      sequence[sequenceCount].patternIndex = item["pattern"];
      sequence[sequenceCount].duration = item["duration"];
      sequenceCount++;
    }
  }
}

void setup() {
  M5.begin();

  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
  if (!LittleFS.begin()) {
    M5.Lcd.println("LittleFS FAILED!");
    while (1);
  }
  M5.Lcd.println("LittleFS OK.");
  loadConfiguration();
  M5.Lcd.printf("SSID: %s\n", ssid.c_str());
  M5.Lcd.printf("OpenAI: %s\n", openaiApiKey.substring(0, 10).c_str());
  M5.Lcd.println("IMU init start...");
  M5.Imu.begin();
  auto imu_type = M5.Imu.getType();
  const char* imu_name;
  switch (imu_type) {
    case m5::imu_none: imu_name = "none"; break;
    case m5::imu_sh200q: imu_name = "sh200q"; break;
    case m5::imu_mpu6886: imu_name = "mpu6886"; break;
    default: imu_name = "unknown"; break;
  }
  M5.Lcd.printf("IMU type: %s\n", imu_name);
  if (imu_type == m5::imu_none) {
      M5.Lcd.println("IMU FAILED!");
  } else {
      M5.Lcd.println("IMU OK.");
  }

  Serial1.begin(1000000, SERIAL_8N1, 2, 1);
  st.pSerial = &Serial1;
  delay(100);

  M5.Lcd.println("Servo setup start.");

  for (int i = 0; i < 6; i++) {
    int id = i + 1;
    if (st.Ping(id) != -1) {
        M5.Lcd.printf("ID%d Ping OK.\n", id);

        // ★ R を r に修正
        int minL_read = st.readWord(id, ADDR_MIN_POSITION_LIMIT);
        // ★ R を r に修正
        int maxL_read = st.readWord(id, ADDR_MAX_POSITION_LIMIT);
        // ★ R を r に修正
        int offset_read = st.readWord(id, ADDR_HOMING_OFFSET);
        
        if (minL_read != -1) {
            servoConfigs[i].min = minL_read;
        } else {
            servoConfigs[i].min = 1000;
            M5.Lcd.printf("ID%d Min Read FAILED!\n", id);
        }

        if (maxL_read != -1) {
            servoConfigs[i].max = maxL_read;
        } else {
            servoConfigs[i].max = 2000;
            M5.Lcd.printf("ID%d Max Read FAILED!\n", id);
        }

        servoConfigs[i].center = (servoConfigs[i].min + servoConfigs[i].max) / 2;
        
        Serial.printf("ID%d limits: min=%d max=%d (homing_offset=%d)\n",
                      id, servoConfigs[i].min, servoConfigs[i].max, (offset_read != -1 ? offset_read : -1));

    } else {
        M5.Lcd.printf("ID%d Ping FAILED!\n", id);
        servoConfigs[i].min = 1000;
        servoConfigs[i].max = 2000;
        servoConfigs[i].center = 1500;
    }
    delay(100);
  }

  delay(10000);

  const int eyeImageSize = 80;
  for (int i = 0; i < eyePatternsCount; i++) {
    eyeSprites[i].setColorDepth(16);
    eyeSprites[i].createSprite(eyeImageSize, eyeImageSize);
    if (LittleFS.exists(eyePatterns[i])) {
      eyeSprites[i].drawPngFile(LittleFS, eyePatterns[i].c_str());
    } else {
      eyeSprites[i].fillScreen(TFT_RED);
      eyeSprites[i].setCursor(0,0);
      eyeSprites[i].printf("NF:%d", i);
    }
  }
  canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
  eyeBaseX = canvas.width() / 2 - eyeImageSize / 2;
  eyeBaseY = canvas.height() / 2 - eyeImageSize / 2;
  lastSequenceChange = millis();
}

void loop() {
  unsigned long currentTime = millis();
  if (M5.Imu.update()) {
    auto data = M5.Imu.getImuData();
    ax = data.accel.x; ay = data.accel.y; az = data.accel.z;
    float delta_ax = ax - prev_ax; float delta_ay = ay - prev_ay;
  
    delta_ax_buffer[buffer_index] = delta_ax;
    delta_ay_buffer[buffer_index] = delta_ay;
    buffer_index = (buffer_index + 1) % AVG_COUNT;
    float avg_delta_ax = 0.0f, avg_delta_ay = 0.0f;
    for (int i = 0; i < AVG_COUNT; i++) {
      avg_delta_ax += delta_ax_buffer[i];
      avg_delta_ay += delta_ay_buffer[i];
    }
    avg_delta_ax /= AVG_COUNT; avg_delta_ay /= AVG_COUNT;
    float offsetX = 0.0f, offsetY = 0.0f;
    if (abs(avg_delta_ax) > imu_threshold) offsetX = avg_delta_ax * imu_scale_factor;
    if (abs(avg_delta_ay) > imu_threshold) offsetY = avg_delta_ay * imu_scale_factor;
    offsetX = max(-50.0f, min(offsetX, 50.0f));
    offsetY = max(-50.0f, min(offsetY, 50.0f));
    prev_ax = ax; prev_ay = ay; prev_az = az;
    if (sequenceCount > 0 && currentTime - lastSequenceChange >= sequence[currentSequenceIndex].duration) {
      currentSequenceIndex = (currentSequenceIndex + 1) % sequenceCount;
      lastSequenceChange = currentTime;
    }
    canvas.fillSprite(TFT_BLACK);
    if (sequenceCount > 0) {
      int patternIdx = sequence[currentSequenceIndex].patternIndex;
      if (patternIdx < eyePatternsCount) {
        int drawX = eyeBaseX + (int)offsetX;
        int drawY = eyeBaseY + (int)offsetY;
      
        eyeSprites[patternIdx].pushSprite(&canvas, drawX, drawY);
      }
    }
    canvas.pushSprite(0, 0);
  }
  M5.update();
  if (M5.BtnA.wasPressed()) {
    currentPattern = (currentPattern + 1) % 3;
    Serial.printf("Pattern changed to %d\n", currentPattern);
  }
  for (int i = 0; i < 6; i++) {
    if (currentTime - lastUpdates[i] >= intervals[i]) {
      lastUpdates[i] = currentTime;
      int offset = positive[i] ? patternOffsets[currentPattern][i][0] : patternOffsets[currentPattern][i][1];
      positive[i] = !positive[i];
      int pos = servoConfigs[i].center + offset;
      pos = max(servoConfigs[i].min, min(pos, servoConfigs[i].max));
      st.WritePosEx(i + 1, pos, 400, 0);
    }
  }
  delay(100);
}