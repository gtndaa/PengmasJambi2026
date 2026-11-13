import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';

import '../consts/colors.dart';
import '../models/sensor_metric.dart';
import '../models/sensor_model.dart';
import '../services/sensor_history_service.dart';

class SensorHistoryChart extends StatefulWidget {
  final SensorMetric metric;

  const SensorHistoryChart({super.key, required this.metric});

  @override
  State<SensorHistoryChart> createState() => _SensorHistoryChartState();
}

enum _Preset {
  day('1 Hari', HistoryGranularity.hour),
  week('1 Minggu', HistoryGranularity.day),
  month('1 Bulan', HistoryGranularity.week),
  threeMonths('3 Bulan', HistoryGranularity.month),
  year('1 Tahun', HistoryGranularity.quarter),
  custom('Kustom', HistoryGranularity.day);

  final String label;
  final HistoryGranularity granularity; // satuan default tiap rentang cepat
  const _Preset(this.label, this.granularity);
}

class _Point {
  final DateTime start; // awal bucket (dulu: day)
  final double? value;
  const _Point(this.start, this.value);
}

class _SensorHistoryChartState extends State<SensorHistoryChart> {
  final _service = SensorHistoryService();

  _Preset _preset = _Preset.week;
  late DateTime _from;
  late DateTime _to;
  HistoryGranularity _granularity = HistoryGranularity.day;
  bool _showDates = false; // true kalau bagian "Atur tanggal" sedang dibuka
  HistoryResult? _result; // hasil ringkasan terakhir (untuk statistik hujan)

  late DateTime _shownFrom;
  late DateTime _shownTo;
  HistoryGranularity _shownGranularity = HistoryGranularity.day;

  int _requestId = 0;

  static const _monthShort = [
    'Jan', 'Feb', 'Mar', 'Apr', 'Mei', 'Jun',
    'Jul', 'Agu', 'Sep', 'Okt', 'Nov', 'Des',
  ];
  static const _monthFull = [
    'Januari', 'Februari', 'Maret', 'April', 'Mei', 'Juni',
    'Juli', 'Agustus', 'September', 'Oktober', 'November', 'Desember',
  ];
  
  List<SensorDataModel> _readings = [];
  bool _loading = true;
  String? _error;

  static const _labelStyle = TextStyle(color: Colors.white70, fontSize: 10);

  @override
  void initState() {
    super.initState();
    _applyPreset(_Preset.week, load: false);
    _shownFrom = _from;
    _shownTo = _to;
    _shownGranularity = _granularity;
    _load();
  }

  static DateTime _today() {
    final n = DateTime.now();
    return DateTime(n.year, n.month, n.day);
  }

  void _applyPreset(_Preset p, {bool load = true}) {
    final today = _today();
    DateTime from;
    switch (p) {
      case _Preset.day:
        from = today;
        break;
      case _Preset.week:
        from = today.subtract(const Duration(days: 6));
        break;
      case _Preset.month:
        from = today.subtract(const Duration(days: 29));
        break;
      case _Preset.threeMonths:
        from = DateTime(today.year, today.month - 2, 1); // 3 bulan penuh
        break;
      case _Preset.year:
        final qStartMonth = ((today.month - 1) ~/ 3) * 3 + 1;
        from = DateTime(today.year, qStartMonth - 9, 1); // 4 kuartal
        break;
      case _Preset.custom:
        return;
    }
    _preset = p;
    _from = from;
    _to = today;
    _granularity = p.granularity;
    if (load) _load();
  }

  int get _spanDays => _to.difference(_from).inDays + 1;

  HistoryGranularity _suggest(int spanDays) {
    if (spanDays <= 2) return HistoryGranularity.hour;
    if (spanDays <= 14) return HistoryGranularity.day;
    if (spanDays <= 60) return HistoryGranularity.week;
    if (spanDays <= 200) return HistoryGranularity.month;
    return HistoryGranularity.quarter;
  }

