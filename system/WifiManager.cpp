#include "headers.h"

WifiManager::WifiManager() {}

bool WifiManager::connect(const char* ssid, const char* password, unsigned long timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
        LOG_INFO("WiFi already connected");
        return true;
    }
    WiFi.begin(ssid, password);
    LOG_INFO("Connecting to WiFi SSID: %s", ssid);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeoutMs) {
            LOG_ERROR("WiFi connection timeout");
            return false;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    LOG_INFO("WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
    return true;
}

bool WifiManager::connectWithFallback(const WifiCredentials& newCreds,
                                       const char* fallbackSsid,
                                       const char* fallbackPassword,
                                       unsigned long timeoutMs) {
    LOG_INFO("Mencoba ganti WiFi ke SSID baru: %s", newCreds.ssid.c_str());

    disconnect();
    delay(100);

    if (connect(newCreds.ssid.c_str(), newCreds.password.c_str(), timeoutMs)) {
        LOG_INFO("Berhasil terhubung ke SSID baru: %s", newCreds.ssid.c_str());
        return true;
    }

    LOG_WARN("Gagal terhubung ke SSID baru (%s), kembali ke SSID awal: %s",
              newCreds.ssid.c_str(), fallbackSsid);

    disconnect();
    delay(100);

    if (connect(fallbackSsid, fallbackPassword, timeoutMs)) {
        LOG_INFO("Berhasil kembali terhubung ke SSID awal: %s", fallbackSsid);
    } else {
        LOG_ERROR("Gagal juga terhubung ke SSID awal: %s", fallbackSsid);
    }

    return false;
}

int WifiManager::getRSSI() const {
    return WiFi.RSSI();
}

void WifiManager::disconnect() {
    WiFi.disconnect();
    LOG_INFO("WiFi disconnected");
}

/**/ // COMMENT THIS SECTION FOR NON ENTERPRISE USAGE
#if __has_include("esp_eap_client.h")
  #include "esp_eap_client.h"   // ESP32 Arduino core 3.x (IDF 5.x)
#else
  #include "esp_wpa2.h"         // ESP32 Arduino core 2.x (IDF 4.x)
#endif

// Matikan mode enterprise di WiFi stack supaya WiFi.begin(ssid, pass)
// biasa berikutnya tidak ikut "terbawa" mode enterprise.
static void wpa2EntDisable() {
#if __has_include("esp_eap_client.h")
    esp_wifi_sta_enterprise_disable();
#else
    esp_wifi_sta_wpa2_ent_disable();
#endif
}

// ---- Debug helper: alasan disconnect terakhir + nama status WiFi ----
static volatile int s_lastDisconnectReason = 0;
static bool s_entEventRegistered = false;

static void onWifiDisconnectEvent(arduino_event_id_t event, arduino_event_info_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        s_lastDisconnectReason = info.wifi_sta_disconnected.reason;
    }
}

static const char* wifiStatusName(wl_status_t s) {
    switch (s) {
        case WL_NO_SHIELD:       return "NO_SHIELD";
        case WL_IDLE_STATUS:     return "IDLE";
        case WL_NO_SSID_AVAIL:   return "NO_SSID_AVAIL (SSID tidak ditemukan)";
        case WL_SCAN_COMPLETED:  return "SCAN_COMPLETED";
        case WL_CONNECTED:       return "CONNECTED";
        case WL_CONNECT_FAILED:  return "CONNECT_FAILED (auth/handshake gagal)";
        case WL_CONNECTION_LOST: return "CONNECTION_LOST";
        case WL_DISCONNECTED:    return "DISCONNECTED";
        default:                 return "UNKNOWN";
    }
}

// Petunjuk untuk reason code yang paling sering muncul di enterprise
static const char* disconnectReasonHint(int r) {
    switch (r) {
        case 0:   return "belum ada event disconnect";
        case 2:   return "AUTH_EXPIRE";
        case 15:  return "4WAY_HANDSHAKE_TIMEOUT (cek username/password/identity)";
        case 201: return "NO_AP_FOUND (cek WIFI_ENT_SSID & jangkauan)";
        case 202: return "AUTH_FAIL (username/password/identity ditolak)";
        case 204: return "HANDSHAKE_TIMEOUT";
        case 205: return "CONNECTION_FAIL";
        default:  return "lihat tabel wifi_err_reason_t di ESP-IDF";
    }
}

bool WifiManager::connectEnterprise(unsigned long timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
        LOG_INFO("WiFi already connected");
        return true;
    }
    if (!s_entEventRegistered) {
        WiFi.onEvent(onWifiDisconnectEvent, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
        s_entEventRegistered = true;
    }
    s_lastDisconnectReason = 0;
    LOG_DEBUG("[ENT] identity=%s username=%s timeout=%lu ms",
              WIFI_ENT_IDENTITY, WIFI_ENT_USERNAME, timeoutMs);   // password sengaja tidak dicetak
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_ENT_SSID, WPA2_AUTH_PEAP,
               WIFI_ENT_IDENTITY, WIFI_ENT_USERNAME, WIFI_ENT_PASSWORD);
    LOG_INFO("Connecting to WPA2-Enterprise SSID: %s", WIFI_ENT_SSID);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeoutMs) {
            LOG_ERROR("WiFi Enterprise connection timeout");
            LOG_ERROR("[ENT] status=%d %s | disconnect reason=%d %s",
                      (int)WiFi.status(), wifiStatusName(WiFi.status()),
                      (int)s_lastDisconnectReason,
                      disconnectReasonHint((int)s_lastDisconnectReason));
            return false;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    LOG_INFO("WiFi Enterprise connected, IP: %s", WiFi.localIP().toString().c_str());
    LOG_INFO("[ENT] OK: SSID=%s RSSI=%d dBm channel=%d waktu=%lu ms",
             WiFi.SSID().c_str(), (int)WiFi.RSSI(), (int)WiFi.channel(),
             (unsigned long)(millis() - start));
    return true;
}

bool WifiManager::connectWithFallbackToEnterprise(const WifiCredentials& newCreds,
                                                   unsigned long timeoutMs) {
    LOG_INFO("Mencoba ganti WiFi (dari Enterprise) ke SSID baru: %s", newCreds.ssid.c_str());

    disconnect();
    delay(100);
    wpa2EntDisable();   // keluar dari mode enterprise sebelum konek personal

    if (connect(newCreds.ssid.c_str(), newCreds.password.c_str(), timeoutMs)) {
        LOG_INFO("Berhasil terhubung ke SSID baru: %s", newCreds.ssid.c_str());
        return true;
    }

    LOG_WARN("Gagal terhubung ke SSID baru (%s), kembali ke WPA2-Enterprise: %s",
              newCreds.ssid.c_str(), WIFI_ENT_SSID);

    disconnect();
    delay(100);

    if (connectEnterprise(WIFI_ENT_TIMEOUT_MS)) {
        LOG_INFO("Berhasil kembali terhubung ke Enterprise: %s", WIFI_ENT_SSID);
    } else {
        LOG_ERROR("Gagal juga terhubung ke Enterprise: %s", WIFI_ENT_SSID);
    }
    return false;
}
/**/
