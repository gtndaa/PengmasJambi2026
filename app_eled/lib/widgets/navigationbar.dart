import 'package:flutter/material.dart';

import '../views/dashboard_screen/dashboard_screen.dart';
import '../views/wifi_screen/wifi_screen.dart';
import '../views/bmkg_screen/bmkg_screen.dart';
import '../views/notification_screen/notification_screen.dart';
import '../views/profile_screen/profile_screen.dart';

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
  Widget build(
    BuildContext context,
  ) {
    return Scaffold(
      body: IndexedStack(
        index:
            currentIndex,

        children:
            pages,
      ),

      bottomNavigationBar:
          BottomNavigationBar(
        currentIndex:
            currentIndex,

        type:
            BottomNavigationBarType
                .fixed,

        onTap:
            (index) {
          setState(() {
            currentIndex =
                index;
          });
        },

        items: const [
          BottomNavigationBarItem(
            icon:
                Icon(
              Icons.dashboard,
            ),

            label:
                'Home',
          ),

          BottomNavigationBarItem(
            icon:
                Icon(
              Icons.wifi,
            ),

            label:
                'WiFi',
          ),

          BottomNavigationBarItem(
            icon:
                Icon(
              Icons.cloud,
            ),

            label:
                'BMKG',
          ),

          BottomNavigationBarItem(
            icon:
                Icon(
              Icons.notifications,
            ),

            label:
                'Alert',
          ),

          BottomNavigationBarItem(
            icon:
                Icon(
              Icons.person,
            ),

            label:
                'Profile',
          ),
        ],
      ),
    );
  }
}