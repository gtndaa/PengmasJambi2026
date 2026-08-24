#include "headers.h"

static volatile uint32_t pulseBuf[MAX_PULSES];
static volatile uint16_t pulseIdx = 0;
static volatile uint32_t lastPulseMs = 0;
static volatile bool     packetReady = false;
static volatile uint32_t risingTs = 0;

static void IRAM_ATTR gdo2ISR() {
    uint32_t now = micros();
    if (digitalRead(GDO2_PIN) == HIGH) {
        risingTs = now;
        return;
    }
    uint32_t dur = now - risingTs;
    if (packetReady) return;
    if ((dur >= PULSE_1_MIN && dur <= PULSE_1_MAX) ||
        (dur >= PULSE_0_MIN && dur <= PULSE_0_MAX)) {
        if (pulseIdx < MAX_PULSES) {
            pulseBuf[pulseIdx++] = dur;
            lastPulseMs = millis();
        }
    }
}

// CC1101 setelah goSleep() (mode SPWD/power-down) TIDAK BISA langsung
// menerima command SPI apapun lagi -- termasuk SRES (reset) yang
// dipanggil di dalam Init(). Datasheet CC1101 mensyaratkan urutan
// "bangun" manual dulu: tarik CS LOW, lalu tunggu pin MISO turun ke LOW
// (tanda kristal osilator sudah stabil & chip benar-benar aktif lagi) --
// BARU SETELAH ITU command SPI lain boleh dikirim. Kalau ini dilewati,
// SRES di dalam Init() bisa diabaikan/gagal karena chip masih separuh
// tidur, membuat re-init di siklus berikutnya (setelah chip pernah
// di-goSleep()-kan di siklus sebelumnya) tidak benar-benar berhasil --
// walau getCC1101() masih terlihat "lolos" (mis. kalau itu cuma baca
// register versi yang kebetulan masih terbaca meski chip belum full
// bangun). Ini realisasinya kemungkinan besar akar masalah "sukses
// sekali di awal (chip belum pernah tidur), lalu gagal terus-menerus
// di semua siklus berikutnya (chip selalu baru saja di-goSleep()-kan
// di akhir siklus sebelumnya)".
//
// Dipanggil di AWAL begin(), SEBELUM Init(). Aman dipanggil terlepas
// dari apakah chip benar-benar sedang tidur atau tidak (pada cold boot
// pertama, MISO harusnya sudah rendah/tidak relevan, jadi loop tunggu
// akan langsung lolos tanpa efek samping).
static void wakeCC1101FromSleep() {
    pinMode(CC1101_CSN, OUTPUT);
    pinMode(CC1101_MISO, INPUT);

    digitalWrite(CC1101_CSN, HIGH);
    delayMicroseconds(10);
    digitalWrite(CC1101_CSN, LOW);

    uint32_t start = millis();
    while (digitalRead(CC1101_MISO) == HIGH) {
        if (millis() - start > 100) break; // timeout jaga-jaga, jangan hang selamanya
    }

    digitalWrite(CC1101_CSN, HIGH);
    delayMicroseconds(10);
}

bool CC1101Driver::begin() {
    wakeCC1101FromSleep();
    ELECHOUSE_cc1101.setSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CSN);
    ELECHOUSE_cc1101.setGDO(GDO0_PIN, GDO2_PIN);
    ELECHOUSE_cc1101.Init();
    if (!ELECHOUSE_cc1101.getCC1101()) return false;
    ELECHOUSE_cc1101.setMHZ(RF_FREQ_MHZ);
    ELECHOUSE_cc1101.setModulation(RF_MODULATION);
    ELECHOUSE_cc1101.setDRate(RF_DATARATE);
    ELECHOUSE_cc1101.setRxBW(RF_RXBW);
    ELECHOUSE_cc1101.SetRx();
    pinMode(GDO2_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(GDO2_PIN), gdo2ISR, CHANGE);
    return true;
}

void CC1101Driver::setReceiveMode() {
    ELECHOUSE_cc1101.SetRx();
}

bool CC1101Driver::isPacketAvailable() {
    if (packetReady) return true;
    if (pulseIdx < 64) return false;
    noInterrupts();
    uint32_t last = lastPulseMs;
    interrupts();
    if (millis() - last >= PACKET_TIMEOUT) {
        packetReady = true;
        return true;
    }
    return false;
}

void CC1101Driver::resetPulseBuffer() {
    noInterrupts();
    pulseIdx = 0;
    packetReady = false;
    interrupts();
}

uint16_t CC1101Driver::getPulseCount() {
    noInterrupts();
    uint16_t c = pulseIdx;
    interrupts();
    return c;
}

void CC1101Driver::copyPulses(uint32_t* dest, uint16_t maxCount) {
    noInterrupts();
    uint16_t c = (pulseIdx < maxCount) ? pulseIdx : maxCount;
    for (uint16_t i = 0; i < c; i++) dest[i] = pulseBuf[i];
    interrupts();
}

void CC1101Driver::enableInterrupt() {
    attachInterrupt(digitalPinToInterrupt(GDO2_PIN), gdo2ISR, CHANGE);
}

void CC1101Driver::disableInterrupt() {
    detachInterrupt(digitalPinToInterrupt(GDO2_PIN));
}

void CC1101Driver::watchdogReset() {
    ELECHOUSE_cc1101.SetRx();
}

void CC1101Driver::sleepRadio() {
    // Lepas interrupt dulu supaya tidak ada ISR nyasar saat CS/GDO
    // dalam kondisi transisi menuju sleep, lalu kirim CC1101 ke SPWD
    // (power-down state) via goSleep() -- ini konsumsi arusnya jauh
    // lebih rendah dibanding sekadar idle (setSidle), cocok dipasangkan
    // dengan ESP32 deep sleep karena keduanya sama-sama nonaktif lama.
    disableInterrupt();
    ELECHOUSE_cc1101.goSleep();
}

void CC1101Driver::reassertSpiPins() {
    ELECHOUSE_cc1101.setSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CSN);
}