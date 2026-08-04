const { Sequelize } = require('sequelize');
const { db } = require("../config/database");
const { DataTypes } = Sequelize;

const SensorData = db.define('sensor_data', {
  logId: {
    type: DataTypes.INTEGER,
    autoIncrement: true,
    primaryKey: true
  },
  datetime: {
    type: DataTypes.DATE
  },
  id: {
    type: DataTypes.STRING
  },
  ch: {
    type: DataTypes.INTEGER
  },
  batt: {
    type: DataTypes.STRING
  },
  temp_out: {
    type: DataTypes.FLOAT
  },
  hum_out: {
    type: DataTypes.FLOAT
  },
  wind_speed: {
    type: DataTypes.FLOAT
  },
  wind_gust: {
    type: DataTypes.FLOAT
  },
  wind_dir: {
    type: DataTypes.STRING
  },
  wind_deg: {
    type: DataTypes.FLOAT
  },
  rain_delta: {
    type: DataTypes.FLOAT
  },
  rain_total: {
    type: DataTypes.FLOAT
  },
  rain_raw: {
    type: DataTypes.INTEGER
  },
  light_lux: {
    type: DataTypes.FLOAT
  }
}, {
  freezeTableName: true,
  timestamps: false
});

module.exports = { SensorData };