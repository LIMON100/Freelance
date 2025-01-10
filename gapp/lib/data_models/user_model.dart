import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/ui/membership/m/membership_model.dart';
import 'customer_plan.dart';
import 'notification_model.dart';
import 'provider_user_model.dart';

class UserModel {
  String uid;
  String? email;
  String fullName;
  String photoURL;
  String contactNumber;
  bool isblocked;
  int joinedOn;
  int? updatedOn;
  String? dateOfBirth;
  String country;
  String state;
  String city;
  LatLng? location;
  String? defaultAddress;
  String? distance;
  List<String>? favoriteProvider;
  List<String>? favoriteService;
  String? notificationToken = "";
  String? pincode = "";
  String? bio = "";
  List<NotificationModel>? notifications;
  ProviderUserModel? providerUserModel;
  MembershipModel? membership;

  int requestsThisMonth =0 ;
  bool? notification_status;


  UserModel({
    required this.uid,
     this.email,
    required this.isblocked,
    required this.photoURL,
    required this.contactNumber,
    required this.fullName,
    required this.joinedOn,
    this.dateOfBirth,
    required this.country,
    required this.state,
    required this.city,
    this.location,
    this.updatedOn,
    this.bio,
    this.providerUserModel,
    this.favoriteProvider,
    this.favoriteService,
    this.notificationToken,
    this.notifications,
    this.membership,
    this.defaultAddress,
    this.pincode,
   required this.requestsThisMonth,
    this.notification_status,
  });

  factory UserModel.fromJson(Map<String, dynamic> json) {
    return UserModel(
      uid: json['uid'] ?? '',
      email: json['email'] ?? '',
      fullName: json['fullName'] ?? '',
      photoURL: json['photoURL'] ?? '',
      contactNumber: json['contactNumber'] ?? '',
      notificationToken: json['notificationToken'] ?? '',
      defaultAddress: json['defaultAddress'] ?? '',
      requestsThisMonth: json['requestsThisMonth'] ??0,
      isblocked: json['isblocked'] ?? false,
      joinedOn: json['joinedOn'] != null ? int.parse(json['joinedOn'].toString()) : 0,
      updatedOn: json['updatedOn'] != null ? int.parse(json['updatedOn'].toString()) : 0,
      dateOfBirth: json['dateOfBirth'].toString(),
      country: json['country'] ?? '',
      state: json['state'] ?? '',
      city: json['city'] ?? '',
      pincode: json['pincode'] ?? '',
      bio: json['bio'] ?? '',
      notification_status: json['notification_status'] ?? false,
      location: json['location'] != null
          ? LatLng(json['location']['latitude'], json['location']['longitude'])
          : LatLng(36.1716, 115.1391),
      favoriteProvider: json['favoriteProvider'] != null
          ? List<String>.from(json['favoriteProvider'])
          : null,
      favoriteService: json['favoriteService'] != null
          ? List<String>.from(json['favoriteService'])
          : null,
      providerUserModel: json['providerUserModel'] != null &&
              json['providerUserModel'] is Map<String, dynamic>
          ? ProviderUserModel.fromJson(json['providerUserModel'])
          : null,
      membership: json['membership'] != null &&
              json['providerUserModel'] is Map<String, dynamic>
          ? MembershipModel.fromJson(json['membership'])
          : null,
   );
  }

  Map<String, dynamic> toJson() {
    final data = <String, dynamic>{
      'uid': uid,
      'notificationToken': notificationToken,
      'email': email,
      'fullName': fullName,
      'photoURL': photoURL,
      'contactNumber': contactNumber,
      'requestsThisMonth' : requestsThisMonth,
      'isblocked': isblocked,
      'joinedOn': joinedOn,
      'updatedOn': updatedOn,
      'dateOfBirth': dateOfBirth,
      'country': country,
      'state': state,
      'city': city,
      'defaultAddress': defaultAddress,
      'pincode': pincode,
      'bio': bio,
      'notification_status': notification_status,
      'location': location != null
          ? {'latitude': location!.latitude, 'longitude': location!.longitude}
          : null,
    };
    if (favoriteProvider != null) {
      data['favoriteProvider'] = favoriteProvider;
    }
    if (favoriteService != null) {
      data['favoriteService'] = favoriteService;
    }
    if (providerUserModel != null) {
      data['providerUserModel'] = providerUserModel!.toJson();
    }
    if (membership != null) {
      data['membership'] = membership!.toJson();
    }

    if (notifications != null) {
      data['notifications'] = notifications!.map((e) => e.toJson()).toList();
    }
    return data;
  }
}
