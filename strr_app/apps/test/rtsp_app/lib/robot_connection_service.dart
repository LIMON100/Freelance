import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

// --- CONFIGURATION ---
const String ROBOT_IP_ADDRESS = '192.168.0.158';
const int ROBOT_COMMAND_PORT = 65432;

// --- DART CLASSES FOR DATA STRUCTURES ---
class UserCommand {
  int commandId = 0; // Default to Idle
  int moveSpeed = 0;
  int turnAngle = 0;
  bool gunTrigger = false;
  bool gunPermission = false;

  Uint8List toBytes() {
    final buffer = ByteData(5);
    buffer.setUint8(0, commandId);
    buffer.setInt8(1, moveSpeed);
    buffer.setInt8(2, turnAngle);
    buffer.setUint8(3, gunTrigger ? 1 : 0);
    buffer.setUint8(4, gunPermission ? 1 : 0);
    return buffer.buffer.asUint8List();
  }
}

class RobotState {
  final int currentState;
  final bool gunTriggerPermissionRequest;

  RobotState({required this.currentState, required this.gunTriggerPermissionRequest});

  factory RobotState.fromBytes(Uint8List bytes) {
    if (bytes.length < 2) {
      print("Warning: Received incomplete RobotState packet.");
      return RobotState(currentState: 0, gunTriggerPermissionRequest: false);
    }
    final buffer = ByteData.sublistView(bytes);
    return RobotState(
      currentState: buffer.getUint8(0),
      gunTriggerPermissionRequest: buffer.getUint8(1) == 1,
    );
  }
}

// --- THE CONNECTION SERVICE ---
class RobotConnectionService {
  Socket? _commandSocket;
  final StreamController<RobotState> _stateController = StreamController<RobotState>.broadcast();
  Timer? _reconnectTimer;
  bool _isConnecting = false;

  Stream<RobotState> get stateStream => _stateController.stream;

  RobotConnectionService() {
    _connect();
  }

  void _connect() async {
    if (_commandSocket != null || _isConnecting) return;
    _isConnecting = true;
    print("Service: Trying to connect...");
    try {
      _commandSocket = await Socket.connect(ROBOT_IP_ADDRESS, ROBOT_COMMAND_PORT, timeout: const Duration(seconds: 3));
      print("Service: Connected to ${_commandSocket!.remoteAddress.address}");
      _isConnecting = false;

      _commandSocket!.listen(
            (Uint8List data) {
          final robotState = RobotState.fromBytes(data);
          _stateController.add(robotState);
        },
        onError: (error) => _disconnect("Connection error: $error"),
        onDone: () => _disconnect("Server closed connection."),
        cancelOnError: true,
      );
      _reconnectTimer?.cancel();
      _reconnectTimer = null;
    } catch (e) {
      _isConnecting = false;
      _disconnect("Failed to connect: $e");
    }
  }

  void _disconnect(String reason) {
    print("Service: Disconnecting. Reason: $reason");
    _commandSocket?.destroy();
    _commandSocket = null;
    _reconnectTimer ??= Timer.periodic(const Duration(seconds: 5), (_) => _connect());
  }

  void sendCommand(UserCommand command) {
    if (_commandSocket != null) {
      try {
        _commandSocket!.add(command.toBytes());
      } catch (e) {
        _disconnect("Error sending command: $e");
      }
    }
  }

  void dispose() {
    _reconnectTimer?.cancel();
    _commandSocket?.destroy();
    _stateController.close();
  }
}