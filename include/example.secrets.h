#ifndef SECRETS_H
#define SECRETS_H

#include <pgmspace.h>

// ============================================================================
// Wi-Fi Credentials
// ============================================================================
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ============================================================================
// AWS IoT Core Configuration
// ============================================================================
// Знаходиться в AWS IoT Core -> Settings -> Domain configurations (або AWS CLI)
#define AWS_IOT_ENDPOINT "xxxxxxxxx-ats.iot.eu-central-1.amazonaws.com"
#define AWS_IOT_PORT 8883

// Client ID для MQTT з'єднання
#define THINGNAME "ESP32_DHT22_Sensor"

// MQTT Topic для відправки телеметрії
#define AWS_IOT_TOPIC "iot-course/YOUR_NAME/sensors/data"

// ============================================================================
// AWS Certificates & Keys
// ============================================================================

// Amazon Root CA 1
// Отримати можна за посиланням: https://www.amazontrust.com/repository/AmazonRootCA1.pem
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR_AMAZON_ROOT_CA_1_HERE
-----END CERTIFICATE-----
)EOF";

// Device Certificate (xxxxxx-certificate.pem.crt)
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
YOUR_DEVICE_CERTIFICATE_HERE
-----END CERTIFICATE-----
)KEY";

// Device Private Key (xxxxxx-private.pem.key)
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
YOUR_DEVICE_PRIVATE_KEY_HERE
-----END RSA PRIVATE KEY-----
)KEY";

#endif // SECRETS_H