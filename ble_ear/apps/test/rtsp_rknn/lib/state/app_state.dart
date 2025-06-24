// lib/state/app_state.dart
import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

class AppState extends ChangeNotifier {
  // Store three full, independent URLs
  String _cam1Url = 'rtsp://192.168.0.184:8554/cam1'; // <-- YOUR PC's URL
  String _cam2Url = 'rtsp://192.168.0.184:8555/cam2'; // <-- YOUR PC's 2nd URL
  String _cam3Url = 'rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov'; // A public URL for testing
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