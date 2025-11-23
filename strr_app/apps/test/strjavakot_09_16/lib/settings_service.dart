import 'package:shared_preferences/shared_preferences.dart';

class SettingsService {
  static const String _ipAddressKey = 'robot_ip_address';
  static const String _dayCameraUrlKey = 'day_camera_url';
  static const String _nightCameraUrlKey = 'night_camera_url';

  Future<void> saveSettings({
    required String ipAddress,
    required String dayCameraUrl, // Changed parameter
    required String nightCameraUrl, // Changed parameter
  }) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_ipAddressKey, ipAddress);
    await prefs.setString(_dayCameraUrlKey, dayCameraUrl);
    await prefs.setString(_nightCameraUrlKey, nightCameraUrl);
  }

  Future<String> loadIpAddress() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getString(_ipAddressKey) ?? '192.168.49.1';
  }

  // This function now loads both URLs into a list
  Future<List<String>> loadCameraUrls() async {
    final prefs = await SharedPreferences.getInstance();
    final dayUrl = prefs.getString(_dayCameraUrlKey) ?? 'rtsp://192.168.49.1:8554/cam0';
    final nightUrl = prefs.getString(_nightCameraUrlKey) ?? 'rtsp://192.168.49.1:8554/cam1';
    return [dayUrl, nightUrl];
  }
}