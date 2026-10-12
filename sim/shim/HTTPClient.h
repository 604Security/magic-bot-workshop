// Pretend HTTP: every request goes to the pretend Sparkle in fake_sparkle.cpp.
#pragma once
#include <WiFi.h>
String fakeSparkle(const String& pathAndQuery, int* jpegSize);
struct HTTPClient {
  String path, body;
  WiFiClient client;
  void setTimeout(int) {}
  bool begin(const String& url) {
    int i = url.indexOf('/', 7);  // after "http://"
    path = i < 0 ? String("/") : url.substring(i);
    return true;
  }
  int GET() {
    int jpeg = 0;
    body = fakeSparkle(path, &jpeg);
    client.left = jpeg;
    return body.length() || jpeg ? 200 : 404;
  }
  String getString() { return body; }
  int getSize() { return client.left; }
  WiFiClient* getStreamPtr() { return &client; }
  void end() {}
};
