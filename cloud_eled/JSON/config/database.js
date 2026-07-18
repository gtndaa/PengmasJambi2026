require('dotenv').config();
   const { Sequelize } = require("sequelize");

   const db = new Sequelize(
     process.env.MYSQL_DATABASE, 
     process.env.MYSQL_USERNAME,
     process.env.MYSQL_ROOT_PASSWORD, 
     {
       host: process.env.MYSQL_ROOT_HOST,
       // IMPORTANT: Use the port provided by Aiven, don't hardcode 3310
       port: process.env.MYSQL_PORT || 3306, 
       logging: console.log,
       maxConcurrentQueries: 100,
       dialect: 'mysql',
       dialectOptions: {
         // Aiven requires SSL connections. This tells Sequelize to accept Aiven's certificate.
         ssl: {
           require: true,
           rejectUnauthorized: false // Set to false to allow Aiven's cert without manually downloading the CA file
         }
       },
       pool: { maxConnections: 5, maxIdleTime: 30 },
       language: 'en',
       timezone: "+07:00"
     }
   );

   const startdb = async () => {
     try {
       await db.authenticate();
       console.log('Database Connected to Aiven Successfully!');
       
       // Optional: If you want Sequelize to automatically create tables based on your models
       // await db.sync({ alter: true }); 
       
     } catch (error) {
       console.error('Unable to connect to the database:', error);
     }
   };

   module.exports = { db, startdb };
