#include <stdio.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#define HTTP_REST_PORT 80
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50

struct Led {
    byte id;
    byte gpio;
    byte status;
} led_resource;

const char* wifi_ssid = "YOUR_SSID";
const char* wifi_passwd = "YOUR_PASSWD";

ESP8266WebServer http_rest_server(HTTP_REST_PORT);

void init_led_resource()
{
    led_resource.id = 0;
    led_resource.gpio = 0;
    led_resource.status = LOW;
}

int init_wifi() {
    int retries = 0;

    Serial.println("Connecting to WiFi AP..........");

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_passwd);
    // check the status of WiFi connection to be WL_CONNECTED
    while ((WiFi.status() != WL_CONNECTED) && (retries < MAX_WIFI_INIT_RETRY)) {
        retries++;
        delay(WIFI_RETRY_DELAY);
        Serial.print("#");
    }
    return WiFi.status(); // return the WiFi connection status
}

void get_leds() {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[200];

    if (led_resource.id == 0)
        http_rest_server.send(204);
    else {
        jsonObj["id"] = led_resource.id;
        jsonObj["gpio"] = led_resource.gpio;
        jsonObj["status"] = led_resource.status;
        jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        http_rest_server.send(200, "application/json", JSONmessageBuffer);
    }
}

void json_to_resource(JsonObject& jsonBody) {
    int id, gpio, status;

    id = jsonBody["id"];
    gpio = jsonBody["gpio"];
    status = jsonBody["status"];

    Serial.println(id);
    Serial.println(gpio);
    Serial.println(status);

    led_resource.id = id;
    led_resource.gpio = gpio;
    led_resource.status = status;
}

void post_put_leds() {
    StaticJsonBuffer<500> jsonBuffer;
    String post_body = http_rest_server.arg("plain");
    Serial.println(post_body);

    JsonObject& jsonBody = jsonBuffer.parseObject(http_rest_server.arg("plain"));

    Serial.print("HTTP Method: ");
    Serial.println(http_rest_server.method());
    
    if (!jsonBody.success()) {
        Serial.println("error in parsin json body");
        http_rest_server.send(400);
    }
    else {   
        if (http_rest_server.method() == HTTP_POST) {
            if ((jsonBody["id"] != 0) && (jsonBody["id"] != led_resource.id)) {
                json_to_resource(jsonBody);
                http_rest_server.sendHeader("Location", "/leds/" + String(led_resource.id));
                http_rest_server.send(201);
                pinMode(led_resource.gpio, OUTPUT);
            }
            else if (jsonBody["id"] == 0)
              http_rest_server.send(404);
            else if (jsonBody["id"] == led_resource.id)
              http_rest_server.send(409);
        }
        else if (http_rest_server.method() == HTTP_PUT) {
            if (jsonBody["id"] == led_resource.id) {
                json_to_resource(jsonBody);
                http_rest_server.sendHeader("Location", "/leds/" + String(led_resource.id));
                http_rest_server.send(200);
                digitalWrite(led_resource.gpio, led_resource.status);
            }
            else
              http_rest_server.send(404);
        }
    }
}

void config_rest_server_routing() {
    http_rest_server.on("/", HTTP_GET, []() {
        http_rest_server.send(200, "text/html",
            "Welcome to the ESP8266 REST Web Server");
    });
    http_rest_server.on("/leds", HTTP_GET, get_leds);
    http_rest_server.on("/leds", HTTP_POST, post_put_leds);
    http_rest_server.on("/leds", HTTP_PUT, post_put_leds);
}

void setup(void) {
    Serial.begin(115200);

    init_led_resource();
    if (init_wifi() == WL_CONNECTED) {
        Serial.print("Connetted to ");
        Serial.print(wifi_ssid);
        Serial.print("--- IP: ");
        Serial.println(WiFi.localIP());
    }
    else {
        Serial.print("Error connecting to: ");
        Serial.println(wifi_ssid);
    }

    config_rest_server_routing();

    http_rest_server.begin();
    Serial.println("HTTP REST Server Started");
}

void loop(void) {
    http_rest_server.handleClient();
}
