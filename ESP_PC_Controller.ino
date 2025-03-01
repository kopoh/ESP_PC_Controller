/*
 *  Copyright [2025] kopoh
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


/***************************************************************
   Пример бота ESP_PC_Controller.
   - Хранит список админов в /admins.txt (LittleFS)
   - Добавляет админов командой /make_admin <chat_id>
   - Обрабатывает команды:
       /help
       Включить / Выключить / Жёсткий сброс ПК
       Мониторинг (Ping)
       restart (ESP)
       /make_admin <id>
   - Показывает INLINE-меню (кнопки) по команде "/menu"
     или при старте, если хотите
 ***************************************************************/

#include <FastBot2.h>
#include <ESP8266WiFi.h>
#include <ESPping.h>
#include <LittleFS.h>

// ================== НАСТРОЙКИ ====================
#define WIFI_SSID      "MyWiFi" // wiwiwi wawawa
#define WIFI_PASS      "MyPass" // wiwiwi wawawa
#define BOT_TOKEN      "12345678:abcdefg..." // Ваш Telegram Bot Tocken
#define BOT_OWNER_ID   "123456789" // Ваш Telegram Chat ID
#define PC_IP          "192.168.1.2" // IP компа для пинга
#define MOSFET_PIN      0                         // GPIO для MOSFET

// ================================================
FastBot2 bot;                  // Объект бота
String admins = BOT_OWNER_ID;  // Список админов (через запятую)
bool restartFlag = false;      // Флаг для перезагрузки

// ------------------------------------------------
//                 ПРОТОТИПЫ
// ------------------------------------------------
void connectWiFi();
void readAdminsFromFile();
void writeAdminsToFile(const String &adminList);
void handleUpdate(fb::Update &u);

bool isAdmin(const String &chatID);
void addAdmin(const String &chatID, const String &cmdText);
void sendHelp(const String &chatID);

void startPC(const String &chatID);
void shutdownPC(const String &chatID);
void hardReset(const String &chatID);
void monitorPC(const String &chatID);

// INLINE-меню
void sendInlineMenu(const String &chatID);
void handleInlineCallback(const String &chatID, const String &data);

void setup() {
  Serial.begin(115200);

  // MOSFET INIT
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW);

  // FS
  if (!LittleFS.begin()) {
    Serial.println("Ошибка монтирования LittleFS!");
  } else {
    Serial.println("LittleFS ок.");
    readAdminsFromFile();
  }

  // WiFi
  connectWiFi();

  // Bot
  bot.setToken(BOT_TOKEN);
  bot.skipUpdates(-1);    // Пропустить старые сообщения
  bot.onUpdate(handleUpdate);
  // bot.setPollMode(fb::Poll::Sync, 4000); // или Long, по желанию

  // Сообщение владельцу
  bot.sendMessage(fb::Message("Бот запущен! Введите /help или /menu", BOT_OWNER_ID));

  Serial.println("Setup завершён.");
}

void loop() {
  bot.tick();  // Опрос Телеграм-сервера

  if (restartFlag) {
    bot.tickManual();
    ESP.restart();
  }
}

// ------------------------------------------------
//          ОСНОВНОЙ ОБРАБОТЧИК ОБНОВЛЕНИЙ
// ------------------------------------------------
void handleUpdate(fb::Update &u) {
  // Если пришло сообщение от пользователя
  if (u.isMessage()) {
    String chatID = u.message().chat().id().toString();
    String text   = u.message().text().decodeUnicode(); // Если хотим декодировать Unicode

    Serial.print("Сообщение от "); Serial.print(chatID);
    Serial.print(": ");           Serial.println(text);

    // Проверим, админ ли
    if (!isAdmin(chatID)) {
      bot.sendMessage(fb::Message("У вас нет доступа!", chatID));
      return;
    }

    // Обрабатываем текст (команды)
    if (text == "/help") {
      sendHelp(chatID);

    } else if (text.startsWith("/make_admin")) {
      addAdmin(chatID, text);

    } else if (text == "/menu") {
      // Покажем inline-кнопки
      sendInlineMenu(chatID);

    } else if (text == "restart") {
      restartFlag = true;
      bot.sendMessage(fb::Message("ESP перезагружается...", chatID));

    } else if (text == "Включить/Выключить ПК") {
      startPC(chatID);
    } else if (text == "Жёсткий сброс") {
      hardReset(chatID);
    } else if (text == "Мониторинг") {
      monitorPC(chatID);
    } 
    // ... любые другие команды/обработки

  }

  // Если нажали на INLINE-кнопку (callback query)
  if (u.isQuery()) {
    // Запомним ID чата и данные коллбэка
    String chatID = u.query().message().chat().id().toString();
    String data   = u.query().data();

    Serial.print("CallbackQuery! chat="); Serial.print(chatID);
    Serial.print(", data="); Serial.println(data);

    // Ответим Телеграму (чтобы анимация кнопки прекратилась)
    bot.answerCallbackQuery(u.query().id()); // пустой ответ

    // Проверим, админ ли
    if (!isAdmin(chatID)) {
      bot.sendMessage(fb::Message("Нет доступа!", chatID));
      return;
    }

    // Обработаем коллбэк
    handleInlineCallback(chatID, data);
  }
}

