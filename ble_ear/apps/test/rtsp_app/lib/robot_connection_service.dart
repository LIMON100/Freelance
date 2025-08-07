import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

// --- CONFIGURATION (from main.dart) ---
const String ROBOT_IP_ADDRESS = '192.168.0.158';
const int ROBOT_COMMAND_PORT = 65432;

// --- DART CLASSES FOR DATA STRUCTURES ---
class UserCommand {
  int commandId = 1;
  int moveSpeed = 0;
  int turnAngle = 0;
  bool gunTrigger = false;
  bool gunPermission = true;

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

  // Factory to create a RobotState object from the 2-byte packet
  factory RobotState.fromBytes(Uint8List bytes) {
    if (bytes.length < 2) {
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
  Socket? _socket;
  final StreamController<RobotState> _stateController = StreamController<RobotState>.broadcast();
  Timer? _reconnectTimer;

  // Public stream for the UI to listen to
  Stream<RobotState> get stateStream => _stateController.stream;

  RobotConnectionService() {
    _connect();
  }

  void _connect() async {
    if (_socket != null) return; // Already connected or connecting

    print("Service: Trying to connect...");
    try {
      _socket = await Socket.connect(ROBOT_IP_ADDRESS, ROBOT_COMMAND_PORT, timeout: const Duration(seconds: 3));
      print("Service: Connected to ${_socket!.remoteAddress.address}");

      // Listen for incoming data from the robot
      _socket!.listen(
            (Uint8List data) {
          final robotState = RobotState.fromBytes(data);
          _stateController.add(robotState); // Push the new state to the UI
        },
        onError: (error) {
          print("Service: Connection error: $error");
          _disconnect();
        },
        onDone: () {
          print("Service: Server closed the connection.");
          _disconnect();
        },
        cancelOnError: true,
      );
      // Stop trying to reconnect if successful
      _reconnectTimer?.cancel();
    } catch (e) {
      print("Service: Failed to connect: $e");
      _disconnect();
    }
  }

  void _disconnect() {
    _socket?.destroy();
    _socket = null;
    // Try to reconnect every 5 seconds
    _reconnectTimer ??= Timer.periodic(const Duration(seconds: 5), (_) => _connect());
  }

  // Method for the UI to send commands
  void sendCommand(UserCommand command) {
    if (_socket != null) {
      _socket!.add(command.toBytes());
    } else {
      print("Service: Not connected. Command not sent.");
    }
  }

  void dispose() {
    _reconnectTimer?.cancel();
    _socket?.destroy();
    _stateController.close();
  }
}