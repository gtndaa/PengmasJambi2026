const { LokasiLahan } = require('../models/lokasiLahanModel');
const getAllLokasiLahan = async (req, res) => {
 try {
 const lokasilahan = await LokasiLahan.findAll ({
 attributes: ['lokasi']
 });
 res.json(lokasilahan);
 } catch (error) {
 console.error('Error fetching locations:', error);
 throw error;
 }
};
const getAllLocationKey = async () => {
 try {
 const locations = await LokasiLahan.findAll({
 attributes: ['locationkey']
 });
 return locations;
 } catch (error) {
 console.error('Error fetching location keys:', error);
 throw error;
 }
};
const getOneLocationKey = async (req, res) => {
 try {
 locationkey = await LokasiLahan.findOne({
 where: { lokasi: req.body.lokasi }
 });
 res.json(locationkey);
 } catch (error) {
 console.error('Error fetching location keys:', error);
 throw error;
 }
};
module.exports = { getAllLokasiLahan, getAllLocationKey, getOneLocationKey };