const { Sequelize } = require('sequelize');
const { db } = require('../config/database');
const { DataTypes } = Sequelize;

const DeviceLog = db.define('device_log', {
  logId:        { type: DataTypes.BIGINT, autoIncrement: true, primaryKey: true },
  device_id:    { type: DataTypes.STRING(32),  allowNull: false },
  session:      { type: DataTypes.BIGINT.UNSIGNED, allowNull: false },
  seq:          { type: DataTypes.INTEGER.UNSIGNED, allowNull: false },
  datetime:     { type: DataTypes.DATE, allowNull: true },
  ms:           { type: DataTypes.INTEGER.UNSIGNED },
  level:        { type: DataTypes.STRING(8) },
  message:      { type: DataTypes.TEXT },
  fw:           { type: DataTypes.STRING(16) },
  boot:         { type: DataTypes.INTEGER.UNSIGNED },
  wake:         { type: DataTypes.INTEGER.UNSIGNED },
  reset_reason: { type: DataTypes.STRING(16) },
  received_at:  { type: DataTypes.DATE, defaultValue: DataTypes.NOW }
}, {
  freezeTableName: true,
  timestamps: false,
  indexes: [
    { unique: true, fields: ['device_id', 'session', 'seq'] },
    { fields: ['device_id', 'datetime'] }
  ]
});

module.exports = { DeviceLog };
