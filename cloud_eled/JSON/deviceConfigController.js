const { DeviceConfig } = require('../models/deviceConfigModel');

const setDeviceConfig = async (req, res) => {
    try {
        const {
            wifiSSID, 
            wifiPassword, 
            uploadInterval, 
            listenWindow, 
            sleepInterval, 
            useDeepSleep
        } = req.body;

        // Cari konfigurasi paling terakhir di database untuk menentukan versi selanjutnya
        const latestConfig = await DeviceConfig.findOne({
            order: [['id', 'DESC']]
        });

        let nextVersion = 1;
        if (latestConfig && latestConfig.configVersion) {
            nextVersion = latestConfig.configVersion + 1;
            // Batas maksimal uint8_t di ESP32 adalah 255
            if (nextVersion > 255) {
                nextVersion = 1;
            }
        }

        // Simpan data beserta configVersion yang dibuat otomatis oleh backend
        await DeviceConfig.create({
            wifiSSID, 
            wifiPassword, 
            uploadInterval, 
            listenWindow, 
            sleepInterval, 
            configVersion: nextVersion, 
            useDeepSleep
        });

        res.status(201).json({ 
            success: true, 
            msg: "Konfigurasi perangkat berhasil disimpan!",
            new_version: nextVersion
        });
    } catch (error) {
        console.error("Error simpan konfigurasi perangkat:", error);
        res.status(500).json({ 
            success: false, 
            msg: "Gagal menyimpan konfigurasi perangkat", 
            error_name: error.name,
            error_message: error.message 
        });
    }
};

const getDeviceConfig = async (req, res) => {
    try {
        const latestConfig = await DeviceConfig.findOne({
            order: [['id', 'DESC']]
        });

        if (!latestConfig) {
            return res.status(404).json({ msg: "Belum ada konfigurasi di database." });
        }

        // Format data yang langsung siap dibaca oleh ArduinoJson di ESP32
        res.status(200).json({
            wifiSSID: latestConfig.wifiSSID,
            wifiPassword: latestConfig.wifiPassword,
            uploadInterval: latestConfig.uploadInterval,
            listenWindow: latestConfig.listenWindow,
            sleepInterval: latestConfig.sleepInterval,
            configVersion: latestConfig.configVersion,
            useDeepSleep: latestConfig.useDeepSleep
        });
    } catch (error) {
        console.error("Error ambil konfigurasi perangkat:", error);
        res.status(500).json({ msg: "Gagal mengambil konfigurasi perangkat" });
    }
};

module.exports = { setDeviceConfig, getDeviceConfig };
