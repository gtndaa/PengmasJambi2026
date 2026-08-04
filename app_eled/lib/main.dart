import 'package:flutter/material.dart';
// =====================================================
// SCREENS
// =====================================================

import 'views/login_screen/login_screen.dart';

import 'views/dashboard_screen/dashboard_screen.dart';

import '../views/data_lahan_screen/data_lahan_screen.dart';

import 'views/wifi_screen/wifi_screen.dart';

import 'views/notification_screen/notification_screen.dart';

// =====================================================
// MAIN
// =====================================================

void main() {
  WidgetsFlutterBinding.ensureInitialized();

  runApp(
    const MyApp(),
  );
}

// =====================================================
// APP
// =====================================================

class MyApp extends StatelessWidget {
  const MyApp({
    super.key,
  });

  @override
  Widget build(
    BuildContext context,
  ) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,

      title: 'Weather Station',

      theme: ThemeData(
        useMaterial3: true,

        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.blue,
        ),

        scaffoldBackgroundColor:
            const Color(0xFFF8FAFC),

        appBarTheme:
            const AppBarTheme(
          centerTitle: false,
          elevation: 0,
        ),

        inputDecorationTheme:
            const InputDecorationTheme(
          border:
              OutlineInputBorder(),

          enabledBorder:
              OutlineInputBorder(),

          focusedBorder:
              OutlineInputBorder(
            borderSide:
                BorderSide(
              width: 2,
            ),
          ),
        ),

        cardTheme:
            const CardThemeData(
          elevation: 2,

          margin:
              EdgeInsets.zero,
        ),
      ),

      // =================================================
      // HALAMAN AWAL
      // =================================================

      home:
          const LoginScreen(),
    );
  }
}

// =====================================================
// MAIN NAVIGATION
// =====================================================

class MainNavigation
    extends StatefulWidget {
  const MainNavigation({
    super.key,
  });

  @override
  State<MainNavigation>
      createState() =>
          _MainNavigationState();
}

// =====================================================
// MAIN NAVIGATION STATE
// =====================================================

class _MainNavigationState
    extends State<MainNavigation> {
  // ===================================================
  // HALAMAN AKTIF
  // ===================================================

  int currentIndex = 0;

  // ===================================================
  // SEMUA HALAMAN
  // ===================================================

  late final List<Widget> pages;

  // ===================================================
  // INIT
  // ===================================================

  @override
  void initState() {
    super.initState();

    pages = [
      // ===============================================
      // DASHBOARD
      // ===============================================

      const DashboardScreen(),

      // ===============================================
      // DATA LAHAN
      // ===============================================

      const BmkgScreen(),

      // ===============================================
      // WIFI
      // ===============================================

      const WifiScreen(),

      // ===============================================
      // NOTIFICATION / ALERT
      // ===============================================

      const NotificationScreen(),
    ];
  }

  // ===================================================
  // BUILD
  // ===================================================

  @override
  Widget build(
    BuildContext context,
  ) {
    return Scaffold(
      // =================================================
      // INDEXED STACK
      // =================================================

      body:
          IndexedStack(
        index:
            currentIndex,

        children:
            pages,
      ),

      // =================================================
      // BOTTOM NAVIGATION
      // =================================================

      bottomNavigationBar:
          NavigationBar(
        selectedIndex:
            currentIndex,

        onDestinationSelected:
            (int index) {
          setState(() {
            currentIndex =
                index;
          });
        },

        destinations:
            const [
          // =============================================
          // DASHBOARD
          // =============================================

          NavigationDestination(
            icon:
                Icon(
              Icons
                  .dashboard_outlined,
            ),

            selectedIcon:
                Icon(
              Icons
                  .dashboard,
            ),

            label:
                'Dashboard',
          ),

          // =============================================
          // DATA LAHAN
          // =============================================

          NavigationDestination(
            icon:
                Icon(
              Icons
                  .agriculture_outlined,
            ),

            selectedIcon:
                Icon(
              Icons
                  .agriculture,
            ),

            label:
                'Data Lahan',
          ),

          // =============================================
          // WIFI
          // =============================================

          NavigationDestination(
            icon:
                Icon(
              Icons
                  .wifi_outlined,
            ),

            selectedIcon:
                Icon(
              Icons
                  .wifi,
            ),

            label:
                'WiFi',
          ),

          // =============================================
          // ALERT
          // =============================================

          NavigationDestination(
            icon:
                Icon(
              Icons
                  .notifications_outlined,
            ),

            selectedIcon:
                Icon(
              Icons
                  .notifications,
            ),

            label:
                'Alert',
          ),
        ],
      ),
    );
  }
}