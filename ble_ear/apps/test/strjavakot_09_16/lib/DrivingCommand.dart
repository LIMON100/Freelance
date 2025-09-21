import 'dart:typed_data';

class DrivingCommand {
  int move_speed;
  int turn_angle;

  DrivingCommand({
    this.move_speed = 0,
    this.turn_angle = 0,
  });

  // This method serializes the object into a 2-byte packet
  Uint8List toBytes() {
    final builder = BytesBuilder();
    // Add move_speed as a signed 8-bit integer
    builder.addByte(Int8List.fromList([move_speed]).buffer.asUint8List()[0]);
    // Add turn_angle as a signed 8-bit integer
    builder.addByte(Int8List.fromList([turn_angle]).buffer.asUint8List()[0]);
    return builder.toBytes();
  }
}