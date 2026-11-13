import 'dart:math' as math;

import 'package:flutter/material.dart';

import '../consts/colors.dart';

/// Avatar maskot awan yang digambar langsung (tanpa file gambar).
///
/// Tema cuaca, arah lirikan, dan ekspresi dipilih dari [seed] (mis. username),
/// jadi tiap akun punya maskot sendiri tapi selalu sama untuk akun yang sama.
class ChibiAvatar extends StatelessWidget {
  final String seed;
  final double size;

  const ChibiAvatar({
    super.key,
    required this.seed,
    this.size = 48,
  });

  @override
  Widget build(BuildContext context) {
    return SizedBox.square(
      dimension: size,
      child: CustomPaint(painter: _CloudPainter(_Look.fromSeed(seed))),
    );
  }
}

// ---------------------------------------------------------------------------
// Tema & variasi
// ---------------------------------------------------------------------------

enum _Theme { sunny, sky, rain, storm }

class _Palette {
  final List<Color> background;
  final Alignment bgBegin;
  final Alignment bgEnd;
  final Color cloudLight; // bagian cloud yang kena cahaya
  final Color cloudShade; // bagian cloud yang teduh
  final Color shadow;

  const _Palette(
    this.background,
    this.cloudLight,
    this.cloudShade,
    this.shadow, {
    this.bgBegin = Alignment.topLeft,
    this.bgEnd = Alignment.bottomRight,
  });
}

const _palettes = <_Theme, _Palette>{
  _Theme.sunny: _Palette(
    [Color(0xFFFFE27A), Color(0xFFBFF0C9), Color(0xFF7FD1C7)],
    Color(0xFFF6FFF4),
    Color(0xFFC6E9DA),
    Color(0xFF4F9D8F),
  ),
  _Theme.sky: _Palette(
    [Color(0xFF3FA9F5), Color(0xFF7CC4FA), Color(0xFFC3E2FF)],
    Color(0xFFF2F9FF),
    Color(0xFFA9D2FA),
    AppColors.primary,
    bgBegin: Alignment.topCenter,
    bgEnd: Alignment.bottomCenter,
  ),
  _Theme.rain: _Palette(
    [Color(0xFF4F9D8F), Color(0xFF86C2B4), Color(0xFFBFE3D9)],
    Color(0xFFF1FBF8),
    Color(0xFFA6D3C8),
    Color(0xFF2F6E63),
    bgBegin: Alignment.topCenter,
    bgEnd: Alignment.bottomCenter,
  ),
  _Theme.storm: _Palette(
    [Color(0xFF6B6FD6), Color(0xFF8E7CFF), Color(0xFFB9B4F5)],
    Color(0xFFF4F3FF),
    Color(0xFFBDB8F5),
    Color(0xFF3B2F8C),
  ),
};

class _Look {
  final _Theme theme;
  final Offset gaze; // arah lirikan (satuan, dikali ukuran nanti)
  final bool sleepy; // kelopak atas setengah turun (mata ngantuk/curiga)
  final bool flip; // aksesori di kiri / kanan

  const _Look(this.theme, this.gaze, this.sleepy, this.flip);

  static const _gazes = [
    Offset(0, 0),
    Offset(0.7, -0.4),
    Offset(-0.7, -0.4),
    Offset(0.8, 0.3),
    Offset(-0.8, 0.3),
    Offset(0, -0.8),
  ];

  factory _Look.fromSeed(String seed) {
    var h = 7;
    for (final c in seed.toLowerCase().codeUnits) {
      h = (h * 31 + c) & 0x7fffffff;
    }
    return _Look(
      _Theme.values[h % _Theme.values.length],
      _gazes[(h ~/ 7) % _gazes.length],
      (h ~/ 41) % 4 == 0, // sekitar 1 dari 4 akun matanya ngantuk
      (h ~/ 13).isOdd,
    );
  }

  @override
  bool operator ==(Object other) =>
      other is _Look &&
      other.theme == theme &&
      other.gaze == gaze &&
      other.sleepy == sleepy &&
      other.flip == flip;

  @override
  int get hashCode => Object.hash(theme, gaze, sleepy, flip);
}

// ---------------------------------------------------------------------------
// Painter
// ---------------------------------------------------------------------------


class _CloudPainter extends CustomPainter {
  final _Look look;
  _CloudPainter(this.look);

