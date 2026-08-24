#include "headers.h"

void BootManager::init() {
    RTCMemory::init();
    RTCMemory::incrementBootCount();
    LOG_INFO("Boot #%d", RTCMemory::getBootCount());
    LOG_INFO("Firmware: %s v%s", PROJECT_NAME, FW_VERSION);
    LOG_INFO("Free heap: %d", ESP.getFreeHeap());

    // Deteksi penyebab reset SPESIFIK (beda dengan cek cold-boot vs
    // deep-sleep-wake yang sudah ada di RTCMemory::init()). esp_reset_reason()
    // bisa membedakan brownout (tegangan sempat drop di bawah ambang
    // detektor internal ESP32) dari power-on normal & penyebab lain.
    //
    // PENTING: fungsi ini jalan di SETIAP wake (bukan cuma cold boot).
    // Kalau esp_reset_reason() ternyata tidak reliabel di board/IDF
    // version ini (kadang lapor UNKNOWN walau sebenarnya wake normal
    // dari deep sleep timer, bukan genuine brownout), maka SETIAP
    // siklus akan memicu NTP resync + rtc.adjust() berulang -- yang
    // artinya RTC "diguncang" ulang tiap siklus, dan prediksi jadwal
    // radio (yang butuh RTC stabil/kontinu) tidak akan pernah sempat
    // mengunci. Log SELALU dicetak (bukan cuma saat trigger) supaya
    // pola ini kelihatan jelas di Serial -- kalau baris "Reset
    // reason=" muncul BROWNOUT/POWERON/UNKNOWN di HAMPIR SETIAP wake
    // (bukan cuma sesekali), itu tandanya deteksi ini salah tangkap,
    // bukan brownout sungguhan tiap siklus.
    esp_reset_reason_t resetReason = esp_reset_reason();
    LOG_INFO("Reset reason=%d (0=UNKNOWN 1=POWERON 3=SW 4=PANIC 5=INT_WDT "
              "6=TASK_WDT 7=WDT 8=DEEPSLEEP 9=BROWNOUT 10=SDIO)", (int)resetReason);

    // Hanya BROWNOUT yang genuinely spesifik & reliable sebagai indikasi
    // masalah tegangan. POWERON & UNKNOWN sengaja DIKELUARKAN dari
    // trigger otomatis di sini -- POWERON legitimately terjadi di boot
    // pertama kali SAJA (bukan tiap wake normal), dan UNKNOWN riskan
    // salah tangkap di sebagian board/IDF version untuk wake normal.
    // Kalau device benar-benar sering brownout, esp_reset_reason() akan
    // tetap menangkapnya lewat kode BROWNOUT yang jelas, tanpa risiko
    // false-positive di wake normal.
    if (resetReason == ESP_RST_BROWNOUT) {
        LOG_WARN("Reset reason=BROWNOUT -- tandai butuh NTP resync");
        RTCManager::markNeedResync();
    }
}

uint32_t BootManager::getBootCount() {
    return RTCMemory::getBootCount();
}