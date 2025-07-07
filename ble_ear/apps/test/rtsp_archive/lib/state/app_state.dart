// lib/state/app_state.dart
import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

class AppState extends ChangeNotifier {
  // --- UPDATED: Default URLs now point to your working RK3588 server ---
  String _cam1Url = 'rtsp://192.168.0.211:8554/cam0';
  String _cam2Url = 'rtsp://192.168.0.211:8554/cam1';
  String _cam3Url = 'rtsp://invalid.url/test'; // A bad URL for testing

  // --- UPDATED: Default command IP now points to your RK3588 ---
  String _robotIpAddress = '192.168.0.211';

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
    // Load saved settings, but if they don't exist, keep the new defaults from above.
    _robotIpAddress = prefs.getString('robotIpAddress') ?? _robotIpAddress;
    _cam1Url = prefs.getString('cam1Url') ?? _cam1Url;
    _cam2Url = prefs.getString('cam2Url') ?? _cam2Url;
    _cam3Url = prefs.getString('cam3Url') ?? _cam3Url;
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