import 'package:flutter/material.dart';

import '../../services/api_service.dart';
import '../../widgets/navigationbar.dart';
import '../login_screen/login_screen.dart';

class SplashScreen
    extends StatefulWidget {
  const SplashScreen({
    super.key,
  });

  @override
  State<SplashScreen>
      createState() =>
          _SplashScreenState();
}

class _SplashScreenState
    extends State<SplashScreen> {
  final ApiService apiService =
      ApiService();

  @override
  void initState() {
    super.initState();

    _checkSession();
  }

  Future<void>
      _checkSession() async {
    await Future.delayed(
      const Duration(
        seconds: 2,
      ),
    );

    final loggedIn =
        await apiService
            .isLoggedIn();

    if (!mounted) {
      return;
    }

    Navigator.pushReplacement(
      context,
      MaterialPageRoute(
        builder: (_) =>
            loggedIn
                ? const NavigationMenu()
                : const LoginScreen(),
      ),
    );
  }

  @override
  Widget build(
    BuildContext context,
  ) {
    return const Scaffold(
      body: Center(
        child: Column(
          mainAxisAlignment:
              MainAxisAlignment
                  .center,
          children: [
            Icon(
              Icons.cloud,
              size: 100,
              color: Colors.blue,
            ),

            SizedBox(
              height: 20,
            ),

            Text(
              'Weather Monitoring',
              style: TextStyle(
                fontSize: 28,
                fontWeight:
                    FontWeight.bold,
              ),
            ),

            SizedBox(
              height: 10,
            ),

            Text(
              'ESP32 • AWS • BMKG',
              style: TextStyle(
                color:
                    Colors.grey,
              ),
            ),

            SizedBox(
              height: 40,
            ),

            CircularProgressIndicator(),
          ],
        ),
      ),
    );
  }
}