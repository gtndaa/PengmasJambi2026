const { Op } = require('sequelize');
const { DeviceLog } = require('../models/deviceLogModel');

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

    await DeviceLog.bulkCreate(rows, { ignoreDuplicates: true });
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

module.exports = { tambahDeviceLog, getDeviceLog };
