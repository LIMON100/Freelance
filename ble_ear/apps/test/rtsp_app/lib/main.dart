import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_vlc_player/flutter_vlc_player.dart';
import 'package:google_fonts/google_fonts.dart';

import 'icon_constants.dart';
import 'splash_screen.dart';
import 'settings_menu_page.dart';

// --- CONFIGURATION FOR ROBOT CONNECTION ---
const String ROBOT_IP_ADDRESS = '192.168.0.158';
const int ROBOT_COMMAND_PORT = 65432;

// --- MAIN APP ENTRY POINT ---
void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setEnabledSystemUIMode(SystemUiMode.manual, overlays: []);
  SystemChrome.setPreferredOrientations([
    DeviceOrientation.landscapeLeft,
    DeviceOrientation.landscapeRight,
  ]);
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: '',
      theme: ThemeData(
        primarySwatch: Colors.blue,
        textTheme: GoogleFonts.rajdhaniTextTheme(),
      ),
      home: const SplashScreen(),
    );
  }
}

// --- DART CLASS REPRESENTING THE COMMAND PACKET ---
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

// --- HOME PAGE WIDGET ---
class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  // Video and UI State
  VlcPlayerController? _vlcPlayerController;
  List<String> _cameraUrls = [
    'rtsp://192.168.0.158:8554/cam0',
    'rtsp://192.168.0.158:8554/cam0',
    'rtsp://192.168.0.158:8554/cam0',
  ];
  int _currentCameraIndex = -1;
  String? _errorMessage;
  int _activeLeftButtonIndex = 0;
  int _activeRightButtonIndex = 0;
  bool _isAutoAttackMode = false;
  bool _isAttackModeOn = false;
  final List<Color> _attackInactiveColors = [const Color(0xffc32121), const Color(0xff831616)];
  final List<Color> _attackActiveColors = [const Color(0xFF424242), const Color(0xFF212121)];

  // Command & Control State
  UserCommand _currentCommand = UserCommand();
  bool _isForwardPressed = false;
  bool _isBackPressed = false;
  bool _isLeftPressed = false;
  bool _isRightPressed = false;
  bool _isStarted = false; // For START/STOP button state
  bool _isGunTriggerPressed = false;


  static const platform = MethodChannel('com.yourcompany/gamepad');
  bool _gamepadConnected = false;
  // Command & Control State
  Timer? _commandTimer;
  Timer? _joystickIdleTimer;


  @override
  void initState() {
    super.initState();
    _switchCamera(0);
    // --- NEW: Initialize the native plugin listener ---
    platform.setMethodCallHandler(_handleGamepadEvent);
  }

  @override
  void dispose() {
    // _commandTimer?.cancel();
    _joystickIdleTimer?.cancel();
    _vlcPlayerController?.dispose();
    super.dispose();
  }

  // Future<void> _handleGamepadEvent(MethodCall call) async {
  //   if (!mounted) return;
  //
  //   // We can infer the gamepad is connected if we get any event.
  //   if (!_gamepadConnected) {
  //     setState(() => _gamepadConnected = true);
  //     print("Gamepad connected (via native plugin)");
  //   }
  //
  //   // A flag to check if we need to send a packet
  //   bool needsSend = false;
  //
  //   switch (call.method) {
  //     case "onMotionEvent":
  //       final Map<dynamic, dynamic> axes = call.arguments;
  //
  //       // --- This logic correctly implements the spec from your client and the Android docs ---
  //
  //       // HORIZONTAL MOVEMENT (turn_left_right_angle)
  //       // Check for left stick X-axis, fall back to right stick X-axis (Z), fall back to D-pad hat X-axis.
  //       double horizontalValue = axes['AXIS_X'] ?? axes['AXIS_Z'] ?? axes['AXIS_HAT_X'] ?? 0.0;
  //       int newAngle = (horizontalValue * 100).round();
  //       if(newAngle.abs() < 15) newAngle = 0; // Deadzone
  //
  //       // VERTICAL MOVEMENT (move_front_back_speed)
  //       // Check for left stick Y-axis, fall back to right stick Y-axis (RZ), fall back to D-pad hat Y-axis.
  //       double verticalValue = axes['AXIS_Y'] ?? axes['AXIS_RZ'] ?? axes['AXIS_HAT_Y'] ?? 0.0;
  //       int newSpeed = (verticalValue * -100).round(); // Invert Y-axis for standard controls
  //       if(newSpeed.abs() < 15) newSpeed = 0; // Deadzone
  //
  //       // GUN TRIGGER
  //       // Check for right trigger, fall back to left trigger.
  //       double triggerValue = axes['AXIS_RTRIGGER'] ?? axes['AXIS_LTRIGGER'] ?? 0.0;
  //       bool newTrigger = triggerValue > 0.5;
  //
  //       // Only update the command and mark for sending if a value actually changed.
  //       if (newAngle != _currentCommand.turnAngle || newSpeed != _currentCommand.moveSpeed || newTrigger != _currentCommand.gunTrigger) {
  //         _currentCommand.turnAngle = newAngle;
  //         _currentCommand.moveSpeed = newSpeed;
  //         _currentCommand.gunTrigger = newTrigger;
  //         needsSend = true;
  //       }
  //       break;
  //
  //   // We can add button handling here later if needed
  //     case "onButtonDown":
  //     // Example: if (call.arguments['button'] == 'KEYCODE_BUTTON_A') { ... }
  //       break;
  //     case "onButtonUp":
  //       break;
  //   }
  //
  //   // If any axis value changed, send the updated packet immediately.
  //   if(needsSend) {
  //     _sendCommandPacket();
  //   }
  // }
  //
  // Future<void> _sendCommandPacket({
  //   int? commandId,
  //   int? moveSpeed,
  //   int? turnAngle,
  //   bool? gunTrigger,
  //   bool? gunPermission,
  // }) async {
  //   // Update the central command state with any new values.
  //   _currentCommand.commandId = commandId ?? _currentCommand.commandId;
  //   _currentCommand.moveSpeed = moveSpeed ?? _currentCommand.moveSpeed;
  //   _currentCommand.turnAngle = turnAngle ?? _currentCommand.turnAngle;
  //   _currentCommand.gunTrigger = gunTrigger ?? _currentCommand.gunTrigger;
  //   _currentCommand.gunPermission = gunPermission ?? _currentCommand.gunPermission;
  //
  //   try {
  //     final socket = await Socket.connect(ROBOT_IP_ADDRESS, ROBOT_COMMAND_PORT, timeout: const Duration(seconds: 1));
  //     socket.add(_currentCommand.toBytes());
  //     await socket.flush();
  //     socket.close();
  //     print('Sent Packet: ID=${_currentCommand.commandId}, Spd=${_currentCommand.moveSpeed}, Ang=${_currentCommand.turnAngle}, Trig=${_currentCommand.gunTrigger}');
  //   } catch (e) {
  //     print('Error sending command packet: $e');
  //   }
  // }

  // void _onLeftButtonPressed(int index, int commandId) {
  //   setState(() {
  //     _activeLeftButtonIndex = index;
  //     _currentCommand.commandId = commandId;
  //   });
  //   _sendCommandPacket(); // Send command immediately
  // }

  Future<void> _handleGamepadEvent(MethodCall call) async {
    if (!mounted) return;

    if (!_gamepadConnected) {
      setState(() => _gamepadConnected = true);
      print("Gamepad connected (via native plugin)");
    }

    if (call.method == "onMotionEvent") {
      // 1. Cancel any pending "idle" timer because we just received new data.
      _joystickIdleTimer?.cancel();

      final Map<dynamic, dynamic> axes = call.arguments;

      double horizontalValue = axes['AXIS_X'] ?? axes['AXIS_Z'] ?? axes['AXIS_HAT_X'] ?? 0.0;
      int newAngle = (horizontalValue * 100).round();
      if (newAngle.abs() < 15) newAngle = 0;

      double verticalValue = axes['AXIS_Y'] ?? axes['AXIS_RZ'] ?? axes['AXIS_HAT_Y'] ?? 0.0;
      int newSpeed = (verticalValue * -100).round();
      if (newSpeed.abs() < 15) newSpeed = 0;

      double triggerValue = axes['AXIS_RTRIGGER'] ?? axes['AXIS_LTRIGGER'] ?? 0.0;
      bool newTrigger = triggerValue > 0.5;

      // Only send a packet if a value actually changed.
      if (newAngle != _currentCommand.turnAngle || newSpeed != _currentCommand.moveSpeed || newTrigger != _currentCommand.gunTrigger) {
        _currentCommand.turnAngle = newAngle;
        _currentCommand.moveSpeed = newSpeed;
        _currentCommand.gunTrigger = newTrigger;
        _sendCommandPacket();
      }

      // 2. Start a new timer. If this timer completes without being cancelled,
      //    it means the user has stopped moving the joystick.
      _joystickIdleTimer = Timer(const Duration(milliseconds: 150), () {
        // Check if the robot is currently moving before sending a stop command.
        if (_currentCommand.moveSpeed != 0 || _currentCommand.turnAngle != 0) {
          print("Joystick Idle: Sending stop command.");
          _currentCommand.moveSpeed = 0;
          _currentCommand.turnAngle = 0;
          _sendCommandPacket();
        }
      });
    }
  }

  // --- Simplified Sending Function ---
  Future<void> _sendCommandPacket() async {
    try {
      final socket = await Socket.connect(ROBOT_IP_ADDRESS, ROBOT_COMMAND_PORT, timeout: const Duration(seconds: 1));
      socket.add(_currentCommand.toBytes());
      await socket.flush();
      socket.close();
      print('Sent Packet: ID=${_currentCommand.commandId}, Speed=${_currentCommand.moveSpeed}, Angle=${_currentCommand.turnAngle}');
    } catch (e) {
      print('Error sending command packet: $e');
    }
  }

  void _onLeftButtonPressed(int index, int commandId) {
    setState(() => _activeLeftButtonIndex = index);
    _currentCommand.commandId = commandId;
    _sendCommandPacket();
  }


  void _onRightButtonPressed(int index) {
    setState(() => _activeRightButtonIndex = index);
    _switchCamera(index);
  }

  Future<void> _switchCamera(int index) async {
    final VlcPlayerController? oldController = _vlcPlayerController;
    setState(() {
      if (index == _currentCameraIndex && _vlcPlayerController != null) {} else
      if (index == _currentCameraIndex) {
        return;
      }
      _currentCameraIndex = index;
      _vlcPlayerController = null;
      _errorMessage = null;
    });
    await oldController?.dispose();
    await Future.delayed(const Duration(milliseconds: 200));
    // final newController = VlcPlayerController.network(_cameraUrls[index], hwAcc: HwAcc.disabled, autoPlay: true, options: VlcPlayerOptions(advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(150)]), video: VlcVideoOptions([VlcVideoOptions.dropLateFrames(true), VlcVideoOptions.skipFrames(true)]), extras: ['--h264-fps=60', '--no-audio']));
    final newController = VlcPlayerController.network(
      _cameraUrls[index],
      hwAcc: HwAcc.disabled,
      autoPlay: true,
      options: VlcPlayerOptions(
        // advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(150)]),
        advanced: VlcAdvancedOptions([
          VlcAdvancedOptions.networkCaching(50),
          VlcAdvancedOptions.clockJitter(0),
          VlcAdvancedOptions.fileCaching(0),
          VlcAdvancedOptions.liveCaching(0),
          VlcAdvancedOptions.clockSynchronization(1),
        ]),
        video: VlcVideoOptions([
          VlcVideoOptions.dropLateFrames(true),
          VlcVideoOptions.skipFrames(true),
        ]),
        extras: ['--h264-fps=60', '--no-audio'],
      ),
    );

    newController.addListener(() {
      if (!mounted) return;
      final state = newController.value;
      if (state.hasError && _errorMessage != state.errorDescription) {
        setState(() {
          _errorMessage =
              state.errorDescription ?? 'An Unknown Error Occurred!';
        });
      }
    });
    if (mounted) {
      setState(() {
        _vlcPlayerController = newController;
      });
    }
  }

  Future<void> _navigateToSettings() async {
    final newUrls = await Navigator.push<List<String>>(context,
        MaterialPageRoute(
            builder: (context) => SettingsMenuPage(cameraUrls: _cameraUrls)));
    if (newUrls != null) {
      setState(() => _cameraUrls = newUrls);
      _switchCamera(_currentCameraIndex);
    }
  }

  Future<bool> _showCustomConfirmationDialog(
      { required BuildContext context, required String iconPath, required String title, required Color titleColor, required String content}) async {
    return await showDialog<bool>(
      context: context,
      barrierDismissible: false,
      builder: (BuildContext dialogContext) {
        return Dialog(shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(20.0)),
          child: Container(width: 500, padding: const EdgeInsets.all(24.0),
            child: Column(mainAxisSize: MainAxisSize.min, children: [
              Row(children: [
                Image.asset(iconPath, height: 40, width: 40),
                const SizedBox(width: 16),
                Text(title, style: GoogleFonts.notoSans(fontSize: 28,
                    fontWeight: FontWeight.bold,
                    color: titleColor))
              ]),
              const SizedBox(height: 24),
              Text(content, textAlign: TextAlign.center,
                  style: GoogleFonts.notoSans(
                      fontSize: 22, color: Colors.black87)),
              const SizedBox(height: 32),
              Row(mainAxisAlignment: MainAxisAlignment.spaceEvenly, children: [
                TextButton(
                    onPressed: () => Navigator.of(dialogContext).pop(false),
                    child: Text('Cancel', style: GoogleFonts.notoSans(
                        fontSize: 24, fontWeight: FontWeight.bold, color: Colors
                        .grey.shade700))),
                SizedBox(height: 40,
                    child: VerticalDivider(
                        color: Colors.grey.shade400, thickness: 1)),
                TextButton(
                    onPressed: () => Navigator.of(dialogContext).pop(true),
                    child: Text('OK', style: GoogleFonts.notoSans(
                        fontSize: 24, fontWeight: FontWeight.bold, color: Colors
                        .blue.shade700))),
              ],
              ),
            ],
            ),
          ),
        );
      },
    ) ?? false;
  }

  Future<void> _handleManualAutoAttackToggle() async {
    if (_isAutoAttackMode) {
      final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_AUTO_ATTACK_ACTIVE, title: "DANGERS", titleColor: Colors.red, content: 'Are you sure you want to stop\n"Auto Attack" mode?');
      if (proceed) {
        setState(() {
          _isAutoAttackMode = false;
          _currentCommand.commandId = 1; // Revert to Move state
        });
      }
    } else {
      final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_MANUAL_ATTACK_ACTIVE, title: "DANGERS", titleColor: Colors.red, content: 'Are you sure you want to start\n"Auto Attack" mode?');
      if (proceed) {
        setState(() {
          _isAutoAttackMode = true;
          _currentCommand.commandId = 4; // 4 = Attack
        });
      }
    }
  }


  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    return Scaffold(
      backgroundColor: Colors.black,
      body: Stack(
        children: [
          Positioned.fill(child: buildPlayerWidget()),

          // --- Left Side Buttons ---
          _buildLeftButton(30, 35, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE, _activeLeftButtonIndex == 0, () => _onLeftButtonPressed(0, 1)),
          _buildLeftButton(30, 185, "Petrol", ICON_PATH_PETROL_INACTIVE, ICON_PATH_PETROL_ACTIVE, _activeLeftButtonIndex == 1, () => _onLeftButtonPressed(1, 2)),
          _buildLeftButton(30, 335, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE, _activeLeftButtonIndex == 2, () => _onLeftButtonPressed(2, 2)),
          _buildLeftButton(30, 485, _isAutoAttackMode ? "Auto Attack" : "Manual Attack", _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_INACTIVE : ICON_PATH_MANUAL_ATTACK_INACTIVE, _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_ACTIVE : ICON_PATH_MANUAL_ATTACK_ACTIVE, _activeLeftButtonIndex == 3, () { setState(() => _activeLeftButtonIndex = 3); _handleManualAutoAttackToggle(); }),
          _buildLeftButton(30, 635, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE, _activeLeftButtonIndex == 4, () => _onLeftButtonPressed(4, 3)),
          _buildLeftButton(30, 785, "Return", ICON_PATH_RETURN_INACTIVE, ICON_PATH_RETURN_ACTIVE, _activeLeftButtonIndex == 5, () => _onLeftButtonPressed(5, 0)),

          // --- Right Side Buttons ---
          _buildRightButton(1690, 30, "Attack View", ICON_PATH_ATTACK_VIEW_INACTIVE, ICON_PATH_ATTACK_VIEW_ACTIVE, _activeRightButtonIndex == 0, () => _onRightButtonPressed(0)),
          _buildRightButton(1690, 250, "Top View", ICON_PATH_TOP_VIEW_INACTIVE, ICON_PATH_TOP_VIEW_ACTIVE, _activeRightButtonIndex == 1, () => _onRightButtonPressed(1)),
          _buildRightButton(1690, 470, "3D View", ICON_PATH_3D_VIEW_INACTIVE, ICON_PATH_3D_VIEW_ACTIVE, _activeRightButtonIndex == 2, () => _onRightButtonPressed(2)),
          _buildRightButton(1690, 720, "Setting", ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, false, () => _navigateToSettings()),

          // --- UPDATED Directional Arrow Buttons ---
          // _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE,
          //       () { // onPress
          //     setState(() => _isForwardPressed = true);
          //     _sendCommandPacket(moveSpeed: 100);
          //   },
          //       () { // onRelease
          //     setState(() => _isForwardPressed = false);
          //     _sendCommandPacket(moveSpeed: 0);
          //   },
          // ),
          // _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE,
          //         () { // onPress
          //       setState(() => _isBackPressed = true);
          //       _sendCommandPacket(moveSpeed: -100);
          //     },
          //         () { // onRelease
          //       setState(() => _isBackPressed = false);
          //       _sendCommandPacket(moveSpeed: 0);
          //     }
          // ),
          // _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE,
          //         () { // onPress
          //       setState(() => _isLeftPressed = true);
          //       _sendCommandPacket(turnAngle: -100);
          //     },
          //         () { // onRelease
          //       setState(() => _isLeftPressed = false);
          //       _sendCommandPacket(turnAngle: 0);
          //     }
          // ),
          // _buildDirectionalButton(1540, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE,
          //         () { // onPress
          //       setState(() => _isRightPressed = true);
          //       _sendCommandPacket(turnAngle: 100);
          //     },
          //         () { // onRelease
          //       setState(() => _isRightPressed = false);
          //       _sendCommandPacket(turnAngle: 0);
          //     }
          // ),

          _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE,
                () { // onPress
              setState(() => _isForwardPressed = true);
              _currentCommand.moveSpeed = 100;
              _sendCommandPacket();
            },
                () { // onRelease
              setState(() => _isForwardPressed = false);
              _currentCommand.moveSpeed = 0;
              _sendCommandPacket();
            },
          ),
          _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE,
                  () { // onPress
                setState(() => _isBackPressed = true);
                _currentCommand.moveSpeed = -100;
                _sendCommandPacket();
              },
                  () { // onRelease
                setState(() => _isBackPressed = false);
                _currentCommand.moveSpeed = 0;
                _sendCommandPacket();
              }
          ),
          _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE,
                  () { // onPress
                setState(() => _isLeftPressed = true);
                _currentCommand.turnAngle = -100;
                _sendCommandPacket();
              },
                  () { // onRelease
                setState(() => _isLeftPressed = false);
                _currentCommand.turnAngle = 0;
                _sendCommandPacket();
              }
          ),
          _buildDirectionalButton(1540, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE,
                  () { // onPress
                setState(() => _isRightPressed = true);
                _currentCommand.turnAngle = 100;
                _sendCommandPacket();
              },
                  () { // onRelease
                setState(() => _isRightPressed = false);
                _currentCommand.turnAngle = 0;
                _sendCommandPacket();
              }
          ),

          Align(
            alignment: Alignment.bottomCenter,
            child: Container(
              margin: const EdgeInsets.only(bottom: 20, left: 20, right: 20),
              decoration: BoxDecoration(color: Colors.black.withOpacity(0.6), borderRadius: BorderRadius.circular(50)),
              child: _buildBottomBar(),
            ),
          ),
        ],
      ),
    );
  }

  Widget buildPlayerWidget() {
    if (_errorMessage != null) {
      return Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 24.0),
                  child: Text('Error: $_errorMessage',
                      textAlign: TextAlign.center,
                      style: const TextStyle(
                          color: Colors.red,
                          fontSize: 16,
                          fontWeight: FontWeight.bold))),
              const SizedBox(height: 20),
              ElevatedButton(
                  onPressed: () => _switchCamera(_currentCameraIndex),
                  style:
                  ElevatedButton.styleFrom(backgroundColor: Colors.grey[700]),
                  child: const Text('Retry', style: TextStyle(color: Colors.white))),
            ],
          ));
    }
    if (_vlcPlayerController == null) {
      return const Center(child: CircularProgressIndicator(color: Colors.white));
    }
    // No aspectRatio here allows the player to fill the space.
    // We added the aspect ratio hint to the VLC options instead.
    return VlcPlayer(
      controller: _vlcPlayerController!,
      placeholder: const Center(child: CircularProgressIndicator(color: Colors.white)), aspectRatio: 16/12,);
  }


  Widget _buildLeftButton(double left, double top, String label, String inactiveIcon, String activeIcon, bool isActive, VoidCallback onPressed) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    return Positioned(
      left: left * widthScale,
      top: top * heightScale,
      child: GestureDetector(
        onTap: onPressed,
        child: Container(
          width: 200 * widthScale,
          height: 120 * heightScale,
          padding: EdgeInsets.symmetric(vertical: 8.0 * heightScale),
          decoration: BoxDecoration(
            color: Colors.black.withOpacity(0.6),
            borderRadius: BorderRadius.circular(10),
            border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.0),
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [
              Expanded(flex: 2, child: Image.asset(isActive ? activeIcon : inactiveIcon, fit: BoxFit.contain)),
              SizedBox(height: 5 * heightScale),
              Text(label, textAlign: TextAlign.center, style: GoogleFonts.notoSans(fontSize: 24 * heightScale, fontWeight: FontWeight.bold, color: const Color(0xFFFFFFFF))),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildRightButton(double left, double top, String label, String inactiveIcon, String activeIcon, bool isActive, VoidCallback onPressed) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    const double buttonWidth = 220;
    const double buttonHeight = 175;

    return Positioned(
      // The `left` coordinate is the starting edge, not the center.
      // This was the main error.
      left: left * widthScale,
      top: top * heightScale,
      child: GestureDetector(
        onTap: onPressed,
        child: Container(
          width: buttonWidth * widthScale,
          height: buttonHeight * heightScale,
          padding: EdgeInsets.symmetric(vertical: 10.0 * heightScale),
          decoration: BoxDecoration(
            color: Colors.black.withOpacity(0.6),
            borderRadius: BorderRadius.circular(15),
            border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.5),
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [
              Expanded(
                flex: 3,
                child: Image.asset(isActive ? activeIcon : inactiveIcon, fit: BoxFit.contain),
              ),
              SizedBox(height: 8 * heightScale),
              Text(label, textAlign: TextAlign.center, style: GoogleFonts.notoSans(fontSize: 26 * heightScale, fontWeight: FontWeight.bold, color: const Color(0xFFFFFFFF))),
            ],
          ),
        ),
      ),
    );
  }


  // --- FIX: UPDATED WIDGET TO ACCEPT CALLBACKS INSTEAD OF STRINGS ---
  Widget _buildDirectionalButton(
      double left,
      double top,
      bool isPressed,
      String inactiveIcon,
      String activeIcon,
      VoidCallback onPress, // Now accepts a function
      VoidCallback onRelease, // Now accepts a function
      ) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    return Positioned(
      left: left * widthScale,
      top: top * heightScale,
      child: GestureDetector(
        // The onTapDown and onTapUp now directly call the functions passed in.
        onTapDown: (_) => onPress(),
        onTapUp: (_) => onRelease(),
        onTapCancel: () => onRelease(),
        child: Image.asset(
          // The isPressed state is now managed outside this widget, so we don't need to track it here.
          // However, for visual feedback, we'll keep the icon change. Let's adjust the main build method.
          isPressed ? activeIcon : inactiveIcon,
          height: 100 * heightScale,
          width: 100 * widthScale,
          fit: BoxFit.contain,
        ),
      ),
    );
  }

  // --- FINAL, ROBUST, AND CONDITIONALLY SCROLLABLE BOTTOM BAR ---
  Widget _buildBottomBar() {
    final screenWidth = MediaQuery.of(context).size.width;
    final widthScale = screenWidth / 1920.0;

    return LayoutBuilder(
      builder: (context, constraints) {
        const double scrollBreakpoint = 1200.0;

        // final List<Widget> leftCluster = [
        //   _buildBottomBarButton(
        //     "ATTACK",
        //     _isAttackModeOn ? ICON_PATH_ATTACK_of : ICON_PATH_ATTACK,
        //     _isAttackModeOn ? _attackActiveColors : _attackInactiveColors,
        //         () async {
        //       if (!_isAttackModeOn) {
        //         final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_ATTACK, title: 'DANGERS', titleColor: Colors.red, content: 'Do you want to proceed the command?');
        //         if (proceed) {
        //           setState(() => _isAttackModeOn = true);
        //           _sendCommandPacket(commandId: 4); // 4 = attack
        //         }
        //       } else {
        //         setState(() => _isAttackModeOn = false);
        //         _sendCommandPacket(commandId: 0); // 0 = stop/idle
        //       }
        //     },
        //   ),
        //   SizedBox(width: 12 * widthScale),
        //   _buildWideBottomBarButton(
        //       _isStarted ? "STOP" : "START",
        //       _isStarted ? ICON_PATH_STOP : ICON_PATH_START,
        //       [const Color(0xff25a625), const Color(0xff127812)],
        //           () {
        //         setState(() => _isStarted = !_isStarted);
        //         _sendCommandPacket(gunTrigger: _isStarted);
        //       }
        //   ),
        //   SizedBox(width: 12 * widthScale),
        //   _buildBottomBarButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () { /* Zoom logic would go here */ }),
        //   SizedBox(width: 12 * widthScale),
        //   _buildBottomBarButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () { /* Zoom logic would go here */ }),
        // ];

        final List<Widget> leftCluster = [
          _buildBottomBarButton(
            "ATTACK",
            _isAttackModeOn ? ICON_PATH_ATTACK_of : ICON_PATH_ATTACK,
            _isAttackModeOn ? _attackActiveColors : _attackInactiveColors,
                () async {
              if (!_isAttackModeOn) {
                final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_ATTACK, title: 'DANGERS', titleColor: Colors.red, content: 'Do you want to proceed the command?');
                if (proceed) {
                  setState(() => _isAttackModeOn = true);
                  _currentCommand.commandId = 4; // 4 = attack
                  _sendCommandPacket();
                }
              } else {
                setState(() => _isAttackModeOn = false);
                _currentCommand.commandId = 0; // 0 = stop/idle
                _sendCommandPacket();
              }
            },
          ),
          SizedBox(width: 12 * widthScale),
          _buildWideBottomBarButton(
              _isStarted ? "STOP" : "START",
              _isStarted ? ICON_PATH_STOP : ICON_PATH_START,
              [const Color(0xff25a625), const Color(0xff127812)],
                  () {
                setState(() {
                  _isStarted = !_isStarted;
                  _currentCommand.gunTrigger = _isStarted;
                });
                _sendCommandPacket();
              }
          ),
          SizedBox(width: 12 * widthScale),
          _buildBottomBarButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () { /* Zoom logic would go here, then call _sendCommandPacket() */ }),
          SizedBox(width: 12 * widthScale),
          _buildBottomBarButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () { /* Zoom logic would go here, then call _sendCommandPacket() */ }),
        ];

        final List<Widget> middleCluster = [
          Row(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.baseline,
            textBaseline: TextBaseline.alphabetic,
            children: [
              Text("0", style: GoogleFonts.notoSans(fontSize: 80, fontWeight: FontWeight.bold, color: Colors.white)),
              SizedBox(width: 8 * widthScale),
              Text("Km/h", style: GoogleFonts.notoSans(fontSize: 36, fontWeight: FontWeight.w500, color: Colors.white)),
            ],
          ),
          SizedBox(width: 20 * widthScale),
          Image.asset(ICON_PATH_WIFI, height: 40),
        ];

        final List<Widget> rightCluster = [
          _buildBottomBarButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)], () async {
            final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_EXIT, title: 'Information', titleColor: Colors.blue.shade700, content: 'Do you want to finish?');
            if (proceed) { SystemNavigator.pop(); }
          }),
        ];

        if (constraints.maxWidth < scrollBreakpoint) {
          return SingleChildScrollView(
            scrollDirection: Axis.horizontal,
            physics: const ClampingScrollPhysics(),
            child: Row(
              children: [
                ...leftCluster,
                SizedBox(width: 100 * widthScale),
                ...middleCluster,
                SizedBox(width: 100 * widthScale),
                ...rightCluster,
              ],
            ),
          );
        } else {
          return Row(
            children: [
              ...leftCluster,
              const Spacer(),
              ...middleCluster,
              const Spacer(),
              ...rightCluster,
            ],
          );
        }
      },
    );
  }

  Widget _buildWideBottomBarButton(String label, String iconPath, List<Color> gradientColors, VoidCallback onPressed) {
    final screenHeight = MediaQuery.of(context).size.height;
    final screenWidth = MediaQuery.of(context).size.width;
    final heightScale = screenHeight / 1080.0;
    final widthScale = screenWidth / 1920.0;

    return GestureDetector(
      onTap: onPressed,
      // This Container enforces a minimum width, making it as wide as the ATTACK button.
      child: Container(
        constraints: BoxConstraints(
          minWidth: 220 * widthScale, // Enforce a minimum width
        ),
        height: 80 * heightScale,
        padding: const EdgeInsets.symmetric(horizontal: 74),
        decoration: BoxDecoration(
          gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
          borderRadius: BorderRadius.circular(25 * heightScale),
        ),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            Image.asset(iconPath, height: 36 * heightScale),
            const SizedBox(width: 12),
            Text(label, style: GoogleFonts.notoSans(fontSize: 36 * heightScale, fontWeight: FontWeight.bold, color: Colors.white)),
          ],
        ),
      ),
    );
  }

  Widget _buildBottomBarButton(String label, String iconPath,
      List<Color> gradientColors, VoidCallback onPressed) {

    final screenHeight = MediaQuery.of(context).size.height;
    final heightScale = screenHeight / 1080.0;

    return GestureDetector(
      onTap: onPressed,
      child: Container(
        height: 80 * heightScale,
        // Increased horizontal padding makes the buttons wider
        padding: EdgeInsets.symmetric(horizontal: label.isEmpty ? 30 : 45),
        decoration: BoxDecoration(
          gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
          // Reduced border radius makes them more rectangular
          borderRadius: BorderRadius.circular(25 * heightScale),
        ),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            Image.asset(iconPath, height: 36 * heightScale),
            if (label.isNotEmpty) ...[
              const SizedBox(width: 12),
              Text(label, style: GoogleFonts.notoSans(fontSize: 36 * heightScale, fontWeight: FontWeight.bold, color: Colors.white)),
            ]
          ],
        ),
      ),
    );
  }
}


