// File: routes/routes.js
const express = require('express');
const router = express.Router();

// Import Controller yang diperlukan
const { getUsers, register, login } = require('../controller/Users');
const { authenticateJWT } = require('../middleware/tokenVerification');
const { setDeviceConfig, getDeviceConfig } = require('../controller/deviceConfigController');
const { getLatestSensorData, tambahSensorData, getSensorRange, exportSensorCSV } = require('../controller/SensorController');
const { tambahDeviceLog, getDeviceLog, getInfoLogsOnly } = require('../controller/deviceLogController');


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
router.get('/sensordata/history', getSensorRange);
router.get('/sensordata/export', exportSensorCSV);

router.post('/config', setDeviceConfig);
router.get('/config', getDeviceConfig);

router.post('/devicelog', tambahDeviceLog);
router.get('/devicelog', getDeviceLog);
router.get('/devicelog/info', getInfoLogsOnly);

module.exports = { router };