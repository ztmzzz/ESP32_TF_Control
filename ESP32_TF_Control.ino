/**
 * \file ESP32_TF_Control.ino
 * \original author Sakurashiling
 * \modified by ztmzzz
 * \HardWareSite https://oshwhub.com/ztmzzz/wo-de-wu-xian-guan-li-tf-ka-mo-kuai
 * \License Apache 2.0 署名原作者
 * \version v2.0
 **/
#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <ESPmDNS.h>
#include "web.h"  // web、样式文件
#include <SD_MMC.h>

// WIFI的SSID和PASSWORD,可以在下边代码中改创建热点或连接WIFI
char ssid[] = "ssid";
char pass[] = "password";
#define servername "tfserver"  // 定义服务器的名字

AsyncWebServer server(80);
bool SD_present = false;  // 控制TF卡是否存在

void setup(void) {
  SD_MMC.setPins(17, 18, 3, 46, 9, 10);
  pinMode(15, OUTPUT);
  pinMode(16, OUTPUT);
  digitalWrite(15, LOW);  // 模拟开关切换到TF卡直出
  digitalWrite(16, LOW);  // TF卡电源开
  Serial.begin(115200);   // 串口115200bps
  /*----------创建热点或连接WIFI二选一------------------*/
  /* 
  WiFi.softAP(ssid, pass); // 生成WIFI热点
  */
  /////////////////////////////////////////////////////
  WiFi.begin(ssid, pass);  // 连接WIFI
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected,IP adrress:");
  Serial.println(WiFi.localIP());
  /*--------------------------------------------------*/
  // 设置服务器名称，如果上边定义服务器的名字为tfserver，则可以使用 http://tfserver.local/ 进入。
  if (!MDNS.begin(servername)) {
    Serial.println("设置服务器名称出错");
  }
  /********* 服务器命令 **********/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.on("/getFiles", HTTP_GET, getFiles);
  server.on("/on", HTTP_GET, connect);
  server.on("/off", HTTP_GET, disconnect);
  server.on(
    "/fileUpload", HTTP_POST, [](AsyncWebServerRequest *request) {
      request->send(200);
    },
    handleFileUpload);
  server.on("/download", HTTP_GET, downloadFile);
  server.on("/delete", HTTP_GET, deleteFile);
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(SD_present));
  });
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
}

void getFiles(AsyncWebServerRequest *request) {
  if (!SD_present) {
    request->send(200, "application/json", "[]");
    return;
  }
  String json = "[";
  File root = SD_MMC.open("/");
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      file = root.openNextFile();
      continue;
    }
    if (json != "[") json += ',';
    json += "{\"name\":\"";
    json += file.name();
    json += "\",\"size\":";
    json += String(file.size());
    json += "}";
    file = root.openNextFile();
  }
  json += "]";
  file.close();
  request->send(200, "application/json", json);
}
void downloadFile(AsyncWebServerRequest *request) {
  String filename;
  if (request->hasParam("file")) {
    filename = request->getParam("file")->value();
    filename = "/" + filename;
  } else {
    request->send(400, "text/plain", "参数错误");
    return;
  }
  if (!SD_MMC.exists(filename)) {
    request->send(404, "text/plain", "文件不存在");
    return;
  }
  request->send(SD_MMC, filename, "application/octet-stream");
}
void deleteFile(AsyncWebServerRequest *request) {
  String filename;
  if (request->hasParam("file")) {
    filename = request->getParam("file")->value();
    filename = "/" + filename;
  } else {
    request->send(400, "text/plain", "参数错误");
    return;
  }
  if (!SD_MMC.exists(filename)) {
    request->send(404, "text/plain", "文件不存在");
    return;
  }
  if (SD_MMC.remove(filename)) {
    request->send(200, "text/plain", "删除成功");
  } else {
    request->send(500, "text/plain", "删除失败");
  }
}
// 将TF卡连接到ESP32
void connect(AsyncWebServerRequest *request) {
  if (SD_present) {
    request->send(200, "text/plain", "1");
    return;
  }
  digitalWrite(16, HIGH);  // TF卡电源关
  delay(500);
  digitalWrite(15, HIGH);  // 模拟开关切换到ESP32
  digitalWrite(16, LOW);   // TF卡电源开
  delay(500);
  if (!SD_MMC.begin("/sdcard", false, false, 80000, 10)) {
    Serial.println("初始化TF卡失败");
    SD_present = false;
  } else {
    Serial.println("初始化TF卡成功");
    SD_present = true;
  }
  request->send(200, "text/plain", String(SD_present));
}
// 将TF卡连接到外部设备(3D打印机)
void disconnect(AsyncWebServerRequest *request) {
  if (!SD_present) {
    request->send(200, "text/plain", "1");
    return;
  }
  SD_MMC.end();
  digitalWrite(16, HIGH);  // TF卡电源关
  delay(500);
  digitalWrite(15, LOW);  // 模拟开关切换到外部设备
  digitalWrite(16, LOW);  // TF卡电源开
  delay(500);
  SD_present = false;
  request->send(200, "text/plain", "1");
}
unsigned long startTime, endTime;
// 上传文件到TF卡
File uploadFile;
void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    filename = "/" + filename;
    if (SD_MMC.exists(filename)) {
      SD_MMC.remove(filename);
    }
    uploadFile = SD_MMC.open(filename, FILE_WRITE);
  }
  if (uploadFile) {
    uploadFile.write(data, len);
  }
  if (final) {
    uploadFile.close();
    Serial.print("Upload Size: ");
    Serial.println(index + len);
    request->send(200, "text/plain", "1");
  }
}
