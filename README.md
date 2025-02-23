# ESP_PC_Controller

**Управление питанием компьютера через ESP8266 (Wemos D1 mini) и Telegram-бот.**  
Проект эмулирует нажатие кнопки Power на материнской плате, используя N-канальный MOSFET (например, Troyka MOSFET от Амперки). Позволяет включать/выключать/перезагружать ПК и проверять его статус (через пинг) по командам в Telegram.

---

## Возможности

- **Включение/выключение компьютера** путём короткого «нажатия» MOSFET на контакты Power SW.  
- **Жёсткий сброс** (5-секундное удержание) для аварийного выключения/рестарта.  
- **Проверка статуса (Мониторинг)** через пинг до IP-адреса машины.  
- **Добавление новых администраторов** бота (хранятся в файле `/admins.txt` на LittleFS).  
- **Inline-кнопки**: удобное меню прямо в чате.  
- **Перезагрузка самого ESP** по команде `restart`.  

---

## Состав проекта

- **`ESP_PC_Controller.ino`** (или другое название `.ino`) — основной скетч Arduino.  
- **`/admins.txt`** (создаётся автоматически в LittleFS) — хранит список Chat ID администраторов.  
- **`LICENSE`** — текст лицензии Apache 2.0.

---

## Требования

### Оборудование

1. **Wemos D1 mini** (ESP8266) или аналог (NodeMCU).  
2. **N-канальный MOSFET-модуль** (например, Troyka N-MOSFET от Амперки).  
3. **Провода**: 3–4 штуки для соединения с Wemos (питание + сигнал), 2 провода для подключения к контактам Power SW на материнке.  
4. **Компьютер**, на котором нужно управлять кнопкой (через F_PANEL контакты).  

### Библиотеки Arduino

1. **[FastBot2](https://github.com/GyverLibs/FastBot2)**  
   - Позволяет создавать Telegram-боты на ESP.  
   - Заменяет старый FastBot. Убедитесь, что старая библиотека удалена/переименована.  
2. **ESP8266WiFi** (входит в стандартный ESP8266 Core).  
3. **ESPping** (можно установить через «Library Manager» Arduino IDE).  
4. **LittleFS** (также входит в ESP8266 Core, но нужно убедиться, что выбрано именно LittleFS, а не SPIFFS).  

Установка через Arduino IDE:
- **FastBot2**: скачайте ZIP из [репозитория](https://github.com/GyverLibs/FastBot2) → `Sketch → Include Library → Add .ZIP Library...`
или  `Tools → Manage Libraries...` → в поиске «FastBot2» → Install.
- **ESPping**: `Tools → Manage Libraries...` → в поиске «ESPping» → Install.

---

## Настройка проекта

1. **Откройте скетч** `ESP_PC_Controller.ino` (или как вы назвали).  
2. В начале файла поменяйте `WIFI_SSID`, `WIFI_PASS`, `BOT_TOKEN`, `BOT_OWNER_ID`, `PC_IP` на свои актуальные значения. Например:
   ```cpp
   #define WIFI_SSID      "MyWiFi"
   #define WIFI_PASS      "MyPass"
   #define BOT_TOKEN      "12345678:abcdefg..."
   #define BOT_OWNER_ID   "123456789" // Ваш Telegram Chat ID
   #define PC_IP          "192.168.1.2" // IP компа для пинга
