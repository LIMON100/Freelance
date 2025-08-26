import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
// import 'package:flutter_vlc_player/flutter_vlc_player.dart'; // <<< REMOVED (This line is not in the code I'm modifying here, but confirming it's gone)
import 'package:google_fonts/google_fonts.dart';
import 'package:rtest1/settings_service.dart';

import 'TouchCoord.dart';
import 'UserCommand.dart';
import 'icon_constants.dart';
import 'splash_screen.dart';
import 'settings_menu_page.dart';

// --- CONFIGURATION FOR ROBOT CONNECTION ---
const String ROBOT_IP_ADDRESS = '192.168.0.158'; // Updated IP as per your provided code
const int ROBOT_COMMAND_PORT = 65432;


// --- MAIN APP ENTRY POINT ---
void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setEnabledSystemUIMode(SystemUiMode.manual, overlays: []); // Hides status and nav bars
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

// --- HOME PAGE WIDGET ---
class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  // Video and UI State
  // VlcPlayerController? _vlcPlayerController; // <<< REMOVED
  String? _errorMessage; // For GStreamer error messages
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
  bool _isGunTriggerPressed = false; // State for the gun trigger
  final GlobalKey _playerKey = GlobalKey(); // Key for the player widget

  static const platform = MethodChannel('com.yourcompany/gamepad');
  bool _gamepadConnected = false;
  Timer? _commandTimer;
  Map<String, double> _gamepadAxisValues = {};
  Uint8List? _lastSentPacket;
  final SettingsService _settingsService = SettingsService();
  bool _isLoading = true; // Loading state flag for initial setup

  // GStreamer specific state (NOT MODIFIED HERE AS PER YOUR REQUEST)
  String _robotIpAddress = '';
  List<String> _cameraUrls = [];
  int _currentCameraIndex = -1;
  Key _gstreamerViewKey = UniqueKey();
  bool _isGStreamerReady = false;
  bool _isGStreamerLoading = true;
  bool _gstreamerHasError = false;
  MethodChannel? _gstreamerChannel;


  @override
  void initState() {
    super.initState();
    _loadSettingsAndInitialize();
    platform.setMethodCallHandler(_handleGamepadEvent);
  }

  @override
  void dispose() {
    _commandTimer?.cancel();
    _gstreamerChannel?.invokeMethod('stopStream');
    super.dispose();
  }

  Future<void> _loadSettingsAndInitialize() async {
    _robotIpAddress = await _settingsService.loadIpAddress();
    _cameraUrls = await _settingsService.loadCameraUrls();

    if (mounted) {
      _switchCamera(0); // Initialize with the first camera
      _startCommandTimer(); // Start sending commands
      setState(() {
        _isLoading = false;
      });
    }
  }

  void _onGStreamerPlatformViewCreated(int id) {
    _gstreamerChannel = MethodChannel('gstreamer_channel_$id');
    setState(() {
      _isGStreamerReady = true;
      _isGStreamerLoading = false;
    });
    _playCurrentCameraStream();
  }

  Future<void> _playCurrentCameraStream() async {
    if (_gstreamerChannel == null || !_isGStreamerReady || _cameraUrls.isEmpty || _currentCameraIndex < 0 || _currentCameraIndex >= _cameraUrls.length) return;

    setState(() {
      _isGStreamerLoading = true;
      _gstreamerHasError = false;
    });

    try {
      final String url = _cameraUrls[_currentCameraIndex];
      final String gstDesc = "rtspsrc location=$url protocols=tcp latency=50 ! decodebin3 ! videoconvert ! autovideosink";
      // await _gstreamerChannel!.invokeMethod('startStream', {'url': url, 'pipeline': gstdesc});
      await _gstreamerChannel!.invokeMethod('startStream', {'url': url, 'pipeline': gstDesc});

      Future.delayed(const Duration(seconds: 2), () {
        if (mounted) {
          setState(() {
            _isGStreamerLoading = false;
          });
        }
      });
    } on PlatformException catch (e) {
      print("GStreamer failed to start stream: '${e.message}'.");
      if (mounted) {
        setState(() {
          _gstreamerHasError = true;
          _isGStreamerLoading = false;
        });
      }
    }
  }

  Future<void> _handleGamepadEvent(MethodCall call) async {
    if (!mounted) return;
    if (!_gamepadConnected && call.method != "onMotionEvent") {
      setState(() => _gamepadConnected = true);
    }
    if (call.method == "onMotionEvent") {
      _gamepadAxisValues = Map<String, double>.from(call.arguments);
    }
    if (call.method == "onButtonDown") {
      final String button = call.arguments['button'];
      if (button == 'KEYCODE_BUTTON_A') {
        setState(() => _currentCommand.commandId = 0); // STOP/IDLE
      }
      if (button == 'KEYCODE_BUTTON_X') {
        setState(() => _currentCommand.gunPermission = true);
      }
      if (button == 'KEYCODE_BUTTON_START') {
        _handleManualAutoAttackToggle();
      }
    }
    if (call.method == "onButtonUp") {
      final String button = call.arguments['button'];
      if (button == 'KEYCODE_BUTTON_X') {
        setState(() => _currentCommand.gunPermission = false);
      }
    }
  }


  void _startCommandTimer() {
    _commandTimer = Timer.periodic(const Duration(milliseconds: 100), (timer) {
      if (_gamepadConnected) {
        double horizontalValue = _gamepadAxisValues['AXIS_X'] ?? _gamepadAxisValues['AXIS_Z'] ?? _gamepadAxisValues['AXIS_HAT_X'] ?? 0.0;
        _currentCommand.turnAngle = (horizontalValue * 100).round();
        if (_currentCommand.turnAngle.abs() < 15) _currentCommand.turnAngle = 0;

        double verticalValue = _gamepadAxisValues['AXIS_Y'] ?? _gamepadAxisValues['AXIS_RZ'] ?? _gamepadAxisValues['AXIS_HAT_Y'] ?? 0.0;
        _currentCommand.moveSpeed = (verticalValue * -100).round();
        if (_currentCommand.moveSpeed.abs() < 15) _currentCommand.moveSpeed = 0;

        double triggerValue = _gamepadAxisValues['AXIS_RTRIGGER'] ?? _gamepadAxisValues['AXIS_LTRIGGER'] ?? 0.0;
        _currentCommand.gunTrigger = triggerValue > 0.5;

      } else {
        _currentCommand.moveSpeed = 0;
        if (_isForwardPressed) _currentCommand.moveSpeed = 100;
        else if (_isBackPressed) _currentCommand.moveSpeed = -100;

        _currentCommand.turnAngle = 0;
        if (_isLeftPressed) _currentCommand.turnAngle = -100;
        else if (_isRightPressed) _currentCommand.turnAngle = 100;

        _currentCommand.gunTrigger = _isGunTriggerPressed; // THIS LINE WAS CORRECT IN THE PREVIOUS WORKING CODE
      }

      _sendCommandPacket(_currentCommand);

      if (_currentCommand.touchX != -1.0) {
        _currentCommand.touchX = -1.0;
        _currentCommand.touchY = -1.0;
      }
    });
  }

  Future<void> _sendCommandPacket(UserCommand command) async {
    if (_robotIpAddress.isEmpty || ROBOT_COMMAND_PORT == 0) {
      print("Error: Robot IP address or port is not configured.");
      return;
    }
    try {
      final socket = await Socket.connect(_robotIpAddress, ROBOT_COMMAND_PORT, timeout: const Duration(milliseconds: 150));
      socket.add(command.toBytes());
      await socket.flush();
      socket.close();
    } catch (e) {
      print('Error sending command packet: $e');
    }
  }

  Future<void> _sendTouchPacket(TouchCoord coord) async {
    try {
      final socket = await Socket.connect(_robotIpAddress, 65433, timeout: const Duration(seconds: 1));
      socket.add(coord.toBytes());
      await socket.flush();
      socket.close();
      print('Sent Touch Packet: X=${coord.x}, Y=${coord.y}');
    } catch (e) {
      print('Error sending touch packet: $e');
    }
  }


  void _onLeftButtonPressed(int index, int commandId) {
    setState(() {
      _activeLeftButtonIndex = index;
      _currentCommand.commandId = commandId;
    });
  }

  void _onRightButtonPressed(int index) {
    setState(() => _activeRightButtonIndex = index);
    _switchCamera(index);
  }

  Future<void> _switchCamera(int index) async {
    if (_cameraUrls.isEmpty || index < 0 || index >= _cameraUrls.length) {
      print("Warning: Camera index out of bounds or no camera URLs configured.");
      setState(() {
        _currentCameraIndex = -1;
        _isGStreamerLoading = true;
        _gstreamerHasError = true;
        _errorMessage = "No cameras configured.";
        _isGStreamerReady = false;
      });
      return;
    }

    if (index == _currentCameraIndex) {
      return;
    }

    if (_gstreamerChannel != null && _isGStreamerReady) {
      try {
        await _gstreamerChannel!.invokeMethod('stopStream');
      } catch (e) {
        print("Error stopping previous stream: $e");
      }
    }

    setState(() {
      _currentCameraIndex = index;
      _gstreamerViewKey = UniqueKey();
      _isGStreamerReady = false;
      _isGStreamerLoading = true;
      _gstreamerHasError = false;
      _errorMessage = null;
    });
  }

  Future<void> _navigateToSettings() async {
    final bool? settingsChanged = await Navigator.push<bool>(
      context,
      MaterialPageRoute(
        builder: (context) => const SettingsMenuPage(),
      ),
    );

    if (settingsChanged == true && mounted) {
      print("Settings changed, reloading...");
      _commandTimer?.cancel();
      await _loadSettingsAndInitialize();
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
      final proceed = await _showCustomConfirmationDialog(
          context: context,
          iconPath: ICON_PATH_AUTO_ATTACK_ACTIVE,
          title: "DANGERS",
          titleColor: Colors.red,
          content: 'Are you sure you want to stop\n"Auto Attack" mode?');
      if (proceed) {
        setState(() {
          _isAutoAttackMode = false;
          _currentCommand.commandId = 0;
          _activeLeftButtonIndex = 0;
        });
      }
    } else {
      final proceed = await _showCustomConfirmationDialog(
          context: context,
          iconPath: ICON_PATH_MANUAL_ATTACK_ACTIVE,
          title: "DANGERS",
          titleColor: Colors.red,
          content: 'Are you sure you want to start\n"Auto Attack" mode?');
      if (proceed) {
        setState(() {
          _isAutoAttackMode = true;
          _currentCommand.commandId = 4;
        });
      }
    }
  }

  // @override
  // Widget build(BuildContext context) {
  //   final screenWidth = MediaQuery.of(context).size.width;
  //   final screenHeight = MediaQuery.of(context).size.height;
  //   final widthScale = screenWidth / 1920.0;
  //   final heightScale = screenHeight / 1080.0;
  //
  //   if (_isLoading) {
  //     return const Scaffold(
  //       body: Center(
  //         child: CircularProgressIndicator(),
  //       ),
  //     );
  //   }
  //
  //   return Scaffold(
  //     backgroundColor: Colors.black,
  //     body: Stack(
  //       children: [
  //         buildPlayerWidgetGestureDetector(),
  //         Positioned.fill(
  //           child: KeyedSubtree(
  //             key: _gstreamerViewKey,
  //             child: AndroidView(
  //               viewType: 'gstreamer_view',
  //               onPlatformViewCreated: _onGStreamerPlatformViewCreated,
  //               creationParamsCodec: const StandardMessageCodec(),
  //             ),
  //           ),
  //         ),
  //         if (_isGStreamerLoading || _gstreamerHasError)
  //           Positioned.fill(
  //             child: Container(
  //               color: Colors.black.withOpacity(0.7),
  //               child: Center(
  //                 child: _gstreamerHasError
  //                     ? Column(
  //                   mainAxisAlignment: MainAxisAlignment.center,
  //                   children: [
  //                     Padding(
  //                       padding: const EdgeInsets.symmetric(horizontal: 24.0),
  //                       child: Text(
  //                         _errorMessage ?? 'Failed to load video stream.',
  //                         textAlign: TextAlign.center,
  //                         style: const TextStyle(color: Colors.red, fontSize: 16, fontWeight: FontWeight.bold),
  //                       ),
  //                     ),
  //                     const SizedBox(height: 20),
  //                     ElevatedButton(
  //                       onPressed: _playCurrentCameraStream,
  //                       style: ElevatedButton.styleFrom(backgroundColor: Colors.grey[700]),
  //                       child: const Text('Retry', style: TextStyle(color: Colors.white)),
  //                     ),
  //                   ],
  //                 )
  //                     : const CircularProgressIndicator(color: Colors.white),
  //               ),
  //             ),
  //           ),
  //
  //         _buildLeftButton(30, 35, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE, _activeLeftButtonIndex == 0, () => _onLeftButtonPressed(0, 1)),
  //         _buildLeftButton(30, 185, "Petrol", ICON_PATH_PETROL_INACTIVE, ICON_PATH_PETROL_ACTIVE, _activeLeftButtonIndex == 1, () => _onLeftButtonPressed(1, 2)),
  //         _buildLeftButton(30, 335, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE, _activeLeftButtonIndex == 2, () => _onLeftButtonPressed(2, 2)),
  //         _buildLeftButton(30, 485, _isAutoAttackMode ? "Auto Attack" : "Manual Attack", _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_INACTIVE : ICON_PATH_MANUAL_ATTACK_INACTIVE, _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_ACTIVE : ICON_PATH_MANUAL_ATTACK_ACTIVE, _activeLeftButtonIndex == 3, () => _handleManualAutoAttackToggle()),
  //         _buildLeftButton(30, 635, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE, _activeLeftButtonIndex == 4, () => _onLeftButtonPressed(4, 3)),
  //         _buildLeftButton(30, 785, "Return", ICON_PATH_RETURN_INACTIVE, ICON_PATH_RETURN_ACTIVE, _activeLeftButtonIndex == 5, () => _onLeftButtonPressed(5, 0)),
  //
  //         _buildRightButton(1690, 30, "Attack View", ICON_PATH_ATTACK_VIEW_INACTIVE, ICON_PATH_ATTACK_VIEW_ACTIVE, _activeRightButtonIndex == 0, () => _onRightButtonPressed(0)),
  //         _buildRightButton(1690, 250, "Top View", ICON_PATH_TOP_VIEW_INACTIVE, ICON_PATH_TOP_VIEW_ACTIVE, _activeRightButtonIndex == 1, () => _onRightButtonPressed(1)),
  //         _buildRightButton(1690, 470, "3D View", ICON_PATH_3D_VIEW_INACTIVE, ICON_PATH_3D_VIEW_ACTIVE, _activeRightButtonIndex == 2, () => _onRightButtonPressed(2)),
  //         _buildRightButton(1690, 720, "Setting", ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, false, () => _navigateToSettings()),
  //
  //         _buildDirectionalButton(
  //             890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE,
  //                 () { setState(() { _isForwardPressed = true; }); },
  //                 () { setState(() { _isForwardPressed = false; }); }
  //         ),
  //         _buildDirectionalButton(
  //             890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE,
  //                 () { setState(() { _isBackPressed = true; }); },
  //                 () { setState(() { _isBackPressed = false; }); }
  //         ),
  //         _buildDirectionalButton(
  //             260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE,
  //                 () { setState(() { _isLeftPressed = true; }); },
  //                 () { setState(() { _isLeftPressed = false; }); }
  //         ),
  //         _buildDirectionalButton(
  //             1540, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE,
  //                 () { setState(() { _isRightPressed = true; }); },
  //                 () { setState(() { _isRightPressed = false; }); }
  //         ),
  //
  //         Align(
  //           alignment: Alignment.bottomCenter,
  //           child: Container(
  //             margin: const EdgeInsets.only(bottom: 20, left: 20, right: 20),
  //             decoration: BoxDecoration(color: Colors.black.withOpacity(0.6), borderRadius: BorderRadius.circular(50)),
  //             child: _buildBottomBar(),
  //           ),
  //         ),
  //       ],
  //     ),
  //   );
  // }


  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    if (_isLoading) {
      return const Scaffold(
        body: Center(
          child: CircularProgressIndicator(),
        ),
      );
    }

    return Scaffold(
      backgroundColor: Colors.black,
      body: Stack(
        children: [
          //  Ensure the GestureDetector is placed FIRST in the stack's children.
          buildPlayerWidgetGestureDetector(), // <<< CALLING THE FUNCTION HERE

          // --- GStreamer Player Widget ---
          Positioned.fill(
            child: KeyedSubtree(
              key: _gstreamerViewKey,
              child: AndroidView(
                viewType: 'gstreamer_view',
                onPlatformViewCreated: _onGStreamerPlatformViewCreated,
                creationParamsCodec: const StandardMessageCodec(),
              ),
            ),
          ),
          //
          if (_isGStreamerLoading || _gstreamerHasError)
            Positioned.fill(
              child: Container(
                color: Colors.black.withOpacity(0.7),
                child: Center(
                  child: _gstreamerHasError
                      ? Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Padding(
                        padding: const EdgeInsets.symmetric(horizontal: 24.0),
                        child: Text(
                          _errorMessage ?? 'Failed to load video stream.',
                          textAlign: TextAlign.center,
                          style: const TextStyle(color: Colors.red, fontSize: 16, fontWeight: FontWeight.bold),
                        ),
                      ),
                      const SizedBox(height: 20),
                      ElevatedButton(
                        onPressed: _playCurrentCameraStream,
                        style: ElevatedButton.styleFrom(backgroundColor: Colors.grey[700]),
                        child: const Text('Retry', style: TextStyle(color: Colors.white)),
                      ),
                    ],
                  )
                      : const CircularProgressIndicator(color: Colors.white),
                ),
              ),
            ),

          _buildLeftButton(30, 35, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE, _activeLeftButtonIndex == 0, () => _onLeftButtonPressed(0, 1)),
          _buildLeftButton(30, 185, "Petrol", ICON_PATH_PETROL_INACTIVE, ICON_PATH_PETROL_ACTIVE, _activeLeftButtonIndex == 1, () => _onLeftButtonPressed(1, 2)),
          _buildLeftButton(30, 335, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE, _activeLeftButtonIndex == 2, () => _onLeftButtonPressed(2, 2)),
          _buildLeftButton(30, 485, _isAutoAttackMode ? "Auto Attack" : "Manual Attack", _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_INACTIVE : ICON_PATH_MANUAL_ATTACK_INACTIVE, _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_ACTIVE : ICON_PATH_MANUAL_ATTACK_ACTIVE, _activeLeftButtonIndex == 3, () => _handleManualAutoAttackToggle()),
          _buildLeftButton(30, 635, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE, _activeLeftButtonIndex == 4, () => _onLeftButtonPressed(4, 3)),
          _buildLeftButton(30, 785, "Return", ICON_PATH_RETURN_INACTIVE, ICON_PATH_RETURN_ACTIVE, _activeLeftButtonIndex == 5, () => _onLeftButtonPressed(5, 0)),

          _buildRightButton(1690, 30, "Attack View", ICON_PATH_ATTACK_VIEW_INACTIVE, ICON_PATH_ATTACK_VIEW_ACTIVE, _activeRightButtonIndex == 0, () => _onRightButtonPressed(0)),
          _buildRightButton(1690, 250, "Top View", ICON_PATH_TOP_VIEW_INACTIVE, ICON_PATH_TOP_VIEW_ACTIVE, _activeRightButtonIndex == 1, () => _onRightButtonPressed(1)),
          _buildRightButton(1690, 470, "3D View", ICON_PATH_3D_VIEW_INACTIVE, ICON_PATH_3D_VIEW_ACTIVE, _activeRightButtonIndex == 2, () => _onRightButtonPressed(2)),
          _buildRightButton(1690, 720, "Setting", ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, false, () => _navigateToSettings()),

          _buildDirectionalButton(
              890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE,
                  () { setState(() { _isForwardPressed = true; }); },
                  () { setState(() { _isForwardPressed = false; }); }
          ),
          _buildDirectionalButton(
              890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE,
                  () { setState(() { _isBackPressed = true; }); },
                  () { setState(() { _isBackPressed = false; }); }
          ),
          _buildDirectionalButton(
              260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE,
                  () { setState(() { _isLeftPressed = true; }); },
                  () { setState(() { _isLeftPressed = false; }); }
          ),
          _buildDirectionalButton(
              1540, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE,
                  () { setState(() { _isRightPressed = true; }); },
                  () { setState(() { _isRightPressed = false; }); }
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

  Widget buildPlayerWidgetGestureDetector() {
    print("DEBUG_TOUCH: buildPlayerWidgetGestureDetector called."); // DEBUG PRINT 1
    return GestureDetector(
      key: _playerKey, // Key is crucial for finding the RenderBox later
      onTapUp: (details) {
        // Only process touch events if the robot is in Manual Attack mode (_activeLeftButtonIndex == 3).
        if (_activeLeftButtonIndex == 3) {
          print("DEBUG_TOUCH: Manual Attack mode detected (_activeLeftButtonIndex == 3)."); // DEBUG PRINT 2

          // Get the RenderBox of the GestureDetector itself to calculate local coordinates.
          final RenderBox? renderBox = _playerKey.currentContext?.findRenderObject() as RenderBox?;
          if (renderBox == null) {
            print("DEBUG_TOUCH: ERROR: Could not get RenderBox for _playerKey."); // DEBUG PRINT 3
            return; // Exit if RenderBox is not available
          }
          print("DEBUG_TOUCH: RenderBox obtained successfully."); // DEBUG PRINT 4

          // Convert global tap position to local coordinates within the GestureDetector's bounds.
          final Offset localPosition = renderBox.globalToLocal(details.globalPosition);
          print("DEBUG_TOUCH: LocalPosition: $localPosition"); // DEBUG PRINT 5

          // Normalize the coordinates to be between 0.0 and 1.0 relative to the player area's size.
          final double x = localPosition.dx / renderBox.size.width;
          final double y = localPosition.dy / renderBox.size.height;
          print("DEBUG_TOUCH: Normalized Coords (x, y): ($x, $y)"); // DEBUG PRINT 6

          // Basic check to ensure coordinates are within expected bounds.
          if (x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0) {
            print("DEBUG_TOUCH: Coords within bounds. Processing touch."); // DEBUG PRINT 7
            // Update the _currentCommand state for the timer loop.
            setState(() {
              _currentCommand.touchX = x;
              _currentCommand.touchY = y;
            });
            // Immediately send the touch coordinates as a separate packet.
            _sendTouchPacket(TouchCoord()..x = x..y = y);
            print("DEBUG_TOUCH: Touch packet sent."); // DEBUG PRINT 8
          } else {
            print("DEBUG_TOUCH: Coords out of bounds. Not processing touch."); // DEBUG PRINT 9
          }
        } else {
          print("DEBUG_TOUCH: Not in Manual Attack mode (_activeLeftButtonIndex != 3). Touch ignored."); // DEBUG PRINT 10
        }
      },
      // A transparent container ensures the GestureDetector receives taps,
      // even if the underlying native view is fully opaque and might consume events.
      child: Container(color: Colors.transparent),
    );
  }

  // --- These methods are unchanged from your original code ---
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

  Widget _buildDirectionalButton(
      double left,
      double top,
      bool isPressed,
      String inactiveIcon,
      String activeIcon,
      VoidCallback onPress,
      VoidCallback onRelease,
      ) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    return Positioned(
      left: left * widthScale,
      top: top * heightScale,
      child: GestureDetector(
        onTapDown: (_) => onPress(),
        onTapUp: (_) => onRelease(),
        onTapCancel: () => onRelease(),
        child: Image.asset(
          isPressed ? activeIcon : inactiveIcon,
          height: 100 * heightScale,
          width: 100 * widthScale,
          fit: BoxFit.contain,
        ),
      ),
    );
  }

  Widget _buildBottomBar() {
    final screenWidth = MediaQuery.of(context).size.width;
    final widthScale = screenWidth / 1920.0;

    return LayoutBuilder(
      builder: (context, constraints) {
        const double scrollBreakpoint = 1200.0;

        final List<Widget> leftCluster = [
          _buildBottomBarButton(
            "ATTACK",
            _isAttackModeOn ? ICON_PATH_ATTACK_of : ICON_PATH_ATTACK,
            _isAttackModeOn ? _attackActiveColors : _attackInactiveColors,
                () async {
              if (!_isAttackModeOn) {
                final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_ATTACK, title: 'DANGERS', titleColor: Colors.red, content: 'Do you want to proceed the command?');
                if (proceed) {
                  setState(() {
                    _isAttackModeOn = true;
                    _currentCommand.commandId = 4;
                  });
                }
              } else {
                setState(() {
                  _isAttackModeOn = false;
                  _currentCommand.commandId = 0;
                });
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
                  _isGunTriggerPressed = _isStarted; //  This line ensures the trigger state matches START/STOP
                  if (!_isStarted) {
                    // If stopping, ensure commands are reset to zero/false
                    _currentCommand.moveSpeed = 0;
                    _currentCommand.turnAngle = 0;
                    _currentCommand.gunTrigger = false;
                    _isGunTriggerPressed = false;
                    _isForwardPressed = false;
                    _isBackPressed = false;
                    _isLeftPressed = false;
                    _isRightPressed = false;
                  }
                });
              }
          ),
          SizedBox(width: 12 * widthScale),
          _buildBottomBarButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () {
          }),
          SizedBox(width: 12 * widthScale),
          _buildBottomBarButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () {
          }),
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
            if (proceed) {
              SystemNavigator.pop();
            }
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

  // --- The rest of the helper methods remain unchanged ---
  Widget _buildWideBottomBarButton(String label, String iconPath, List<Color> gradientColors, VoidCallback onPressed) {
    final screenHeight = MediaQuery.of(context).size.height;
    final screenWidth = MediaQuery.of(context).size.width;
    final heightScale = screenHeight / 1080.0;
    final widthScale = screenWidth / 1920.0;

    return GestureDetector(
      onTap: onPressed,
      child: Container(
        constraints: BoxConstraints(
          minWidth: 220 * widthScale,
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
        padding: EdgeInsets.symmetric(horizontal: label.isEmpty ? 30 : 45),
        decoration: BoxDecoration(
          gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
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