import 'dart:async';
import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:rtest1/settings_service.dart';
import 'CommandIds.dart';
import 'TouchCoord.dart';
import 'UserCommand.dart';
import 'icon_constants.dart';
import 'splash_screen.dart';
import 'settings_menu_page.dart';

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
      theme: ThemeData(primarySwatch: Colors.blue),
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
  // --- STATE MANAGEMENT ---
  UserCommand _currentCommand = UserCommand();
  bool _isLoading = true;
  Timer? _commandTimer;

  // NEW: State variables for the new UI logic
  int _selectedModeIndex = -1;
  bool _isModeActive = false;
  bool _isPermissionToAttackOn = false;

  // On-screen button press states
  bool _isForwardPressed = false;
  bool _isBackPressed = false;
  bool _isLeftPressed = false;
  bool _isRightPressed = false;

  // Gamepad State
  static const platform = MethodChannel('com.yourcompany/gamepad');
  bool _gamepadConnected = false;
  Map<String, double> _gamepadAxisValues = {};
  bool _isZoomInPressed = false;
  bool _isZoomOutPressed = false;


  // GStreamer State
  String? _errorMessage;
  final SettingsService _settingsService = SettingsService();
  String _robotIpAddress = '';
  List<String> _cameraUrls = [];
  int _currentCameraIndex = 0;
  Key _gstreamerViewKey = UniqueKey();
  bool _isGStreamerReady = false;
  bool _isGStreamerLoading = true;
  bool _gstreamerHasError = false;
  MethodChannel? _gstreamerChannel;
  Timer? _streamTimeoutTimer;

  final Map<int, int> _buttonIndexToCommandId = {
    0: CommandIds.DRIVING,
    1: CommandIds.RECON,
    2: CommandIds.MANUAL_ATTACK,
    3: CommandIds.AUTO_ATTACK,
    4: CommandIds.DRONE,
  };

  final Map<int, Color> _buttonActiveColor = {
    0: const Color(0xff25a625), // Green for Driving
    1: const Color(0xff25a625), // CHANGED: Green for Recon
    2: const Color(0xffc32121), // Red for Manual Attack
    3: const Color(0xffc32121), // Red for Auto Attack
    4: const Color(0xff25a625), // CHANGED: Green for Drone
  };

  final List<Color> _permissionDisabledColors = [const Color(0xFF424242), const Color(0xFF212121)]; // Black/Grey
  final List<Color> _permissionOffColors = [const Color(0xffc32121), const Color(0xff831616)];      // Red
  final List<Color> _permissionOnColors = [const Color(0xff6b0000), const Color(0xff520000)];

  @override
  void initState() {
    super.initState();
    _loadSettingsAndInitialize();
    platform.setMethodCallHandler(_handleGamepadEvent);
  }

  @override
  void dispose() {
    _commandTimer?.cancel();
    _stopGStreamer();
    super.dispose();
  }

  // --- LOGIC METHODS ---

  void _onModeSelected(int index) {
    setState(() {
      // CHANGED: If a mode is currently active, do nothing, as per client request.
      if (_isModeActive) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('Please press STOP before selecting a new mode.'),
            duration: Duration(seconds: 2),
          ),
        );
        return;
      }
      _selectedModeIndex = index;
    });
  }

  void _onStartStopPressed() {
    setState(() {
      if (_isModeActive) {
        _stopCurrentMode();
      } else {
        if (_selectedModeIndex != -1) {
          _isModeActive = true;
          _currentCommand.command_id = _buttonIndexToCommandId[_selectedModeIndex] ?? CommandIds.IDLE;
        }
      }
    });
  }

  void _stopCurrentMode() {
    _isModeActive = false;
    _selectedModeIndex = -1;
    _currentCommand.command_id = CommandIds.IDLE;
    _isForwardPressed = false;
    _isBackPressed = false;
    _isLeftPressed = false;
    _isRightPressed = false;
  }

  void _onPermissionPressed() {
    setState(() {
      _isPermissionToAttackOn = !_isPermissionToAttackOn;
      _currentCommand.attack_permission = _isPermissionToAttackOn;
    });
  }

  Future<void> _handleGamepadEvent(MethodCall call) async {
    if (!mounted) return;
    if (!_gamepadConnected) setState(() => _gamepadConnected = true);

    if (call.method == "onMotionEvent") {
      setState(() => _gamepadAxisValues = Map<String, double>.from(call.arguments));
    } else if (call.method == "onButtonDown") {
      final String button = call.arguments['button'];
      setState(() {
        switch (button) {
          case 'KEYCODE_BUTTON_B': // Start
            _onStartStopPressed();
            break;
          case 'KEYCODE_BUTTON_A': // Stop
            if (_isModeActive) _onStartStopPressed();
            break;
          case 'KEYCODE_BUTTON_X': // Permission
            _onPermissionPressed();
            break;
          case 'KEYCODE_BUTTON_L1':
            _isZoomInPressed = true;
            break;
          case 'KEYCODE_BUTTON_L2':
            _isZoomOutPressed = true;
            break;
        }
      });
    } else if (call.method == "onButtonUp") {
      final String button = call.arguments['button'];
      setState(() {
        switch (button) {
          case 'KEYCODE_BUTTON_L1':
            _isZoomInPressed = false;
            break;
          case 'KEYCODE_BUTTON_L2':
            _isZoomOutPressed = false;
            break;
        }
      });
    }
  }

  void _startCommandTimer() {
    _commandTimer = Timer.periodic(const Duration(milliseconds: 100), (timer) {
      if (_gamepadConnected) {
        _currentCommand.move_speed = ((_gamepadAxisValues['AXIS_Y'] ?? 0.0) * -100).round();
        _currentCommand.turn_angle = ((_gamepadAxisValues['AXIS_X'] ?? 0.0) * 100).round();
        _currentCommand.tilt_speed = ((_gamepadAxisValues['AXIS_RZ'] ?? 0.0) * -100).round();
        _currentCommand.pan_speed = ((_gamepadAxisValues['AXIS_Z'] ?? 0.0) * 100).round();

        // gun_trigger is removed as per client request

        _currentCommand.zoom_command = 0; // Default to no zoom
        if (_isZoomInPressed) {
          _currentCommand.zoom_command = 1; // Zoom In
        } else if (_isZoomOutPressed || (_gamepadAxisValues['AXIS_BRAKE'] ?? 0.0) > 0.5) {
          _currentCommand.zoom_command = -1; // Zoom Out (digital or analog)
        }

      } else {
        _currentCommand.move_speed = 0;
        if (_isForwardPressed) _currentCommand.move_speed = 100;
        if (_isBackPressed) _currentCommand.move_speed = -100;
        _currentCommand.turn_angle = 0;
        if (_isLeftPressed) _currentCommand.turn_angle = -100;
        if (_isRightPressed) _currentCommand.turn_angle = 100;

        _currentCommand.pan_speed = 0;
        _currentCommand.tilt_speed = 0;
        _currentCommand.zoom_command = 0;
      }

      _sendCommandPacket(_currentCommand);

      if (_currentCommand.touch_x != -1.0) {
        _currentCommand.touch_x = -1.0;
        _currentCommand.touch_y = -1.0;
      }
    });
  }

  // --- UI WIDGETS ---

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.black,
      body: _isLoading
          ? const Center(child: CircularProgressIndicator())
          : Stack(
        children: [
          Positioned.fill(
            child: KeyedSubtree(
              key: _gstreamerViewKey,
              child: AndroidView(
                viewType: 'gstreamer_view',
                onPlatformViewCreated: _onGStreamerPlatformViewCreated,
              ),
            ),
          ),

          if (!_isGStreamerLoading && !_gstreamerHasError)
            Positioned.fill(child: _buildTouchDetector()),

          Positioned.fill(child: _buildStreamOverlay()),
          // Positioned.fill(child: _buildTouchDetector()),

          _buildModeButton(0, 30, 30, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE),
          _buildModeButton(1, 30, 214, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE),
          _buildModeButton(2, 30, 398, "Manual Attack", ICON_PATH_MANUAL_ATTACK_INACTIVE, ICON_PATH_MANUAL_ATTACK_ACTIVE),
          _buildModeButton(3, 30, 582, "Auto Attack", ICON_PATH_AUTO_ATTACK_INACTIVE, ICON_PATH_AUTO_ATTACK_ACTIVE),
          _buildModeButton(4, 30, 766, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE),

          _buildViewButton(1690, 30, "Attack View", ICON_PATH_ATTACK_VIEW_ACTIVE, true),
          _buildViewButton(1690, 720, "Setting", ICON_PATH_SETTINGS, false, onPressed: _navigateToSettings),

          _buildDirectionalControls(),
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

  Widget _buildModeButton(int index, double left, double top, String label, String inactiveIcon, String activeIcon) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    final bool isSelected = _selectedModeIndex == index;
    final Color color = isSelected ? (_buttonActiveColor[index] ?? Colors.grey) : Colors.black.withOpacity(0.6);
    final String icon = isSelected ? activeIcon : inactiveIcon;

    return Positioned(
      left: left * widthScale,
      top: top * heightScale,
      child: GestureDetector(
        onTap: () => _onModeSelected(index),
        child: Container(
          width: 200 * widthScale,
          height: 120 * heightScale,
          padding: EdgeInsets.symmetric(vertical: 4.0 * heightScale),
          decoration: BoxDecoration(
            color: color,
            borderRadius: BorderRadius.circular(9),
            border: Border.all(color: isSelected ? Colors.white : Colors.transparent, width: 2.0),
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Expanded(flex: 2, child: Image.asset(icon, fit: BoxFit.contain)),
              SizedBox(height: 5 * heightScale),
              Text(label, textAlign: TextAlign.center, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.bold, fontSize: 25 * heightScale, color: Colors.white)),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildViewButton(double left, double top, String label, String iconPath, bool isActive, {VoidCallback? onPressed}) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    final Color color = isActive ? const Color(0xFFc32121) : Colors.black.withOpacity(0.6);

    return Positioned(
      left: left * widthScale,
      top: top * heightScale,
      child: GestureDetector(
        onTap: onPressed,
        child: Container(
          width: 220 * widthScale,
          height: 175 * heightScale,
          padding: EdgeInsets.symmetric(vertical: 10.0 * heightScale),
          decoration: BoxDecoration(
            color: color,
            borderRadius: BorderRadius.circular(15),
            border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.5),
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Expanded(flex: 3, child: Image.asset(iconPath, fit: BoxFit.contain)),
              SizedBox(height: 8 * heightScale),
              Text(label, textAlign: TextAlign.center, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 26 * heightScale, color: Colors.white)),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildBottomBar() {
    final screenWidth = MediaQuery.of(context).size.width;
    final widthScale = screenWidth / 1920.0;

    return LayoutBuilder(
      builder: (context, constraints) {

        List<Color> permissionButtonColors;
        bool isCombatModeActive = _isModeActive && (_selectedModeIndex == 2 || _selectedModeIndex == 3);

        if (isCombatModeActive) {
          // If a combat mode is active, the button is either ON or OFF
          permissionButtonColors = _isPermissionToAttackOn ? _permissionOnColors : _permissionOffColors;
        } else {
          // If IDLE or in a non-combat mode, the button is DISABLED
          permissionButtonColors = _permissionDisabledColors;
        }

        final List<Widget> leftCluster = [
          _buildBottomBarButton(
            "PERMISSION TO ATTACK",
            null,
            _isPermissionToAttackOn ? [const Color(0xffc32121), const Color(0xff831616)] : [const Color(0xFF424242), const Color(0xFF212121)],
            _onPermissionPressed,
          ),

          SizedBox(width: 12 * widthScale),
          _buildWideBottomBarButton(
            _isModeActive ? "STOP" : "START",
            _isModeActive ? ICON_PATH_STOP : ICON_PATH_START,
            [const Color(0xff25a625), const Color(0xff127812)],
            _onStartStopPressed,
          ),
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
              const Text("0", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 75, color: Colors.white)),
              SizedBox(width: 8 * widthScale),
              const Text("Km/h", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 37, color: Colors.white)),
            ],
          ),
          SizedBox(width: 20 * widthScale),
          Image.asset(ICON_PATH_WIFI, height: 40),
        ];

        final List<Widget> rightCluster = [
          _buildBottomBarButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)], () async {
            final proceed = await _showExitDialog();
            if (proceed) SystemNavigator.pop();
          }),
        ];

        return Row(
          children: [
            ...leftCluster,
            const Spacer(),
            ...middleCluster,
            const Spacer(),
            ...rightCluster,
          ],
        );
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
        constraints: BoxConstraints(minWidth: 220 * widthScale),
        height: 80 * heightScale,
        padding: const EdgeInsets.symmetric(horizontal: 74),
        decoration: BoxDecoration(
          gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
          borderRadius: BorderRadius.circular(25 * heightScale),
        ),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Image.asset(iconPath, height: 36 * heightScale),
            const SizedBox(width: 12),
            Text(label, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 36 * heightScale, color: Colors.white)),
          ],
        ),
      ),
    );
  }

  Widget _buildBottomBarButton(String label, String? iconPath, List<Color> gradientColors, VoidCallback onPressed) {
    final screenHeight = MediaQuery.of(context).size.height;
    final heightScale = screenHeight / 1080.0;

    return GestureDetector(
      onTap: onPressed,
      child: Container(
        height: 80 * heightScale,
        padding: const EdgeInsets.symmetric(horizontal: 30),
        decoration: BoxDecoration(
          gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
          borderRadius: BorderRadius.circular(25 * heightScale),
        ),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            if (iconPath != null && iconPath.isNotEmpty) ...[
              Image.asset(iconPath, height: 36 * heightScale),
              if (label.isNotEmpty) const SizedBox(width: 12),
            ],
            if (label.isNotEmpty)
              Text(label, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 30 * heightScale, color: Colors.white)),
          ],
        ),
      ),
    );
  }

  Widget _buildDirectionalControls() {
    return Stack(
      children: [
        _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE, () => setState(() => _isForwardPressed = true), () => setState(() => _isForwardPressed = false)),
        _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE, () => setState(() => _isBackPressed = true), () => setState(() => _isBackPressed = false)),
        _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE, () => setState(() => _isLeftPressed = true), () => setState(() => _isLeftPressed = false)),
        _buildDirectionalButton(1540, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE, () => setState(() => _isRightPressed = true), () => setState(() => _isRightPressed = false)),
      ],
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
        child: Image.asset(isPressed ? activeIcon : inactiveIcon, height: 100 * heightScale, width: 100 * widthScale, fit: BoxFit.contain),
      ),
    );
  }

  Widget _buildTouchDetector() {
    return GestureDetector(
      key: const GlobalObjectKey('_playerKey'),
      behavior: HitTestBehavior.opaque,
      onTapUp: (details) {
        if (_isModeActive && _selectedModeIndex == 2) { // 2 corresponds to Manual Attack
          final RenderBox? renderBox = context.findRenderObject() as RenderBox?;
          if (renderBox != null) {
            final Offset localPosition = renderBox.globalToLocal(details.globalPosition);
            final double x = localPosition.dx / renderBox.size.width;
            final double y = localPosition.dy / renderBox.size.height;
            if (x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0) {
              _sendTouchPacket(TouchCoord()..x = x..y = y);
            }
          }
        }
      },
      child: Container(color: Colors.transparent),
    );
  }

  Future<bool> _showExitDialog() async {
    return await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Confirm Exit'),
        content: const Text('Do you want to close the application?'),
        actions: [
          TextButton(onPressed: () => Navigator.of(context).pop(false), child: const Text('Cancel')),
          TextButton(onPressed: () => Navigator.of(context).pop(true), child: const Text('Exit')),
        ],
      ),
    ) ?? false;
  }

  Future<void> _handleGStreamerMessages(MethodCall call) async {
    if (!mounted) return;
    _streamTimeoutTimer?.cancel();
    switch (call.method) {
      case 'onStreamReady':
        setState(() { _isGStreamerLoading = false; _gstreamerHasError = false; });
        break;
      case 'onStreamError':
        final String? error = call.arguments?['error'];
        setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "Stream error: ${error ?? 'Unknown native error'}"; });
        break;
    }
  }

  void _retryStream() {
    if (_currentCameraIndex == -1) {
      print("Retry failed: No camera index selected.");
      return;
    }

    print("Retrying stream for camera index: $_currentCameraIndex");

    setState(() {
      _gstreamerViewKey = UniqueKey(); // This is the MOST IMPORTANT line. It forces the widget to be destroyed and recreated.
      _isGStreamerReady = false;
      _isGStreamerLoading = true; // Show the loading spinner again
      _gstreamerHasError = false; // Clear the error state
      _errorMessage = null;
    });

  }

  Future<void> _loadSettingsAndInitialize() async {
    _robotIpAddress = await _settingsService.loadIpAddress();
    _cameraUrls = await _settingsService.loadCameraUrls();
    if (mounted) {
      _switchCamera(0);
      _startCommandTimer();
      setState(() => _isLoading = false);
    }
  }

  Future<void> _sendCommandPacket(UserCommand command) async {
    if (_robotIpAddress.isEmpty) return;
    try {
      final socket = await Socket.connect(_robotIpAddress, 65432, timeout: const Duration(milliseconds: 150));
      socket.add(command.toBytes());
      await socket.flush();
      socket.close();
    } catch (e) {}
  }

  Future<void> _sendTouchPacket(TouchCoord coord) async {
    if (_robotIpAddress.isEmpty) return;
    try {
      const int TOUCH_PORT = 65433;
      final socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
      socket.send(coord.toBytes(), InternetAddress(_robotIpAddress), TOUCH_PORT);
      socket.close();
      print('Sent Touch Packet (UDP): X=${coord.x}, Y=${coord.y}');
    } catch (e) {
      print('Error sending touch UDP packet: $e');
    }
  }

  void _stopGStreamer() {
    if (_gstreamerChannel != null && _isGStreamerReady) {
      _gstreamerChannel!.invokeMethod('stopStream').catchError((e) => print("Error stopping stream: $e"));
    }
    _isGStreamerReady = false;
  }

  Future<void> _switchCamera(int index) async {
    if (_cameraUrls.isEmpty || index < 0 || index >= _cameraUrls.length) return;
    if (index == _currentCameraIndex && !_gstreamerHasError) return;
    if (_gstreamerChannel != null && _isGStreamerReady) {
      try { await _gstreamerChannel!.invokeMethod('stopStream'); } catch (e) {}
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


  void _onGStreamerPlatformViewCreated(int id) {
    _gstreamerChannel = MethodChannel('gstreamer_channel_$id');
    _gstreamerChannel!.setMethodCallHandler(_handleGStreamerMessages);
    setState(() {
      _isGStreamerReady = true;
      _isGStreamerLoading = false;
    });
    _playCurrentCameraStream();
  }

  Future<void> _playCurrentCameraStream() async {
    _streamTimeoutTimer?.cancel();
    if (_cameraUrls.isEmpty || _currentCameraIndex < 0 || _currentCameraIndex >= _cameraUrls.length) {
      if (mounted) setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "No valid camera URL."; });
      return;
    }
    if (_gstreamerChannel == null || !_isGStreamerReady) {
      if (mounted) setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "GStreamer channel not ready."; });
      return;
    }
    setState(() { _isGStreamerLoading = true; _gstreamerHasError = false; _errorMessage = null; });
    _streamTimeoutTimer = Timer(const Duration(seconds: 8), () {
      if (mounted && _isGStreamerLoading) {
        setState(() { _gstreamerHasError = true; _errorMessage = "Connection timed out."; _isGStreamerLoading = false; });
      }
    });
    try {
      final String url = _cameraUrls[_currentCameraIndex];
      await _gstreamerChannel!.invokeMethod('startStream', {'url': url});
    } catch (e) {
      _streamTimeoutTimer?.cancel();
      if (mounted) {
        setState(() { _gstreamerHasError = true; _errorMessage = "Failed to start stream: ${e.toString()}"; _isGStreamerLoading = false; });
      }
    }
  }

  Widget _buildStreamOverlay() {
    if (_isGStreamerLoading) {
      return Container(
        color: Colors.black.withOpacity(0.5),
        child: const Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [CircularProgressIndicator(color: Colors.white), SizedBox(height: 20), Text('Connecting to stream...', style: TextStyle(color: Colors.white, fontSize: 18))])),
      );
    }
    if (_gstreamerHasError) {
      return Container(
        color: Colors.black.withOpacity(0.7),
        child: Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
          const Icon(Icons.error_outline, color: Colors.redAccent, size: 70),
          const SizedBox(height: 20),
          Padding(padding: const EdgeInsets.symmetric(horizontal: 40.0), child: Text(_errorMessage ?? 'Stream failed to load.', textAlign: TextAlign.center, style: const TextStyle(color: Colors.white, fontSize: 20))),
          const SizedBox(height: 30),
          ElevatedButton.icon(
            icon: const Icon(Icons.refresh),
            label: const Text('Retry'),
            onPressed: () { if (_currentCameraIndex != -1) _switchCamera(_currentCameraIndex); },
            style: ElevatedButton.styleFrom(foregroundColor: Colors.white, backgroundColor: Colors.blueGrey, padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 15), textStyle: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          ),
        ])),
      );
    }
    return const SizedBox.shrink();
  }

  Future<void> _navigateToSettings() async {
    final bool? settingsChanged = await Navigator.push<bool>(context, MaterialPageRoute(builder: (context) => const SettingsMenuPage()));
    if (settingsChanged == true && mounted) {
      _commandTimer?.cancel();
      await _loadSettingsAndInitialize();
    }
  }
}




