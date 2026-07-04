const { Sequelize } = require('sequelize');
const { db } = require("../config/database");
const { DataTypes } = Sequelize;
const Kalender = db.define('kalender', {
 idkegiatan: {
 type: DataTypes.INTEGER,
 autoIncrement: true,
 primaryKey: true
 },
 tanggal: {
 type: DataTypes.DATE
 },
 detailkegiatan: {
 type: DataTypes.STRING
 },
 username: {
 type: DataTypes.STRING
 }
}, {
 freezeTableName: true,
 timestamps: false
});
module.exports = { Kalender };