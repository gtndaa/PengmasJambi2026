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

  String get fullName =>
      "$firstName $lastName";

  factory UserModel.fromJson(
    Map<String, dynamic> json,
  ) {
    return UserModel(
      // Backend kadang mengirim firstname
      // kadang firstName
      firstName:
          json["firstName"] ??
          json["firstname"] ??
          "",

      lastName:
          json["lastName"] ??
          json["lastname"] ??
          "",

      username:
          json["username"] ?? "",

      email:
          json["email"] ?? "",

      password:
          json["password"] ?? "",
    );
  }

  Map<String, dynamic> toJson() {
    return {
      "firstName": firstName,
      "lastName": lastName,
      "username": username,
      "email": email,
      "password": password,
    };
  }
}