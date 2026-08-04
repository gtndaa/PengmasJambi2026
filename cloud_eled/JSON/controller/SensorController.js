const { SensorData } = require('../models/SensorData');

// 1. Fungsi GET: Mengambil 1 data paling terakhir dimasukkan (paling baru)
const getLatestSensorData = async (req, res) => {
    try {
        const latestData = await SensorData.findOne({
            order: [['logId', 'DESC']] // Diubah ke logId DESC agar data terbaru di atas
        });

        if (!latestData) {
            return res.status(404).json({ message: "Data sensor belum tersedia." });
        }

        res.status(200).json({
            success: true,
            data: latestData
        });
    } catch (error) {
        console.error("Error mengambil data sensor:", error);
        res.status(500).json({ success: false, message: "Terjadi kesalahan pada server." });
    }
};

// 2. Fungsi POST: Menerima data JSON dari ESP32 / Postman
const tambahSensorData = async (req, res) => {
    try {
        const {
            datetime,
            id,
            ch,
            batt,
            temp_out,
            hum_out,
            wind_speed,
            wind_gust,
            wind_dir,
            wind_deg,
            rain_delta,
            rain_total,
            rain_raw,
            light_lux
        } = req.body;

        const newData = await SensorData.create({
            datetime,
            id,
            ch,
            batt,
            temp_out,
            hum_out,
            wind_speed,
            wind_gust,
            wind_dir,
            wind_deg,
            rain_delta,
            rain_total,
            rain_raw,
            light_lux
        });

        res.status(201).json({
            success: true,
            message: "Data sensor berhasil disimpan!",
            data: newData
        });
    } catch (error) {
        console.error("Error simpan data sensor:", error);
        res.status(500).json({
            success: false,
            message: "Gagal menyimpan data sensor",
            detail: error.message
        });
    }
};

module.exports = { getLatestSensorData, tambahSensorData };