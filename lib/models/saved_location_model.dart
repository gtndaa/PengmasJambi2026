class SavedLocation {
  final String provinceCode;
  final String provinceName;

  final String regencyCode;
  final String regencyName;

  final String districtCode;
  final String districtName;

  final String villageCode;
  final String villageName;

  final String adm4;

  SavedLocation({
    required this.provinceCode,
    required this.provinceName,
    required this.regencyCode,
    required this.regencyName,
    required this.districtCode,
    required this.districtName,
    required this.villageCode,
    required this.villageName,
    required this.adm4,
  });

  Map<String, dynamic> toJson() {
    return {
      'provinceCode': provinceCode,
      'provinceName': provinceName,
      'regencyCode': regencyCode,
      'regencyName': regencyName,
      'districtCode': districtCode,
      'districtName': districtName,
      'villageCode': villageCode,
      'villageName': villageName,
      'adm4': adm4,
    };
  }

  factory SavedLocation.fromJson(
    Map<String, dynamic> json,
  ) {
    return SavedLocation(
      provinceCode: json['provinceCode'],
      provinceName: json['provinceName'],
      regencyCode: json['regencyCode'],
      regencyName: json['regencyName'],
      districtCode: json['districtCode'],
      districtName: json['districtName'],
      villageCode: json['villageCode'],
      villageName: json['villageName'],
      adm4: json['adm4'],
    );
  }
}