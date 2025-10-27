import 'package:flutter/material.dart';

// Import the RTSP settings page we just created.
import 'rtsp_settings_page.dart';

class SettingsMenuPage extends StatelessWidget {
  // This page needs the camera URLs to pass them along to the next page.
  final List<String> cameraUrls;
  const SettingsMenuPage({super.key, required this.cameraUrls});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Settings'),
        backgroundColor: Colors.grey[200],
        foregroundColor: Colors.black87,
      ),
      body: ListView(
        children: [
          // Option 1: RTSP Settings
          ListTile(
            leading: const Icon(Icons.router_outlined, size: 30),
            title: const Text('RTSP Settings', style: TextStyle(fontSize: 18)),
            subtitle: const Text('Configure camera stream URLs'),
            trailing: const Icon(Icons.arrow_forward_ios),
            onTap: () async {
              // Navigate to the RtspSettingsPage and wait for a result.
              final result = await Navigator.push(
                context,
                MaterialPageRoute(
                  builder: (context) => RtspSettingsPage(cameraUrls: cameraUrls),
                ),
              );

              // If the RtspSettingsPage returns data (the new URLs),
              // then pop this menu page and pass the data back to the HomePage.
              if (result != null && context.mounted) {
                Navigator.pop(context, result);
              }
            },
          ),
          const Divider(),

          // Option 2: WiFi Settings (Placeholder)
          ListTile(
            leading: const Icon(Icons.wifi, size: 30),
            title: const Text('WiFi Settings', style: TextStyle(fontSize: 18)),
            subtitle: const Text('Connect to a WiFi network'),
            trailing: const Icon(Icons.arrow_forward_ios),
            onTap: () {
              // For now, this does nothing but show a message.
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(
                  content: Text(''),
                  duration: Duration(seconds: 2),
                ),
              );
            },
          ),
          const Divider(),
        ],
      ),
    );
  }
}