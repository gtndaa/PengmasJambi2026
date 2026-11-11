import 'package:flutter/material.dart';

import '../views/dashboard_screen/dashboard_screen.dart';
import '../views/wifi_screen/wifi_screen.dart';
import '../views/data_lahan_screen/data_lahan_screen.dart';
import '../views/notification_screen/notification_screen.dart';
import '../views/profile_screen/profile_screen.dart';
import '../consts/colors.dart';

class NavigationMenu
    extends StatefulWidget {
  const NavigationMenu({
    super.key,
  });

  @override
  State<NavigationMenu> createState() =>
      _NavigationMenuState();
}

class _NavigationMenuState
    extends State<NavigationMenu> {
  int currentIndex = 0;

  final List<Widget> pages = [
    const DashboardScreen(),

    const WifiScreen(),

    const BmkgScreen(),

    const NotificationScreen(),

    const ProfileScreen(),
  ];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppColors.background,
      body: IndexedStack(
        index: currentIndex,
        children: pages,
      ),
      bottomNavigationBar: SafeArea(
        child: Container(
          margin: const EdgeInsets.fromLTRB(16, 0, 16, 12),
          decoration: BoxDecoration(
            color: Colors.white,
            borderRadius: BorderRadius.circular(28),
            boxShadow: [
              BoxShadow(
                color: Colors.black.withValues(alpha: 0.08),
                blurRadius: 20,
                offset: const Offset(0, 8),
              ),
            ],
          ),
          child: ClipRRect(
            borderRadius: BorderRadius.circular(28),
            child: BottomNavigationBar(
              currentIndex: currentIndex,
              type: BottomNavigationBarType.fixed,
              backgroundColor: Colors.transparent,
              elevation: 0,
              selectedItemColor: AppColors.primary,
              unselectedItemColor: Colors.grey.shade400,
              showUnselectedLabels: false,
              onTap: (index) {
                setState(() {
                  currentIndex = index;
                });
              },
              items: const [
                BottomNavigationBarItem(
                  icon: Icon(Icons.dashboard),
                  label: 'Home',
                ),
                BottomNavigationBarItem(
                  icon: Icon(Icons.wifi),
                  label: 'WiFi',
                ),
                BottomNavigationBarItem(
                  icon: Icon(Icons.agriculture),
                  label: 'Data Lahan',
                ),
                BottomNavigationBarItem(
                  icon: Icon(Icons.notifications),
                  label: 'Alert',
                ),
                BottomNavigationBarItem(
                  icon: Icon(Icons.person),
                  label: 'Profile',
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}