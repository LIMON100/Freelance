import 'dart:typed_data';

class UserCommand {
  int move_speed;   // For left joystick
  int turn_angle;   // For left joystick

  // --- State variables that get sent over TCP ---
  int command_id;
  bool attack_permission;
  int pan_speed;    // For right joystick
  int tilt_speed;   // For right joystick
  int zoom_command;
  double lateral_wind_speed;
  int resolution_setting;

  int bitrate_mode;       // 0 for CBR, 1 for VBR
  int target_bitrate;     // Bitrate in bp


  UserCommand({
    this.move_speed = 0, 
    this.turn_angle = 0, 
    this.command_id = 0,
    this.attack_permission = false,
    this.pan_speed = 0,
    this.tilt_speed = 0,
    this.zoom_command = 0,
    this.lateral_wind_speed = 0.0,
    this.resolution_setting = 0,
    this.bitrate_mode = 0,       // Default to CBR
    this.target_bitrate = 2000000, // Default to 2 Mbps
  });

  // This method serializes ONLY the state data into a 13-byte packet
  Uint8List toBytes() {
    final builder = BytesBuilder();
    builder.addByte(command_id);
    builder.addByte(attack_permission ? 1 : 0);
    builder.addByte(Int8List.fromList([pan_speed]).buffer.asUint8List()[0]);
    builder.addByte(Int8List.fromList([tilt_speed]).buffer.asUint8List()[0]);
    builder.addByte(Int8List.fromList([zoom_command]).buffer.asUint8List()[0]);

    final floatData = ByteData(4);
    floatData.setFloat32(0, lateral_wind_speed, Endian.little);
    builder.add(floatData.buffer.asUint8List());

    builder.addByte(resolution_setting);

    builder.addByte(bitrate_mode);
    final intData = ByteData(4);
    intData.setUint32(0, target_bitrate, Endian.little);
    builder.add(intData.buffer.asUint8List());

    return builder.toBytes();
  }
}