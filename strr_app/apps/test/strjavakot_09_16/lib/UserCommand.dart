import 'dart:typed_data';

class UserCommand {
  // --- State variables for on-screen controls ---
  int move_speed;   // For left joystick
  int turn_angle;   // For left joystick

  // --- State variables that get sent over TCP ---
  int command_id;
  bool attack_permission;
  int pan_speed;    // For right joystick
  int tilt_speed;   // For right joystick
  int zoom_command;
  // double touch_x;
  // double touch_y;
  double lateral_wind_speed;

  UserCommand({
    this.move_speed = 0, // Add default value
    this.turn_angle = 0, // Add default value
    this.command_id = 0,
    this.attack_permission = false,
    this.pan_speed = 0,
    this.tilt_speed = 0,
    this.zoom_command = 0,
    // this.touch_x = -1.0,
    // this.touch_y = -1.0,
    this.lateral_wind_speed = 0.0,
  });

  // This method serializes ONLY the state data into a 13-byte packet
  Uint8List toBytes() {
    final builder = BytesBuilder();
    builder.addByte(command_id);
    builder.addByte(attack_permission ? 1 : 0);
    builder.addByte(Int8List.fromList([pan_speed]).buffer.asUint8List()[0]);
    builder.addByte(Int8List.fromList([tilt_speed]).buffer.asUint8List()[0]);
    builder.addByte(Int8List.fromList([zoom_command]).buffer.asUint8List()[0]);

    // final floatData = ByteData(8);
    // floatData.setFloat32(0, touch_x, Endian.little);
    // floatData.setFloat32(4, touch_y, Endian.little);
    // builder.add(floatData.buffer.asUint8List());
    //
    // return builder.toBytes();
    // Add the 4-byte float for wind speed
    final floatData = ByteData(4);
    floatData.setFloat32(0, lateral_wind_speed, Endian.little);
    builder.add(floatData.buffer.asUint8List());

    return builder.toBytes();
  }
}