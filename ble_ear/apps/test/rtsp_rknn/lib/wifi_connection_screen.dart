import 'package:flutter/material.dart';
import 'dart:async';

// Import the new package
import 'package:wifi_iot/wifi_iot.dart';

import 'main.dart'; // To navigate to the HomePage

// Enum to manage the different UI states cleanly
enum WifiConnectionState {
  initial,            // Shows "Connect WiFi"
  waitingForConnection, // Shows "Waiting for connection..." with a spinner
  connected,          // Shows "Connected" and "Go to Home" button
}

class WifiConnectionScreen extends StatefulWidget {
  const WifiConnectionScreen({super.key});

  @override
  State<WifiConnectionScreen> createState() => _WifiConnectionScreenState();
}

// Add WidgetsBindingObserver to detect when the user returns to the app
class _WifiConnectionScreenState extends State<WifiConnectionScreen> with WidgetsBindingObserver {
  WifiConnectionState _currentState = WifiConnectionState.initial;
  Timer? _pollingTimer;

  @override
  void initState() {
    super.initState();
    // Listen for app lifecycle changes (e.g., user returns from settings)
    WidgetsBinding.instance.addObserver(this);
    // Check the connection status as soon as the screen loads
    _checkConnectionStatus();
  }

  @override
  void dispose() {
    // Clean up the observer and timer to prevent memory leaks
    WidgetsBinding.instance.removeObserver(this);
    _pollingTimer?.cancel();
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    // When the user returns to the app, check the connection status again.
    if (state == AppLifecycleState.resumed) {
      _checkConnectionStatus();
    }
  }

  // --- REAL WIFI LOGIC using wifi_iot ---

  /// Checks the current connection status and updates the UI.
  Future<void> _checkConnectionStatus() async {
    bool isConnected = await WiFiForIoTPlugin.isConnected();
    if (isConnected) {
      if(mounted) {
        setState(() => _currentState = WifiConnectionState.connected);
      }
      _pollingTimer?.cancel();
    } else {
      // If we aren't connected and not already waiting, reset to initial.
      if (_currentState != WifiConnectionState.waitingForConnection) {
        if(mounted) {
          setState(() => _currentState = WifiConnectionState.initial);
        }
      }
    }
  }

  /// Opens the native WiFi settings and starts polling for a connection.
  Future<void> _openWifiSettingsAndPoll() async {
    // Update UI to show we are waiting
    setState(() => _currentState = WifiConnectionState.waitingForConnection);

    // This is the key function: It ensures WiFi is on and opens settings.
    await WiFiForIoTPlugin.setEnabled(true, shouldOpenSettings: true);

    // Start a timer that checks for a connection every 2 seconds.
    // This runs after the user comes back from the settings page.
    _pollingTimer?.cancel(); // Cancel any old timer
    _pollingTimer = Timer.periodic(const Duration(seconds: 2), (timer) {
      _checkConnectionStatus();
    });
  }

  void _navigateToHome() {
    Navigator.pushReplacement(
      context,
      MaterialPageRoute(builder: (context) => const HomePage()),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        fit: StackFit.expand,
        children: [
          Image.asset('assets/icons/02_Bluetooth_BG.png', fit: BoxFit.cover),
          Center(child: _buildContentForState()),
        ],
      ),
    );
  }

  Widget _buildContentForState() {
    switch (_currentState) {
      case WifiConnectionState.waitingForConnection:
        return _buildStateUI(
            iconPath: 'assets/icons/03_Main_sen4.png',
            buttonText: "Waiting for Connection...",
            onPressed: null, // Disable button while waiting
            showSpinner: true); // Show a loading spinner

      case WifiConnectionState.connected:
        return _buildStateUI(
          iconPath: 'assets/icons/02_Bluetooth_Reconnect_icon.png',
          buttonText: "Go to Home",
          onPressed: _navigateToHome,
        );

      case WifiConnectionState.initial:
      default:
        return _buildStateUI(
          iconPath: 'assets/icons/03_Main_sen4.png',
          buttonText: "Connect WiFi",
          onPressed: _openWifiSettingsAndPoll,
        );
    }
  }

  Widget _buildStateUI({
    required String iconPath,
    required String buttonText,
    required VoidCallback? onPressed,
    bool showSpinner = false,
  }) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        Container(
          padding: const EdgeInsets.all(20),
          decoration: BoxDecoration(
            shape: BoxShape.circle,
            color: Colors.white.withOpacity(0.9),
            boxShadow: [BoxShadow(color: Colors.black.withOpacity(0.3), blurRadius: 15)],
          ),
          child: showSpinner
              ? const SizedBox(
            height: 60,
            width: 60,
            child: Padding(
              padding: EdgeInsets.all(8.0),
              child: CircularProgressIndicator(strokeWidth: 3, color: Colors.black54),
            ),
          )
              : Image.asset(iconPath, height: 60, width: 60),
        ),
        const SizedBox(height: 40),
        GestureDetector(
          onTap: onPressed,
          child: Container(
            width: 280,
            padding: const EdgeInsets.symmetric(vertical: 14),
            decoration: BoxDecoration(
              color: onPressed == null ? Colors.grey.shade400.withOpacity(0.8) : Colors.white.withOpacity(0.9),
              borderRadius: BorderRadius.circular(30),
              border: Border.all(color: Colors.grey.shade400, width: 1.5),
            ),
            child: Center(
              child: Text(
                buttonText,
                style: const TextStyle(
                  color: Colors.black87,
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ),
          ),
        ),
      ],
    );
  }
}