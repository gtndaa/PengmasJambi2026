class Province {
  final String code;
  final String name;

  Province({
    required this.code,
    required this.name,
  });

  factory Province.fromJson(
    Map<String, dynamic> json,
  ) {
    return Province(
      code: json['code'].toString(),
      name: json['name'].toString(),
    );
  }
}


class Regency {
  final String code;
  final String name;

  Regency({
    required this.code,
    required this.name,
  });

  factory Regency.fromJson(
    Map<String, dynamic> json,
  ) {
    return Regency(
      code: json['code'].toString(),
      name: json['name'].toString(),
    );
  }
}


class District {
  final String code;
  final String name;

  District({
    required this.code,
    required this.name,
  });

  factory District.fromJson(
    Map<String, dynamic> json,
  ) {
    return District(
      code: json['code'].toString(),
      name: json['name'].toString(),
    );
  }
}


class Village {
  final String code;
  final String name;
  final String adm4;

  Village({
    required this.code,
    required this.name,
    required this.adm4,
  });

  factory Village.fromJson(
    Map<String, dynamic> json,
  ) {
    return Village(
      code: json['code'].toString(),
      name: json['name'].toString(),
      adm4:
          json['adm4']?.toString() ??
          json['code']?.toString() ??
          '',
    );
  }
}