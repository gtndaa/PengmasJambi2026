const { WeatherCondition } = require('../models/weatherConditionModel');
const { AccuweatherForecast } = require('../models/accuweatherForecastModel');
const { fetchData, fetchOneHourlyData, fetchTwelveHourlyData } =
require('../apiaccuweather');
const { getAllLocationKey } = require('./LokasiLahan');
const nodeCron = require('node-cron');
const { Op } = require('sequelize');
const days = ["Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu"];
const months =
["Januari","Februari","Maret","April","Mei","Juni","Juli","Agustus","September","Oktober","November","Desember"];
const getAllWeatherCondition = async (req, res) => {
 try {
 const weathercondition = await WeatherCondition.findAll ({
 attributes: ['id', 'datetime', 'epochdatetime', 'weathericon',
'iconphrase', 'hasprecipitation', 'precipitationtype',
'precipitationintensity', 'isdaylight', 'temperaturevalue', 'temperatureunit',
'temperatureunittype', 'precipitationprobability']
 });
 res.json(weathercondition);
 } catch (error) {
 console.log(error);
 }
};
const getLatestWeatherCondition = async (req, res) => {
 try {
 insertOneHourlyWeatherCondition(req.body.locationkey);
 const weathercondition = await WeatherCondition.findOne({
 where: {
 locationkey: req.body.locationkey // Add your constraint here
 },
 order: [
 ['dateTime', 'DESC'] // Order by dateTime in descending order to get the most recent record
 ]
 });
 day = days[weathercondition.dateTime.getDay()];
 date = weathercondition.dateTime.getDate().toString();
 month = months[weathercondition.dateTime.getMonth()];
 year = weathercondition.dateTime.getFullYear();
 const latestweathercondition = {data: [{
 dateInfo: (`${day}, ${date} ${month} ${year}`),
 time: (`${weathercondition.dateTime.getHours()}.00`),
 weatherIcon: weathercondition.weatherIcon,
 iconPhrase: weathercondition.iconPhrase,
 temperature: (`${Math.round((weathercondition.temperatureValue -
32) * 5 / 9)}°C`)
 }]}
 res.json(latestweathercondition);
 } catch (error) {
 console.log(error);
 }
};
const get12HoursForecasts = async (req, res) => {
 try {
 // Get the current date and time
 const currentTime = new Date();
 // Fetch forecast data where dateTime is greater than the current time
 const forecasts = await AccuweatherForecast.findAll({
 where: {
 locationkey: req.body.locationkey,
 dateTime: {
[Op.gt]: currentTime // Using Op.gt (greater than) operator from Sequelize
 }
 },
 order: [['dateTime', 'ASC']] // Order by dateTime in ascending order
 });

 const transformedData = forecasts.map(data => ({
 day: days[new Date(data.dateTime).getDay()],
 date: new Date(data.dateTime).getDate().toString(),
 month: months[new Date(data.dateTime).getMonth()],
 year: new Date(data.dateTime).getFullYear(),
 dateInfo: `${new Date(data.dateTime).getDate()} ${months[new
Date(data.dateTime).getMonth()]} ${new Date(data.dateTime).getFullYear()}`,
 time: `${new Date(data.dateTime).getHours()}.00`,
 temperature: `${Math.round((data.temperatureValue - 32) * 5 / 9)}°C`
 }));
 res.json(transformedData);
 } catch (error) {
 console.log(error);
 res.status(500).json({ error: 'Internal server error' });
 }
};
const insertOneHourlyWeatherCondition = async (locationKey) => {
 try {
 const fetchedData = await fetchOneHourlyData(locationKey);
 const weathercondition = await WeatherCondition.create({
 dateTime: new Date(fetchedData[0].DateTime),
 locationKey: locationKey,
 weatherIcon: fetchedData[0].WeatherIcon,
 iconPhrase: fetchedData[0].IconPhrase,
 hasPrecipitation: fetchedData[0].HasPrecipitation,
 precipitationType: (fetchedData[0].PrecipitationType || 'N/A'),
 precipitationIntensity: (fetchedData[0].PrecipitationIntensity ||
'N/A'),
 isDaylight: fetchedData[0].IsDaylight,
 temperatureValue: fetchedData[0].Temperature.Value,
 temperatureUnit: fetchedData[0].Temperature.Unit,
 temperatureUnitType: fetchedData[0].Temperature.UnitType,
 precipitationProbability: fetchedData[0].PrecipitationProbability,
 });
 console.log({
 msg: 'Weather condition terbaru dari AccuWeather berhasil ditambahkan'
 });
 } catch (error) {
 console.log(error);
 }
};
const insertTwelveHourlyWeatherCondition = async (locationKey) => {
 try {
 const fetchedData = await fetchTwelveHourlyData(locationKey);
 const transformedData = fetchedData.map(data => ({
 dateTime: new Date(data.DateTime),
 locationKey: locationKey,
 weatherIcon: data.WeatherIcon,
 iconPhrase: data.IconPhrase,
 hasPrecipitation: data.HasPrecipitation,
 precipitationType: data.PrecipitationType || 'N/A',
 precipitationIntensity: data.PrecipitationIntensity || 'N/A',
 isDaylight: data.IsDaylight,
 temperatureValue: data.Temperature.Value,
 temperatureUnit: data.Temperature.Unit,
 temperatureUnitType: data.Temperature.UnitType,
 precipitationProbability: data.PrecipitationProbability
 }));
 const weathercondition = await
AccuweatherForecast.bulkCreate(transformedData);
 console.log({
 msg: '12 Weather forecast terbaru dari AccuWeather berhasil ditambahkan'
 });
 } catch (error) {
 console.log(error);
 }
};
const updateWeatherForecast = async (req, res) => {
 try {
 const alllocationkeys = await getAllLocationKey();
 // alllocationkeys is likely an array of objects
 alllocationkeys.forEach(location => {
 //console.log(location.locationkey);
 insertTwelveHourlyWeatherCondition(location.locationkey);
 });
 res.status(200).json({ message: 'Weather forecast updated successfully'
});
 } catch (error) {
 console.error('Error updating weather forecast:', error);
 res.status(500).json({ message: 'Internal Server Error' });
 }
};
//const job = nodeCron.schedule('0 32 * * * *', insertWeatherCondition);
module.exports = { getAllWeatherCondition, getLatestWeatherCondition,
insertOneHourlyWeatherCondition, insertTwelveHourlyWeatherCondition,
updateWeatherForecast, get12HoursForecasts };