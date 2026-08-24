#include "headers.h"

void Scheduler::goToSleep() {
    // Durasi dihitung berdasarkan hasil siklus terakhir (sinkronisasi
    // jadwal paket 48 detik, lihat SystemManager::computeNextSleepMs()).
    uint32_t sleepMs = SystemManager::computeNextSleepMs();
    goToSleep(sleepMs);
}

void Scheduler::goToSleep(uint32_t sleepMs) {
    if (sleepMs < MIN_SLEEP_MS) sleepMs = MIN_SLEEP_MS;

    // Matikan periferal yang boros arus sebelum tidur.
    //
    // PENTING: CC1101 di board ini TIDAK PERNAH benar-benar power-cycle
    // lintas siklus -- dia berbagi rail 3.3V yang sama dengan ESP32, dan
    // deep sleep ESP32 tidak memutus daya peripheral eksternal (tidak
    // ada load-switch pemutus). Jadi CC1101 adalah chip fisik yang SAMA
    // yang tetap hidup terus, cuma di-reinit lewat SPI tiap wake.
    //
    // Antara receiveWeather() (yang terakhir kali menyentuh CC1101) dan
    // titik ini, storeData()/upload() sudah memanggil SDManager::begin()
    // yang menimpa konfigurasi objek SPI global (clock 1MHz + mode milik
    // SD card) -- SPI itu dipakai bersama CC1101 (SCK/MISO/MOSI sama
    // persis, cuma beda CS). Kalau ELECHOUSE_cc1101.goSleep() di bawah
    // ini mengirim command SPI tanpa re-assert config miliknya sendiri
    // dulu, command itu bisa terkirim dengan clock/mode yang SALAH (sisa
    // punya SD) -> command sleep jadi korup/tidak lengkap -> CC1101 bisa
    // tersangkut di state internal yang aneh. Karena chip tidak pernah
    // benar-benar reset fisik, state korup itu BERTAHAN ke siklus
    // berikutnya, dan begin() ulang di wake selanjutnya belum tentu
    // cukup memulihkannya. Ini kemungkinan besar penyebab pola "cuma
    // tertangkap sekali di awal, lalu tidak pernah lagi walau sudah
    // resync berulang".
    //
    // Fix: paksa CC1101 re-init PENUH (assert ulang pin & register SPI
    // miliknya) tepat sebelum kirim command sleep, supaya command
    // terakhir ini selalu terkirim dengan config yang benar, terlepas
    // dari apapun yang ditinggalkan SD sebelumnya.
    //
    // UPDATE: pendekatan begin() PENUH di atas ternyata malah bikin
    // CC1101 gagal terdeteksi di siklus BERIKUTNYA ("CC1101 gagal
    // inisialisasi") -- kemungkinan besar karena begin() (Init -> SetRx)
    // langsung disusul goSleep() tanpa jeda melanggar minimum settling
    // time CC1101 antar transisi mode. Diganti jadi versi ringan:
    // re-assert HANYA pin routing SPI (tidak mengirim command
    // Init/SetRx/reset apapun ke chip, cuma update variabel pin di
    // library), cukup untuk memastikan goSleep() terkirim ke pin yang
    // benar tanpa risiko melanggar timing chip.
    CC1101Driver radio;
    radio.reassertSpiPins();
    radio.sleepRadio();

    LightSensor light;
    light.powerDown();

    if (WiFi.getMode() != WIFI_OFF) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }

    PowerManager pm;
    pm.prepareDeepSleep((uint64_t)sleepMs * 1000ULL);

    LOG_INFO("Deep sleep %lu ms (sinkron=%s, missed=%d, wake#%lu)",
             (unsigned long)sleepMs,
             RTCMemory::isSynced() ? "ya" : "tidak",
             RTCMemory::getMissedCycles(),
             (unsigned long)RTCMemory::getWakeCounter());

    Serial.flush();
    pm.deepSleepNow();
}
