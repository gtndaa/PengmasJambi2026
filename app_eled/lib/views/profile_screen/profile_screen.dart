import 'package:flutter/material.dart';

import '../../models/user_model.dart';
import '../../services/api_service.dart';
import '../login_screen/login_screen.dart';
import '../../consts/colors.dart';
import '../../widgets/gradient_header.dart';

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
            padding: const EdgeInsets.all(18),
            child: Column(
              children: [
                // ---------- HEADER ----------
                Container(
                  width: double.infinity,
                  padding: const EdgeInsets.fromLTRB(20, 28, 20, 24),
                  decoration: BoxDecoration(
                    gradient: const LinearGradient(
                      colors: AppColors.violetGradient,
                      begin: Alignment.topCenter,
                      end: Alignment.bottomCenter,
                    ),
                    borderRadius: BorderRadius.circular(28),
                    boxShadow: [
                      BoxShadow(
                        color: AppColors.violetGradient.first
                            .withValues(alpha: 0.30),
                        blurRadius: 22,
                        offset: const Offset(0, 10),
                      ),
                    ],
                  ),
                  child: Column(
                    children: [
                      Container(
                        padding: const EdgeInsets.all(4),
                        decoration: BoxDecoration(
                          shape: BoxShape.circle,
                          color: Colors.white.withValues(alpha: 0.35),
                        ),
                        child: CircleAvatar(
                          radius: 46,
                          backgroundColor: Colors.white,
                          child: Text(
                            initial,
                            style: TextStyle(
                              fontSize: 36,
                              fontWeight: FontWeight.w600,
                              color: AppColors.violetGradient.first,
                            ),
                          ),
                        ),
                      ),
                      const SizedBox(height: 14),
                      Text(
                        displayedName,
                        textAlign: TextAlign.center,
                        style: const TextStyle(
                          color: Colors.white,
                          fontSize: 22,
                          fontWeight: FontWeight.w600,
                        ),
                      ),
                      const SizedBox(height: 8),
                      Container(
                        padding: const EdgeInsets.symmetric(
                          horizontal: 14,
                          vertical: 5,
                        ),
                        decoration: BoxDecoration(
                          color: Colors.white.withValues(alpha: 0.22),
                          borderRadius: BorderRadius.circular(20),
                        ),
                        child: Text(
                          '@${user.username}',
                          style: const TextStyle(
                            color: Colors.white,
                            fontSize: 13,
                          ),
                        ),
                      ),
                    ],
                  ),
                ),

                const SizedBox(height: 18),

                // ---------- INFO AKUN ----------
                Card(
                  child: Padding(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 16,
                      vertical: 8,
                    ),
                    child: Column(
                      children: [
                        _ProfileItem(
                          icon: Icons.person_outline,
                          color: AppColors.primary,
                          title: 'Username',
                          value: user.username.isNotEmpty ? user.username : '-',
                        ),
                        const Divider(height: 1),
                        _ProfileItem(
                          icon: Icons.email_outlined,
                          color: AppColors.primary,
                          title: 'Email',
                          value: user.email.isNotEmpty ? user.email : '-',
                        ),
                        const Divider(height: 1),
                        _ProfileItem(
                          icon: Icons.badge_outlined,
                          color: const Color(0xFF8E7CFF),
                          title: 'Nama',
                          value: displayedName,
                        ),
                      ],
                    ),
                  ),
                ),

                const SizedBox(height: 24),

                // ---------- LOGOUT ----------
                SizedBox(
                  width: double.infinity,
                  height: 52,
                  child: ElevatedButton.icon(
                    onPressed: logout,
                    icon: const Icon(Icons.logout),
                    label: const Text('LOGOUT'),
                    style: ElevatedButton.styleFrom(
                      elevation: 0,
                      backgroundColor: const Color(0xFFFFEBEE),
                      foregroundColor: const Color(0xFFD32F2F),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(16),
                      ),
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

class _ProfileItem extends StatelessWidget {
  final IconData icon;
  final Color color;
  final String title;
  final String value;

  const _ProfileItem({
    required this.icon,
    required this.color,
    required this.title,
    required this.value,
  });

  @override
  Widget build(BuildContext context) {
    return ListTile(
      contentPadding: EdgeInsets.zero,
      leading: IconBadge(icon: icon, color: color),
      title: Text(
        title,
        style: const TextStyle(color: AppColors.inkSoft, fontSize: 12),
      ),
      subtitle: Text(
        value,
        style: const TextStyle(
          fontSize: 15,
          fontWeight: FontWeight.w600,
          color: AppColors.ink,
        ),
      ),
    );
  }
}