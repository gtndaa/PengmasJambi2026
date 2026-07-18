require('dotenv').config();
const express = require('express');
const { startdb } = require('./config/database');
const { router } = require('./routes/routes');

const app = express();
const PORT = process.env.PORT || 5000;

// Middleware for parsing JSON and URL-encoded data from the mobile frontend
app.use(express.json());
app.use(express.urlencoded({ extended: false }));

// Mount the centralized routes file provided in your B400 document
app.use('/', router);

// Global Error Handler
app.use((err, req, res, next) => {
  console.error(err.stack);
  res.status(500).json({ error: true, msg: 'Something went wrong on the server!' });
});

// Bootstrap Database Connection and Start Web Server
const bootstrap = async () => {
  try {
    // 1. Authenticate with Aiven MySQL database
    await startdb();
    
    // 2. Launch Express listener
    app.listen(PORT, () => {
      console.log(`Server berjalan di port ${PORT}`);
    });
  } catch (error) {
    console.error('Failed to bootstrap the backend server:', error);
    process.exit(1);
  }
};

bootstrap();