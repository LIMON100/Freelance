import 'package:shared_preferences/shared_preferences.dart';

class SettingsService {
  static const String _ipAddressKey = 'robot_ip_address';
  static const String _dayCameraUrlKey = 'day_camera_url';
  static const String _nightCameraUrlKey = 'night_camera_url';
  static const String _videoQualityKey = 'video_quality';
  static const String _videoBitrateKey = 'video_bitrate';
  static const String _bitrateModeKey = 'bitrate_mode';

  Future<void> saveSettings({
    required String ipAddress,
    required String dayCameraUrl,
    required String nightCameraUrl,
    required String videoQuality,
    required int videoBitrate, // Storing bitrate as an integer
    required String bitrateMode,
  }) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_ipAddressKey, ipAddress);
    await prefs.setString(_dayCameraUrlKey, dayCameraUrl);
    await prefs.setString(_nightCameraUrlKey, nightCameraUrl);
    await prefs.setString(_videoQualityKey, videoQuality);
    await prefs.setInt(_videoBitrateKey, videoBitrate); // Save as int
    await prefs.setString(_bitrateModeKey, bitrateMode);
  }

  Future<String> loadIpAddress() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getString(_ipAddressKey) ?? '192.168.0.158';
  }

  Future<String> loadVideoQuality() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getString(_videoQualityKey) ?? 'sd';
  }

  Future<int> loadVideoBitrate() async {
    final prefs = await SharedPreferences.getInstance();
    // Default to 1 Mbps for SD quality
    return prefs.getInt(_videoBitrateKey) ?? 2000000;
  }

  Future<String> loadBitrateMode() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getString(_bitrateModeKey) ?? 'auto';
  }

  Future<List<String>> loadCameraUrls() async {
    final prefs = await SharedPreferences.getInstance();
    final dayUrl = prefs.getString(_dayCameraUrlKey) ?? 'rtsp://192.168.0.158:8554/cam0';
    final nightUrl = prefs.getString(_nightCameraUrlKey) ?? 'rtsp://192.168.0.158:8554/cam1';
    return [dayUrl, nightUrl];
  }
}