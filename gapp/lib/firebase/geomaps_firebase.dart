import 'package:get/get.dart';
import 'package:groom/firebase/user_firebase.dart';
import 'package:groom/repo/session_repo.dart';
import '../../data_models/user_model.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:geolocator/geolocator.dart';
import 'package:geocoding/geocoding.dart';
import '../repo/setting_repo.dart';
import '../states/provider_service_state.dart';

class GeoMapsFirebase {
  ProviderServiceState providerServiceState = Get.put(ProviderServiceState());

  UserFirebase userFirebase = UserFirebase();

  Future<void> checkAndFetchLocation() async {
    await getSession();
    if (auth.value.defaultAddress!.isEmpty) {
      getLocationAndUpdateUser();
    }
  }

  Future<void> getLocationAndUpdateUser() async {
    Position location = await _determinePosition();
    LatLng newLocation = LatLng(location.latitude, location.longitude);
    print('newLocation ${newLocation}');
    auth.value.location = newLocation;
    auth.value.defaultAddress = await getAddress();
    if (auth.value.uid.isNotEmpty) {
      await userFirebase.updateUser(auth.value);
      await userFirebase.getUserDetails(auth.value.uid);
      providerServiceState.getAllProviders();
      providerServiceState.getAllServices();
    } else {
      auth.value = UserModel.fromJson({
        "defaultAddress": await getAddress(),
        "location": {
          'latitude': location.latitude,
          'longitude': location.longitude
        }
      });
      providerServiceState.getAllProviders();
      providerServiceState.getAllServices();
    }
  }

  Future<void> compareAndUpdateLocation(UserModel user) async {
    Position location = await _determinePosition();
    LatLng newLocation = LatLng(location.latitude, location.longitude);
    user.location = newLocation;
    double distance = Geolocator.distanceBetween(
          user.location!.latitude,
          user.location!.longitude,
          location.latitude,
          location.longitude,
        ) /
        1000;
    if (distance > 1) {
      getAddress();
      await userFirebase.updateUserLocation(
          user.uid, LatLng(location.latitude, location.longitude));
    }
  }

  /// Determine the current position of the device.
  ///
  /// When the location services are not enabled or permissions
  /// are denied the `Future` will return an error.
  Future<Position> _determinePosition() async {
    Position position = Position(
        longitude: defalultLatLng.latitude,
        latitude: defalultLatLng.longitude,
        timestamp: DateTime.now(),
        accuracy: 1,
        altitude: 1,
        altitudeAccuracy: 1,
        heading: 1,
        headingAccuracy: 1,
        speed: 1,
        speedAccuracy: 1);
    bool serviceEnabled;
    LocationPermission permission;

    // Test if location services are enabled.
    serviceEnabled = await Geolocator.isLocationServiceEnabled();
    if (!serviceEnabled) {
      return position;
    }

    permission = await Geolocator.checkPermission();
    if (permission == LocationPermission.denied) {
      permission = await Geolocator.requestPermission();
      if (permission == LocationPermission.denied) {
        // Permissions are denied, next time you could try
        // requesting permissions again (this is also where
        // Android's shouldShowRequestPermissionRationale
        // returned true. According to Android guidelines
        // your App should show an explanatory UI now.
        return position;
      }
    }

    if (permission == LocationPermission.deniedForever) {
      // Permissions are denied forever, handle appropriately.
      return position;
    }

    // When we reach here, permissions are granted and we can
    // continue accessing the position of the device.
    return await Geolocator.getCurrentPosition();
  }

  Future<String> getAddress() async {
    if (auth.value.location!.latitude <= -90.0) {
      auth.value.location = defalultLatLng;
    }
    List<Placemark> placemarks = await placemarkFromCoordinates(
        auth.value.location!.latitude, auth.value.location!.longitude);
    StringBuffer b = StringBuffer();
    Placemark address = placemarks.first;
    // if (address.street != null) {
    //   b.write("${address.street}, ");
    // }
    if (address.locality != null) {
      b.write("${address.locality}, ");
    }
    if (address.street != null) {
      b.write("${address.street}, ");
    }
    if (address.subAdministrativeArea != null) {
      b.write("${address.subAdministrativeArea}, ");
    }

    return b.toString();
  }
}
