// File: controller/SensorController.js
const { SensorData } = require('../models/SensorData');

const getLatestSensorData = async (req, res) => {
    try {
        // Mengambil 1 data paling terakhir dimasukkan (paling baru)
        const latestData = await SensorData.findOne({
            order: [['created_at', 'DESC']]
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

module.exports = { getLatestSensorData };
