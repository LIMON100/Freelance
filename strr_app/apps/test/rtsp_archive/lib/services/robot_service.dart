// lib/services/robot_service.dart
import 'dart:io';
import 'dart:convert';

class RobotService {
  final String ipAddress;
  final int port; // You'll need to know the port the ECU listens on
  Socket? _socket;

  RobotService({required this.ipAddress, this.port = 9999}); // Example port

  Future<void> connect() async {
    try {
      _socket = await Socket.connect(ipAddress, port, timeout: Duration(seconds: 5));
      print('Connected to: ${_socket?.remoteAddress.address}:${_socket?.remotePort}');
    } catch (e) {
      print("Connection failed: $e");
      _socket = null;
    }
  }

  void sendCommand(String command) {
    if (_socket != null) {
      print("Sending command: $command");
      // The ECU expects a specific format. A command string followed by a newline is common.
      _socket?.writeln(command);
    } else {
      print("Cannot send command: not connected.");
      // Optionally, try to reconnect
      // connect();
    }
  }

  void dispose() {
    _socket?.close();
  }
}