const { Op } = require('sequelize');
const { DeviceLog } = require('../models/deviceLogModel');
const { SensorData } = require('../models/SensorData')

const LEVELS = ['DEBUG', 'INFO', 'WARN', 'ERROR'];

const tambahDeviceLog = async (req, res) => {
  try {
    const { id, fw, session, boot, wake, reset_reason, dropped, logs } = req.body;
    if (!id || session === undefined || !Array.isArray(logs)) {
      return res.status(400).json({ success: false, message: 'Field id, session, dan logs wajib ada' });
    }

    const rows = logs
      .filter(l => Number.isInteger(l.seq))
      .map(l => ({
        device_id: id,
        session,
        seq: l.seq,
        datetime: l.dt || null,
        ms: l.ms,
        level: LEVELS.includes(l.lvl) ? l.lvl : 'INFO',
        message: String(l.msg ?? ''),
        fw, boot, wake, reset_reason
      }));

    if (dropped > 0 && rows.length > 0) {
      rows.unshift({
        device_id: id, session, seq: Math.max(0, rows[0].seq - 1),
        datetime: rows[0].datetime, ms: 0, level: 'WARN',
        message: `[SERVER] ${dropped} baris log terbuang di device sebelum batch ini (buffer penuh)`,
        fw, boot, wake, reset_reason
      });
    }

    // 1. Simpan log baru ke database
    await DeviceLog.bulkCreate(rows, { ignoreDuplicates: true });

    // =======================================================
    // 2. KOREKSI STATUS BATERAI (SENSOR DATA) BERDASARKAN LOG
    // =======================================================
    let isSupercapOk = false;

    // Pindai setiap baris log yang baru masuk untuk mencari angka tegangan
    for (const row of rows) {
        // Regex ini akan menangkap angka desimal sebelum huruf V (misal: "11.50V" atau "12.1 v")
        // Atau menangkap angka setelah kata cap/volt (misal: "Supercap voltage: 11.45")
        const match = row.message.match(/V=(\d{1,2}\.\d+)/);

        if (match) {
            const volt = parseFloat(match[1]);
            if (volt > 11.0) {
                isSupercapOk = true;
                break; // Hentikan pencarian jika sudah ketemu
            }
        }
    }

    // Jika tegangan terpantau di atas 11V, perbaiki tabel Sensor Data
    if (isSupercapOk) {
        // Tarik data cuaca yang paling terakhir dimasukkan oleh alat
        const latestSensor = await SensorData.findOne({
            order: [['logId', 'DESC']]
        });

        // Jika data terakhir tersebut statusnya masih 'on cap', timpa dengan 'OK'
        if (latestSensor && latestSensor.batt === 'ON_CAP') {
            latestSensor.batt = 'OK';
            await latestSensor.save(); // Simpan perubahan secara permanen
        }
    }

    // =======================================================
    // 3. AUTO-DELETE LOG LAMA (RETENSI 7 HARI)
    // =======================================================
    const RETENTION_DAYS = 7;
    const cutoffDate = new Date();
    cutoffDate.setDate(cutoffDate.getDate() - RETENTION_DAYS);

    DeviceLog.destroy({
      where: {
        received_at: { [Op.lt]: cutoffDate }
      }
    }).catch(err => console.error('Error saat auto-delete log lama:', err));

    res.status(201).json({ success: true, received: rows.length });
  } catch (error) {
    console.error('Error simpan device log:', error);
    res.status(500).json({ success: false, message: 'Gagal menyimpan device log', detail: error.message });
  }
};

const getDeviceLog = async (req, res) => {
  try {
    const { id, level, since, until } = req.query;
    const limit = Math.min(parseInt(req.query.limit, 10) || 200, 2000);
    const where = {};
    if (id) where.device_id = id;
    if (level && LEVELS.includes(level)) where.level = { [Op.in]: LEVELS.slice(LEVELS.indexOf(level)) };
    if (since || until) {
      where.datetime = {};
      if (since) where.datetime[Op.gte] = since;
      if (until) where.datetime[Op.lte] = until;
    }
    const data = await DeviceLog.findAll({
      where,
      order: [['logId', 'DESC']],
      limit
    });
    res.status(200).json({ success: true, count: data.length, data });
  } catch (error) {
    console.error('Error ambil device log:', error);
    res.status(500).json({ success: false, message: 'Gagal mengambil device log', detail: error.message });
  }
};

const getInfoLogsOnly = async (req, res) => {
  try {
    const limit = Math.min(parseInt(req.query.limit, 10) || 200, 2000);
    
    // Tambahkan logId ke attributes dan gunakan raw: true
    const data = await DeviceLog.findAll({
      where: { level: 'INFO' },
      attributes: ['logId', 'datetime', 'message'], 
      order: [['logId', 'DESC']],
      limit,
      raw: true // PENTING: Memaksa Sequelize mengembalikan JSON biasa, bukan objek Model yang berat
    });
    
    const logMessages = data.map(row => {
      let time = 'No-Time';
      if (row.datetime) {
        try {
          const d = new Date(row.datetime);
            if (!isNaN(d.getTime())) {
            // Tambahkan offset 7 jam (untuk WIB / Asia/Jakarta)
              const tzOffset = 7 * 60 * 60 * 1000; 
              const localTime = new Date(d.getTime() + tzOffset);
                    
              time = localTime.toISOString().replace('T', ' ').slice(0, 19);
            } else {
              time = String(row.datetime);
            }
        } catch (e) {
          time = String(row.datetime);
        }
      }
      return `[${time}] ${row.message || ''}`;
    });

    const plainTextLogs = logMessages.join('\n');
    res.setHeader('Content-Type', 'text/plain');
    return res.status(200).send(plainTextLogs);

  } catch (error) {
    console.error('Error ambil device log INFO:', error);
    // Jika masih error, detailnya akan dimunculkan di response
    res.status(500).json({ success: false, message: 'Gagal mengambil log', detail: error.message || String(error) });
  }
};

module.exports = { tambahDeviceLog, getDeviceLog, getInfoLogsOnly };
