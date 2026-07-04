const { Sequelize } = require('sequelize');
const { db } = require("../config/database");
const { DataTypes } = Sequelize;
const LokasiLahan = db.define('lokasilahan', {
 idLahan: {
 type: DataTypes.INTEGER,
 autoIncrement: true,
 primaryKey: true
 },
 lokasi: {
 type: DataTypes.STRING
 },
 locationkey: {
 type: DataTypes.INTEGER
 }
}, {
 freezeTableName: true,
 timestamps: false
});
module.exports = { LokasiLahan };
