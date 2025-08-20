import 'dart:typed_data';

class UserCommand {
  int commandId = 0;
  int moveSpeed = 0;
  int turnAngle = 0;
  bool gunTrigger = false;
  bool gunPermission = false;
  // We use -1.0 as a sentinel value to indicate "no touch event"
  double touchX = -1.0;
  double touchY = -1.0;

  Uint8List toBytes() {
    final buffer = ByteData(13); // 5 bytes for ints/bools + 8 for floats
    buffer.setUint8(0, commandId);
    buffer.setInt8(1, moveSpeed);
    buffer.setInt8(2, turnAngle);
    buffer.setUint8(3, gunTrigger ? 1 : 0);
    buffer.setUint8(4, gunPermission ? 1 : 0);
    buffer.setFloat32(5, touchX, Endian.little);
    buffer.setFloat32(9, touchY, Endian.little);
    return buffer.buffer.asUint8List();
  }
}