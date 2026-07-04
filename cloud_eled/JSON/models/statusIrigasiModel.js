const { Sequelize } = require('sequelize');
const { db } = require("../config/database");
const { DataTypes } = Sequelize;
const StatusIrigasi = db.define('statusirigasi', {
 dateTime: {
 type: DataTypes.DATE,
 primaryKey: true
 },
 persentaseSisaAir: {
 type: DataTypes.INTEGER
 },
 sisaAirLiter: {
 type: DataTypes.INTEGER
 },
 isActive: {
 type: DataTypes.BOOLEAN
 }
}, {
 freezeTableName: true,
 timestamps: false
});
module.exports = { StatusIrigasi };