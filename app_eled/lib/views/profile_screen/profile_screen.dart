import 'package:flutter/material.dart';

import '../../models/user_model.dart';
import '../../services/api_service.dart';
import '../login_screen/login_screen.dart';

class ProfileScreen extends StatefulWidget {
  const ProfileScreen({
    super.key,
  });

  @override
  State<ProfileScreen> createState() =>
      _ProfileScreenState();
}

class _ProfileScreenState
    extends State<ProfileScreen> {
  final ApiService apiService =
      ApiService();

  late Future<UserModel?>
      userFuture;

  @override
  void initState() {
    super.initState();

    userFuture =
        apiService.getCurrentUser();
  }

  // =====================================================
  // LOGOUT
  // =====================================================

  Future<void> logout() async {
    await apiService.logout();

    if (!mounted) {
      return;
    }

    Navigator.pushAndRemoveUntil(
      context,
      MaterialPageRoute(
        builder: (_) =>
            const LoginScreen(),
      ),
      (route) => false,
    );
  }

  @override
  Widget build(
    BuildContext context,
  ) {
    return SafeArea(
      child: FutureBuilder<UserModel?>(
        future: userFuture,
        builder: (
          context,
          snapshot,
        ) {
          // =================================================
          // SAAT DATA MASIH DIMUAT
          // =================================================

          if (snapshot.connectionState ==
              ConnectionState.waiting) {
            return const Center(
              child:
                  CircularProgressIndicator(),
            );
          }

          // =================================================
          // AMBIL DATA USER
          // =================================================

          final UserModel? user =
              snapshot.data;

          if (user == null) {
            return const Center(
              child: Text(
                'Data user tidak ditemukan',
              ),
            );
          }

          // =================================================
          // NAMA UNTUK DITAMPILKAN
          // =================================================

          final String displayedName =
              user.fullName.isNotEmpty
                  ? user.fullName
                  : user.username.isNotEmpty
                      ? user.username
                      : 'Nama belum tersedia';

          // =================================================
          // HURUF AWAL UNTUK AVATAR
          // =================================================

          final String initial =
              user.firstName.isNotEmpty
                  ? user.firstName[0]
                      .toUpperCase()
                  : user.username.isNotEmpty
                      ? user.username[0]
                          .toUpperCase()
                      : '?';

          return SingleChildScrollView(
            padding:
                const EdgeInsets.all(
              24,
            ),
            child: Column(
              children: [
                const SizedBox(
                  height: 20,
                ),

                // =============================================
                // FOTO PROFIL
                // =============================================

                CircleAvatar(
                  radius: 48,
                  child: Text(
                    initial,
                    style:
                        const TextStyle(
                      fontSize: 36,
                      fontWeight:
                          FontWeight.bold,
                    ),
                  ),
                ),

                const SizedBox(
                  height: 16,
                ),

                // =============================================
                // NAMA UTAMA
                // =============================================

                Text(
                  displayedName,
                  textAlign:
                      TextAlign.center,
                  style:
                      const TextStyle(
                    fontSize: 24,
                    fontWeight:
                        FontWeight.bold,
                  ),
                ),

                const SizedBox(
                  height: 4,
                ),

                // =============================================
                // USERNAME
                // =============================================

                Text(
                  '@${user.username}',
                  style:
                      const TextStyle(
                    color:
                        Colors.grey,
                  ),
                ),

                const SizedBox(
                  height: 32,
                ),

                // =============================================
                // USERNAME
                // =============================================

                _ProfileItem(
                  icon:
                      Icons.person_outline,
                  title:
                      'Username',
                  value:
                      user.username.isNotEmpty
                          ? user.username
                          : '-',
                ),

                const Divider(),

                // =============================================
                // EMAIL
                // =============================================

                _ProfileItem(
                  icon:
                      Icons.email_outlined,
                  title:
                      'Email',
                  value:
                      user.email.isNotEmpty
                          ? user.email
                          : '-',
                ),

                const Divider(),

                // =============================================
                // NAMA
                // =============================================

                _ProfileItem(
                  icon:
                      Icons.badge_outlined,
                  title:
                      'Nama',
                  value:
                      displayedName,
                ),

                const SizedBox(
                  height: 40,
                ),

                // =============================================
                // TOMBOL LOGOUT
                // =============================================

                SizedBox(
                  width:
                      double.infinity,
                  height:
                      50,
                  child:
                      OutlinedButton.icon(
                    onPressed:
                        logout,
                    icon:
                        const Icon(
                      Icons.logout,
                    ),
                    label:
                        const Text(
                      'LOGOUT',
                    ),
                  ),
                ),
              ],
            ),
          );
        },
      ),
    );
  }
}

// =====================================================
// WIDGET ITEM PROFIL
// =====================================================

class _ProfileItem
    extends StatelessWidget {
  final IconData icon;

  final String title;

  final String value;

  const _ProfileItem({
    required this.icon,
    required this.title,
    required this.value,
  });

  @override
  Widget build(
    BuildContext context,
  ) {
    return ListTile(
      contentPadding:
          EdgeInsets.zero,

      leading:
          CircleAvatar(
        child:
            Icon(icon),
      ),

      title:
          Text(
        title,
        style:
            const TextStyle(
          color:
              Colors.grey,
          fontSize:
              13,
        ),
      ),

      subtitle:
          Text(
        value,
        style:
            const TextStyle(
          fontSize:
              16,
          fontWeight:
              FontWeight.w600,
        ),
      ),
    );
  }
}