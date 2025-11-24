// import 'package:flutter/material.dart';
// import 'settings_service.dart';
//
// class SettingsMenuPage extends StatefulWidget {
//   const SettingsMenuPage({super.key});
//
//   @override
//   State<SettingsMenuPage> createState() => _SettingsMenuPageState();
// }
//
// class _SettingsMenuPageState extends State<SettingsMenuPage> {
//   final SettingsService _settingsService = SettingsService();
//
//   // Use two controllers for clarity
//   late TextEditingController _dayUrlController;
//   late TextEditingController _nightUrlController;
//   late TextEditingController _ipController;
//
//   bool _isLoading = true;
//
//   @override
//   void initState() {
//     super.initState();
//     _loadSettings();
//   }
//
//   Future<void> _loadSettings() async {
//     final ip = await _settingsService.loadIpAddress();
//     final urls = await _settingsService.loadCameraUrls(); // This will return a list with 2 URLs
//
//     if (mounted) {
//       setState(() {
//         _ipController = TextEditingController(text: ip);
//         _dayUrlController = TextEditingController(text: urls.isNotEmpty ? urls[0] : '');
//         _nightUrlController = TextEditingController(text: urls.length > 1 ? urls[1] : '');
//         _isLoading = false;
//       });
//     }
//   }
//
//   @override
//   void dispose() {
//     if (!_isLoading) {
//       _ipController.dispose();
//       _dayUrlController.dispose();
//       _nightUrlController.dispose();
//     }
//     super.dispose();
//   }
//
//   Future<void> _saveAndReturn() async {
//     await _settingsService.saveSettings(
//       ipAddress: _ipController.text,
//       dayCameraUrl: _dayUrlController.text,
//       nightCameraUrl: _nightUrlController.text,
//     );
//
//     if (mounted) {
//       Navigator.pop(context, true);
//     }
//   }
//
//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       appBar: AppBar(title: const Text('Settings')),
//       body: _isLoading
//           ? const Center(child: CircularProgressIndicator())
//           : ListView(
//         padding: const EdgeInsets.all(16.0),
//         children: [
//           // IP Address field (unchanged)
//           const Text('Robot IP Address', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
//           const SizedBox(height: 8),
//           TextFormField(controller: _ipController, decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., 192.168.0.158')),
//           const SizedBox(height: 24),
//           const Divider(),
//           const SizedBox(height: 16),
//
//           // Day Camera URL Field
//           const Text('Day Camera (EO) URL', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
//           const SizedBox(height: 8),
//           TextFormField(controller: _dayUrlController, decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., rtsp://.../cam0')),
//           const SizedBox(height: 24),
//
//           // Night Camera URL Field
//           const Text('Night Camera (IR) URL', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
//           const SizedBox(height: 8),
//           TextFormField(controller: _nightUrlController, decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., rtsp://.../cam1')),
//           const SizedBox(height: 24),
//
//           ElevatedButton(
//             onPressed: _saveAndReturn,
//             child: const Text('Save Changes'),
//           ),
//         ],
//       ),
//     );
//   }
// }

// File: settings_menu_page.dart

import 'package:flutter/material.dart';
import 'settings_service.dart';

class SettingsMenuPage extends StatefulWidget {
  const SettingsMenuPage({super.key});

  @override
  State<SettingsMenuPage> createState() => _SettingsMenuPageState();
}

class _SettingsMenuPageState extends State<SettingsMenuPage> {
  final SettingsService _settingsService = SettingsService();

  late TextEditingController _ipController;
  late TextEditingController _dayUrlController;
  late TextEditingController _nightUrlController;
  String _selectedQuality = 'hd';
  bool _isLoading = true;

  @override
  void initState() {
    super.initState();
    _loadSettings();
  }

  Future<void> _loadSettings() async {
    final ip = await _settingsService.loadIpAddress();
    final quality = await _settingsService.loadVideoQuality();
    // Load the full URLs to populate the text fields
    final urls = await _settingsService.loadCameraUrls();

    if (mounted) {
      setState(() {
        _ipController = TextEditingController(text: ip);
        _selectedQuality = quality;
        _dayUrlController = TextEditingController(text: urls[0]);
        _nightUrlController = TextEditingController(text: urls[1]);
        _isLoading = false;
      });
    }
  }

  @override
  void dispose() {
    if (!_isLoading) {
      _ipController.dispose();
      _dayUrlController.dispose();
      _nightUrlController.dispose();
    }
    super.dispose();
  }

  // --- THIS IS THE NEW LOGIC TO UPDATE URLS WHEN QUALITY CHANGES ---
  void _updateUrlsForQuality(String newQuality) {
    final ip = _ipController.text;

    // Define the base paths
    String dayBasePath = "rtsp://$ip:8554/cam0";
    String nightBasePath = "rtsp://$ip:8554/cam1";

    // Update the text controllers with the new suffix
    _dayUrlController.text = "${dayBasePath}_$newQuality";
    _nightUrlController.text = "${nightBasePath}_$newQuality";
  }

  Future<void> _saveAndReturn() async {
    // Save the full URLs from the text fields
    await _settingsService.saveSettings(
      ipAddress: _ipController.text,
      dayCameraUrl: _dayUrlController.text,
      nightCameraUrl: _nightUrlController.text,
      videoQuality: _selectedQuality,
    );

    if (mounted) {
      Navigator.pop(context, true);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Settings')),
      body: _isLoading
          ? const Center(child: CircularProgressIndicator())
          : ListView(
        padding: const EdgeInsets.all(16.0),
        children: [
          const Text('Robot IP Address', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          TextFormField(
            controller: _ipController,
            decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., 192.168.0.184'),
            onChanged: (value) {
              // When IP changes, update the URLs
              _updateUrlsForQuality(_selectedQuality);
            },
          ),
          const SizedBox(height: 24),
          const Divider(),
          const SizedBox(height: 16),

          const Text('Video Quality', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          DropdownButtonFormField<String>(
            value: _selectedQuality,
            items: const [
              DropdownMenuItem(value: 'hd', child: Text('HD (1280x720)')),
              DropdownMenuItem(value: 'sd', child: Text('SD (640x360)')),
            ],
            onChanged: (String? newValue) {
              if (newValue != null) {
                setState(() {
                  _selectedQuality = newValue;
                  // When quality changes, update the URLs
                  _updateUrlsForQuality(newValue);
                });
              }
            },
            decoration: const InputDecoration(border: OutlineInputBorder()),
          ),
          const SizedBox(height: 24),

          // --- THE URL TEXT FIELDS ARE BACK ---
          const Text('Day Camera (EO) URL', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          TextFormField(controller: _dayUrlController, decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., rtsp://.../cam0_hd')),
          const SizedBox(height: 24),

          const Text('Night Camera (IR) URL', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          TextFormField(controller: _nightUrlController, decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., rtsp://.../cam1_hd')),
          const SizedBox(height: 32),

          ElevatedButton(
            onPressed: _saveAndReturn,
            child: const Text('Save Changes'),
          ),
        ],
      ),
    );
  }
}