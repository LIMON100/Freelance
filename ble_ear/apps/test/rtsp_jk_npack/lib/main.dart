import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/foundation.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:rtest1/settings_service.dart';
import 'TouchCoord.dart';
import 'UserCommand.dart';
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
  String? _errorMessage;
  int _activeLeftButtonIndex = -1; //0
  int _activeRightButtonIndex = 0;
  bool _isAutoAttackMode = false;
  bool _isAttackModeOn = false;
  final List<Color> _attackInactiveColors = [const Color(0xffc32121), const Color(0xff831616)];
  final List<Color> _attackActiveColors = [const Color(0xFF424242), const Color(0xFF212121)];
  UserCommand _currentCommand = UserCommand();
  bool _isForwardPressed = false;
  bool _isBackPressed = false;
  bool _isLeftPressed = false;
  bool _isRightPressed = false;
  bool _isStarted = false;
  bool _isGunTriggerPressed = false;
  final GlobalKey _playerKey = GlobalKey();
  static const platform = MethodChannel('com.yourcompany/gamepad');
  bool _gamepadConnected = false;
  Timer? _commandTimer;
  Map<String, double> _gamepadAxisValues = {};
  Uint8List? _lastSentPacket;
  final SettingsService _settingsService = SettingsService();
  bool _isLoading = true;
  String _robotIpAddress = '';
  List<String> _cameraUrls = [];
  int _currentCameraIndex = -1;
  Key _gstreamerViewKey = UniqueKey();
  bool _isGStreamerReady = false;
  bool _isGStreamerLoading = true;
  bool _gstreamerHasError = false;
  MethodChannel? _gstreamerChannel;
  bool _isGStreamerInitialized = false;
  bool _novalidurl = false;
  Timer? _streamTimeoutTimer;
  bool _isManualTouchModeEnabled = false;


  @override
  void initState() {
    super.initState();
    _loadSettingsAndInitialize();
    platform.setMethodCallHandler(_handleGamepadEvent);
  }

  @override
  void dispose() {
    _commandTimer?.cancel();
    // _gstreamerChannel?.invokeMethod('stopStream');
    _stopGStreamer();
    super.dispose();
  }

  void _stopGStreamer() {
    if (_gstreamerChannel != null && _isGStreamerReady) {
      print("DEBUG_DISPOSE: Invoking GStreamer stopStream.");
      try {
        _gstreamerChannel!.invokeMethod('stopStream').catchError((e) {
          print("DEBUG_DISPOSE: Error invoking stopStream: $e");
        });
      } catch (e) {
        print("DEBUG_DISPOSE: Exception invoking stopStream: $e");
      }
    }

    setState(() {
      _isGStreamerReady = false;
      _isGStreamerLoading = true;
      _gstreamerHasError = false;
      _errorMessage = null;
      _gstreamerViewKey = UniqueKey(); // Invalidate key to force rebuild on next load
      _isGStreamerInitialized = false; // Reset initialization flag
    });
    print("DEBUG_DISPOSE: GStreamer state reset.");
  }


  Future<void> _loadSettingsAndInitialize() async {
    _robotIpAddress = await _settingsService.loadIpAddress();
    _cameraUrls = await _settingsService.loadCameraUrls();

    if (mounted) {
      _switchCamera(0);
      _startCommandTimer();
      setState(() {
        _isLoading = false;
      });
    }
  }

  Future<void> _handleGStreamerMessages(MethodCall call) async {
    if (!mounted) return;

    _streamTimeoutTimer?.cancel(); // Cancel timeout on any message from native

    switch (call.method) {
      case 'onStreamReady':
        print("GSTREAMER_CALLBACK: Stream is ready and playing.");
        setState(() {
          _isGStreamerLoading = false;
          _gstreamerHasError = false;
          _novalidurl = false;
          _errorMessage = null;
        });
        break;

      case 'onStreamError':
        final String? error = call.arguments['error'];
        print("GSTREAMER_CALLBACK: Received stream error: $error");
        setState(() {
          _isGStreamerLoading = false;
          _gstreamerHasError = true;
          _novalidurl = true;
          _errorMessage = "Stream error";
        });
        break;
    }
  }

  void _onGStreamerPlatformViewCreated(int id) {
    _gstreamerChannel = MethodChannel('gstreamer_channel_$id');
    _gstreamerChannel!.setMethodCallHandler(_handleGStreamerMessages); // Set the handler

    setState(() {
      _isGStreamerReady = true;
      _isGStreamerLoading = false;
      _gstreamerHasError = false;
      _errorMessage = null;
    });
    _playCurrentCameraStream();
  }


  Future<void> _playCurrentCameraStream() async {
    _streamTimeoutTimer?.cancel();

    if (_cameraUrls.isEmpty || _currentCameraIndex < 0 || _currentCameraIndex >= _cameraUrls.length) {
      print("ERROR: No valid camera URL configured.");
      if (mounted) {
        setState(() {
          _isGStreamerLoading = false;
          _gstreamerHasError = true;
          _novalidurl = true;
          _errorMessage = "No valid camera URL. Please check settings.";
        });
      }
      return;
    }

    if (_gstreamerChannel == null || !_isGStreamerReady) {
      print("ERROR: GStreamer channel is not ready.");
      if (mounted) {
        setState(() {
          _isGStreamerLoading = false;
          _gstreamerHasError = true;
          _novalidurl = true;
          _errorMessage = "GStreamer channel not ready.";
        });
      }
      return;
    }

    print("INFO: Attempting to start GStreamer stream...");
    setState(() {
      _isGStreamerLoading = true;
      _gstreamerHasError = false;
      _novalidurl = false;
      _errorMessage = null;
    });

    const timeoutDuration = Duration(seconds: 8); // Increased timeout slightly
    _streamTimeoutTimer = Timer(timeoutDuration, () {
      if (mounted && _isGStreamerLoading) {
        print("ERROR: GStreamer stream timed out after $timeoutDuration.");
        setState(() {
          _gstreamerHasError = true;
          _novalidurl = true;
          _errorMessage = "Connection timed out. Check network.";
          _isGStreamerLoading = false;
        });
      }
    });

    try {
      final String url = _cameraUrls[_currentCameraIndex];
      // The pipeline is now created in the native code, we just send the URL
      await _gstreamerChannel!.invokeMethod('startStream', {'url': url});
    } on PlatformException catch (e) {
      print("GStreamer start failed (PlatformException): ${e.message}");
      _streamTimeoutTimer?.cancel();
      if (mounted) {
        setState(() {
          _gstreamerHasError = true;
          _novalidurl = true;
          _errorMessage = "Platform error: ${e.message}";
          _isGStreamerLoading = false;
        });
      }
    } catch (e) {
      print("GStreamer start failed (Unknown Error): $e");
      _streamTimeoutTimer?.cancel();
      if (mounted) {
        setState(() {
          _gstreamerHasError = true;
          _novalidurl = true;
          _errorMessage = "An unknown error occurred.";
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
        setState(() => _currentCommand.commandId = 0);
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

        _currentCommand.gunTrigger = _isGunTriggerPressed;
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
      // The port for touch commands
      const int TOUCH_PORT = 65433;

      // Create a UDP socket, send the data, and immediately close the socket.
      // This is extremely fast and avoids connection state issues.
      final socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
      socket.send(coord.toBytes(), InternetAddress(_robotIpAddress), TOUCH_PORT);
      socket.close();

      print('Sent Touch Packet (UDP): X=${coord.x}, Y=${coord.y}');
    } catch (e) {
      print('Error sending touch UDP packet: $e');
    }
  }

  void _onLeftButtonPressed(int index, int commandId) {
    setState(() {
      _activeLeftButtonIndex = index;
      _currentCommand.commandId = commandId;
      _isManualTouchModeEnabled = false;
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
        _isGStreamerLoading = false; // Not loading if no URL
        _gstreamerHasError = true;
        _novalidurl = true;
        _errorMessage = "No cameras configured.";
        _isGStreamerReady = false;
      });
      return;
    }

    if (index == _currentCameraIndex && !_gstreamerHasError) {
      return; // Don't reload if it's the same and not in an error state
    }

    // Call stopStream on the existing channel before creating a new view
    if (_gstreamerChannel != null && _isGStreamerReady) {
      try {
        await _gstreamerChannel!.invokeMethod('stopStream');
      } catch (e) {
        print("Error stopping previous stream: $e");
      }
    }

    setState(() {
      _currentCameraIndex = index;
      _gstreamerViewKey = UniqueKey(); // This forces the AndroidView to be recreated
      _isGStreamerReady = false;
      _isGStreamerLoading = true;
      _gstreamerHasError = false;
      _novalidurl = false;
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
      {required BuildContext context, required String iconPath, required String title, required Color titleColor, required String content}) async {
    return await showDialog<bool>(
      context: context,
      barrierDismissible: false,
      builder: (BuildContext dialogContext) {
        return Dialog(
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20.0)),
          child: Container(
            width: 500,
            padding: const EdgeInsets.all(24.0),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Row(
                  children: [
                    Image.asset(iconPath, height: 40, width: 40),
                    const SizedBox(width: 16),
                    Text(title,
                      // style: GoogleFonts.notoSans(fontSize: 28, fontWeight: FontWeight.bold, color: titleColor)
                      style: TextStyle(
                        fontFamily: 'NotoSans',
                        fontWeight: FontWeight.w700, // For Medium
                        // or FontWeight.w700 for Bold
                        fontSize: 28,
                        color: titleColor,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 24),
                Text(content, textAlign: TextAlign.center,
                  // style: GoogleFonts.notoSans(fontSize: 22, color: Colors.black87)
                  style: TextStyle(
                    fontFamily: 'NotoSans',
                    fontWeight: FontWeight.w700, // For Medium
                    // or FontWeight.w700 for Bold
                    fontSize: 22,
                    color: Colors.black87,
                  ),
                ),
                const SizedBox(height: 32),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    TextButton(
                      onPressed: () => Navigator.of(dialogContext).pop(false),
                      child: Text('Cancel',
                        // style: GoogleFonts.notoSans(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.grey.shade700)
                        style: TextStyle(
                          fontFamily: 'NotoSans',
                          fontWeight: FontWeight.w700, // For Medium
                          // or FontWeight.w700 for Bold
                          fontSize: 23,
                          color: Colors.grey.shade700,
                        ),

                      ),
                    ),
                    SizedBox(height: 40, child: VerticalDivider(color: Colors.grey.shade400, thickness: 1)),
                    TextButton(
                      onPressed: () => Navigator.of(dialogContext).pop(true),
                      child: Text('OK',
                        // style: GoogleFonts.notoSans(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.blue.shade700)
                        style: TextStyle(
                          fontFamily: 'NotoSans',
                          fontWeight: FontWeight.w700, // For Medium
                          // or FontWeight.w700 for Bold
                          fontSize: 23,
                          color: Colors.blue.shade700,
                        ),
                      ),
                    ),
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
    if (_isAutoAttackMode) { // Currently in Auto Attack, switching OFF
      final proceed = await _showCustomConfirmationDialog(
          context: context,
          iconPath: ICON_PATH_AUTO_ATTACK_ACTIVE,
          title: "DANGERS",
          titleColor: Colors.red,
          content: 'Are you sure you want to stop\n"Auto Attack" mode?');
      if (proceed) {
        setState(() {
          _isAutoAttackMode = false;
          _currentCommand.commandId = 0; // Assuming 0 is a neutral/stop command
          // When Auto Attack is turned off, return the selector to the default state (e.g., Driving)
          _activeLeftButtonIndex = -1; // Set to the index for "Driving"
          _isManualTouchModeEnabled = false;
        });
      }
    } else { // Currently NOT in Auto Attack, switching ON to Manual Attack
      final proceed = await _showCustomConfirmationDialog(
          context: context,
          iconPath: ICON_PATH_MANUAL_ATTACK_ACTIVE,
          title: "DANGERS",
          titleColor: Colors.red,
          content: 'Are you sure you want to start\n"Auto Attack" mode?');
      if (proceed) {
        setState(() {
          _isAutoAttackMode = true;
          _currentCommand.commandId = 4; // Command ID for Manual/Auto Attack
          // *** ADD THIS LINE ***
          // When entering Manual/Auto Attack, highlight the Manual Attack button visually
          _activeLeftButtonIndex = 3; // Set to the index for "Manual Attack"
        });
      }
    }
  }

  Widget buildPlayerWidgetGestureDetector() {
    return GestureDetector(
      key: _playerKey,
      behavior: HitTestBehavior.opaque, // Ensure GestureDetector captures all touch events
      // onTapDown: (_) {
      //   print("DEBUG_TOUCH: Touch DOWN detected on video stream area."); // New debug print for touch down
      // },
      onTapUp: (details) {
        if (_isManualTouchModeEnabled) {
          if (_activeLeftButtonIndex == 3) {
            // print("DEBUG_TOUCH: Manual Attack mode detected (_activeLeftButtonIndex == 3).");
            final RenderBox? renderBox = _playerKey.currentContext
                ?.findRenderObject() as RenderBox?;
            if (renderBox == null) {
              print(
                  "DEBUG_TOUCH: ERROR: Could not get RenderBox for _playerKey.");
              return;
            }
            final Offset localPosition = renderBox.globalToLocal(
                details.globalPosition);
            final double x = localPosition.dx / renderBox.size.width;
            final double y = localPosition.dy / renderBox.size.height;
            if (x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0) {
              setState(() {
                _currentCommand.touchX = x;
                _currentCommand.touchY = y;
              });
              _sendTouchPacket(TouchCoord()
                ..x = x
                ..y = y);
            } else {
              print("DEBUG_TOUCH: Coords out of bounds. Not processing touch.");
            }
          } else {
            print(
                "DEBUG_TOUCH: Not in Manual Attack mode (_activeLeftButtonIndex != 3). Touch ignored.");
          }
        }
      },
      child: Container(color: Colors.transparent),
    );
  }

  Widget _buildStreamOverlay() {
    if (_isGStreamerLoading) {
      return Container(
        color: Colors.black.withOpacity(0.5),
        child: const Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              CircularProgressIndicator(color: Colors.white),
              SizedBox(height: 20),
              Text(
                'Connecting to stream...',
                style: TextStyle(color: Colors.white, fontSize: 18),
              ),
            ],
          ),
        ),
      );
    }

    if (_novalidurl && _gstreamerHasError) {
      return Container(
        color: Colors.black.withOpacity(0.7),
        child: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const Icon(Icons.error_outline, color: Colors.redAccent, size: 70),
              const SizedBox(height: 20),
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 40.0),
                child: Text(
                  _errorMessage ?? 'Stream failed to load.',
                  textAlign: TextAlign.center,
                  style: const TextStyle(color: Colors.white, fontSize: 20),
                ),
              ),
              const SizedBox(height: 30),
              ElevatedButton.icon(
                icon: const Icon(Icons.refresh),
                label: const Text('Retry'),
                onPressed: () {
                  if (_currentCameraIndex != -1) {
                    _switchCamera(_currentCameraIndex);
                  }
                },
                style: ElevatedButton.styleFrom(
                  foregroundColor: Colors.white,
                  backgroundColor: Colors.blueGrey,
                  padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 15),
                  textStyle: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ),
            ],
          ),
        ),
      );
    }

    return const SizedBox.shrink();
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
          Positioned.fill(child: buildPlayerWidgetGestureDetector()),
          Positioned.fill(
            child: IgnorePointer(
              child: KeyedSubtree(
                key: _gstreamerViewKey,
                child: AndroidView(
                  viewType: 'gstreamer_view',
                  onPlatformViewCreated: _onGStreamerPlatformViewCreated,
                  creationParamsCodec: const StandardMessageCodec(),
                ),
              ),
            ),
          ),

          Positioned(child: _buildStreamOverlay()),

          _buildLeftButton(
              30, 35, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE,
              _activeLeftButtonIndex != -1 && _activeLeftButtonIndex == 0,
                  () => _onLeftButtonPressed(0, 1)
          ),
          _buildLeftButton(
              30, 185, "Petrol", ICON_PATH_PETROL_INACTIVE, ICON_PATH_PETROL_ACTIVE,
              _activeLeftButtonIndex != -1 && _activeLeftButtonIndex == 1,
                  () => _onLeftButtonPressed(1, 2)
          ),
          _buildLeftButton(
              30, 335, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE,
              _activeLeftButtonIndex != -1 && _activeLeftButtonIndex == 2,
                  () => _onLeftButtonPressed(2, 2)
          ),

          _buildLeftButton(
              30, 485, _isAutoAttackMode ? "Auto Attack" : "Manual Attack",
              _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_INACTIVE : ICON_PATH_MANUAL_ATTACK_INACTIVE,
              _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_ACTIVE : ICON_PATH_MANUAL_ATTACK_ACTIVE,
              _activeLeftButtonIndex == 3,
                  () {
                // When this button is pressed, ensure _activeLeftButtonIndex is set to 3
                setState(() {
                  _activeLeftButtonIndex = 3;
                  _isManualTouchModeEnabled = true; // <-- ADD THIS LINE to enable touch mode
                });
                _handleManualAutoAttackToggle();
              }
          ),

          _buildLeftButton(
              30, 635, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE,
              _activeLeftButtonIndex != -1 && _activeLeftButtonIndex == 4,
                  () => _onLeftButtonPressed(4, 3)
          ),
          _buildLeftButton(
              30, 785, "Return", ICON_PATH_RETURN_INACTIVE, ICON_PATH_RETURN_ACTIVE,
              _activeLeftButtonIndex != -1 && _activeLeftButtonIndex == 5,
                  () => _onLeftButtonPressed(5, 0)
          ),

          // --- RIGHT BUTTONS ---
          _buildRightButton(1690, 30, "Attack View", ICON_PATH_ATTACK_VIEW_INACTIVE, ICON_PATH_ATTACK_VIEW_ACTIVE, _activeRightButtonIndex == 0, () => _onRightButtonPressed(0)),
          _buildRightButton(1690, 250, "Top View", ICON_PATH_TOP_VIEW_INACTIVE, ICON_PATH_TOP_VIEW_ACTIVE, _activeRightButtonIndex == 1, () => _onRightButtonPressed(1)),
          _buildRightButton(1690, 470, "3D View", ICON_PATH_3D_VIEW_INACTIVE, ICON_PATH_3D_VIEW_ACTIVE, _activeRightButtonIndex == 2, () => _onRightButtonPressed(2)),
          _buildRightButton(1690, 720, "Setting", ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, false, () => _navigateToSettings()),

          // --- DIRECTIONAL BUTTONS --- (No changes needed here)
          _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE, () => setState(() => _isForwardPressed = true), () => setState(() => _isForwardPressed = false)),
          _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE, () => setState(() => _isBackPressed = true), () => setState(() => _isBackPressed = false)),
          _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE, () => setState(() => _isLeftPressed = true), () => setState(() => _isLeftPressed = false)),
          _buildDirectionalButton(1540, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE, () => setState(() => _isRightPressed = true), () => setState(() => _isRightPressed = false)),
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

  Widget _buildLeftButton(double left, double top, String label, String inactiveIcon, String activeIcon, bool isActive, VoidCallback onPressed) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    // *** THIS IS THE ONLY CHANGE NEEDED ***
    // The background color is now determined by the 'isActive' state, not a temporary press.
    final containerColor = isActive ? Color(0xFFc32121) : Colors.black.withOpacity(0.6);

    return Positioned(
      left: left * widthScale,
      top: top * heightScale,
      child: GestureDetector(
        onTap: onPressed, // Use the simple onTap
        child: Container(
          width: 200 * widthScale,
          height: 120 * heightScale,
          padding: EdgeInsets.symmetric(vertical: 8.0 * heightScale),
          decoration: BoxDecoration(
            color: containerColor, // <-- APPLY THE COLOR HERE
            borderRadius: BorderRadius.circular(10),
            border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.0),
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [
              Expanded(flex: 2, child: Image.asset(isActive ? activeIcon : inactiveIcon, fit: BoxFit.contain)),
              SizedBox(height: 5 * heightScale),
              Text(label, textAlign: TextAlign.center,
                style: TextStyle(
                  fontFamily: 'NotoSans',
                  fontWeight: FontWeight.bold,
                  fontSize: 24 * heightScale,
                  color: Colors.white,
                ),
              ),
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

    // The background color is determined by the 'isActive' state.
    final containerColor = isActive ? Color(0xFFc32121) : Colors.black.withOpacity(0.6);

    return Positioned(
      left: left * widthScale,
      top: top * heightScale,
      child: GestureDetector(
        onTap: onPressed, // Use the simple onTap
        child: Container(
          width: buttonWidth * widthScale,
          height: buttonHeight * heightScale,
          padding: EdgeInsets.symmetric(vertical: 10.0 * heightScale),
          decoration: BoxDecoration(
            color: containerColor,
            borderRadius: BorderRadius.circular(15),
            border: Border.all(
              color: isActive ? Colors.white : Colors.transparent,
              width: 2.5,
            ),
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [
              Expanded(
                flex: 3,
                child: Image.asset(isActive ? activeIcon : inactiveIcon,
                    fit: BoxFit.contain),
              ),
              SizedBox(height: 8 * heightScale),
              Text(
                label,
                textAlign: TextAlign.center,
                style: TextStyle(
                  fontFamily: 'NotoSans',
                  fontWeight: FontWeight.w700,
                  fontSize: 26 * heightScale,
                  color: Colors.white,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildDirectionalButton(double left, double top, bool isPressed, String inactiveIcon, String activeIcon, VoidCallback onPress, VoidCallback onRelease) {
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
                    _activeLeftButtonIndex = 3;
                  });
                }
              } else {
                setState(() {
                  _isAttackModeOn = false;
                  _currentCommand.commandId = 0;
                  _activeLeftButtonIndex = -1;
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
                  _isGunTriggerPressed = _isStarted;
                  if (!_isStarted) {
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
              }),
          SizedBox(width: 12 * widthScale),
          _buildBottomBarButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () {}),
          SizedBox(width: 12 * widthScale),
          _buildBottomBarButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () {}),
        ];

        final List<Widget> middleCluster = [
          Row(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.baseline,
            textBaseline: TextBaseline.alphabetic,
            children: [
              Text("0",
                // style: GoogleFonts.notoSans(fontSize: 80, fontWeight: FontWeight.bold, color: Colors.white)
                style: TextStyle(
                  fontFamily: 'NotoSans',
                  fontWeight: FontWeight.w700, // For Medium
                  // or FontWeight.w700 for Bold
                  fontSize: 75,
                  color: Colors.white,
                ),
              ),
              SizedBox(width: 8 * widthScale),
              Text("Km/h",
                // style: GoogleFonts.notoSans(fontSize: 36, fontWeight: FontWeight.w500, color: Colors.white)
                style: TextStyle(
                  fontFamily: 'NotoSans',
                  fontWeight: FontWeight.w700, // For Medium
                  // or FontWeight.w700 for Bold
                  fontSize: 37,
                  color: Colors.white,
                ),
              ),
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
            Text(label,
              // style: GoogleFonts.notoSans(fontSize: 36 * heightScale, fontWeight: FontWeight.bold, color: Colors.white)
              style: TextStyle(
                fontFamily: 'NotoSans',
                fontWeight: FontWeight.w700, // For Medium
                // or FontWeight.w700 for Bold
                fontSize: 36 * heightScale,
                color: Colors.white,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildBottomBarButton(String label, String iconPath, List<Color> gradientColors, VoidCallback onPressed) {
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
              Text(label,
                // style: GoogleFonts.notoSans(fontSize: 36 * heightScale, fontWeight: FontWeight.bold, color: Colors.white)
                style: TextStyle(
                  fontFamily: 'NotoSans',
                  fontWeight: FontWeight.w700, // For Medium
                  // or FontWeight.w700 for Bold
                  fontSize: 36 * heightScale,
                  color: Colors.white,
                ),
              ),
            ]
          ],
        ),
      ),
    );
  }
}