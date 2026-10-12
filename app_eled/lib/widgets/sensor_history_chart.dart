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

class _DayValue {
  final DateTime day;
  final double? value;
  const _DayValue(this.day, this.value);
}

class _SensorHistoryChartState extends State<SensorHistoryChart> {
  final _service = SensorHistoryService();

  int _days = 7;
  List<SensorDataModel> _readings = [];
  bool _loading = true;
  String? _error;

  static const _labelStyle = TextStyle(color: Colors.white70, fontSize: 10);

  @override
  void initState() {
    super.initState();
    _load();
  }

  Future<void> _load() async {
    setState(() {
      _loading = true;
      _error = null;
    });
    try {
      final result = await _service.getHistory(_days);
      if (!mounted) return;
      setState(() {
        _readings = result;
        _loading = false;
      });
    } catch (e) {
      if (!mounted) return;
      setState(() {
        _error = e.toString();
        _loading = false;
      });
    }
  }

  void _setRange(int days) {
    if (days == _days) return;
    setState(() => _days = days);
    _load();
  }

  // Gabungkan bacaan sensor menjadi 1 nilai per hari
  List<_DayValue> _aggregate() {
    final now = DateTime.now();
    final start = DateTime(now.year, now.month, now.day)
        .subtract(Duration(days: _days - 1));

    final buckets = <int, List<double>>{};
    for (final r in _readings) {
      final day = DateTime(r.datetime.year, r.datetime.month, r.datetime.day);
      final i = day.difference(start).inDays;
      if (i < 0 || i >= _days) continue;
      buckets.putIfAbsent(i, () => []).add(widget.metric.valueOf(r));
    }

    return List.generate(_days, (i) {
      final values = buckets[i];
      double? v;
      if (values != null && values.isNotEmpty) {
        final sum = values.fold<double>(0, (a, b) => a + b);
        v = widget.metric.isSum ? sum : sum / values.length;
      } else if (widget.metric.isSum) {
        v = 0;
      }
      return _DayValue(start.add(Duration(days: i)), v);
    });
  }

  String _f(double v) {
    final a = v.abs();
    if (a >= 1000) return '${(v / 1000).toStringAsFixed(1)}k';
    if (a >= 100) return v.toStringAsFixed(0);
    return v.toStringAsFixed(1);
  }

  @override
  Widget build(BuildContext context) {
    final m = widget.metric;
    final points = _aggregate();
    final values = [
      for (final p in points)
        if (p.value != null) p.value!
    ];
    final empty = _readings.isEmpty || values.isEmpty;

    final total = values.fold<double>(0, (a, b) => a + b);
    final maxV = empty ? 0.0 : values.reduce((a, b) => a > b ? a : b);
    final minV = empty ? 0.0 : values.reduce((a, b) => a < b ? a : b);
    final avg = empty ? 0.0 : total / values.length;
    final rainyDays = values.where((v) => v > 0.1).length;

    final stats = m.isSum
        ? [
            ['Total', '${_f(total)} ${m.unit}'],
            ['Hari hujan', '$rainyDays hari'],
            ['Tertinggi', '${_f(maxV)} ${m.unit}'],
          ]
        : [
            ['Rata-rata', '${_f(avg)} ${m.unit}'],
            ['Tertinggi', '${_f(maxV)} ${m.unit}'],
            ['Terendah', '${_f(minV)} ${m.unit}'],
          ];

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
                  m.isSum ? 'Curah Hujan Harian' : m.label,
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
              m.isSum ? 'Total per hari' : 'Rata-rata per hari',
              style: const TextStyle(color: Colors.white70, fontSize: 11),
            ),
          ),
          const SizedBox(height: 10),
          Row(
            children: [
              _rangeChip(7, '7 hari'),
              const SizedBox(width: 6),
              _rangeChip(30, '30 hari'),
            ],
          ),
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
          const SizedBox(height: 18),
          SizedBox(height: 190, child: _body(points, empty)),
        ],
      ),
    );
  }

  Widget _rangeChip(int days, String label) {
    final selected = _days == days;
    return GestureDetector(
      onTap: () => _setRange(days),
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
    );
  }

  Widget _body(List<_DayValue> points, bool empty) {
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

  FlTitlesData _titles(List<_DayValue> points, double interval) {
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
            final step = _days <= 7 ? 1 : 5;
            if (i < 0 || i >= points.length || i % step != 0) {
              return const SizedBox.shrink();
            }
            final d = points[i].day;
            return SideTitleWidget(
              meta: meta,
              child: Text('${d.day}/${d.month}', style: _labelStyle),
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

  Widget _line(List<_DayValue> points) {
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
        maxX: (_days - 1).toDouble(),
        minY: minY,
        maxY: maxY,
        borderData: FlBorderData(show: false),
        gridData: _grid(interval),
        titlesData: _titles(points, interval),
        lineTouchData: LineTouchData(
          touchTooltipData: LineTouchTooltipData(
            getTooltipColor: (_) => Colors.white,
            getTooltipItems: (touched) => touched.map((s) {
              final d = points[s.x.toInt()].day;
              return LineTooltipItem(
                '${d.day}/${d.month}\n${_f(s.y)} ${widget.metric.unit}',
                _tooltipStyle,
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
            dotData: FlDotData(show: _days <= 7),
            belowBarData: BarAreaData(
              show: true,
              color: Colors.white.withValues(alpha: 0.15),
            ),
          ),
        ],
      ),
    );
  }

  Widget _bar(List<_DayValue> points) {
    final maxMm = points.fold<double>(0, (m, p) => (p.value ?? 0) > m ? p.value! : m);
    final maxY = ((maxMm / 10).ceil() * 10).clamp(10, 1000).toDouble();
    final interval = maxY / 2;

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
              final d = points[group.x].day;
              return BarTooltipItem(
                '${d.day}/${d.month}\n${_f(rod.toY)} mm',
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
                  width: _days <= 7 ? 14 : 5,
                  borderRadius: BorderRadius.circular(4),
                  color: (points[i].value ?? 0) > 0.1
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