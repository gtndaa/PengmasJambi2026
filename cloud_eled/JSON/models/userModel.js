const { Sequelize } = require('sequelize');
const { db } = require("../config/database");
const { DataTypes } = Sequelize;
const Users = db.define('users', {
 firstName: {
 type: DataTypes.STRING
 },
 lastName: {
 type: DataTypes.STRING
 },
 username: {
 type: DataTypes.STRING,
 primaryKey: true
 },
 email: {
 type: DataTypes.STRING
 },
 password: {
 type: DataTypes.STRING
 },
 punyaAlat: {
 type: DataTypes.BOOLEAN
 },
 lokasiLahan: {
 type: DataTypes.STRING
 },
 refreshToken: {
 type: DataTypes.STRING
 },
 tanaman: {
 type: DataTypes.STRING
 }
}, {
 freezeTableName: true,
 timestamps: false
});
module.exports = { Users };