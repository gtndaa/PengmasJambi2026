const express = require('express');
const router = express.Router();

// Import Controllers
const { getUsers, register, login, ubahTanaman } = require('../controller/Users');
const { getAllLokasiLahan, getAllLocationKey, getOneLocationKey } = require('../controller/LokasiLahan');
const { getAllKegiatan } = require('../controller/Kalender');
const { authenticateJWT } = require('../middleware/tokenVerification');

// (Nanti tambahkan ini setelah Anda buat SensorController)
// const { saveSensorData, getLatestSensorData } = require('../controller/SensorController');

// Home Route
router.get('/', (req, res) => {
    res.status(200).json({ message: "Welcome to Agrical Backend!" });
});

// Auth & User Routes
router.get('/protected', authenticateJWT, (req, res) => {
    res.json({ message: `Welcome, ${req.user.username}` });
});
router.get('/users', getUsers);
router.post('/register', register);
router.post('/login', login);
router.post('/ubahtanaman', ubahTanaman);

// Lokasi Lahan Routes
router.get('/getalllokasilahan', getAllLokasiLahan);
router.get('/alllocationkey', getAllLocationKey);
router.post('/getonelocationkey', getOneLocationKey);

// Kalender Routes
router.get('/allkegiatan', getAllKegiatan);

module.exports = { router };
