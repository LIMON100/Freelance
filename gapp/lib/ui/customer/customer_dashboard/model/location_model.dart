class LocationModel {
  LocationModel({
      this.latitude, 
      this.longitude, 
      this.address, 
      this.id,});

  LocationModel.fromJson(dynamic json) {
    latitude = json['latitude'];
    longitude = json['longitude'];
    address = json['address'];
    id = json['id'];
  }
  double? latitude;
  double? longitude;
  String? address;
  String? id;

  Map<String, dynamic> toJson() {
    final map = <String, dynamic>{};
    map['latitude'] = latitude;
    map['longitude'] = longitude;
    map['address'] = address;
    map['id'] = id;
    return map;
  }

}