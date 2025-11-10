// import 'dart:typed_data';
//
// class StatusPacket {
//   final int currentModeId;
//   final bool isRtspRunning;
//   final double lateralWindSpeed;
//   final int windDirectionIndex;
//   final int permissionRequestActive;
//   final double crosshairX; // NEW
//   final double crosshairY; // NEW
//
//   StatusPacket({
//     required this.currentModeId,
//     required this.isRtspRunning,
//     required this.lateralWindSpeed,
//     required this.windDirectionIndex,
//     required this.permissionRequestActive,
//     required this.crosshairX,
//     required this.crosshairY,
//   });
//
//   // Factory constructor to create a StatusPacket from a byte buffer
//   factory StatusPacket.fromBytes(Uint8List bytes) {
//     // The packet should be exactly 7 bytes long
//     if (bytes.length < 7) {
//       throw ArgumentError("Invalid byte list for StatusPacket, expected 7 bytes but got ${bytes.length}");
//     }
//
//     var byteData = ByteData.sublistView(bytes);
//
//     // --- THIS IS THE CORRECT PARSING LOGIC ---
//     return StatusPacket(
//       // C++ offset 0 -> rtsp_server_status
//       isRtspRunning: byteData.getUint8(0) == 1,
//
//       // C++ offset 1 -> active_mode_id
//       currentModeId: byteData.getUint8(1),
//
//       // C++ offset 2 -> lateral_wind_speed (4 bytes)
//       lateralWindSpeed: byteData.getFloat32(2, Endian.little),
//
//       // C++ offset 6 -> wind_direction_index
//       windDirectionIndex: byteData.getUint8(6),
//
//       permissionRequestActive: byteData.getUint8(7),
//
//       crosshairX: byteData.getFloat32(8, Endian.little), // Offset 8
//       crosshairY: byteData.getFloat32(12, Endian.little), // Offset 12
//     );
//   }
// }


import 'dart:typed_data';
import 'dart:io';

class StatusPacket {
  final int currentModeId;
  final bool isRtspRunning;
  final int permissionRequestActive;
  final double crosshairX;
  final double crosshairY;

  // REMOVED: lateralWindSpeed and windDirectionIndex are gone

  StatusPacket({
    required this.currentModeId,
    required this.isRtspRunning,
    required this.permissionRequestActive,
    required this.crosshairX,
    required this.crosshairY,
  });

  factory StatusPacket.fromBytes(Uint8List bytes) {
    // The new packet size is 1 (rtsp) + 1 (mode) + 1 (perm) + 4 (x) + 4 (y) = 11 bytes
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