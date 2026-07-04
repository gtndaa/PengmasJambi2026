const { Kalender } = require('../models/kalenderModel');
const getAllKegiatan = async (req, res) => {
 try {
 const kalender = await Kalender.findAll ({
 attributes: ['idkegiatan', 'tanggal', 'detailkegiatan', 'username']
 });
 res.json(kalender);
 } catch (error) {
    console.log(error);
 }
};
module.exports = { getAllKegiatan };