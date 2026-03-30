#include <Preferences.h>
#include <ArduinoJson.h>

Preferences prefs;

void setup() {
  Serial.begin(921600);
  
  unsigned long start = millis();
  while (!Serial && millis() - start < 10000) {
    delay(10);  // даём время USB CDC и не перегружаем CPU
  }

//const char * _ssid[] = { "Color.muz", "Hamer.net", "ColorM.hb" };
//const char * _password[] = { "123456789", "HomeIgor68", "123456789" };


// Создаём JSON‑массив
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();

  JsonObject net1 = arr.add<JsonObject>();
  net1["ssid"] = "Hamer.net";
  net1["pass"] = "HomeIgor68";

  JsonObject net2 = arr.add<JsonObject>();
  net2["ssid"] = "Color.muz";
  net2["pass"] = "123456789";

  JsonObject net3 = arr.add<JsonObject>();
  net3["ssid"] = "ColorM.hb";
  net3["pass"] = "123456789";

  // Сериализуем в строку
  String out;
  serializeJson(doc, out);




  prefs.begin("settings", false); // false = запись

  prefs.clear();   // удаляет все ключи внутри namespace "settings"

  prefs.putString("wifi_list", out);
  prefs.putUChar("brightness", 180);
  prefs.putUChar("mode", 2);
  prefs.end();
  
  Serial.println("✅ Настройки сохранены в NVS!");


  prefs.begin("settings", true);
  String json = prefs.getString("wifi_list", "[]");
  prefs.end();
  JsonDocument docr;                 // без размера
  DeserializationError err = deserializeJson(docr, json);


  if (err) {
      Serial.print("Ошибка парсинга: ");
      Serial.println(err.c_str());
      return;
    }

  JsonArray arr2 = docr.as<JsonArray>();

  Serial.printf("Количество SSID: %d\n", arr2.size());

  for (JsonObject net : arr2) {
    String ssid = net["ssid"] | "";
    String pass = net["pass"] | "";
    Serial.printf("SSID: %s, PASS: %s\n", ssid.c_str(), pass.c_str());
  }
  Serial.println("✅ Проверка Настройки сохраненых в NVS!");
  
}

void loop() {
}