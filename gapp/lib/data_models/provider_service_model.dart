import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/data_models/user_model.dart';

class ProviderServiceModel {
  String userId = '';
  String serviceId = '';
  String description = '';
  String? serviceName;
  String serviceType = '';
  String? cancelationPolicy;
  String? address;
  String? distance;
  int? serviceDeposit;
  int? servicePrice;
  int? bufferTime;
  dynamic workingHours;
  dynamic appointmentDuration;
  int? maximumBookingPerDay;
  LatLng? location;
  List<String>? serviceImages;
  UserModel? provider;

  ProviderServiceModel(
      {required this.userId,
      required this.serviceId,
      required this.description,
        this.serviceName,
      required this.serviceType,
        this.serviceDeposit,
      this.servicePrice,
      this.location,
      this.provider,
      this.bufferTime,
      this.maximumBookingPerDay,
      this.cancelationPolicy,
      this.address,
      this.workingHours,
      this.appointmentDuration,
      this.serviceImages});

  factory ProviderServiceModel.fromJson(Map<String, dynamic> json) {
    return ProviderServiceModel(
      userId: json["userId"]??"",
      serviceId: json["serviceId"]??"",
      description: json["description"]??"",
      serviceName: json["serviceName"],
      serviceType: json["serviceType"]??"",
      cancelationPolicy: json["cancellationPolicy"]??"",
      address: json["address"]??"",
      workingHours: json["workingHours"]??"0",
      appointmentDuration: json["appointmentDuration"]??0,
      serviceDeposit: int.parse(json["serviceDeposit"].toString()),
      servicePrice: int.parse(json["servicePrice"].toString()),
      bufferTime: json["bufferTime"]==null?0:int.parse(json["bufferTime"].toString()),
      maximumBookingPerDay:  json["maximumBookingPerDay"]==null?0:int.parse(json["maximumBookingPerDay"].toString()),
      serviceImages: json['serviceImages'] != null
          ? List<String>.from(json['serviceImages'])
          : null,
      location: json['location'] != null
          ? LatLng(
              json['location']['latitude'] ?? 0.0,
              json['location']['longitude'] ?? 0.0,
            )
          : null,
    );
  }

  Map<String, dynamic> toJson() {
    final data = <String, dynamic>{
      'userId': userId,
      'serviceId': serviceId,
      'description': description,
      'serviceName': serviceName,
      'serviceType': serviceType,
      'servicePrice': servicePrice,
      'serviceDeposit': serviceDeposit,
      'bufferTime': bufferTime,
      'maximumBookingPerDay': maximumBookingPerDay,
      'cancellationPolicy': cancelationPolicy,
      'address': address,
      'appointmentDuration': appointmentDuration,
      'workingHours': workingHours,
    };

    if (serviceImages != null) {
      data['serviceImages'] = serviceImages;
    }
    if (location != null) {
      data['location'] = {
        'latitude': location!.latitude,
        'longitude': location!.longitude,
      };
    }
    return data;
  }
}
