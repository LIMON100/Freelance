// import 'package:shared_preferences/shared_preferences.dart';
//
// class SettingsService {
//   static const String _ipAddressKey = 'robot_ip_address';
//   static const String _dayCameraUrlKey = 'day_camera_url';
//   static const String _nightCameraUrlKey = 'night_camera_url';
//
//   Future<void> saveSettings({
//     required String ipAddress,
//     required String dayCameraUrl, // Changed parameter
//     required String nightCameraUrl, // Changed parameter
//   }) async {
//     final prefs = await SharedPreferences.getInstance();
//     await prefs.setString(_ipAddressKey, ipAddress);
//     await prefs.setString(_dayCameraUrlKey, dayCameraUrl);
//     await prefs.setString(_nightCameraUrlKey, nightCameraUrl);
//   }
//
//   Future<String> loadIpAddress() async {
//     final prefs = await SharedPreferences.getInstance();
//     return prefs.getString(_ipAddressKey) ?? '192.168.49.1';
//   }
//
//   // This function now loads both URLs into a list
//   Future<List<String>> loadCameraUrls() async {
//     final prefs = await SharedPreferences.getInstance();
//     final dayUrl = prefs.getString(_dayCameraUrlKey) ?? 'rtsp://192.168.49.1:8554/cam0';
//     final nightUrl = prefs.getString(_nightCameraUrlKey) ?? 'rtsp://192.168.49.1:8554/cam1';
//     return [dayUrl, nightUrl];
//   }
// }

import 'package:shared_preferences/shared_preferences.dart';

class SettingsService {
  static const String _ipAddressKey = 'robot_ip_address';
  static const String _dayCameraUrlKey = 'day_camera_url';
  static const String _nightCameraUrlKey = 'night_camera_url';
  static const String _videoQualityKey = 'video_quality';

  Future<void> saveSettings({
    required String ipAddress,
    required String dayCameraUrl,
    required String nightCameraUrl,
    required String videoQuality,
  }) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_ipAddressKey, ipAddress);
    await prefs.setString(_dayCameraUrlKey, dayCameraUrl);
    await prefs.setString(_nightCameraUrlKey, nightCameraUrl);
    await prefs.setString(_videoQualityKey, videoQuality);
  }

  Future<String> loadIpAddress() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getString(_ipAddressKey) ?? '192.168.0.158'; // A sensible default IP
  }

  Future<String> loadVideoQuality() async {
    final prefs = await SharedPreferences.getInstance();
    // --- FIX: Default to 'sd' (Standard Definition) ---
    return prefs.getString(_videoQualityKey) ?? 'sd';
  }

  Future<List<String>> loadCameraUrls() async {
    final prefs = await SharedPreferences.getInstance();
    final ip = await loadIpAddress();
    final quality = await loadVideoQuality();

    // --- THIS IS THE KEY FIX ---
    // Load the saved URLs.
    String? savedDayUrl = prefs.getString(_dayCameraUrlKey);
    String? savedNightUrl = prefs.getString(_nightCameraUrlKey);

    // If a saved URL is null or empty, generate a default one.
    // This guarantees the app always has a valid URL to connect to.
    final dayUrl = (savedDayUrl == null || savedDayUrl.isEmpty)
        ? 'rtsp://$ip:8554/cam0_$quality'
        : savedDayUrl;

    final nightUrl = (savedNightUrl == null || savedNightUrl.isEmpty)
        ? 'rtsp://$ip:8554/cam1_$quality'
        : savedNightUrl;
    // --- END OF FIX ---

    return [dayUrl, nightUrl];
  }
}