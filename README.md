# ESP32 → AWS IoT Core → DynamoDB

Домашнє завдання: телеметрія з ESP32 (Wokwi, DHT22) через MQTT/TLS в AWS IoT Core,
Rules Engine зберігає її в DynamoDB, окреме правило логує перегрів у CloudWatch.

## Архітектура

```
┌────────────────────┐
│  ESP32 (Wokwi)     │  DHT22 → D4, кнопка → D5
└─────────┬──────────┘
          │ MQTT over TLS, порт 8883, Client ID = esp32-Ruslan_Chernyi
          │ кожні 30 с → iot-course/Ruslan_Chernyi/telemetry
          │   {"device_id","timestamp","temperature","humidity"}
          │ помилка DHT → iot-course/Ruslan_Chernyi/events
          ▼
┌──────────────────────────────────────────────────────────────┐
│  AWS IoT Core (eu-central-1) — Rules Engine                  │
│                                                              │
│  StoreTelemetry ──── dynamoDBv2 ───► DynamoDB iot_telemetry  │
│        │ error action                                        │
│        └──────────── CloudWatch Logs /aws/iot/rules/errors   │
│                                                              │
│  OverheatAlert (temperature > 28) ──► CloudWatch Logs        │
│                                       /aws/iot/rules/overheat│
└──────────────────────────────────────────────────────────────┘
          │
          ▼
   backend/ (FastAPI) — /sensors/latest, /sensors/history
```

## Параметри

| Що | Значення |
|---|---|
| Регіон | `eu-central-1` |
| Thing / Client ID / `device_id` | `esp32-Ruslan_Chernyi` |
| Топік телеметрії | `iot-course/Ruslan_Chernyi/telemetry` |
| Топік подій (помилки сенсора) | `iot-course/Ruslan_Chernyi/events` |
| Таблиця DynamoDB | `iot_telemetry` (`device_id` String, `received_at` Number) |
| Правило збереження | `StoreTelemetry` |
| Правило алерту | `OverheatAlert` |

## Частина 1. Пристрій

### Thing і Policy

1. AWS IoT Core → **Manage → Things → Create thing** → `esp32-Ruslan_Chernyi`
2. Auto-generate certificate → завантажити `certificate.pem.crt`, `private.pem.key`, `AmazonRootCA1.pem`
3. Policy — тільки власний Client ID і власні топіки:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:eu-central-1:<ACCOUNT_ID>:client/esp32-Ruslan_Chernyi"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": "arn:aws:iot:eu-central-1:<ACCOUNT_ID>:topic/iot-course/Ruslan_Chernyi/*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": "arn:aws:iot:eu-central-1:<ACCOUNT_ID>:topicfilter/iot-course/Ruslan_Chernyi/*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Receive",
      "Resource": "arn:aws:iot:eu-central-1:<ACCOUNT_ID>:topic/iot-course/Ruslan_Chernyi/*"
    }
  ]
}
```

<!-- Скріншот: Thing + прикріплений сертифікат і Policy -->

### Прошивка

`include/secrets.h` (у `.gitignore`) містить `root_ca`, `device_cert`, `private_key`.
Збірка: `pio run`, симуляція: `F1 → Wokwi: Start Simulator`, Serial 115200.

Обробка збоїв:

| Збій | Поведінка |
|---|---|
| Wi-Fi не підключився | не блокує старт; `loop()` викликає `WiFi.reconnect()` раз на 5 с |
| NTP не пройшов за 10 с | старт продовжується, `timestamp` = `0`; SNTP досинхронізується у фоні |
| MQTT відвалився | reconnect раз на 5 с без `delay()` і без ліміту спроб, у Serial — номер спроби та `state()` |
| DHT22 повернув NaN | телеметрія не публікується (щоб у таблицю не потрапили фейкові `0.0`), натомість подія `{"error":"dht_read_failed"}` у топік `events` |

Кнопка (D5) — позачергове зчитування й публікація телеметрії.

<!-- Скріншот: Serial Monitor з рядком [MQTT] Публікуємо: ... -->
<!-- Скріншот: MQTT test client, підписка на iot-course/Ruslan_Chernyi/telemetry -->

## Частина 2. Rules Engine + DynamoDB

Таблиця `iot_telemetry`: partition key `device_id` (String), sort key `received_at` (Number), On-demand.

Правило `StoreTelemetry`, SQL version `2016-03-23`:

```sql
SELECT *,
       timestamp() AS received_at,
       clientid()  AS client_id
FROM 'iot-course/Ruslan_Chernyi/telemetry'
```

- Дія: **DynamoDBv2** → `iot_telemetry`, роль `iot_rule_ddb_role`
- Error action: **CloudWatch Logs** → `/aws/iot/rules/errors`, роль `iot_rule_cw_role`

Перевірка error action: прибрати `timestamp() AS received_at` з SQL → у таблиці
нових записів немає, у `/aws/iot/rules/errors` з'являється повідомлення з
`failures[].errorMessage` і `base64OriginalPayload`. Потім повернути рядок назад.

<!-- Скріншот: DynamoDB → Explore table items (device_id, received_at, client_id, ...) -->
<!-- Скріншот: CloudWatch → /aws/iot/rules/errors з повідомленням про помилку -->

## Частина 3. Алерт на перегрів

Окреме правило `OverheatAlert` — бізнес-логіка, не error action:

```sql
SELECT device_id, temperature, humidity, timestamp() AS received_at
FROM 'iot-course/Ruslan_Chernyi/telemetry'
WHERE temperature > 28
```

- Дія: **CloudWatch Logs** → `/aws/iot/rules/overheat`
- Error action: **CloudWatch Logs** → `/aws/iot/rules/errors`

Симуляція перегріву: у Wokwi клікнути на DHT22 і підняти температуру вище 28 °C,
потім натиснути кнопку для негайної публікації.

<!-- Скріншот: Wokwi з температурою > 28 -->
<!-- Скріншот: CloudWatch → /aws/iot/rules/overheat -->

## Backend

```bash
cd backend
pip install -r requirenments.txt
uvicorn main:app --reload
```

`.env`: `AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY`, `AWS_DEFAULT_REGION=eu-central-1`,
`TABLE_NAME=iot_telemetry`, `DEVICE_ID=esp32-Ruslan_Chernyi`.

| Ендпоінт | Що повертає |
|---|---|
| `GET /health` | `{"status":"ok"}` |
| `GET /sensors/latest` | останній запис пристрою |
| `GET /sensors/history?minutes=30` | записи за останні N хвилин |
