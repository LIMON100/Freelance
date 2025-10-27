import 'dart:typed_data';

class TouchCoord {
  double x = 0.0;
  double y = 0.0;

  Uint8List toBytes() {
    final buffer = ByteData(8); // 2 floats = 8 bytes
    buffer.setFloat32(0, x, Endian.little);
    buffer.setFloat32(4, y, Endian.little);
    return buffer.buffer.asUint8List();
  }
}