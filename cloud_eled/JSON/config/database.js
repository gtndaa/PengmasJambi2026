require('dotenv').config();
const fs = require('fs'); // Perlu untuk membaca sertifikat SSL
const { Sequelize } = require("sequelize");

const db = new Sequelize(
  process.env.MYSQL_DATABASE, 
  process.env.MYSQL_USERNAME,
  process.env.MYSQL_ROOT_PASSWORD, 
  {
    host: process.env.MYSQL_ROOT_HOST,
    port: process.env.MYSQL_PORT || 22639, // Sesuaikan dengan port Aiven Anda
    dialect: 'mysql',
    logging: false, // Ubah ke false agar log tidak memenuhi terminal saat produksi
    dialectOptions: {
      ssl: {
        require: true,
        rejectUnauthorized: true,
        // Download file 'ca.pem' dari dashboard Aiven, simpan di folder config/
        ca: fs.readFileSync('./config/ca.pem') 
      }
    },
    pool: { max: 5, min: 0, idle: 10000 },
    timezone: "+07:00"
  }
);

const startdb = async () => {
  try {
    await db.authenticate();
    console.log('✅ Database Connected successfully to Aiven!');
  } catch (error) {
    console.error('❌ Unable to connect to the database:', error);
  }
};

module.exports = { db, startdb };
