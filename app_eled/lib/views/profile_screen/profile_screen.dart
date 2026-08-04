import 'package:flutter/material.dart';

import '../../models/user_model.dart';
import '../../services/api_service.dart';
import '../login_screen/login_screen.dart';

class ProfileScreen
    extends StatefulWidget {
  const ProfileScreen({
    super.key,
  });

  @override
  State<ProfileScreen>
      createState() =>
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
      child:
          FutureBuilder<UserModel?>(
        future: userFuture,
        builder: (
          context,
          snapshot,
        ) {
          if (snapshot
                  .connectionState ==
              ConnectionState
                  .waiting) {
            return const Center(
              child:
                  CircularProgressIndicator(),
            );
          }

          final user =
              snapshot.data;

          if (user == null) {
            return const Center(
              child: Text(
                'Data user tidak ditemukan',
              ),
            );
          }

          final initial =
              user.firstName
                      .isNotEmpty
                  ? user.firstName[0]
                      .toUpperCase()
                  : user.username
                          .isNotEmpty
                      ? user
                          .username[0]
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

                Text(
                  user.fullName,
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

                _ProfileItem(
                  icon:
                      Icons
                          .person_outline,
                  title:
                      'Username',
                  value:
                      user.username,
                ),

                const Divider(),

                _ProfileItem(
                  icon:
                      Icons
                          .email_outlined,
                  title:
                      'Email',
                  value:
                      user.email,
                ),

                const Divider(),

                _ProfileItem(
                  icon:
                      Icons
                          .badge_outlined,
                  title:
                      'Nama',
                  value:
                      user.fullName,
                ),

                const SizedBox(
                  height: 40,
                ),

                SizedBox(
                  width:
                      double.infinity,
                  height: 50,
                  child:
                      OutlinedButton
                          .icon(
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