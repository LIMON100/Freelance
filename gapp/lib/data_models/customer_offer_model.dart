import 'package:google_maps_flutter/google_maps_flutter.dart';

class CustomerOfferModel {
  String userId;
  String offerId;
  String description;
  String? serviceType;
  String? priceRange;
  String? selectedTime;
  DateTime? dateTime;
  LatLng? location;
  List<String>? offerImages;
  bool? deposit;
  String? radius;
  String? address;
  String? status;
  String? acceptedRequestId;
  String? serviceName;

  CustomerOfferModel({
    required this.userId,
    required this.offerId,
    required this.description,
    required this.location,
    this.serviceType,
    this.priceRange,
    this.dateTime,
    this.offerImages,
    this.deposit,
    this.selectedTime,
    this.radius,
    this.address,
    this.status,
    this.acceptedRequestId,
    this.serviceName,
  });

  factory CustomerOfferModel.fromJson(Map<String, dynamic> json) {
    return CustomerOfferModel(
      userId: json['userId'],
      serviceName: json['serviceName'],
      offerId: json['offerId'],
      description: json['description'],
      serviceType: json['serviceType'],
      priceRange: json['priceRange'],
      deposit: json['deposit'],
      selectedTime: json['selectedTime'],
      status: json['status']??"pending",
      acceptedRequestId: json['acceptedRequestId'],
      offerImages: json['offerImages'] != null
          ? List<String>.from(json['offerImages'])
          : [],
      location: json['location'] != null
          ? LatLng(
        json['location']['latitude'] ?? 0.0,
        json['location']['longitude'] ?? 0.0,
      )
          : null,
      dateTime:
      json['dateTime'] != null ? DateTime.parse(json['dateTime']) : null,
      radius: json['radius'].toString()??"1km",
      address: json['address']??"",
    );
  }

  Map<String, dynamic> toJson() {
    final data = <String, dynamic>{
      'userId': userId,
      'offerId': offerId,
      'description': description,
      'serviceType': serviceType,
      'priceRange': priceRange,
      'deposit': deposit,
      'selectedTime': selectedTime,
      'dateTime': dateTime?.toIso8601String(),
      'radius': radius,
      'address': address,
      'status': status,
      'serviceName': serviceName,
    };

    if (offerImages != null) {
      data['offerImages'] = offerImages;
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
