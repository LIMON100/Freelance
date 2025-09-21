// import 'package:flutter/material.dart';
// import 'package:google_fonts/google_fonts.dart';
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
//   late List<TextEditingController> _urlControllers;
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
//     final urls = await _settingsService.loadCameraUrls();
//
//     if (mounted) {
//       setState(() {
//         _ipController = TextEditingController(text: ip);
//
//         // Ensure we only ever work with a single URL
//         final singleUrl = urls.isNotEmpty ? urls.first : '';
//         _urlControllers = [TextEditingController(text: singleUrl)];
//
//         _isLoading = false;
//       });
//     }
//   }
//
//   @override
//   void dispose() {
//     if (!_isLoading) {
//       _ipController.dispose();
//       for (var controller in _urlControllers) {
//         controller.dispose();
//       }
//     }
//     super.dispose();
//   }
//
//   Future<void> _saveAndReturn() async {
//     await _settingsService.saveSettings(
//       ipAddress: _ipController.text,
//       cameraUrls: _urlControllers.map((c) => c.text).toList(),
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
//       appBar: AppBar(
//         title: const Text('Settings'),
//         backgroundColor: Colors.grey[200],
//         foregroundColor: Colors.black87,
//       ),
//       body: _isLoading
//           ? const Center(child: CircularProgressIndicator())
//           : ListView(
//         padding: const EdgeInsets.all(16.0),
//         children: [
//           const Text('Robot IP Address', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
//           const SizedBox(height: 8),
//           TextFormField(
//             controller: _ipController,
//             decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., 192.168.0.158'),
//             keyboardType: TextInputType.phone,
//           ),
//           const SizedBox(height: 24),
//           const Divider(),
//           const SizedBox(height: 16),
//
//           // This logic will now only build one URL field
//           for (int i = 0; i < _urlControllers.length; i++)
//             Padding(
//               padding: const EdgeInsets.only(bottom: 16.0),
//               child: Column(
//                 crossAxisAlignment: CrossAxisAlignment.start,
//                 children: [
//                   Text('Camera URL', style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)), // Changed label to be generic
//                   const SizedBox(height: 8),
//                   TextFormField(
//                     controller: _urlControllers[i],
//                     decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., rtsp://...'),
//                   ),
//                 ],
//               ),
//             ),
//
//           const SizedBox(height: 20),
//           ElevatedButton(
//             onPressed: _saveAndReturn,
//             style: ElevatedButton.styleFrom(
//               padding: const EdgeInsets.symmetric(vertical: 16.0),
//               backgroundColor: Theme.of(context).primaryColor,
//               foregroundColor: Colors.white,
//             ),
//             child: Text('Save Changes', style: GoogleFonts.rajdhani(fontSize: 18, fontWeight: FontWeight.bold)),
//           ),
//         ],
//       ),
//     );
//   }
// }













import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'settings_service.dart';

class SettingsMenuPage extends StatefulWidget {
  const SettingsMenuPage({super.key});

  @override
  State<SettingsMenuPage> createState() => _SettingsMenuPageState();
}

// In settings_menu_page.dart

class _SettingsMenuPageState extends State<SettingsMenuPage> {
  final SettingsService _settingsService = SettingsService();

  // Use two controllers for clarity
  late TextEditingController _dayUrlController;
  late TextEditingController _nightUrlController;
  late TextEditingController _ipController;

  bool _isLoading = true;

  @override
  void initState() {
    super.initState();
    _loadSettings();
  }

  Future<void> _loadSettings() async {
    final ip = await _settingsService.loadIpAddress();
    final urls = await _settingsService.loadCameraUrls(); // This will return a list with 2 URLs

    if (mounted) {
      setState(() {
        _ipController = TextEditingController(text: ip);
        _dayUrlController = TextEditingController(text: urls.isNotEmpty ? urls[0] : '');
        _nightUrlController = TextEditingController(text: urls.length > 1 ? urls[1] : '');
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

  Future<void> _saveAndReturn() async {
    await _settingsService.saveSettings(
      ipAddress: _ipController.text,
      dayCameraUrl: _dayUrlController.text,
      nightCameraUrl: _nightUrlController.text,
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
          // IP Address field (unchanged)
          const Text('Robot IP Address', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          TextFormField(controller: _ipController, decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., 192.168.0.158')),
          const SizedBox(height: 24),
          const Divider(),
          const SizedBox(height: 16),

          // Day Camera URL Field
          const Text('Day Camera (EO) URL', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          TextFormField(controller: _dayUrlController, decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., rtsp://.../cam0')),
          const SizedBox(height: 24),

          // Night Camera URL Field
          const Text('Night Camera (IR) URL', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          TextFormField(controller: _nightUrlController, decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., rtsp://.../cam1')),
          const SizedBox(height: 24),

          ElevatedButton(
            onPressed: _saveAndReturn,
            child: const Text('Save Changes'),
          ),
        ],
      ),
    );
  }
}