  Future<void> _pickDate({required bool isFrom}) async {
    final picked = await showDatePicker(
      context: context,
      initialDate: isFrom ? _from : _to,
      firstDate: DateTime(2020),
      lastDate: _today(),
      helpText: isFrom ? 'Dari tanggal' : 'Sampai tanggal',
      confirmText: 'Pilih',
      cancelText: 'Batal',
    );
    if (picked == null || !mounted) return;

    final d = DateTime(picked.year, picked.month, picked.day);
    setState(() {
      if (isFrom) {
        _from = d;
        if (_to.isBefore(_from)) _to = _from;
      } else {
        _to = d;
        if (_from.isAfter(_to)) _from = _to;
      }
      // batas rentang: maksimal 400 hari (data diunduh mentah); sisi yang lain ikut bergeser
      if (_spanDays > 400) {
        if (isFrom) {
          final end = _from.add(const Duration(days: 399));
          _to = end.isAfter(_today()) ? _today() : end;
        } else {
          _from = _to.subtract(const Duration(days: 399));
        }
      }
      _preset = _Preset.custom;
      _granularity = _suggest(_spanDays); // satuan titik dipilih otomatis
    });
    _load();
  }

  /// Mode 1 Hari: pilih satu tanggal saja, data tetap per jam untuk hari itu.
  Future<void> _pickSingleDay() async {
    final picked = await showDatePicker(
      context: context,
      initialDate: _from,
      firstDate: DateTime(2020),
      lastDate: _today(),
      helpText: 'Pilih tanggal',
      confirmText: 'Pilih',
      cancelText: 'Batal',
    );
    if (picked == null || !mounted) return;

    final d = DateTime(picked.year, picked.month, picked.day);
    setState(() {
      _from = d;
      _to = d;
      _preset = _Preset.day; // tetap "1 Hari"
      _granularity = HistoryGranularity.hour;
    });
    _load();
  }

  Future<void> _load() async {
    final id = ++_requestId;
    final from = _from, to = _to, granularity = _granularity;

    setState(() {
      _loading = true;
      _error = null;
    });
    try {
      final result = await _service.getHistory(
        from: from,
        to: to,
        granularity: granularity,
      );
      if (!mounted || id != _requestId) return; // user sudah ganti pilihan
      setState(() {
        _result = result;
        _readings = result.rows;
        _shownFrom = from;
        _shownTo = to;
        _shownGranularity = granularity;
        _loading = false;
      });
    } catch (e) {
      if (!mounted || id != _requestId) return;
      setState(() {
        _error = e.toString().replaceFirst('Exception: ', '');
        _loading = false;
      });
    }
  }

  DateTime _bucketStart(DateTime d, HistoryGranularity g) {
    switch (g) {
      case HistoryGranularity.hour:
        return DateTime(d.year, d.month, d.day, d.hour);
      case HistoryGranularity.day:
        return DateTime(d.year, d.month, d.day);
      case HistoryGranularity.week:
        return DateTime(d.year, d.month, d.day - (d.weekday - 1));
      case HistoryGranularity.month:
        return DateTime(d.year, d.month, 1);
      case HistoryGranularity.quarter:
        return DateTime(d.year, ((d.month - 1) ~/ 3) * 3 + 1, 1);
    }
  }

  DateTime _nextBucket(DateTime d, HistoryGranularity g) {
    switch (g) {
      case HistoryGranularity.hour:
        return DateTime(d.year, d.month, d.day, d.hour + 1);
      case HistoryGranularity.day:
        return DateTime(d.year, d.month, d.day + 1);
      case HistoryGranularity.week:
        return DateTime(d.year, d.month, d.day + 7);
      case HistoryGranularity.month:
        return DateTime(d.year, d.month + 1, 1);
      case HistoryGranularity.quarter:
        return DateTime(d.year, d.month + 3, 1);
    }
  }

  List<_Point> _points() {
    final g = _shownGranularity;
    final byTime = <int, SensorDataModel>{
      for (final r in _readings)
        _bucketStart(r.datetime, g).millisecondsSinceEpoch: r,
    };

    var end = DateTime(_shownTo.year, _shownTo.month, _shownTo.day, 23, 59, 59);
    // Tampilan per jam untuk hari ini: sumbu berhenti di jam sekarang,
    // jam yang belum terjadi tidak digambar.
    final now = DateTime.now();
    if (g == HistoryGranularity.hour && end.isAfter(now)) end = now;
    final result = <_Point>[];
    var t = _bucketStart(_shownFrom, g);
    while (!t.isAfter(end)) {
      final r = byTime[t.millisecondsSinceEpoch];
      result.add(_Point(t, r == null ? null : widget.metric.valueOf(r)));
      t = _nextBucket(t, g);
    }
    return result;
  }

