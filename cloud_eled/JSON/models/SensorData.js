// File: models/SensorData.js
const { Sequelize } = require('sequelize');
const { db } = require("../config/database");
const { DataTypes } = Sequelize;

const SensorData = db.define('sensor_data', {
  id: {
    type: DataTypes.INTEGER,
    autoIncrement: true,
    primaryKey: true
  },
  suhu: {
    type: DataTypes.FLOAT,
    allowNull: false
  },
  kelembaban: {
    type: DataTypes.FLOAT,
    allowNull: false
  },
  created_at: {
    type: DataTypes.DATE,
    defaultValue: DataTypes.NOW
  }
}, {
  freezeTableName: true,
  timestamps: false // Dimatikan karena kita pakai 'created_at' manual
});

module.exports = { SensorData };