// import fetch from "node-fetch";
const fetch = require('node-fetch');
const apiKey = process.env.APIKEY_ACCUWEATHER;
const oneHourlyApiUrl =
'http://dataservice.accuweather.com/forecasts/v1/hourly/1hour/';
const twelveHourlyApiUrl =
'http://dataservice.accuweather.com/forecasts/v1/hourly/12hour/';
//const locationKey = 3454195;
// Append the API key as a query parameter to the URL
// Define a function to fetch data
const fetchData = async (apiUrl, locationKey, apiKey) => {
 const apiUrlWithApiKey = `${apiUrl}${locationKey}?apikey=${apiKey}`;
 try {
 const response = await fetch(apiUrlWithApiKey);
 if (!response.ok) {
 throw new Error(`HTTP error! status: ${response.status}`);
 }
 const data = await response.json();
 return data;
 } catch (error) {
 console.log(error);
 }
};
// Fetch one hourly data
const fetchOneHourlyData = async (locationKey) => {
 try {
 const data = await fetchData(oneHourlyApiUrl, locationKey, apiKey);
 return data;
 } catch (error) {
 console.log(error);
 }
};
// Fetch 12 hourly data
const fetchTwelveHourlyData = async (locationKey) => {
 try {
 const data = await fetchData(twelveHourlyApiUrl, locationKey, apiKey);
 return data;
 } catch (error) {
 console.log(error);
 }
};
module.exports = { fetchData, fetchOneHourlyData, fetchTwelveHourlyData };