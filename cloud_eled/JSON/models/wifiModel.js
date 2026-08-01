const { Sequelize } = require('sequelize');
const { db } = require("../config/database");
const { DataTypes } = Sequelize;

const WifiConfig = db.define('wifi_config', {
    id: {
        type: DataTypes.INTEGER,
        autoIncrement: true,
        primaryKey: true
    },
    ssid: {
        type: DataTypes.STRING,
        allowNull: false
    },
    password: {
        type: DataTypes.STRING,
        allowNull: false
    }
}, {
    freezeTableName: true,
    timestamps: false
});

module.exports = { WifiConfig };
