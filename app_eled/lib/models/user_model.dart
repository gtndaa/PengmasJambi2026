class UserModel {
  final String firstName;
  final String lastName;
  final String username;
  final String email;
  final String password;

  const UserModel({
    required this.firstName,
    required this.lastName,
    required this.username,
    required this.email,
    required this.password,
  });

  String get fullName {
    final name =
        '$firstName $lastName'.trim();

    // Jika firstName dan lastName kosong,
    // gunakan username sebagai cadangan.
    if (name.isEmpty) {
      return username;
    }

    return name;
  }

  factory UserModel.fromJson(
    Map<String, dynamic> json,
  ) {
    // Menangani kemungkinan data user
    // berada di dalam key "user".
    final dynamic nestedUser =
        json['user'];

    final Map<String, dynamic>
        userJson =
        nestedUser
                is Map<String, dynamic>
            ? nestedUser
            : json;

    String readValue(
      List<String> keys,
    ) {
      for (final key in keys) {
        final value =
            userJson[key];

        if (value != null &&
            value
                .toString()
                .trim()
                .isNotEmpty) {
          return value
              .toString()
              .trim();
        }
      }

      return '';
    }

    // Menerima berbagai kemungkinan
    // nama key dari backend.
    final firstName =
        readValue([
      'firstName',
      'firstname',
      'first_name',
      'namaDepan',
      'nama_depan',
    ]);

    final lastName =
        readValue([
      'lastName',
      'lastname',
      'last_name',
      'namaBelakang',
      'nama_belakang',
    ]);

    final username =
        readValue([
      'username',
      'userName',
      'user_name',
    ]);

    final email =
        readValue([
      'email',
      'emailAddress',
      'email_address',
    ]);

    final password =
        readValue([
      'password',
    ]);

    // Jika backend hanya mengirim
    // "name" atau "nama",
    // pecah menjadi firstName dan lastName.
    final fullNameFromApi =
        readValue([
      'fullName',
      'fullname',
      'full_name',
      'name',
      'nama',
    ]);

    String finalFirstName =
        firstName;

    String finalLastName =
        lastName;

    if (finalFirstName.isEmpty &&
        finalLastName.isEmpty &&
        fullNameFromApi
            .isNotEmpty) {
      final nameParts =
          fullNameFromApi
              .split(
            RegExp(r'\s+'),
          );

      finalFirstName =
          nameParts.first;

      if (nameParts.length > 1) {
        finalLastName =
            nameParts
                .skip(1)
                .join(' ');
      }
    }

    return UserModel(
      firstName:
          finalFirstName,

      lastName:
          finalLastName,

      username:
          username,

      email:
          email,

      password:
          password,
    );
  }

  Map<String, dynamic>
      toJson() {
    return {
      'firstName':
          firstName,

      'lastName':
          lastName,

      'username':
          username,

      'email':
          email,

      'password':
          password,
    };
  }
}