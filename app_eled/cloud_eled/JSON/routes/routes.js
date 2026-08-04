// File: routes/routes.js
const express = require('express');
const router = express.Router();

// Import Controller yang diperlukan
const { getUsers, register, login } = require('../controller/Users');
const { authenticateJWT } = require('../middleware/tokenVerification');
const { setDeviceConfig, getDeviceConfig } = require('../controller/deviceConfigController');
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

// Route Sensor (Baru) -> Ini yang akan dipanggil Flutter
router.get('/sensordata', getLatestSensorData);
router.post('/sensordata', tambahSensorData);

router.post('/config', setDeviceConfig);
router.get('/config', getDeviceConfig);

module.exports = { router };
