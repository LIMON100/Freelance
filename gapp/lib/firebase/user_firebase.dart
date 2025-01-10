import 'dart:convert';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_storage/firebase_storage.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'dart:io';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/constant/global_configuration.dart';
import 'package:groom/data_models/provider_user_model.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/ui/provider/schedule/schedule_model.dart';
import 'package:url_launcher/url_launcher.dart';

import '../data_models/provider_service_model.dart';
import '../data_models/user_model.dart';
import '../repo/session_repo.dart';
import '../ui/customer/customer_dashboard/model/location_model.dart';
import '../ui/customer/customer_dashboard/model/resent_search_model.dart';

class UserFirebase {
  final DatabaseReference _databaseReference = FirebaseDatabase.instanceFor(
          app: Firebase.app(),
          databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();
  final FirebaseStorage _storage = FirebaseStorage.instance;

  Future<void> addUser(UserModel user) async {
    final User? currentUser = FirebaseAuth.instance.currentUser;
    if (currentUser != null && currentUser.uid.isNotEmpty) {
      user.uid = currentUser.uid;
      final DatabaseReference userRef =
          _databaseReference.child('users').child(currentUser.uid);
      await userRef.set(user.toJson());
      await getUserDetails(user.uid);
    } else {
      print(
          "User is not logged in"); // Handle the case where the user is not logged in
    }
  }

  Future<void> updateUser(UserModel user) async {
    if (user.uid.isEmpty) {
      final User? currentUser = FirebaseAuth.instance.currentUser;
      user.uid = currentUser!.uid;
    }
    final DatabaseReference userRef =
        _databaseReference.child('users').child(user.uid);
    await userRef.update(user.toJson());
  }

  Future<void> updateUserEdit(UserModel user) async {
    if (user.uid.isEmpty) {
      final User? currentUser = FirebaseAuth.instance.currentUser;
      user.uid = currentUser!.uid;
    }
    final DatabaseReference userRef =
        _databaseReference.child('users').child(user.uid);
    await userRef.update(user.toJson());
  }

  Future<void> updateNotificationStatus(bool notification) async {
    final DatabaseReference userRef =
        _databaseReference.child('users').child(auth.value.uid);
    await userRef.update({"notification_status": notification});
  }

  Future<void> updateUserLocation(String userId, LatLng location) async {
    try {
      await _databaseReference.child('users').child(userId).update(
        {
          'location': {
            'latitude': location.latitude,
            'longitude': location.longitude
          },
        },
      );
    } catch (e) {
      print('Failed to update user location: $e');
    }
  }

  Future<UserModel> getUser(String UserId) async {
    var source = await _databaseReference.child("users").child(UserId).once();
    var values = source.snapshot;
    UserModel? userModel;
    userModel = UserModel.fromJson(
      jsonDecode(
        jsonEncode(values.value),
      ),
    );
    if (auth.value.uid == UserId) {
      isGuest.value = false;
      auth.value = userModel;
      CreateSession(userModel.toJson(), true);
    }
    return userModel;
  }

  Future<UserModel> getUserDetails(String UserId) async {
    var source = await _databaseReference.child("users").child(UserId).once();
    var values = source.snapshot;
    if (values.value == null) {
      await FirebaseAuth.instance.signOut();
      auth.value = UserModel.fromJson({});
      isProvider.value = false;
      isGuest.value = false;
      Logout();
      Get.offAllNamed('/login');
    }
    UserModel? userModel;
    userModel = UserModel.fromJson(
      jsonDecode(
        jsonEncode(values.value),
      ),
    );
    isGuest.value = false;
    auth.value = userModel;
    CreateSession(userModel.toJson(), true);
    return userModel;
  }

  Future<String?> getImage(String UserId) async {
    String? imageurl = "";
    var source = await _databaseReference.child("users").child(UserId).once();
    var value = source.snapshot;
    UserModel? userModel;
    return imageurl;
  }

  Future<bool> updateUserFile(String userId) async {
    try {
      var source = await _databaseReference.child('users').child(userId).once();
      var value = source.snapshot;

      return true;
    } catch (e) {
      print(e);
      return false;
    }
  }

  Future<bool> checkIfPhoneExists(String phoneNumber) async {
    try {
      final source = await _databaseReference
          .child('users')
          .orderByChild("contactNumber")
          .equalTo(phoneNumber)
          .once();
      var results = source.snapshot;
      if (results.value != null) {
        return true;
      }
      return false;
    } catch (error) {
      print('Error checking application: $error');
      return false;
    }
  }

  Future<bool> checkIfEmailExists(String email) async {
    try {
      final source = await _databaseReference
          .child('users')
          .orderByChild("email")
          .equalTo(email)
          .once();
      var results = source.snapshot;
      if (results.value != null) {
        return true;
      }
      return false;
    } catch (error) {
      print('Error checking application: $error');
      return false;
    }
  }

  Future<String> uploadImage(File image, String userID) async {
    try {
      String fileName =
          'users/$userID/imageProfile/${DateTime.now().millisecondsSinceEpoch}.jpg';
      Reference ref = _storage.ref().child(fileName);
      UploadTask uploadTask = ref.putFile(image);
      TaskSnapshot snapshot = await uploadTask;
      return await snapshot.ref.getDownloadURL();
    } catch (e) {
      print(e);
      return '';
    }
  }

  Future<bool> removeServiceToFavorites(String userId, String serviceId) async {
    try {
      final userRef = _databaseReference
          .child('users')
          .child(userId)
          .child('favoriteService');
      final snapshot = await userRef.once();

      List<String> currentFavorites =
          (snapshot.snapshot.value as List<dynamic>?)
                  ?.map((e) => e.toString())
                  .toList() ??
              [];

      if (currentFavorites.contains(serviceId)) {
        currentFavorites.remove(serviceId);
        await userRef.set(currentFavorites);
        return true;
      }
    } catch (e) {
      print('Failed to remove service from favorites: $e');
    }
    return false;
  }

  Future<void> addServiceToFavorites(String userId, String serviceId) async {
    final userRef = _databaseReference.ref
        .child('users')
        .child(userId)
        .child('favoriteService');
    await userRef.once().then((snapshot) {
      List<String> favoriteServices =
          (snapshot.snapshot.value as List<dynamic>?)
                  ?.map((e) => e.toString())
                  .toList() ??
              [];
      if (!favoriteServices.contains(serviceId)) {
        favoriteServices.add(serviceId);
        userRef.set(favoriteServices);
      }
    });
  }

  Future<void> addProviderToFavorites(String userId, String serviceId) async {
    final userRef = _databaseReference.ref
        .child('users')
        .child(userId)
        .child('favoriteProvider');
    await userRef.once().then((snapshot) {
      List<String> favoriteServices =
          (snapshot.snapshot.value as List<dynamic>?)
                  ?.map((e) => e.toString())
                  .toList() ??
              [];
      if (!favoriteServices.contains(serviceId)) {
        favoriteServices.add(serviceId);
        userRef.set(favoriteServices);
      }
    });
  }

  Future<void> addLocation(Map<String, dynamic> map) async {
    final userRef = _databaseReference.ref
        .child('users')
        .child(auth.value.uid)
        .child('locations');
    String? key = userRef.push().key;
    map['id'] = key;
    userRef.child(key!).set(map);
  }

  Future<void> addResentSearch(Map<String, dynamic> map) async {
    final userRef = _databaseReference.ref
        .child('users')
        .child(auth.value.uid)
        .child('resentSearch');
    String? key = userRef.push().key;
    map['id'] = key;
    userRef.child(key!).set(map);
  }

  Future<void> addScheduleTime(Map<String, dynamic> map) async {
    final userRef = _databaseReference.ref
        .child('users')
        .child(auth.value.uid)
        .child('providerUserModel')
        .child('schedule');
    String? key = map['id'] ?? userRef.push().key;
    map['id'] = key;
    userRef.child(key!).set(map);
  }

  Future<void> updateProviderSkill(List<String> skills) async {
    try {
      await _databaseReference
          .child('users')
          .child(auth.value.uid)
          .child('providerUserModel')
          .update(
        {
          'skills': skills,
        },
      );
    } catch (e) {
      print('Failed to update user location: $e');
    }
  }

  Future<void> updateProviderCategory(List<String> skills) async {
    try {
      await _databaseReference
          .child('users')
          .child(auth.value.uid)
          .child('providerUserModel')
          .update(
        {
          'categories': skills,
        },
      );
    } catch (e) {
      print('Failed to update user location: $e');
    }
  }

  Future<List<String>> getProviderSkills(String userId) async {
    final userSnapshot = await _databaseReference
        .child('users')
        .child(userId)
        .child('providerUserModel')
        .child('skills')
        .get();

    if (userSnapshot.exists) {
      final data = userSnapshot.value;
      if (data is List) {
        return data.map((e) => e.toString()).toList();
      }
    }
    return [];
  }

  Future<List<String>> getProviderCategory(String userId) async {
    final userSnapshot = await _databaseReference
        .child('users')
        .child(userId)
        .child('providerUserModel')
        .child('categories')
        .get();

    if (userSnapshot.exists) {
      final data = userSnapshot.value;
      if (data is List) {
        return data.map((e) => e.toString()).toList();
      }
    }
    return [];
  }

  Future<void> deleteLocation(id) async {
    final userRef = _databaseReference.ref
        .child('users')
        .child(auth.value.uid)
        .child('locations')
        .child(id);
    userRef.remove();
  }

  Future<void> deleteSchedule(id) async {
    final userRef = _databaseReference.ref
        .child('users')
        .child(auth.value.uid)
        .child('providerUserModel')
        .child('schedule')
        .child(id);
    userRef.remove();
  }

  Future<void> deleteResentSearch(id) async {
    final userRef = _databaseReference.ref
        .child('users')
        .child(auth.value.uid)
        .child('resentSearch')
        .child(id);
    userRef.remove();
  }

  Future<void> deleteAllResentSearch() async {
    final userRef = _databaseReference.ref
        .child('users')
        .child(auth.value.uid)
        .child('resentSearch');
    userRef.remove();
  }

  Future<bool> checkServiceFavorite(String userId, String serviceId) async {
    final userRef = _databaseReference.ref
        .child('users')
        .child(userId)
        .child('favoriteService');
    final snapshot = await userRef.once();
    List<String> favoriteServices = (snapshot.snapshot.value as List<dynamic>?)
            ?.map((e) => e.toString())
            .toList() ??
        [];
    print(favoriteServices);
    print(serviceId);
    return favoriteServices.contains(serviceId);
  }

  Future<bool> updateUserNotificationToken(
      String userId, String notificationToken) async {
    try {
      await _databaseReference.ref
          .child('users')
          .child(userId)
          .update({'notificationToken': notificationToken});
      return true;
    } catch (e) {
      print(e);
    }
    return false;
  }

  Future<List<String>> getFavoriteServiceIds(String userId) async {
    final userSnapshot =
        await _databaseReference.child('users').child(userId).once();
    if (userSnapshot != null) {
      UserModel user = UserModel.fromJson(
          Map<String, dynamic>.from(userSnapshot.snapshot.value as Map));
      return user.favoriteService ?? [];
    } else {
      return [];
    }
  }

  Future<UserModel> getUser1(String userId) async {
    final snapshot =
        await _databaseReference.child('users').child(userId).get();
    if (snapshot.exists) {
      return UserModel.fromJson(
          Map<String, dynamic>.from(snapshot.value as Map));
    } else {
      throw Exception('User not found');
    }
  }

  Future<List<String>> getFavoriteServiceIds1(String userId) async {
    final userSnapshot =
        await _databaseReference.child('users').child(userId).get();

    if (userSnapshot.exists) {
      final data = userSnapshot.value;

      if (data is Map) {
        List<dynamic> favoriteServiceDynamic = data['favoriteService'] ?? [];
        List<String> favoriteService =
            favoriteServiceDynamic.map((e) => e.toString()).toList();
        return favoriteService;
      } else {
        throw Exception('Unexpected data format');
      }
    } else {
      return [];
    }
  }

  Future<List<String>> getFavoriteProvidersIds(String userId) async {
    final userSnapshot =
        await _databaseReference.child('users').child(userId).get();

    if (userSnapshot.exists) {
      final data = userSnapshot.value;

      if (data is Map) {
        List<dynamic> favoriteServiceDynamic = data['favoriteProvider'] ?? [];
        List<String> favoriteService =
            favoriteServiceDynamic.map((e) => e.toString()).toList();
        return favoriteService;
      } else {
        throw Exception('Unexpected data format');
      }
    } else {
      return [];
    }
  }

  Future<List<String>> getFavoriteServiceIds11(String userId) async {
    final userSnapshot = await _databaseReference
        .child('users')
        .child(userId)
        .child('favoriteService')
        .get();

    if (userSnapshot.exists) {
      final data = userSnapshot.value;
      if (data is List) {
        return data.map((e) => e.toString()).toList();
      }
    }
    return [];
  }

  Future<List<ProviderServiceModel>> getFavoriteServices(String userId) async {
    List<String> favoriteServiceIds = await fetchFavoriteServices(userId);

    if (favoriteServiceIds.isEmpty) {
      return []; // No favorite services to retrieve
    }

    List<ProviderServiceModel> favoriteServices = [];
    for (String serviceId in favoriteServiceIds) {
      final serviceSnapshot = await _databaseReference
          .child('providerServices')
          .child(serviceId)
          .get();

      if (serviceSnapshot.exists) {
        final data = serviceSnapshot.value;
        if (data is Map) {
          favoriteServices.add(
              ProviderServiceModel.fromJson(Map<String, dynamic>.from(data)));
        }
      }
    }

    return favoriteServices;
  }

  Future<List<LocationModel>> getLocations() async {
    List<LocationModel> list = [];
    var source = await _databaseReference
        .child('users')
        .child(auth.value.uid)
        .child('locations')
        .once();
    var data = source.snapshot;
    if (data.value != null) {
      for (var element in data.children) {
        var model = LocationModel.fromJson(
          jsonDecode(
            jsonEncode(element.value),
          ),
        );
        list.add(model);
      }
    }
    return list;
  }

  Future<List<ResentSearchModel>> getResentSearch() async {
    List<ResentSearchModel> list = [];
    var source = await _databaseReference
        .child('users')
        .child(auth.value.uid)
        .child('resentSearch')
        .once();
    var data = source.snapshot;
    if (data.value != null) {
      for (var element in data.children) {
        var model = ResentSearchModel.fromJson(
          jsonDecode(
            jsonEncode(element.value),
          ),
        );
        list.add(model);
      }
    }
    return list;
  }

  Future<List<ScheduleModel>> getScheduleList(String providerId) async {
    List<ScheduleModel> list = [];
    var source = await _databaseReference
        .child('users')
        .child(providerId)
        .child('providerUserModel')
        .child('schedule')
        .once();
    var data = source.snapshot;
    if (data.value != null) {
      for (var element in data.children) {
        var model = ScheduleModel.fromJson(
          jsonDecode(
            jsonEncode(element.value),
          ),
        );
        list.add(model);
      }
    }
    return list;
  }

  Future<List<String>> fetchFavoriteServices(String userId) async {
    final userRef = _databaseReference.ref
        .child('users')
        .child(userId)
        .child('favoriteService');
    final snapshot = await userRef.once();
    List<String> favoriteServices = (snapshot.snapshot.value as List<dynamic>?)
            ?.map((e) => e.toString())
            .toList() ??
        [];
    return favoriteServices;
  }

  Future<void> launchGoogleMaps(
      {required double destinationLatitude,
      required double destinationLongitude}) async {
    final uri = Uri(
        scheme: "google.navigation",
        // host: '"0,0"',  {here we can put host}
        queryParameters: {'q': '$destinationLatitude, $destinationLongitude'});
    if (await canLaunchUrl(uri)) {
      await launchUrl(uri);
    } else {
      debugPrint('An error occurred');
    }
  }
}