  @override
  void paint(Canvas canvas, Size size) {
    final w = size.width;
    final pal = _palettes[look.theme]!;
    // posisi relatif (0..1) -> piksel; benda langit bisa dicerminkan
    Offset p(double x, double y) => Offset(x * w, y * w);
    Offset pa(double x, double y) => p(look.flip ? 1 - x : x, y);
    final rect = Offset.zero & size;

    canvas.save();
    canvas.clipPath(Path()..addOval(rect));

    // --- latar ---
    canvas.drawRect(
      rect,
      Paint()
        ..shader = LinearGradient(
          colors: pal.background,
          begin: pal.bgBegin,
          end: pal.bgEnd,
        ).createShader(rect),
    );

    // --- benda langit di belakang awan ---
    switch (look.theme) {
      case _Theme.sunny:
        _sun(canvas, pa(0.26, 0.28), w);
        break;
      case _Theme.sky:
        _rainbow(canvas, pa(0.76, 0.52), w);
        break;
      case _Theme.rain:
        _drops(canvas, [pa(0.18, 0.20), pa(0.32, 0.08), pa(0.84, 0.22)], w);
        break;
      case _Theme.storm:
        _bolt(canvas, pa(0.76, 0.14), w);
        break;
    }

    // --- awan besar, mengisi hampir seluruh bingkai ---
    final cloudPath = [
      Path()..addOval(Rect.fromCircle(center: p(0.54, 0.52), radius: w * 0.27)),
      Path()..addOval(Rect.fromCircle(center: p(0.18, 0.70), radius: w * 0.20)),
      Path()..addOval(Rect.fromCircle(center: p(0.87, 0.67), radius: w * 0.20)),
      Path()..addRect(Rect.fromLTRB(-w * 0.1, w * 0.70, w * 1.1, w * 1.2)),
    ].reduce((a, b) => Path.combine(PathOperation.union, a, b));

    canvas.drawShadow(cloudPath, pal.shadow.withValues(alpha: 0.7), w * 0.04, false);

    // badan awan: lembut seperti kaca buram
    canvas.drawPath(
      cloudPath,
      Paint()
        ..shader = RadialGradient(
          center: const Alignment(-0.25, -0.45),
          radius: 1.0,
          colors: [Colors.white, pal.cloudLight, pal.cloudShade],
          stops: const [0.0, 0.40, 1.0],
        ).createShader(rect),
    );

    canvas.save();
    canvas.clipPath(cloudPath);
    // warna latar yang "tembus" ke bagian bawah awan
    final bottom = Rect.fromLTRB(0, w * 0.78, w, w);
    canvas.drawRect(
      bottom,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            pal.background.last.withValues(alpha: 0.0),
            pal.background.last.withValues(alpha: 0.35),
          ],
        ).createShader(bottom),
    );
    // garis cahaya halus menyusuri tepi awan
    canvas.drawPath(
      cloudPath,
      Paint()
        ..color = Colors.white.withValues(alpha: 0.85)
        ..style = PaintingStyle.stroke
        ..strokeWidth = w * 0.035
        ..maskFilter = MaskFilter.blur(BlurStyle.normal, w * 0.02),
    );
    canvas.restore();

    // kilau glossy di puncak awan
    canvas.drawOval(
      Rect.fromCenter(center: p(0.44, 0.34), width: w * 0.24, height: w * 0.10),
      Paint()
        ..color = Colors.white.withValues(alpha: 0.8)
        ..maskFilter = MaskFilter.blur(BlurStyle.normal, w * 0.025),
    );

    // --- mata ---
    _eyes(canvas, w, p, pal);

    canvas.restore();
  }

  void _eyes(
    Canvas canvas,
    double w,
    Offset Function(double, double) p,
    _Palette pal,
  ) {
    final centers = [p(0.36, 0.71), p(0.66, 0.71)];
    final pupilShift = look.gaze * (w * 0.035);

    for (var i = 0; i < centers.length; i++) {
      final e = centers[i];
      final white = Rect.fromCenter(center: e, width: w * 0.25, height: w * 0.23);

      // bayangan lembut di bawah bola mata
      canvas.drawOval(
        white.shift(Offset(0, w * 0.018)),
        Paint()
          ..color = pal.shadow.withValues(alpha: 0.30)
          ..maskFilter = MaskFilter.blur(BlurStyle.normal, w * 0.015),
      );

      // bola mata: putih kebiruan, mengilap
      canvas.drawOval(
        white,
        Paint()
          ..shader = const RadialGradient(
            center: Alignment(-0.2, -0.35),
            radius: 0.9,
            colors: [Colors.white, Color(0xFFF3FAFF), Color(0xFFCFE6F7)],
            stops: [0.0, 0.6, 1.0],
          ).createShader(white),
      );

      canvas.drawOval(
        white,
        Paint()
          ..color = pal.shadow.withValues(alpha: 0.18)
          ..style = PaintingStyle.stroke
          ..strokeWidth = w * 0.008,
      );

      canvas.save();
      canvas.clipPath(Path()..addOval(white));

      // pupil besar, ikut arah lirikan
      final pupil = e + pupilShift;
      final pr = w * 0.08;
      canvas.drawCircle(
        pupil,
        pr,
        Paint()
          ..shader = const RadialGradient(
            center: Alignment(-0.3, -0.3),
            colors: [Color(0xFF2A2F3A), Color(0xFF05070B)],
          ).createShader(Rect.fromCircle(center: pupil, radius: pr)),
      );

      // kilau kecil di pupil
      if (!look.sleepy) {
        canvas.drawCircle(
          pupil + Offset(-pr * 0.35, -pr * 0.40),
          pr * 0.22,
          Paint()..color = Colors.white.withValues(alpha: 0.95),
        );
      }

      // kelopak atas setengah turun (miring ke arah luar)
      if (look.sleepy) {
        final outer = i == 0 ? -1.0 : 1.0;
        final lidY = e.dy - w * 0.005;
        final tilt = -w * 0.015 * outer;
        final lid = Path()
          ..moveTo(white.left - w * 0.02, lidY + tilt)
          ..lineTo(white.right + w * 0.02, lidY - tilt)
          ..lineTo(white.right + w * 0.02, white.top - w * 0.02)
          ..lineTo(white.left - w * 0.02, white.top - w * 0.02)
          ..close();
        canvas.drawPath(
          lid,
          Paint()
            ..shader = LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: [pal.cloudLight, pal.cloudShade],
            ).createShader(white),
        );
        canvas.drawLine(
          Offset(white.left, lidY + tilt * 0.9),
          Offset(white.right, lidY - tilt * 0.9),
          Paint()
            ..color = pal.shadow.withValues(alpha: 0.35)
            ..strokeWidth = w * 0.012,
        );
      }
      canvas.restore();
    }
  }

  // ---------------- benda langit ----------------

  void _sun(Canvas canvas, Offset c, double w) {
    canvas.drawCircle(
      c,
      w * 0.34,
      Paint()
        ..shader = RadialGradient(colors: [
          const Color(0xFFFFF3A6).withValues(alpha: 0.9),
          const Color(0xFFFFE27A).withValues(alpha: 0.0),
        ]).createShader(Rect.fromCircle(center: c, radius: w * 0.34)),
    );
    final r = w * 0.19;
    canvas.drawCircle(
      c,
      r,
      Paint()
        ..shader = const RadialGradient(
          center: Alignment(-0.3, -0.3),
          colors: [Color(0xFFFFF59D), Color(0xFFFFD54F), Color(0xFFFFB238)],
          stops: [0.0, 0.6, 1.0],
        ).createShader(Rect.fromCircle(center: c, radius: r)),
    );
  }

  void _rainbow(Canvas canvas, Offset c, double w) {
    const colors = [
      Color(0xFFFF8FA3),
      Color(0xFFFFC94D),
      Color(0xFF8EE0B5),
      Color(0xFF8E7CFF),
    ];
    for (var i = 0; i < colors.length; i++) {
      final r = w * (0.38 - i * 0.045);
      canvas.drawArc(
        Rect.fromCircle(center: c, radius: r),
        math.pi,
        math.pi,
        false,
        Paint()
          ..color = colors[i].withValues(alpha: 0.75)
          ..style = PaintingStyle.stroke
          ..strokeWidth = w * 0.045,
      );
    }
  }

  void _drops(Canvas canvas, List<Offset> centers, double w) {
    for (final c in centers) {
      final s = w * 0.045;
      final drop = Path()
        ..moveTo(c.dx, c.dy - s * 1.4)
        ..quadraticBezierTo(c.dx + s * 1.1, c.dy, c.dx, c.dy + s * 0.8)
        ..quadraticBezierTo(c.dx - s * 1.1, c.dy, c.dx, c.dy - s * 1.4)
        ..close();
      canvas.drawPath(
        drop,
        Paint()
          ..shader = const LinearGradient(
            colors: [Color(0xFFFFFFFF), Color(0xFFBFE6FF)],
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ).createShader(drop.getBounds()),
      );
    }
  }

  void _bolt(Canvas canvas, Offset c, double w) {
    final s = w * 0.13;
    final bolt = Path()
      ..moveTo(c.dx + s * 0.15, c.dy - s * 0.2)
      ..lineTo(c.dx - s * 0.45, c.dy + s * 0.75)
      ..lineTo(c.dx - s * 0.02, c.dy + s * 0.75)
      ..lineTo(c.dx - s * 0.25, c.dy + s * 1.5)
      ..lineTo(c.dx + s * 0.5, c.dy + s * 0.45)
      ..lineTo(c.dx + s * 0.05, c.dy + s * 0.45)
      ..lineTo(c.dx + s * 0.35, c.dy - s * 0.2)
      ..close();
    canvas.drawPath(
      bolt,
      Paint()
        ..color = const Color(0xFFFFE27A).withValues(alpha: 0.5)
        ..maskFilter = MaskFilter.blur(BlurStyle.normal, w * 0.025),
    );
    canvas.drawPath(bolt, Paint()..color = const Color(0xFFFFD54F));
  }

  @override
  bool shouldRepaint(_CloudPainter old) => old.look != look;
}
