import 'dart:typed_data';
import 'dart:io';

class StatusPacket {
  final int currentModeId;
  final bool isRtspRunning;
  final int permissionRequestActive;
  final double crosshairX;
  final double crosshairY;

  StatusPacket({
    required this.currentModeId,
    required this.isRtspRunning,
    required this.permissionRequestActive,
    required this.crosshairX,
    required this.crosshairY,
  });

  factory StatusPacket.fromBytes(Uint8List bytes) {
    if (bytes.length < 11) {
      throw ArgumentError("Invalid byte list for StatusPacket, expected 11 bytes but got ${bytes.length}");
    }

    var byteData = ByteData.sublistView(bytes);

    return StatusPacket(
      isRtspRunning: byteData.getUint8(0) == 1,
      currentModeId: byteData.getUint8(1),
      permissionRequestActive: byteData.getUint8(2),
      crosshairX: byteData.getFloat32(3, Endian.little),
      crosshairY: byteData.getFloat32(7, Endian.little),
    );
  }
}