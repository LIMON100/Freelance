import 'package:flutter/material.dart';
import 'dart:async';

import 'main.dart'; // To navigate to the HomePage

// Enum to manage the different UI states cleanly
enum WifiConnectionState {
  initial,      // Shows "Connect WiFi"
  connecting,   // Shows "Connecting..." with progress
  connected,    // Shows "Connected" and "Go to Home" button
  error         // Shows "Error" and "Reconnect" button
}

class WifiConnectionScreen extends StatefulWidget {
  const WifiConnectionScreen({super.key});

  @override
  State<WifiConnectionScreen> createState() => _WifiConnectionScreenState();
}

class _WifiConnectionScreenState extends State<WifiConnectionScreen> {
  // Start in the initial state
  WifiConnectionState _currentState = WifiConnectionState.initial;
  String _statusMessage = "Connect WiFi";

  // --- MOCK WIFI LOGIC ---
  // In a real app, this would use a package like 'wifi_scan'
  void _startConnectionProcess() {
    setState(() {
      _currentState = WifiConnectionState.connecting;
      _statusMessage = "Connecting WiFi...";
    });

    // Simulate a network connection attempt (e.g., 3 seconds)
    Timer(const Duration(seconds: 3), () {
      // In a real app, you would check the result of the connection attempt.
      // Here, we'll just pretend it succeeded.
      // To test the error state, you can change this to `_handleConnectionError()`.
      _handleConnectionSuccess();
    });
  }

  void _handleConnectionSuccess() {
    if (mounted) {
      setState(() {
        _currentState = WifiConnectionState.connected;
        _statusMessage = "WiFi Connected";
      });
    }
  }

  void _handleConnectionError() {
    if (mounted) {
      setState(() {
        _currentState = WifiConnectionState.error;
        _statusMessage = "Connection Failed";
      });
    }
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
          // 1. Full-screen background image
          Image.asset(
            'assets/icons/02_Bluetooth_BG.png',
            fit: BoxFit.cover,
          ),

          // 2. Centered content based on the current state
          Center(
            child: _buildContentForState(),
          ),
        ],
      ),
    );
  }

  // This widget dynamically builds the UI based on the _currentState
  Widget _buildContentForState() {
    switch (_currentState) {
      case WifiConnectionState.connecting:
        return _buildStateUI(
          iconPath: 'assets/icons/03_Main_sen4.png', // As per your "Connecting" image
          buttonText: "Connecting...",
          onPressed: null, // Disable button while connecting
        );
      case WifiConnectionState.connected:
        return _buildStateUI(
          iconPath: 'assets/icons/02_Bluetooth_Reconnect_icon.png', // Show success with WiFi icon
          buttonText: "Go to Home",
          onPressed: _navigateToHome,
        );
      case WifiConnectionState.error:
        return _buildStateUI(
          iconPath: 'assets/icon_error.png',
          buttonText: "Reconnect WiFi",
          onPressed: _startConnectionProcess,
        );
      case WifiConnectionState.initial:
      default:
        return _buildStateUI(
          iconPath: 'assets/icons/03_Main_sen4.png',
          buttonText: "Connect WiFi",
          onPressed: _startConnectionProcess,
        );
    }
  }

  // A generic helper to build the common UI structure (Icon + Button)
  Widget _buildStateUI({required String iconPath, required String buttonText, required VoidCallback? onPressed}) {
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
          child: Image.asset(iconPath, height: 60, width: 60),
        ),
        const SizedBox(height: 40),

        // The main button
        GestureDetector(
          onTap: onPressed,
          child: Container(
            width: 280,
            padding: const EdgeInsets.symmetric(vertical: 14),
            decoration: BoxDecoration(
              color: Colors.white.withOpacity(0.9),
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