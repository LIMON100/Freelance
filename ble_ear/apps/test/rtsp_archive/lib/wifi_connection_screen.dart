import 'package:flutter/material.dart';
import 'dart:async';

// Import the package
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

class _WifiConnectionScreenState extends State<WifiConnectionScreen> with WidgetsBindingObserver {
  WifiConnectionState _currentState = WifiConnectionState.initial;
  Timer? _pollingTimer;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _checkConnectionStatus();
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    _pollingTimer?.cancel();
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) {
      _checkConnectionStatus();
    }
  }

  Future<void> _checkConnectionStatus() async {
    bool isConnected = await WiFiForIoTPlugin.isConnected();
    if (isConnected) {
      if(mounted) {
        setState(() => _currentState = WifiConnectionState.connected);
      }
      _pollingTimer?.cancel();
    } else {
      if (_currentState != WifiConnectionState.waitingForConnection) {
        if(mounted) {
          setState(() => _currentState = WifiConnectionState.initial);
        }
      }
    }
  }

  Future<void> _openWifiSettingsAndPoll() async {
    setState(() => _currentState = WifiConnectionState.waitingForConnection);
    await WiFiForIoTPlugin.setEnabled(true, shouldOpenSettings: true);
    _pollingTimer?.cancel();
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

  // --- MODIFIED: This function now passes the status text ---
  Widget _buildContentForState() {
    switch (_currentState) {
      case WifiConnectionState.waitingForConnection:
        return _buildStateUI(
            iconPath: 'assets/icons/03_Main_sen4.png',
            statusText: "Waiting for Connection...", // Status text for this state
            buttonText: "Open Settings Again",
            onPressed: _openWifiSettingsAndPoll,
            showSpinner: true);

      case WifiConnectionState.connected:
        return _buildStateUI(
          iconPath: 'assets/icons/02_Bluetooth_Reconnect_icon.png',
          statusText: "WiFi Connected", // Status text for this state
          buttonText: "Go to Home",
          onPressed: _navigateToHome,
        );

      case WifiConnectionState.initial:
      default:
        return _buildStateUI(
          iconPath: 'assets/icons/03_Main_sen4.png',
          statusText: "Please connect WiFi", // Status text for this state
          buttonText: "Connect WiFi",
          onPressed: _openWifiSettingsAndPoll,
        );
    }
  }

  // --- MODIFIED: This widget now includes the status text ---
  Widget _buildStateUI({
    required String iconPath,
    required String statusText, // New parameter for the status text
    required String buttonText,
    required VoidCallback? onPressed,
    bool showSpinner = false,
  }) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        // The main icon
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

        // --- NEW WIDGET: The status text ---
        Text(
          statusText,
          style: const TextStyle(
            color: Colors.white,
            fontSize: 20,
            fontWeight: FontWeight.w500,
            shadows: [
              Shadow(
                blurRadius: 4.0,
                color: Colors.black,
                offset: Offset(1.0, 1.0),
              ),
            ],
          ),
        ),
        const SizedBox(height: 20), // Spacing between status text and button

        // The main button
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