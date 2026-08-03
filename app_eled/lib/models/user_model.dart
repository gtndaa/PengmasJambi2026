class UserModel {
  final String firstName;
  final String lastName;
  final String username;
  final String email;

  // Untuk prototype lokal.
  // Password asli TIDAK disimpan.
  final String passwordHash;
  final String passwordSalt;

  const UserModel({
    required this.firstName,
    required this.lastName,
    required this.username,
    required this.email,
    required this.passwordHash,
    required this.passwordSalt,
  });

  String get fullName {
    return '$firstName $lastName'.trim();
  }

  Map<String, dynamic> toJson() {
    return {
      'firstName': firstName,
      'lastName': lastName,
      'username': username,
      'email': email,
      'passwordHash': passwordHash,
      'passwordSalt': passwordSalt,
    };
  }

  factory UserModel.fromJson(Map<String, dynamic> json) {
    return UserModel(
      firstName: json['firstName'] ?? '',
      lastName: json['lastName'] ?? '',
      username: json['username'] ?? '',
      email: json['email'] ?? '',
      passwordHash: json['passwordHash'] ?? '',
      passwordSalt: json['passwordSalt'] ?? '',
    );
  }
}