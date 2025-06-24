// lib/screens/settings_screen.dart
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:rtsp_rknn/state/app_state.dart';

class SettingsScreen extends StatefulWidget {
  @override
  _SettingsScreenState createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  // Create a controller for each input field
  late final TextEditingController _ipController;
  late final TextEditingController _cam1UrlController;
  late final TextEditingController _cam2UrlController;
  late final TextEditingController _cam3UrlController;

  @override
  void initState() {
    super.initState();
    // Initialize controllers with the current values from AppState
    final appState = Provider.of<AppState>(context, listen: false);
    _ipController = TextEditingController(text: appState.robotIpAddress);
    _cam1UrlController = TextEditingController(text: appState.cameraUrls[0]);
    _cam2UrlController = TextEditingController(text: appState.cameraUrls[1]);
    _cam3UrlController = TextEditingController(text: appState.cameraUrls[2]);
  }

  @override
  void dispose() {
    // Clean up controllers when the widget is removed
    _ipController.dispose();
    _cam1UrlController.dispose();
    _cam2UrlController.dispose();
    _cam3UrlController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final appState = Provider.of<AppState>(context, listen: false);

    return Scaffold(
      appBar: AppBar(title: Text("Settings")),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text("Command Settings", style: Theme.of(context).textTheme.titleLarge),
            SizedBox(height: 8),
            TextField(
              controller: _ipController,
              decoration: InputDecoration(
                labelText: 'Robot ECU IP Address (for commands)',
                border: OutlineInputBorder(),
              ),
            ),
            SizedBox(height: 24),
            Text("RTSP Camera URLs", style: Theme.of(context).textTheme.titleLarge),
            SizedBox(height: 16),
            TextField(
              controller: _cam1UrlController,
              decoration: InputDecoration(
                labelText: 'Camera 1 URL',
                border: OutlineInputBorder(),
              ),
            ),
            SizedBox(height: 16),
            TextField(
              controller: _cam2UrlController,
              decoration: InputDecoration(
                labelText: 'Camera 2 URL',
                border: OutlineInputBorder(),
              ),
            ),
            SizedBox(height: 16),
            TextField(
              controller: _cam3UrlController,
              decoration: InputDecoration(
                labelText: 'Camera 3 URL',
                border: OutlineInputBorder(),
              ),
            ),
            SizedBox(height: 30),
            SizedBox(
              width: double.infinity,
              child: ElevatedButton(
                onPressed: () {
                  // Call the updated updateSettings method
                  appState.updateSettings(
                    newIp: _ipController.text,
                    newCam1Url: _cam1UrlController.text,
                    newCam2Url: _cam2UrlController.text,
                    newCam3Url: _cam3UrlController.text,
                  );
                  ScaffoldMessenger.of(context).showSnackBar(
                    SnackBar(content: Text('Settings Saved!')),
                  );
                  Navigator.pop(context);
                },
                child: Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: Text('Save Settings'),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}