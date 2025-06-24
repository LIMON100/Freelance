// // lib/state/app_state.dart
// import 'package:flutter/material.dart';
// import 'package:shared_preferences/shared_preferences.dart';
//
// class AppState extends ChangeNotifier {
//   String _robotIpAddress = '192.168.1.100'; // Default IP
//   String _cam1Url = 'rtsp://192.168.1.100:8554/cam1'; // Default URL
//   String _cam2Url = 'rtsp://192.168.1.100:8554/cam2';
//   String _cam3Url = 'rtsp://192.168.1.100:8554/cam3';
//
//   String get robotIpAddress => _robotIpAddress;
//   List<String> get cameraUrls => [_cam1Url, _cam2Url, _cam3Url];
//
//   int _selectedCameraIndex = 0;
//   int get selectedCameraIndex => _selectedCameraIndex;
//   String get currentCameraUrl => cameraUrls[_selectedCameraIndex];
//
//   AppState() {
//     loadSettings();
//   }
//
//   void selectCamera(int index) {
//     _selectedCameraIndex = index;
//     notifyListeners(); // This will tell the UI to rebuild
//   }
//
//   Future<void> loadSettings() async {
//     final prefs = await SharedPreferences.getInstance();
//     _robotIpAddress = prefs.getString('robotIpAddress') ?? '192.168.1.100';
//     // Update URLs based on new IP
//     await updateSettings(_robotIpAddress);
//     notifyListeners();
//   }
//
//   Future<void> updateSettings(String newIp) async {
//     _robotIpAddress = newIp;
//     _cam1Url = 'rtsp://$_robotIpAddress:8554/cam1';
//     _cam2Url = 'rtsp://$_robotIpAddress:8554/cam2';
//     _cam3Url = 'rtsp://$_robotIpAddress:8554/cam3';
//
//     final prefs = await SharedPreferences.getInstance();
//     await prefs.setString('robotIpAddress', _robotIpAddress);
//     notifyListeners();
//   }
// }


// lib/state/app_state.dart
import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

class AppState extends ChangeNotifier {
  // Store three full, independent URLs
  String _cam1Url = 'rtsp://192.168.1.100:8554/cam1'; // Default
  String _cam2Url = 'rtsp://example.com:8554/stream2'; // Default
  String _cam3Url = 'rtsp://10.0.0.5:554/another_path';  // Default

  // Keep track of the robot's IP for sending commands
  String _robotIpAddress = '192.168.1.100'; // Default IP for commands

  // Getters that the UI will use
  String get robotIpAddress => _robotIpAddress;
  List<String> get cameraUrls => [_cam1Url, _cam2Url, _cam3Url];

  int _selectedCameraIndex = 0;
  int get selectedCameraIndex => _selectedCameraIndex;
  String get currentCameraUrl => cameraUrls[_selectedCameraIndex];

  AppState() {
    loadSettings();
  }

  void selectCamera(int index) {
    if (index >= 0 && index < cameraUrls.length) {
      _selectedCameraIndex = index;
      notifyListeners(); // Tell the UI to rebuild
    }
  }

  // Load all saved settings from phone storage
  Future<void> loadSettings() async {
    final prefs = await SharedPreferences.getInstance();
    _robotIpAddress = prefs.getString('robotIpAddress') ?? '192.168.1.100';
    _cam1Url = prefs.getString('cam1Url') ?? 'rtsp://192.168.1.100:8554/cam1';
    _cam2Url = prefs.getString('cam2Url') ?? 'rtsp://192.168.1.100:8554/cam2';
    _cam3Url = prefs.getString('cam3Url') ?? 'rtsp://192.168.1.100:8554/cam3';
    notifyListeners();
  }

  // Save all settings to phone storage
  Future<void> updateSettings({
    required String newIp,
    required String newCam1Url,
    required String newCam2Url,
    required String newCam3Url,
  }) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('robotIpAddress', newIp);
    await prefs.setString('cam1Url', newCam1Url);
    await prefs.setString('cam2Url', newCam2Url);
    await prefs.setString('cam3Url', newCam3Url);

    // Update the state in memory after saving
    _robotIpAddress = newIp;
    _cam1Url = newCam1Url;
    _cam2Url = newCam2Url;
    _cam3Url = newCam3Url;

    notifyListeners();
  }
}