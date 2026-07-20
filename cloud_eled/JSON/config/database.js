require('dotenv').config();
const { Sequelize } = require("sequelize");
const db = new Sequelize(process.env.MYSQL_DATABASE, process.env.MYSQL_USERNAME,
process.env.MYSQL_ROOT_PASSWORD, {
 host: process.env.MYSQL_ROOT_HOST,
 port: 3310,
 logging: console.log,
 maxConcurrentQueries: 100,
 dialect: 'mysql',
 dialectOptions: {
 ssl:'Amazon RDS'
 },
 pool: { maxConnections: 5, maxIdleTime: 30},
 language: 'en',
 timezone: "+07:00"
})
const startdb = async () => {
 try {
 await db.authenticate();
 console.log('Database Connected');
 } catch (error) {
 console.log(error);
 }
}
module.exports = { db, startdb };
