const { Sequelize } = require('sequelize');
const { db } = require("../config/database");
const { DataTypes } = Sequelize;

const DeviceConfig = db.define('device_config', {
    id: {
        type: DataTypes.INTEGER,
        autoIncrement: true,
        primaryKey: true
    },
    wifiSSID: {
        type: DataTypes.STRING(32),
        allowNull: false
    },
    wifiPassword: {
        type: DataTypes.STRING(64),
        allowNull: false
    },
    uploadInterval: {
        type: DataTypes.INTEGER,
        allowNull: false
    },
    listenWindow: {
        type: DataTypes.INTEGER,
        allowNull: false
    },
    sleepInterval: {
        type: DataTypes.INTEGER,
        allowNull: false
    },
    configVersion: {
        type: DataTypes.INTEGER,
        allowNull: false
    },
    useDeepSleep: {
        type: DataTypes.BOOLEAN,
        allowNull: false
    }
}, {
    freezeTableName: true,
    timestamps: false
});

module.exports = { DeviceConfig };