  String _f(double v) {
    final a = v.abs();
    if (a >= 1000) return '${(v / 1000).toStringAsFixed(1)}k';
    if (a >= 100) return v.toStringAsFixed(0);
    return v.toStringAsFixed(1);
  }

  String _two(int v) => v.toString().padLeft(2, '0');
  String _fullDate(DateTime d) => '${d.day} ${_monthShort[d.month - 1]} ${d.year}';
  int get _shownSpanDays => _shownTo.difference(_shownFrom).inDays + 1;

  String _axisLabel(DateTime d) {
    switch (_shownGranularity) {
      case HistoryGranularity.hour:
        return _shownSpanDays > 1
            ? '${d.day}/${d.month} ${_two(d.hour)}:00'
            : '${_two(d.hour)}:00';
      case HistoryGranularity.day:
      case HistoryGranularity.week:
        return '${d.day}/${d.month}';
      case HistoryGranularity.month:
        return _shownFrom.year != _shownTo.year
            ? '${_monthShort[d.month - 1]} ${_two(d.year % 100)}'
            : _monthShort[d.month - 1];
      case HistoryGranularity.quarter:
        return 'Q${(d.month - 1) ~/ 3 + 1} ${_two(d.year % 100)}';
    }
  }

  String _tooltipTitle(DateTime d) {
    switch (_shownGranularity) {
      case HistoryGranularity.hour:
        return '${d.day}/${d.month}/${d.year} ${_two(d.hour)}:00';
      case HistoryGranularity.day:
        return '${d.day}/${d.month}/${d.year}';
      case HistoryGranularity.week:
        final e = d.add(const Duration(days: 6));
        return '${d.day}/${d.month} - ${e.day}/${e.month}/${e.year}';
      case HistoryGranularity.month:
        return '${_monthFull[d.month - 1]} ${d.year}';
      case HistoryGranularity.quarter:
        return '${_monthShort[d.month - 1]}-${_monthShort[d.month + 1]} ${d.year}';
    }
  }

