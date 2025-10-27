import 'dart:typed_data';

class StatusPacket {
  final int currentModeId;
  final bool isRtspRunning;
  final bool isDrivingThreadActive;
  final double lateralWindSpeed;
  final int windDirectionIndex;

  StatusPacket({
    required this.currentModeId,
    required this.isRtspRunning,
    required this.isDrivingThreadActive,
    required this.lateralWindSpeed,
    required this.windDirectionIndex,
  });

  // Factory constructor to create a StatusPacket from a byte buffer
  factory StatusPacket.fromBytes(Uint8List bytes) {
    if (bytes.length < 7) { // 1+1+1+4 = 7 bytes
      throw ArgumentError("Invalid byte list for StatusPacket");
    }
    var byteData = ByteData.sublistView(bytes);
    return StatusPacket(
      currentModeId: byteData.getUint8(0),
      isRtspRunning: byteData.getUint8(1) == 1,
      isDrivingThreadActive: byteData.getUint8(2) == 1,
      lateralWindSpeed: byteData.getFloat32(3, Endian.little),
      windDirectionIndex: byteData.getUint8(7),
    );
  }
}