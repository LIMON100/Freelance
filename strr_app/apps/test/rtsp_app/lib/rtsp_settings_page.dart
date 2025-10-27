import 'package:flutter/material.dart';

// Note: I've renamed the class to RtspSettingsPage for clarity.
class RtspSettingsPage extends StatefulWidget {
  final List<String> cameraUrls;
  const RtspSettingsPage({super.key, required this.cameraUrls});

  @override
  State<RtspSettingsPage> createState() => _RtspSettingsPageState();
}

class _RtspSettingsPageState extends State<RtspSettingsPage> {
  late List<TextEditingController> _urlControllers;

  @override
  void initState() {
    super.initState();
    _urlControllers =
        widget.cameraUrls.map((url) => TextEditingController(text: url)).toList();
  }

  @override
  void dispose() {
    for (var controller in _urlControllers) {
      controller.dispose();
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
          title: const Text('RTSP Settings'),
          backgroundColor: Colors.grey[200],
          foregroundColor: Colors.black87),
      body: ListView(
        padding: const EdgeInsets.all(16.0),
        children: [
          for (int i = 0; i < _urlControllers.length; i++)
            Padding(
              padding: const EdgeInsets.only(bottom: 16.0),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('Camera ${i + 1} URL',
                      style:
                      const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                  const SizedBox(height: 8),
                  TextFormField(
                    controller: _urlControllers[i],
                    decoration: const InputDecoration(
                        border: OutlineInputBorder(), hintText: 'e.g., rtsp://...'),
                  ),
                ],
              ),
            ),
          const SizedBox(height: 20),
          ElevatedButton(
            onPressed: () {
              final newUrls =
              _urlControllers.map((controller) => controller.text).toList();
              // When popping, send the new URLs back to the page that opened this one.
              Navigator.pop(context, newUrls);
            },
            style: ElevatedButton.styleFrom(
              padding: const EdgeInsets.symmetric(vertical: 16.0),
              backgroundColor: Theme.of(context).primaryColor,
              foregroundColor: Colors.white,
            ),
            child: const Text('Save Changes'),
          ),
        ],
      ),
    );
  }
}