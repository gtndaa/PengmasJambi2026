const { SensorData } = require('../models/SensorData');
const { Op } = require('sequelize');

// 1. Fungsi GET: Mengambil 1 data (default) atau banyak data berdasarkan query ?limit=
const getLatestSensorData = async (req, res) => {
    try {
        // Tangkap parameter '?limit=' dari URL, default ke 1 jika tidak ada
        const requestedLimit = req.query.limit ? parseInt(req.query.limit) : 1;
        
        // Batas aman maksimal 1000 baris agar tidak membebani server/database
        const safeLimit = Math.min(requestedLimit, 1000);

        // Gunakan findAll untuk mengambil array data
        const data = await SensorData.findAll({
            limit: safeLimit,
            order: [['logId', 'DESC']]
        });

        if (!data || data.length === 0) {
            return res.status(404).json({ success: false, message: "Data sensor belum tersedia." });
        }

        // Jika hanya minta 1 data, return data tunggal (seperti format lama)
        if (requestedLimit === 1) {
            return res.status(200).json({
                success: true,
                data: data[0]
            });
        }

        // Jika minta lebih dari 1 data, return seluruh array
        res.status(200).json({
            success: true,
            count: data.length,
            data: data
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

// 3. fungsi range 
const getSensorRange = async (req, res) => {
    try {
        const { from, to } = req.query;
        const ok = (s) => /^\d{4}-\d{2}-\d{2}$/.test(s || '');
        if (!ok(from) || !ok(to)) {
            return res.status(400).json({ success: false, message: "Parameter from & to wajib berformat YYYY-MM-DD" });
        }
        
        const end = new Date(to + 'T00:00:00');
        end.setDate(end.getDate() + 1);
        const endStr = end.toISOString().slice(0, 10);

        const rows = await SensorData.findAll({
            where: { 
                datetime: { 
                    [Op.gte]: `${from} 00:00:00`, 
                    [Op.lt]: `${endStr} 00:00:00` // PERBAIKAN: Ditambahkan backtick (`) di sini
                } 
            },
            order: [['datetime', 'ASC']],
            limit: 20000
        });
        
        res.status(200).json({ success: true, data: rows });
    } catch (error) {
        console.error("Error mengambil riwayat sensor:", error);
        res.status(500).json({ success: false, message: "Terjadi kesalahan pada server." });
    }
};

// Pastikan export-nya diubah menjadi seperti ini:
module.exports = { getLatestSensorData, tambahSensorData, getSensorRange };