// ------------------------------------------------
//             ОБРАБОТЧИК INLINE КНОПОК
// ------------------------------------------------
void handleInlineCallback(const String &chatID, const String &data) {
  // data — это то, что мы передали вторым параметром addButton("Text","data")
  if (data == "startPC") {
    startPC(chatID);

  } else if (data == "shutdownPC") {
    shutdownPC(chatID);

  } else if (data == "hardReset") {
    hardReset(chatID);

  } else if (data == "monitor") {
    monitorPC(chatID);

  } else if (data == "restart") {
    restartFlag = true;
    bot.sendMessage(fb::Message("ESP перезагружается...", chatID));

  } else if (data == "makeAdmin") {
    // Можно предложить ввести команду: /make_admin <id>
    bot.sendMessage(fb::Message("Введите /make_admin <id>", chatID));
  }
}

// ------------------------------------------------
//            ФУНКЦИИ ДЛЯ ПК
// ------------------------------------------------
void startPC(const String &chatID) {
  digitalWrite(MOSFET_PIN, HIGH);
  delay(1000);
  digitalWrite(MOSFET_PIN, LOW);
  bot.sendMessage(fb::Message("PC включён.", chatID));
}

void shutdownPC(const String &chatID) {
  digitalWrite(MOSFET_PIN, HIGH);
  delay(1000);
  digitalWrite(MOSFET_PIN, LOW);
  bot.sendMessage(fb::Message("PC выключен.", chatID));
}

void hardReset(const String &chatID) {
  digitalWrite(MOSFET_PIN, HIGH);
  delay(5000);
  digitalWrite(MOSFET_PIN, LOW);
  bot.sendMessage(fb::Message("Выполнен жёсткий сброс.", chatID));
}

void monitorPC(const String &chatID) {
  IPAddress ip;
  ip.fromString(PC_IP);
  if (Ping.ping(ip, 5)) {
    float avg = Ping.averageTime();
    bot.sendMessage(fb::Message("Статус ПК: Включён. Пинг ~ " + String(avg, 1) + " мс", chatID));
  } else {
    bot.sendMessage(fb::Message("Статус ПК: Выключен. Пинг не прошёл.", chatID));
  }
}

// ------------------------------------------------
//               СПРАВКА И АДМИНЫ
// ------------------------------------------------
void sendHelp(const String &chatID) {
  String helpText =
    "Команды:\n"
    "/help - Показать это сообщение\n"
    "/menu - Показать inline-кнопки\n"
    "/make_admin <id> - Добавить администратора\n"
    "Включить ПК, Выключить ПК, Жёсткий сброс\n"
    "Мониторинг - Проверка статуса\n"
    "restart - Перезагрузка ESP\n";
  bot.sendMessage(fb::Message(helpText, chatID));
}

// Добавление администратора
void addAdmin(const String &chatID, const String &cmdText) {
  // cmdText: "/make_admin 12345678"
  String newAdmin = cmdText.substring(12);
  newAdmin.trim();

  if (newAdmin.toInt() == 0) {
    bot.sendMessage(fb::Message("Укажите корректный ID (число).", chatID));
    return;
  }
  if (isAdmin(newAdmin)) {
    bot.sendMessage(fb::Message("Этот ID уже админ.", chatID));
    return;
  }
  admins += "," + newAdmin;
  writeAdminsToFile(admins);

  bot.sendMessage(fb::Message("Пользователь " + newAdmin + " теперь администратор!", chatID));
  bot.sendMessage(fb::Message("Вас назначили администратором!", newAdmin));
}

// Проверка, админ ли
bool isAdmin(const String &chatID) {
  return (admins.indexOf(chatID) != -1);
}

// ------------------------------------------------
//             INLINE-МЕНЮ (КНОПКИ)
// ------------------------------------------------
void sendInlineMenu(const String &chatID) {
  // Создаём сообщение с текстом
  fb::Message msg("Выберите действие:", chatID);

  // Создаём inline-меню
  fb::InlineMenu menu;
  // addButton("Надпись на кнопке", "callback_data")

  // Первая строка
  menu.addButton("Включить/Выключить ПК", "startPC"); // data = "shutdownPC"

  // Вторая строка
  menu.newRow();
  menu.addButton("Жёсткий сброс","hardReset");
  menu.addButton("Мониторинг",   "monitor");

  // Третья строка
  menu.newRow();
  menu.addButton("restart",      "restart");
  menu.addButton("+ Admin",      "makeAdmin");

  // Прикрепляем это меню к сообщению
  msg.setInlineMenu(menu);

  // Отправляем
  bot.sendMessage(msg);
}

// ------------------------------------------------
//        ЧТЕНИЕ/ЗАПИСЬ СПИСКА АДМИНОВ
// ------------------------------------------------
void readAdminsFromFile() {
  if (!LittleFS.exists("/admins.txt")) {
    Serial.println("/admins.txt не найден. Создаём с OWNER_ID.");
    writeAdminsToFile(admins);
    return;
  }
  File f = LittleFS.open("/admins.txt", "r");
  if (!f) {
    Serial.println("Ошибка чтения /admins.txt");
    return;
  }
  String s = f.readString();
  f.close();
  s.trim();
  if (s.length() > 0) admins = s;
  Serial.println("Считан список админов: " + admins);
}

void writeAdminsToFile(const String &adminList) {
  File f = LittleFS.open("/admins.txt", "w");
  if (!f) {
    Serial.println("Ошибка записи /admins.txt");
    return;
  }
  f.print(adminList);
  f.close();
  Serial.println("Записан список админов: " + adminList);
}

// ------------------------------------------------
//             ПОДКЛЮЧЕНИЕ К WI-FI
// ------------------------------------------------
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi подключён!");
}