  @override
  Widget build(BuildContext context) {
    final m = widget.metric;
    final g = _shownGranularity;
    final points = _points();
    final values = [
      for (final p in points)
        if (p.value != null) p.value!
    ];
    final empty = _readings.isEmpty || values.isEmpty;

    final total = values.fold<double>(0, (a, b) => a + b);
    final maxV = empty ? 0.0 : values.reduce((a, b) => a > b ? a : b);
    final minV = empty ? 0.0 : values.reduce((a, b) => a < b ? a : b);
    final avg = empty ? 0.0 : total / values.length;
    final isHourly = g == HistoryGranularity.hour;
    final daysWithData = _result?.daysWithData ?? 0;
    final rainCount = isHourly
        ? values.where((v) => v > 0).length
        : (_result?.rainyDays ?? 0);

    final stats = m.isSum
        ? [
            ['Total', '${_f(total)} ${m.unit}'],
            [
              isHourly ? 'Jam hujan' : 'Hari hujan',
              '$rainCount ${isHourly ? 'jam' : 'hari'}',
            ],
            ['Terbasah', '${_f(maxV)} ${m.unit}'],
          ]
        : [
            ['Rata-rata', '${_f(avg)} ${m.unit}'],
            ['Tertinggi', '${_f(maxV)} ${m.unit}'],
            ['Terendah', '${_f(minV)} ${m.unit}'],
          ];

    String? rainInfo;
    if (m.isSum && !empty && !isHourly && daysWithData > 0) {
      // hari kering (0 mm) ikut dihitung, hari tanpa data tidak
      final dailyAvg = total / daysWithData;
      rainInfo =
          'Rata-rata harian ${_f(dailyAvg)} mm/hari (dari $daysWithData hari berdata)';
      // perbandingan hari ini terhadap rata-rata (tampilan per hari, mis. 1 Minggu)
      if (g == HistoryGranularity.day && _shownTo == _today()) {
        final todayMs = _today().millisecondsSinceEpoch;
        final todayV = points
            .where((p) => p.start.millisecondsSinceEpoch == todayMs)
            .map((p) => p.value)
            .firstOrNull;
        if (todayV != null) {
          final diff = todayV - dailyAvg;
          final sign = diff >= 0 ? '+' : '−';
          rainInfo +=
              '\nHari ini ${_f(todayV)} mm ($sign${_f(diff.abs())} mm dari rata-rata)';
        }
      }
      if (g == HistoryGranularity.month && values.length > 1) {
        rainInfo += '\nRata-rata per bulan ${_f(total / values.length)} mm';
      }
    }

    final title = m.isSum? 'Curah Hujan' : m.label;
    final subtitle = m.isSum
        ? 'Total ${g.label.toLowerCase()}'
        : m.isMax
            ? 'Tertinggi ${g.label.toLowerCase()}'
            : 'Rata-rata ${g.label.toLowerCase()}';

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.fromLTRB(18, 16, 18, 18),
      decoration: BoxDecoration(
        gradient: const LinearGradient(
          colors: AppColors.blueGradient,
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
        ),
        borderRadius: BorderRadius.circular(28),
        boxShadow: [
          BoxShadow(
            color: AppColors.primary.withValues(alpha: 0.30),
            blurRadius: 20,
            offset: const Offset(0, 10),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(m.icon, color: Colors.white, size: 20),
              const SizedBox(width: 8),
              Expanded(
                child: Text(
                  title,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                  style: const TextStyle(
                    color: Colors.white,
                    fontSize: 15,
                    fontWeight: FontWeight.w600,
                  ),
                ),
              ),
            ],
          ),
          Padding(
            padding: const EdgeInsets.only(left: 28, top: 2),
            child: Text(
              subtitle,
              style: const TextStyle(color: Colors.white70, fontSize: 11),
            ),
          ),
          const SizedBox(height: 12),

          // Rentang cepat
          Wrap(
            spacing: 6,
            runSpacing: 6,
            children: [
              for (final p in _Preset.values)
                if (p != _Preset.custom)
                  _chip(
                    p.label,
                    selected: _preset == p,
                    onTap: () {
                      setState(() => _applyPreset(p, load: false));
                      _load();
                    },
                  ),
              if (_preset == _Preset.custom)
                _chip('Kustom', selected: true, onTap: () {}),
            ],
          ),
          const SizedBox(height: 4),

          // Rentang yang sedang dipakai + tombol membuka pemilih tanggal
          Row(
            children: [
              const Icon(Icons.calendar_today, color: Colors.white70, size: 14),
              const SizedBox(width: 6),
              Expanded(
                child: Text(
                  _from == _to
                      ? _fullDate(_from)
                      : '${_fullDate(_from)} – ${_fullDate(_to)}',
                  style: const TextStyle(color: Colors.white70, fontSize: 11),
                ),
              ),
              GestureDetector(
                onTap: () => setState(() => _showDates = !_showDates),
                child: Row(
                  children: [
                    const Text(
                      'Atur tanggal',
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: 12,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                    Icon(
                      _showDates ? Icons.expand_less : Icons.expand_more,
                      color: Colors.white,
                      size: 18,
                    ),
                  ],
                ),
              ),
            ],
          ),

          if (_showDates) ...[
            const SizedBox(height: 8),
            if (_preset == _Preset.day)
              // 1 Hari: cukup satu tanggal
              _dateField('Tanggal', _from, _pickSingleDay)
            else
              Row(
                children: [
                  Expanded(
                    child: _dateField('Dari', _from, () => _pickDate(isFrom: true)),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: _dateField('Sampai', _to, () => _pickDate(isFrom: false)),
                  ),
                ],
              ),
            const SizedBox(height: 6),
            Text(
              'Satuan titik dipilih otomatis: ${_granularity.label.toLowerCase()}',
              style: const TextStyle(color: Colors.white70, fontSize: 11),
            ),
          ],
          const SizedBox(height: 14),

          Row(
            children: [
              for (final s in stats)
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        s[0],
                        style: TextStyle(
                          color: Colors.white.withValues(alpha: 0.8),
                          fontSize: 11,
                        ),
                      ),
                      const SizedBox(height: 2),
                      Text(
                        s[1],
                        style: const TextStyle(
                          color: Colors.white,
                          fontSize: 14,
                          fontWeight: FontWeight.w600,
                        ),
                      ),
                    ],
                  ),
                ),
            ],
          ),
          if (rainInfo != null) ...[
            const SizedBox(height: 8),
            Text(
              rainInfo,
              style: const TextStyle(color: Colors.white70, fontSize: 11),
            ),
          ],
          const SizedBox(height: 18),
          SizedBox(height: 190, child: _body(points, empty)),
          if (m.isSum && !empty)
            const Padding(
              padding: EdgeInsets.only(top: 8),
              child: Text(
                'Kuning = paling basah · Oranye = data hari itu tidak lengkap',
                style: TextStyle(color: Colors.white70, fontSize: 10),
              ),
            ),
        ],
      ),
    );
  }

  Widget _chip(
    String label, {
    required bool selected,
    required VoidCallback onTap,
    bool enabled = true,
  }) {
    return GestureDetector(
      onTap: enabled ? onTap : null,
      child: Opacity(
        opacity: enabled ? 1 : 0.35,
        child: Container(
          padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
          decoration: BoxDecoration(
            color: selected ? Colors.white : Colors.white.withValues(alpha: 0.22),
            borderRadius: BorderRadius.circular(14),
          ),
          child: Text(
            label,
            style: TextStyle(
              fontSize: 11,
              fontWeight: FontWeight.w600,
              color: selected ? AppColors.primary : Colors.white,
            ),
          ),
        ),
      ),
    );
  }

  Widget _dateField(String label, DateTime value, VoidCallback onTap) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 7),
        decoration: BoxDecoration(
          color: Colors.white.withValues(alpha: 0.22),
          borderRadius: BorderRadius.circular(14),
        ),
        child: Row(
          children: [
            const Icon(Icons.calendar_today, color: Colors.white, size: 14),
            const SizedBox(width: 6),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(label,
                      style: TextStyle(
                          color: Colors.white.withValues(alpha: 0.8), fontSize: 9)),
                  Text(_fullDate(value),
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                      style: const TextStyle(
                          color: Colors.white,
                          fontSize: 12,
                          fontWeight: FontWeight.w600)),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _body(List<_Point> points, bool empty) {
    if (_loading) {
      return const Center(child: CircularProgressIndicator(color: Colors.white));
    }
    if (_error != null) {
      return Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              _error!,
              textAlign: TextAlign.center,
              maxLines: 3,
              overflow: TextOverflow.ellipsis,
              style: const TextStyle(color: Colors.white),
            ),
            TextButton(
              onPressed: _load,
              child: const Text('Coba lagi', style: TextStyle(color: Colors.white)),
            ),
          ],
        ),
      );
    }
    if (empty) {
      return const Center(
        child: Text(
          'Belum ada data pada rentang ini.',
          style: TextStyle(color: Colors.white),
        ),
      );
    }
    return widget.metric.isSum ? _bar(points) : _line(points);
  }

  FlTitlesData _titles(List<_Point> points, double interval) {
    final multiDayHours =
        _shownGranularity == HistoryGranularity.hour && _shownSpanDays > 1;
    final maxLabels = multiDayHours ? 4 : 6;
    final step = (points.length / maxLabels).ceil().clamp(1, 100000).toInt();

    return FlTitlesData(
      topTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)),
      rightTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)),
      leftTitles: AxisTitles(
        sideTitles: SideTitles(
          showTitles: true,
          reservedSize: 36,
          interval: interval,
          getTitlesWidget: (value, meta) => SideTitleWidget(
            meta: meta,
            child: Text(_f(value), style: _labelStyle),
          ),
        ),
      ),
      bottomTitles: AxisTitles(
        sideTitles: SideTitles(
          showTitles: true,
          reservedSize: 24,
          interval: 1,
          getTitlesWidget: (value, meta) {
            final i = value.toInt();
            if (value != i.toDouble() || i < 0 || i >= points.length || i % step != 0) {
              return const SizedBox.shrink();
            }
            return SideTitleWidget(
              meta: meta,
              child: Text(_axisLabel(points[i].start), style: _labelStyle),
            );
          },
        ),
      ),
    );
  }

  FlGridData _grid(double interval) => FlGridData(
        show: true,
        drawVerticalLine: false,
        horizontalInterval: interval,
        getDrawingHorizontalLine: (_) => FlLine(
          color: Colors.white.withValues(alpha: 0.25),
          strokeWidth: 1,
          dashArray: [4, 4],
        ),
      );

  static const _tooltipStyle = TextStyle(
    color: AppColors.ink,
    fontWeight: FontWeight.w600,
    fontSize: 12,
  );

  Widget _line(List<_Point> points) {
    final spots = <FlSpot>[
      for (int i = 0; i < points.length; i++)
        if (points[i].value != null) FlSpot(i.toDouble(), points[i].value!),
    ];

    var lo = spots.map((s) => s.y).reduce((a, b) => a < b ? a : b);
    var hi = spots.map((s) => s.y).reduce((a, b) => a > b ? a : b);
    if (hi - lo < 1) hi = lo + 1;
    final pad = (hi - lo) * 0.15;
    final minY = (lo >= 0 && lo - pad < 0) ? 0.0 : lo - pad;
    final maxY = hi + pad;
    final interval = (maxY - minY) / 2;

    return LineChart(
      LineChartData(
        minX: 0,
        maxX: (points.length - 1).clamp(1, 100000).toDouble(),
        minY: minY,
        maxY: maxY,
        borderData: FlBorderData(show: false),
        gridData: _grid(interval),
        titlesData: _titles(points, interval),
        lineTouchData: LineTouchData(
          touchTooltipData: LineTouchTooltipData(
            getTooltipColor: (_) => Colors.white,
            getTooltipItems: (touched) => touched.map((s) {
              final d = points[s.x.toInt()].start;
              return LineTooltipItem('${_tooltipTitle(d)}\n${_f(s.y)} ${widget.metric.unit}', _tooltipStyle
              );
            }).toList(),
          ),
        ),
        lineBarsData: [
          LineChartBarData(
            spots: spots,
            isCurved: true,
            color: Colors.white,
            barWidth: 3,
            dotData: FlDotData(show: points.length <= 31),
            belowBarData: BarAreaData(
              show: true,
              color: Colors.white.withValues(alpha: 0.15),
            ),
          ),
        ],
      ),
    );
  }

  double _barWidth(int n) {
    if (n <= 8) return 18;
    if (n <= 31) return 10;
    if (n <= 62) return 5;
    if (n <= 200) return 2.5;
    return 1.2;
  }

  Widget _bar(List<_Point> points) {
    final maxMm = points.fold<double>(0, (m, p) => (p.value ?? 0) > m ? p.value! : m);
    final double maxY;
    if (maxMm <= 10) {
      maxY = 10;
    } else if (maxMm <= 100) {
      maxY = (maxMm / 10).ceil() * 10.0;
    } else {
      maxY = (maxMm / 100).ceil() * 100.0;
    }
    final interval = maxY / 2;

    final cov = _result?.hoursByBucket ?? const <int, int>{};
    int wettest = -1;
    double wettestV = 0;
    for (int i = 0; i < points.length; i++) {
      final v = points[i].value ?? 0;
      if (v > wettestV) {
        wettestV = v;
        wettest = i;
      }
    }
    // hari dianggap bolong kalau punya data < 20 dari 24 jam
    bool partial(int i) =>
        _shownGranularity == HistoryGranularity.day &&
        points[i].value != null &&
        (cov[points[i].start.millisecondsSinceEpoch] ?? 24) < 20;

    return BarChart(
      BarChartData(
        maxY: maxY,
        alignment: BarChartAlignment.spaceAround,
        borderData: FlBorderData(show: false),
        gridData: _grid(interval),
        titlesData: _titles(points, interval),
        barTouchData: BarTouchData(
          touchTooltipData: BarTouchTooltipData(
            getTooltipColor: (_) => Colors.white,
            getTooltipItem: (group, groupIndex, rod, rodIndex) {
              final d = points[group.x].start;
              // jam/hari tanpa data bukan 0 mm, tapi memang tidak ada data
              if (points[group.x].value == null) {
                return BarTooltipItem(
                  '${_tooltipTitle(d)}\nTidak ada data',
                  _tooltipStyle,
                );
              }
              final h = cov[d.millisecondsSinceEpoch];
              final note = (_shownGranularity == HistoryGranularity.day && h != null)
                  ? '\nData: $h/24 jam'
                  : '';
              return BarTooltipItem(
                '${_tooltipTitle(d)}\n${_f(rod.toY)} mm$note',
                _tooltipStyle,
              );
            },
          ),
        ),
        barGroups: [
          for (int i = 0; i < points.length; i++)
            BarChartGroupData(
              x: i,
              barRods: [
                BarChartRodData(
                  toY: points[i].value ?? 0,
                  width: _barWidth(points.length),
                  borderRadius: BorderRadius.circular(4),
                  color: i == wettest
                      ? Colors.yellowAccent
                      : partial(i)
                          ? Colors.orangeAccent
                          : (points[i].value ?? 0) > 0.1
                              ? Colors.white
                              : Colors.white.withValues(alpha: 0.25),
                ),
              ],
            ),
        ],
      ),
    );
  }
}