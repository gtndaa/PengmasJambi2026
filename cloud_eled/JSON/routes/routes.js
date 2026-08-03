// File: routes/routes.js
const express = require('express');
const router = express.Router();

// Import Controller yang diperlukan
const { getUsers, register, login, ubahTanaman } = require('../controller/Users');
const { getAllLokasiLahan, getAllLocationKey, getOneLocationKey } = require('../controller/LokasiLahan');
const { getAllKegiatan } = require('../controller/Kalender');
const { authenticateJWT } = require('../middleware/tokenVerification');
const { setWifiConfig, getWifiConfig } = require('../controllers/wifiController.js');
const { getLatestSensorData, tambahSensorData } = require('../controller/SensorController');

// Home Route
router.get('/', async (req, res) => {
  res.status(200).json({ message: "Welcome to Agrical Backend API!" });
});

// Route User & Auth
router.get('/protected', authenticateJWT, (req, res) => {
  res.json({ message: `Welcome, ${req.user.username}` });
});
router.get('/users', getUsers);
router.post('/register', register);
router.post('/login', login);
router.post('/ubahtanaman', ubahTanaman);

// Route Sensor (Baru) -> Ini yang akan dipanggil Flutter
router.get('/sensordata', getLatestSensorData);
router.post('/postsensordata', tambahSensorData);

router.post('/wifi', setWifiConfig); // Pintu masuk dari Flutter
router.get('/wifi', getWifiConfig);  // Pintu keluar ke ESP32

module.exports = { router };
