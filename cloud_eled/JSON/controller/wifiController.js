const { WifiConfig } = require('../models/wifiModel');

// 1. Fungsi POST: Dipanggil oleh Flutter untuk menyimpan WiFi baru
const setWifiConfig = async (req, res) => {
    try {
        const { ssid, password } = req.body;

        await WifiConfig.create({
            ssid: ssid,
            password: password
        });

        res.status(201).json({ 
            success: true, 
            msg: "Konfigurasi WiFi berhasil disimpan ke database!" 
        });
    } catch (error) {
        console.error("Error simpan konfigurasi WiFi:", error);
        res.status(500).json({ 
            success: false, 
            msg: "Gagal menyimpan konfigurasi WiFi", 
            detail: error.message 
        });
    }
};

// 2. Fungsi GET: Dipanggil oleh ESP32 untuk mengambil WiFi terbaru
const getWifiConfig = async (req, res) => {
    try {
        // Mengambil 1 baris data yang paling terakhir dimasukkan
        const latestWifi = await WifiConfig.findOne({
            order: [['id', 'DESC']]
        });

        if (!latestWifi) {
            return res.status(404).json({ msg: "Belum ada konfigurasi WiFi di database." });
        }

        // ESP32 biasanya lebih suka format JSON yang rata dan simpel
        res.status(200).json({
            ssid: latestWifi.ssid,
            password: latestWifi.password
        });
    } catch (error) {
        console.error("Error ambil konfigurasi WiFi:", error);
        res.status(500).json({ msg: "Gagal mengambil konfigurasi WiFi" });
    }
};

module.exports = { setWifiConfig, getWifiConfig };
