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

  String _selectedQuality = 'sd';
  int _selectedBitrate = 1000000; // Store bitrate as integer
  String _selectedBitrateMode = 'auto';

  bool _isLoading = true;

  // Define the bitrate options for each quality level ---
  final Map<String, Map<String, int>> _bitrateOptions = {
    'sd': {
      'Low (0.5 Mbps)': 500000,
      'Medium (1.0 Mbps)': 1000000,
      'High (2.0 Mbps)': 2000000,
    },
    'hd': {
      'Low (2.0 Mbps)': 2000000,
      'Medium (4.0 Mbps)': 4000000,
      'High (6.0 Mbps)': 6000000,
    }
  };

  @override
  void initState() {
    super.initState();
    _loadSettings();
  }

  Future<void> _loadSettings() async {
    final ip = await _settingsService.loadIpAddress();
    final quality = await _settingsService.loadVideoQuality();
    final bitrate = await _settingsService.loadVideoBitrate();
    final bitrateMode = await _settingsService.loadBitrateMode();
    final urls = await _settingsService.loadCameraUrls();

    if (mounted) {
      setState(() {
        _ipController = TextEditingController(text: ip);
        _selectedQuality = quality;
        _selectedBitrate = bitrate;
        _selectedBitrateMode = bitrateMode;
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

  Future<void> _saveAndReturn() async {
    await _settingsService.saveSettings(
      ipAddress: _ipController.text,
      dayCameraUrl: _dayUrlController.text,
      nightCameraUrl: _nightUrlController.text,
      videoQuality: _selectedQuality,
      videoBitrate: _selectedBitrate,
      bitrateMode: _selectedBitrateMode,
    );

    if (mounted) {
      Navigator.pop(context, true);
    }
  }

  @override
  Widget build(BuildContext context) {
    // Get the current list of bitrate options based on the selected quality
    final currentBitrateOptions = _bitrateOptions[_selectedQuality]!;

    if (!currentBitrateOptions.containsValue(_selectedBitrate)) {
      _selectedBitrate = currentBitrateOptions.values.first;
    }

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
                  // When quality changes, reset bitrate to the default for that quality
                  _selectedBitrate = _bitrateOptions[newValue]!.values.first;
                });
              }
            },
            decoration: const InputDecoration(border: OutlineInputBorder()),
          ),

          const SizedBox(height: 24),

          const Text('Bitrate Mode', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          DropdownButtonFormField<String>(
            value: _selectedBitrateMode,
            items: const [
              DropdownMenuItem(value: 'auto', child: Text('Auto (Default)')), // <-- ADD THIS
              DropdownMenuItem(value: 'cbr', child: Text('Constant Bitrate (CBR)')),
              DropdownMenuItem(value: 'vbr', child: Text('Variable Bitrate (VBR)')),
            ],
            onChanged: (String? newValue) {
              if (newValue != null) {
                setState(() { _selectedBitrateMode = newValue; });
              }
            },
            decoration: const InputDecoration(border: OutlineInputBorder()),
          ),
          const SizedBox(height: 24),

          const Text('Target Bitrate', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          DropdownButtonFormField<int>(
            value: _selectedBitrate,
            items: currentBitrateOptions.entries.map((entry) {
              return DropdownMenuItem<int>(
                value: entry.value,
                child: Text(entry.key),
              );
            }).toList(),
            onChanged: (int? newValue) {
              if (newValue != null) {
                setState(() {
                  _selectedBitrate = newValue;
                });
              }
            },
            decoration: const InputDecoration(border: OutlineInputBorder()),
          ),
          const SizedBox(height: 24),

          const Text('Day Camera (EO) URL', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          TextFormField(controller: _dayUrlController, decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., rtsp://.../cam0')),
          const SizedBox(height: 24),

          const Text('Night Camera (IR) URL', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          const SizedBox(height: 8),
          TextFormField(controller: _nightUrlController, decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., rtsp://.../cam1')),
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