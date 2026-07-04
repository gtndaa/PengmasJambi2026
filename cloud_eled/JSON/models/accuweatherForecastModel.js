const { Sequelize } = require('sequelize');
const { db } = require("../config/database");
const { DataTypes } = Sequelize;
const AccuweatherForecast = db.define('accuweatherforecast', {
 dateTime: {
 type: DataTypes.DATE,
 defaultValue: DataTypes.NOW,
 primaryKey: true
 },
 locationKey: {
 type: DataTypes.INTEGER
 },
 weatherIcon: {
 type: DataTypes.INTEGER
 },
 iconPhrase: {
 type: DataTypes.STRING
 },
 hasPrecipitation: {
 type: DataTypes.BOOLEAN
 },
 precipitationType: {
 type: DataTypes.STRING
 },
 precipitationIntensity: {
 type: DataTypes.STRING
 },
 isDaylight: {
 type: DataTypes.BOOLEAN
 },
 temperatureValue: {
 type: DataTypes.INTEGER
 },
 temperatureUnit: {
 type: DataTypes.STRING
 },
 temperatureUnitType: {
 type: DataTypes.INTEGER
 },
 precipitationProbability: {
 type: DataTypes.INTEGER
 }
}, {
 freezeTableName: true,
 timestamps: false
});
module.exports = { AccuweatherForecast };
