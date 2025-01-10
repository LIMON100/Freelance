import 'package:google_maps_flutter/google_maps_flutter.dart';

import '../ui/membership/m/membership_model.dart';

class ProviderUserModel {
  String about;
  String workDayFrom;
  String workDayTo;
  String addressLine;
  String providerType;
  int createdOn;
  String? salonTitle;
  dynamic basePrice;
  List<String> providerServices;
  List<String>? skills;
  LatLng? location;
  List<String>? providerImages;
  double overallRating; // Make non-nullable
  int numReviews; // Make non-nullable

  ProviderUserModel({
    required this.about,
    required this.workDayFrom,
    required this.workDayTo,
    required this.location,
    required this.addressLine,
    required this.createdOn,
    required this.providerType,
    required this.providerServices,
    this.providerImages,
    this.salonTitle,
    this.skills,
    this.basePrice,
    this.overallRating = 0.0, // Initialize with default value
    this.numReviews = 0, // Initialize with default value
  });

  factory ProviderUserModel.fromJson(Map<String, dynamic> json) {
    return ProviderUserModel(
      about: json['about'] ?? '',
      workDayFrom: json['workDayFrom'] ?? '',
      workDayTo: json['workDayTo'] ?? '',
      providerType: json['providerType'] ?? '',
      addressLine: json['addressLine'] ?? '',
      createdOn: json['createdOn'] != null ? int.parse(json['createdOn'].toString()) : 0,
      salonTitle: json['salonTitle'],
      basePrice: json['basePrice'],
      providerServices: List<String>.from(json['providerServices'] ?? []),
      providerImages: json['providerImages'] != null ? List<String>.from(json['providerImages']) : null,
      skills: json['skills'] != null ? List<String>.from(json['skills']) : [],
      location: json['location'] != null
          ? LatLng(
        json['location']['latitude'] ?? 0.0,
        json['location']['longitude'] ?? 0.0,
      )
          : null,
      overallRating: (json['overallRating'] ?? 0.0).toDouble(),
      numReviews: json['numReviews'] ?? 0,
    );
  }

  Map<String, dynamic> toJson() {
    final data = <String, dynamic>{
      'about': about,
      'workDayFrom': workDayFrom,
      'workDayTo': workDayTo,
      'addressLine': addressLine,
      'createdOn': createdOn,
      'providerType': providerType,
      'providerServices': providerServices,
      'salonTitle': salonTitle,
      'overallRating': overallRating,
      'numReviews': numReviews,
      'basePrice': basePrice,
    };
    if (providerImages != null) {
      data['providerImages'] = providerImages;
    }
    if (skills != null) {
      data['skills'] = skills;
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
