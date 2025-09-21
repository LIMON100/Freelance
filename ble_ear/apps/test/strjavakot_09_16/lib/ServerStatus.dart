import 'dart:typed_data';
import 'CommandIds.dart';

class ServerStatus {
  final int currentModeId;
  final bool isRtspRunning;
  final bool isDrivingThreadActive;
  final double lateralWindSpeed;
  final int windDirectionIndex;

  // Default "disconnected" state
  ServerStatus.disconnected()
      : currentModeId = CommandIds.IDLE,
        isRtspRunning = false,
        isDrivingThreadActive = false,
        lateralWindSpeed = 0.0,
        windDirectionIndex = 0;

  // Constructor from server data
  ServerStatus({
    required this.currentModeId,
    required this.isRtspRunning,
    required this.isDrivingThreadActive,
    required this.lateralWindSpeed,
    required this.windDirectionIndex,
  });

  factory ServerStatus.fromBytes(Uint8List bytes) {
    if (bytes.length < 8) { // Updated size check
      throw ArgumentError("Invalid byte list for ServerStatus");
    }
    var byteData = ByteData.sublistView(bytes);
    return ServerStatus(
      currentModeId: byteData.getUint8(0),
      isRtspRunning: byteData.getUint8(1) == 1,
      isDrivingThreadActive: byteData.getUint8(2) == 1,
      lateralWindSpeed: byteData.getFloat32(3, Endian.little),
      windDirectionIndex: byteData.getUint8(7),
    );
  }
}