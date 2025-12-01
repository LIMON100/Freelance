import 'dart:io';
import 'package:logger/logger.dart';

class RemoteLogOutput extends LogOutput {
  final String remoteAddress;
  final int remotePort;
  RawDatagramSocket? _socket;

  RemoteLogOutput(this.remoteAddress, this.remotePort);

  @override
  Future<void> init() async {
    try {
      // Bind the socket once. We reuse it for all log messages.
      _socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
      // Use a standard print here, as the logger isn't fully ready yet.
      print("Remote logger initialized. Sending logs to $remoteAddress:$remotePort");
    } catch (e) {
      print("Failed to initialize remote logger: $e");
    }
  }

  @override
  void output(OutputEvent event) {
    if (_socket == null) return;

    // We take the formatted log lines and send them.
    for (var line in event.lines) {
      // Convert the string to bytes and send it over UDP.
      _socket?.send(line.codeUnits, InternetAddress(remoteAddress), remotePort);
    }
  }

  @override
  Future<void> destroy() async {
    _socket?.close();
    _socket = null;
    print("Remote logger destroyed.");
  }
}