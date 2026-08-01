// File: routes/routes.js
const express = require('express');
const router = express.Router();

// Import Controller yang diperlukan
const { getUsers, register, login, ubahTanaman } = require('../controller/Users');
const { getAllLokasiLahan, getAllLocationKey, getOneLocationKey } = require('../controller/LokasiLahan');
const { authenticateJWT } = require('../middleware/tokenVerification');

// Import Controller Sensor (Baru)
const {
  getLatestSensorData,
  tambahSensorData
} = require('../controller/SensorController');

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

module.exports = { router };