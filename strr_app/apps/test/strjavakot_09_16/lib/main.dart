import 'dart:async';
import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:rtest1/settings_service.dart';
import 'package:wifi_iot/wifi_iot.dart';
import 'CommandIds.dart';
import 'ServerStatus.dart';
import 'StatusPacket.dart';
import 'TouchCoord.dart';
import 'UserCommand.dart';
import 'icon_constants.dart';
import 'splash_screen.dart';
import 'settings_menu_page.dart';
import 'package:flutter_joystick/flutter_joystick.dart';
import 'DrivingCommand.dart';
import 'package:wakelock_plus/wakelock_plus.dart';

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


// class _HomePageState extends State<HomePage> {
class _HomePageState extends State<HomePage> with WidgetsBindingObserver {
  // --- STATE MANAGEMENT ---
  UserCommand _currentCommand = UserCommand();
  bool _isLoading = true;
  Timer? _commandTimer;

  // State variables for the UI logic
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

  double _currentZoomLevel = 1.0;
  double _lateralWindSpeed = 0.0;
  int _windDirectionIndex = 0;
  double _pendingLateralWindSpeed = 0.0;

  final TransformationController _transformationController = TransformationController();

  Socket? _statusSocket;
  StreamSubscription? _statusSocketSubscription;
  bool _isServerConnected = false;

  ServerStatus _serverStatus = ServerStatus.disconnected();

  bool _isUiZoomInPressed = false;
  bool _isUiZoomOutPressed = false;
  int _confirmedServerModeId = CommandIds.IDLE;

  bool _permissionRequestIsActive = false; // From server
  bool _permissionHasBeenGranted = false;  // Local UI state

  bool _isStoppingMode = false;
  bool _isSwitchingCamera = false;

  bool _showCrosshair = false;
  double _crosshairX = -1.0;
  double _crosshairY = -1.0;

  Timer? _wifiSignalTimer;
  int _wifiSignalLevel = 0;

  bool _wasGamepadActive = false;

  RawDatagramSocket? _drivingSocket;
  RawDatagramSocket? _touchSocket;

  Socket? _commandSocket;

  bool _isNavigatingAway = false;
  bool _isReinitializing = false;

  String _videoQuality = 'sd';

  String _bitrateMode = 'cbr';
  int _targetBitrate = 2000000;

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

  final List<Color> _permissionDisabledColors = [const Color(0xffcccccc), const Color(0xffcccccc)]; // Black/Grey
  final List<Color> _permissionOffColors = [const Color(0xffc32121), const Color(0xff831616)];      // Red
  final List<Color> _permissionOnColors = [const Color(0xff6b0000), const Color(0xff520000)];

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this); // Register to listen for events
    WakelockPlus.enable();
    _initializeSockets();

    _loadSettingsAndInitialize();
    platform.setMethodCallHandler(_handleGamepadEvent);

    _startWifiSignalChecker();
  }

  Future<void> _initializeSockets() async {
    try {
      _drivingSocket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
      _touchSocket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
      print("UDP Sockets initialized successfully.");
    } catch (e) {
      print("Error initializing UDP sockets: $e");
    }
  }

  @override
  void dispose() {
    WakelockPlus.disable();
    _commandTimer?.cancel();
    _wifiSignalTimer?.cancel();

    _drivingSocket?.close();
    _touchSocket?.close();  

    _commandSocket?.destroy(); 

    _stopGStreamer();
    _transformationController.dispose();
    _statusSocketSubscription?.cancel();
    _statusSocket?.destroy();
    WidgetsBinding.instance.removeObserver(this); // Unregister the listener
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    super.didChangeAppLifecycleState(state);
    print("App lifecycle state changed to: $state");

    // Check if the GStreamer channel is even available to talk to.
    if (_gstreamerChannel == null || !_isGStreamerReady) {
      return;
    }

    if (state == AppLifecycleState.paused) {
      // App is going to the background. Pause the stream.
      print("App paused, pausing GStreamer stream.");
      try {
        _gstreamerChannel!.invokeMethod('pause'); 
      } catch (e) {
        print("Error pausing stream: $e");
      }
    } else if (state == AppLifecycleState.resumed) {
      print("App resumed, ensuring GStreamer stream is playing.");
      try {
        _gstreamerChannel!.invokeMethod('play'); 
      } catch (e) {
        print("Error playing stream on resume: $e");
      }
    }
  }

  Future<void> _connectCommandServer() async {
    if (_robotIpAddress.isEmpty || _commandSocket != null) return;

    print("Attempting to connect to command server...");
    try {
      _commandSocket = await Socket.connect(_robotIpAddress, 65432, timeout: const Duration(seconds: 3));
      print("Connected to command server!");

      // Listen for data (optional) and errors/disconnections
      _commandSocket!.listen(
            (Uint8List data) {
        },
        onError: (error) {
          print("Command socket error: $error");
          _handleCommandDisconnect();
        },
        onDone: () {
          print("Command server disconnected.");
          _handleCommandDisconnect();
        },
        cancelOnError: true,
      );
    } catch (e) {
      print("Failed to connect to command server: $e");
      _handleCommandDisconnect();
    }
  }

  void _handleCommandDisconnect() {
    if (!mounted) return;

    _commandSocket?.destroy();
    _commandSocket = null;

    // Attempt to reconnect after a delay
    Future.delayed(const Duration(seconds: 5), () {
      if (mounted) {
        _connectCommandServer();
      }
    });
  }

  String _getWifiIconPath() {
    switch (_wifiSignalLevel) {
      case 0:
        return ICON_PATH_WIFI_0; // Disconnected
      case 1:
        return ICON_PATH_WIFI_1; // Poor
      case 2:
        return ICON_PATH_WIFI_2; // Fair
      case 3:
        return ICON_PATH_WIFI_3; // Good
      case 4:
        return ICON_PATH_WIFI_4; // Excellent
      default:
        return ICON_PATH_WIFI_0;
    }
  }

  //  Function to periodically check Wi-Fi signal ---
  void _startWifiSignalChecker() {
    _wifiSignalTimer = Timer.periodic(const Duration(seconds: 3), (timer) async {
      try {
        // 1. First, check if the Wi-Fi is enabled on the device at all.
        bool? isWifiEnabled = await WiFiForIoTPlugin.isEnabled();

        // If Wi-Fi is disabled or we can't determine its state, show the "disconnected" icon.
        if (isWifiEnabled != true) {
          if (mounted) {
            setState(() => _wifiSignalLevel = 0);
          }
          return; // Stop further processing
        }

        // 2. Only if Wi-Fi is enabled, proceed to get the signal strength.
        int? rssi = await WiFiForIoTPlugin.getCurrentSignalStrength();

        if (rssi == null) {
          // This can happen if Wi-Fi is on but not connected to a network.
          setState(() => _wifiSignalLevel = 0);
          return;
        }

        // Map the RSSI value to our 0-4 level system.
        int level;
        if (rssi >= -55) {
          level = 4; // Excellent
        } else if (rssi >= -67) {
          level = 3; // Good
        } else if (rssi >= -80) {
          level = 2; // Fair
        } else {
          level = 1; // Poor
        }

        if (mounted) {
          setState(() => _wifiSignalLevel = level);
        }
      } catch (e) {
        print("Could not get Wi-Fi signal strength: $e");
        if (mounted) {
          setState(() => _wifiSignalLevel = 0);
        }
      }
    });
  }

  Future<void> _connectToStatusServer() async {
    // If we are already connected or have no IP, do nothing.
    if (_robotIpAddress.isEmpty || _isServerConnected) return;

    print("Attempting to connect to status server...");
    try {
      const int STATUS_PORT = 65435;
      // Try to connect the socket with a timeout.
      _statusSocket = await Socket.connect(_robotIpAddress, STATUS_PORT, timeout: const Duration(seconds: 3));

      // If the line above doesn't throw an exception, the connection was successful.
      if (mounted) {
        setState(() {
          _isServerConnected = true;
        });
      }
      print("Connected to status server!");

      // Listen for data, errors, or the connection closing.
      _statusSocketSubscription = _statusSocket!.listen(
            (Uint8List data) {
          // This is where you handle incoming data from the server.
          try {
            if (_isStoppingMode) return;
            final status = StatusPacket.fromBytes(data);
            if (mounted) {
              setState(() {
                _confirmedServerModeId = status.currentModeId;
                bool serverRequest = status.permissionRequestActive == 1;
                _crosshairX = status.crosshairX;
                _crosshairY = status.crosshairY;

                if (!serverRequest) {
                  _permissionRequestIsActive = false;
                  _permissionHasBeenGranted = false;
                } else {
                  _permissionRequestIsActive = true;
                }
              });
            }
          } catch (e) {
            print("Error parsing status packet: $e");
          }
        },
        onError: (error) {
          print("Status socket error: $error");
          _handleDisconnect(); // Use the centralized handler for all disconnect events.
        },
        onDone: () {
          print("Status server disconnected.");
          _handleDisconnect(); // Use the centralized handler for all disconnect events.
        },
        cancelOnError: true,
      );
    } catch (e) {
      // This catch block handles connection failures (e.g., timeout, host not found).
      print("Failed to connect to status server: $e");
      _handleDisconnect(); // Use the centralized handler for all disconnect events.
    }
  }

  void _handleDisconnect() {
    if (!mounted) return;

    // Only trigger a state update and reconnect if we were previously connected.
    if (_isServerConnected) {
      setState(() {
        _isServerConnected = false;
      });
    }

    // Clean up old resources before attempting to reconnect
    _statusSocketSubscription?.cancel();
    _statusSocket?.destroy();
    _statusSocket = null;
    _statusSocketSubscription = null;

    // Schedule a single reconnect attempt after a delay.
    // This prevents a rapid, resource-leaking loop.
    Future.delayed(const Duration(seconds: 5), () {
      if (mounted && !_isServerConnected) {
        _connectToStatusServer();
      }
    });
  }

  void _reconnectStatusServer() {
    // Simple reconnect logic: try again after a delay
    Future.delayed(const Duration(seconds: 5), () {
      if (mounted && !_isServerConnected) {
        print("Attempting to reconnect to status server...");
        _connectToStatusServer();
      }
    });
  }


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
    _isStoppingMode = true;
    _isModeActive = false;
    _selectedModeIndex = -1;
    _currentCommand.command_id = CommandIds.IDLE;
    _isForwardPressed = false;
    _isBackPressed = false;
    _isLeftPressed = false;
    _isRightPressed = false;
    _isPermissionToAttackOn = false;
    _currentCommand.attack_permission = false;
    _permissionRequestIsActive = false;
    _permissionHasBeenGranted = false;

    Future.delayed(const Duration(milliseconds: 500), () {
      if (mounted) {
        setState(() {
          _isStoppingMode = false;
        });
      }
    });
  }

  void _onPermissionPressed() {
    if (_permissionRequestIsActive && !_permissionHasBeenGranted) {
      setState(() {
        _isPermissionToAttackOn = true; // Send the "ON" command
        _currentCommand.attack_permission = true;
        _permissionHasBeenGranted = true; // Lock the button in the "Permitted" state
      });
    }
  }

  Future<void> _handleGamepadEvent(MethodCall call) async {
    if (!mounted) return;

    //  Handle disconnection robustly ---
    if (call.method == "onGamepadDisconnected") {
      setState(() {
        _gamepadConnected = false;
        _gamepadAxisValues.clear();
        _isZoomInPressed = false; // Reset gamepad-specific button states
        _isZoomOutPressed = false;
        print("Gamepad disconnected.");
      });
      return; // Stop further processing
    }

    if (!_gamepadConnected) {
      setState(() {
        _gamepadConnected = true;
        _pendingLateralWindSpeed = _lateralWindSpeed;
        print("Gamepad connected.");
      });
    }

    if (call.method == "onMotionEvent") {
      final newAxisValues = Map<String, double>.from(call.arguments);
      final double hatX = newAxisValues['AXIS_HAT_X'] ?? 0.0;
      final double prevHatX = _gamepadAxisValues['AXIS_HAT_X'] ?? 0.0;
      if (hatX != 0 && prevHatX == 0) {
        setState(() {
          if (hatX > 0.5) _pendingLateralWindSpeed += 0.1;
          else if (hatX < -0.5) _pendingLateralWindSpeed -= 0.1;
        });
      }
      setState(() => _gamepadAxisValues = newAxisValues);
    } else if (call.method == "onButtonDown") {
      final String button = call.arguments['button'];
      setState(() {
        switch (button) {
          case 'KEYCODE_BUTTON_B': _onStartStopPressed(); break;
          case 'KEYCODE_BUTTON_A': if (_isModeActive) _onStartStopPressed(); break;
          case 'KEYCODE_BUTTON_X':
            if ((_pendingLateralWindSpeed - _lateralWindSpeed).abs() > 0.01) {
              _lateralWindSpeed = _pendingLateralWindSpeed;
              ScaffoldMessenger.of(context).showSnackBar(
                SnackBar(content: Text('Wind speed set to ${_lateralWindSpeed.toStringAsFixed(1)}'), duration: const Duration(seconds: 2)),
              );
            } else {
              _onPermissionPressed();
            }
            break;
          case 'KEYCODE_BUTTON_L1': _isZoomInPressed = true; break;
          case 'KEYCODE_BUTTON_L2': _isZoomOutPressed = true; break;
        }
      });
    } else if (call.method == "onButtonUp") {
      final String button = call.arguments['button'];
      setState(() {
        switch (button) {
          case 'KEYCODE_BUTTON_L1': _isZoomInPressed = false; break;
          case 'KEYCODE_BUTTON_L2': _isZoomOutPressed = false; break;
        }
      });
    }
  }

  void _startCommandTimer() {
    bool wasGamepadDriving = false;
    bool wasGamepadAiming = false;

    _commandTimer = Timer.periodic(const Duration(milliseconds: 100), (timer) {
      // 1. Calculate Raw Inputs from Gamepad
      int gpMove = 0;
      int gpTurn = 0;
      int gpPan = 0;
      int gpTilt = 0;

      if (_gamepadConnected) {
        // --- LEFT STICK (Hybrid Analog + Hat) ---
        double analogMove = (_gamepadAxisValues['AXIS_Y'] ?? 0.0) * -100;
        double analogTurn = (_gamepadAxisValues['AXIS_X'] ?? 0.0) * 100;
        double hatMove    = (_gamepadAxisValues['AXIS_HAT_Y'] ?? 0.0) * -100;
        double hatTurn    = (_gamepadAxisValues['AXIS_HAT_X'] ?? 0.0) * 100;

        // Use whichever is stronger
        double rawMove = analogMove.abs() > hatMove.abs() ? analogMove : hatMove;
        double rawTurn = analogTurn.abs() > hatTurn.abs() ? analogTurn : hatTurn;

        // --- RIGHT STICK (Hybrid Z/RZ + RX/RY) ---
        double axisZ  = (_gamepadAxisValues['AXIS_Z'] ?? 0.0) * 100;
        double axisRZ = (_gamepadAxisValues['AXIS_RZ'] ?? 0.0) * -100;
        double axisRX = (_gamepadAxisValues['AXIS_RX'] ?? 0.0) * 100;
        double axisRY = (_gamepadAxisValues['AXIS_RY'] ?? 0.0) * -100;

        double rawPan  = axisZ.abs() > axisRX.abs() ? axisZ : axisRX;
        double rawTilt = axisRZ.abs() > axisRY.abs() ? axisRZ : axisRY;

        // --- DEADZONE PROCESSING ---
        if (rawMove.abs() > 15 || rawTurn.abs() > 15) {
          gpMove = rawMove.round().clamp(-100, 100);
          gpTurn = rawTurn.round().clamp(-100, 100);
        }
        if (rawPan.abs() > 15 || rawTilt.abs() > 15) {
          gpPan = rawPan.round().clamp(-100, 100);
          gpTilt = rawTilt.round().clamp(-100, 100);
        }
      }

      // --- DRIVING LOGIC ---
      int finalMove = 0;
      int finalTurn = 0;

      if (gpMove != 0 || gpTurn != 0) {
        // Case A: Gamepad is moving -> Use Gamepad
        finalMove = gpMove;
        finalTurn = gpTurn;
        wasGamepadDriving = true; // Remember we are using gamepad
      } else if (wasGamepadDriving) {
        // Case B: Gamepad JUST stopped (went to 0) -> Force 0 to stop robot
        finalMove = 0;
        finalTurn = 0;
        wasGamepadDriving = false; // Reset flag
      } else {
        // Case C: Gamepad is idle -> Use Virtual Joystick
        finalMove = _currentCommand.move_speed;
        finalTurn = _currentCommand.turn_angle;
      }

      // --- AIMING LOGIC ---
      int finalPan = 0;
      int finalTilt = 0;

      if (gpPan != 0 || gpTilt != 0) {
        // Case A: Gamepad is moving -> Use Gamepad
        finalPan = gpPan;
        finalTilt = gpTilt;
        wasGamepadAiming = true; // Remember we are using gamepad
      } else if (wasGamepadAiming) {
        // Case B: Gamepad JUST stopped -> Force 0 to stop camera
        finalPan = 0;
        finalTilt = 0;
        wasGamepadAiming = false; // Reset flag
      } else {
        // Case C: Gamepad is idle -> Use Virtual Joystick
        finalPan = _currentCommand.pan_speed;
        finalTilt = _currentCommand.tilt_speed;
      }

      // 3. UPDATE STATE
      DrivingCommand drivingCommand = DrivingCommand(
        move_speed: finalMove,
        turn_angle: finalTurn,
      );

      _currentCommand.pan_speed = finalPan;
      _currentCommand.tilt_speed = finalTilt;
      _currentCommand.lateral_wind_speed = _lateralWindSpeed;

      // --- ZOOM LOGIC ---
      _currentCommand.zoom_command = 0;
      if (_isZoomInPressed || _isUiZoomInPressed) {
        _currentCommand.zoom_command = 1;
      } else if (_isZoomOutPressed ||
          (_gamepadAxisValues['AXIS_LTRIGGER'] ?? 0.0) > 0.5 ||
          (_gamepadAxisValues['AXIS_BRAKE'] ?? 0.0) > 0.5 ||
          _isUiZoomOutPressed) {
        _currentCommand.zoom_command = -1;
      }
      if ((_gamepadAxisValues['AXIS_RTRIGGER'] ?? 0.0) > 0.5) {
        _currentCommand.zoom_command = 1;
      }

      // --- SETTINGS ---
      _currentCommand.resolution_setting = (_videoQuality == 'hd') ? 1 : 0;
      if (_bitrateMode == 'vbr') _currentCommand.bitrate_mode = 1;
      else if (_bitrateMode == 'cbr') _currentCommand.bitrate_mode = 2;
      else _currentCommand.bitrate_mode = 0;
      _currentCommand.target_bitrate = _targetBitrate;

      // 4. SEND DATA
      _sendCommandPacket(_currentCommand);
      _sendDrivingPacket(drivingCommand);
    });
  }

  Future<void> _sendDrivingPacket(DrivingCommand command) async {
    if (_robotIpAddress.isEmpty || _drivingSocket == null) return;
    try {
      const int DRIVING_PORT = 65434;
      _drivingSocket!.send(command.toBytes(), InternetAddress(_robotIpAddress), DRIVING_PORT);
    } catch (e) {
      print("Error sending driving packet: $e");
    }
  }

  Widget _buildCrosshair() {
    bool isCrosshairVisible = _confirmedServerModeId == CommandIds.MANUAL_ATTACK ||
        _confirmedServerModeId == CommandIds.AUTO_ATTACK ||
        _confirmedServerModeId == CommandIds.RECON ||
        _confirmedServerModeId == CommandIds.DRONE;

    if (!isCrosshairVisible) {
      return const SizedBox.shrink(); // Don't show anything if not in a relevant mode
    }

    bool isTargetLocked = _crosshairX >= 0.0 && _crosshairY >= 0.0;

    // Determine color based on lock state
    final Color crosshairColor = isTargetLocked ? Colors.red : Colors.white;

    // Define the size here. You can now change this to 450, 900, or any other value,
    // and the centering will still work perfectly.
    final double crosshairSize = 450.0;

    return Image.asset(
      'assets/new_icons/crosshair.png',
      width: crosshairSize,
      height: crosshairSize,
      color: crosshairColor,
      colorBlendMode: BlendMode.srcIn,
    );
  }

  Widget _buildModeStatusBanner() {
    final screenWidth = MediaQuery.of(context).size.width;
    final widthScale = screenWidth / 1920.0;

    final Map<int, String> modeIdToText = {
      CommandIds.DRIVING: "DRIVING MODE",
      CommandIds.RECON: "RECON MODE",
      CommandIds.MANUAL_ATTACK: "MANUAL ATTACK MODE",
      CommandIds.AUTO_ATTACK: "AUTO ATTACK MODE",
      CommandIds.DRONE: "DRONE MODE",
    };

    final String statusText = modeIdToText[_confirmedServerModeId] ?? "";

    if (statusText.isEmpty) {
      return const SizedBox.shrink();
    }

    // Use an Align widget for easy centering
    return Align(
      alignment: Alignment.topCenter,
      child: Padding(
        // Use padding to push it down from the top edge
        padding: EdgeInsets.only(top: 40 * widthScale),
        child: Container(
          padding: EdgeInsets.symmetric(horizontal: 24 * widthScale, vertical: 12 * widthScale),
          decoration: BoxDecoration(
            color: Colors.black.withOpacity(0.6),
            borderRadius: BorderRadius.circular(10),
          ),
          child: Text(
            statusText,
            style: TextStyle(
              fontFamily: 'NotoSans',
              fontSize: 40 * widthScale,
              fontWeight: FontWeight.w600,
              color: Colors.white,
            ),
          ),
        ),
      ),
    );
  }

  // Widget to display the current zoom level
  Widget _buildZoomDisplay() {
    final screenWidth = MediaQuery.of(context).size.width;
    final widthScale = screenWidth / 1920.0;

    return Positioned(
      // Positioned according to the client's reference image (x=1360, y=30)
      top: 30 * widthScale,
      right: (1920 - 1360 - 200) * widthScale, // Approximate right position based on image
      child: Container(
        padding: EdgeInsets.symmetric(horizontal: 20 * widthScale, vertical: 10 * widthScale),
        decoration: BoxDecoration(
          color: Colors.black.withOpacity(0.5),
          borderRadius: BorderRadius.circular(10),
        ),
        child: Text(
          '${_currentZoomLevel.toStringAsFixed(1)} x', // Formats to one decimal place, e.g., "1.3 x"
          style: TextStyle(
            fontFamily: 'NotoSans',
            fontSize: 60 * widthScale, // Scaled font size
            fontWeight: FontWeight.w600, // Medium weight
            color: Colors.white,
          ),
        ),
      ),
    );
  }

  Widget _buildWindIndicator() {
    final screenWidth = MediaQuery.of(context).size.width;
    final widthScale = screenWidth / 1920.0;

    final String currentWindIcon = ICON_PATH_WIND_W;

    return Positioned(
      top: 40 * widthScale,
      left: 280 * widthScale,
      child: Row(
        children: [
          Image.asset(
            currentWindIcon,
            width: 40 * widthScale,
            height: 40 * widthScale,
          ),
          SizedBox(width: 10 * widthScale),
          Text(
            (_gamepadConnected ? _pendingLateralWindSpeed : _lateralWindSpeed).toStringAsFixed(1),
            style: TextStyle(
              fontFamily: 'NotoSans',
              fontSize: 60 * widthScale,
              fontWeight: FontWeight.w600,
              color: Colors.white,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildMovementJoystick() {
    final screenWidth = MediaQuery.of(context).size.width;
    final widthScale = screenWidth / 1920.0;

    return Positioned(
      left: 260 * widthScale,
      bottom: 220 * widthScale,
      child: Opacity(
        opacity: _isServerConnected ? 1.0 : 0.5,
        child: Joystick(
          includeInitialAnimation: false,
          mode: JoystickMode.all,
          stick: const CircleAvatar(
            radius: 30,
            backgroundColor: Colors.blue,
          ),
          base: Container(
            width: 150,
            height: 150,
            decoration: BoxDecoration(
              color: Colors.grey,
              shape: BoxShape.circle,
              border: Border.all(color: Colors.white38, width: 2),
            ),
          ),
          listener: (details) {
            if (!_isServerConnected) return; // Do nothing if disconnected
            setState(() {
              // Y-axis controls forward/backward speed
              _currentCommand.move_speed = (details.y * -100).round();
              // X-axis controls left/right turning
              _currentCommand.turn_angle = (details.x * 100).round();
            });
          },
        ),
      ),
    );
  }

  Widget _buildPanTiltJoystick() {
    final screenWidth = MediaQuery.of(context).size.width;
    final widthScale = screenWidth / 1920.0;

    return Positioned(
      right: 260 * widthScale,
      bottom: 220 * widthScale,
      child: Opacity(
        opacity: _isServerConnected ? 1.0 : 0.5,
        child: Joystick(
          includeInitialAnimation: false,
          mode: JoystickMode.all,
          stick: const CircleAvatar(
            radius: 30,
            backgroundColor: Colors.blue,
          ),
          base: Container(
            width: 150,
            height: 150,
            decoration: BoxDecoration(
              color: Colors.grey,
              shape: BoxShape.circle,
              border: Border.all(color: Colors.white38, width: 2),
            ),
          ),
          listener: (details) {
            if (!_isServerConnected) return; // Do nothing if disconnected
            setState(() {
              // X-axis controls pan (left/right) speed
              _currentCommand.pan_speed = (details.x * 100).round();
              // Y-axis controls tilt (up/down) speed
              _currentCommand.tilt_speed = (details.y * -100).round(); // Y is often inverted for camera controls
            });
          },
        ),
      ),
    );
  }

  Widget _buildPermissionButton({
    required String label,
    required String backgroundImagePath,
    required VoidCallback? onPressed,
  }) {
    final screenHeight = MediaQuery.of(context).size.height;
    final screenWidth = MediaQuery.of(context).size.width;
    final heightScale = screenHeight / 1080.0;
    final widthScale = screenWidth / 1920.0;
    final bool isEnabled = onPressed != null;

    final double buttonWidth = 450 * widthScale;
    final double buttonHeight = 80 * heightScale;

    return GestureDetector(
      onTap: onPressed,
      child: Opacity(
        opacity: isEnabled ? 1.0 : 1.0,
        child: SizedBox(
          width: buttonWidth,
          height: buttonHeight,
          // Use a Stack to layer the background image and the text.
          child: Stack(
            fit: StackFit.expand, // Make the Stack's children fill the SizedBox
            children: [
              // 1. The Background Image
              // Use ClipRRect to enforce the rounded corners on the image.
              ClipRRect(
                borderRadius: BorderRadius.circular(22 * heightScale), // Adjust this value for more/less rounding
                child: Image.asset(
                  backgroundImagePath,
                  fit: BoxFit.cover, // Cover maintains aspect ratio while filling
                ),
              ),
              // 2. The Text, centered on top of the image
              Center(
                child: Text(
                  label,
                  style: TextStyle(
                    fontFamily: 'NotoSans',
                    fontWeight: FontWeight.w700,
                    fontSize: 34 * heightScale,
                    color: Colors.white,
                    shadows: [
                      Shadow(
                        blurRadius: 2.0,
                        color: Colors.black.withOpacity(0.5),
                        offset: Offset(1.0, 1.0),
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildConnectionStatusBanner() {
    if (_isServerConnected) {
      return const SizedBox.shrink();
    }
    return Positioned(
      top: 0,
      left: 0,
      right: 0,
      child: Container(
        padding: const EdgeInsets.all(8.0),
        color: Colors.red.withOpacity(0.8),
        child: const Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(Icons.error, color: Colors.white, size: 16),
            SizedBox(width: 8),
            Text(
              'Connection Lost - Attempting to reconnect...',
              style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
            ),
          ],
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;

    // --- Calculate crosshair position here ---
    bool useServerPosition = _crosshairX >= 0.0 && _crosshairY >= 0.0;
    final double targetX = useServerPosition ? _crosshairX : 0.5;
    final double targetY = useServerPosition ? _crosshairY : 0.5;
    final double crosshairSize = 900.0;
    final double left = (targetX * screenWidth) - (crosshairSize / 2);
    final double top = (targetY * screenHeight) - (crosshairSize / 2);

    return Scaffold(
      backgroundColor: Colors.black,
      body: _isLoading
          ? const Center(child: CircularProgressIndicator())
          : Stack(
        children: [
          // --- LAYER 1: VIDEO ---
          Positioned.fill(
            child: InteractiveViewer(
              transformationController: _transformationController,
              minScale: 1.0,
              maxScale: 5.0,
              panEnabled: false,
              child: _isNavigatingAway
                  ? Container(color: Colors.black) // Show a black placeholder while away
                  : KeyedSubtree( // Re-create the view when returning
                key: _gstreamerViewKey,
                child: AndroidView(
                  viewType: 'gstreamer_view',
                  onPlatformViewCreated: _onGStreamerPlatformViewCreated,
                ),
              ),
            ),
          ),

          // --- LAYER 2: DANGER OVERLAY (Visual only) ---
          _buildDangerOverlay(),

          // --- LAYER 3: STREAM OVERLAYS (Loading/Error) ---
          Positioned.fill(child: _buildStreamOverlay()),

          // --- LAYER 4: TOUCH DETECTOR ---
          if (!_isGStreamerLoading && !_gstreamerHasError)
            Positioned.fill(child: _buildTouchDetector()),

          // --- LAYER 5: VISUAL-ONLY UI ELEMENTS ---
          _buildZoomDisplay(),
          _buildModeStatusBanner(),
          _buildConnectionStatusBanner(),

          Align(
            // The alignment property takes normalized coordinates from -1.0 to 1.0.
            // We need to convert our server coordinates (0.0 to 1.0) to this range.
            // Formula: (value * 2) - 1
            alignment: Alignment(
              (_crosshairX >= 0.0 ? _crosshairX * 2 - 1 : 0.0), // Default to center (0.0) if no server X
              (_crosshairY >= 0.0 ? _crosshairY * 2 - 1 : 0.0), // Default to center (0.0) if no server Y
            ),
            child: IgnorePointer(
              child: _buildCrosshair(),
            ),
          ),


          // --- LAYER 6: INTERACTIVE UI ELEMENTS ---
          _buildModeButton(0, 30, 30, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE),
          _buildModeButton(1, 30, 214, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE),
          _buildModeButton(2, 30, 398, "Manual Attack", ICON_PATH_MANUAL_ATTACK_INACTIVE, ICON_PATH_MANUAL_ATTACK_ACTIVE),
          _buildModeButton(3, 30, 582, "Auto Attack", ICON_PATH_AUTO_ATTACK_INACTIVE, ICON_PATH_AUTO_ATTACK_ACTIVE),
          _buildModeButton(4, 30, 766, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE),

          _buildViewButton(
            1690, 30, "",
            ICON_PATH_DAY_VIEW_ACTIVE,
            ICON_PATH_DAY_VIEW_INACTIVE,
            _currentCameraIndex == 0,
            onPressed: () => _switchCamera(0),
          ),
          _buildViewButton(
            1690, 214, "",
            ICON_PATH_NIGHT_VIEW_ACTIVE,
            ICON_PATH_NIGHT_VIEW_INACTIVE,
            _currentCameraIndex == 1,
            onPressed: () => _switchCamera(1),
          ),
          _buildViewButton(
              1690, 720, "Setting",
              ICON_PATH_SETTINGS,
              ICON_PATH_SETTINGS,
              false,
              onPressed: _navigateToSettings
          ),

          _buildMovementJoystick(),
          _buildPanTiltJoystick(),

          Align(
            alignment: Alignment.bottomCenter,
            child: Container(
              margin: const EdgeInsets.only(bottom: 20, left: 20, right: 20),
              decoration: BoxDecoration(color: Colors.black.withOpacity(0.6), borderRadius: BorderRadius.circular(50)),
              child: _buildBottomBar(),
            ),
          ),

          if (_isReinitializing) _buildReinitializingOverlay(),
        ],
      ),
    );
  }

  Widget _buildReinitializingOverlay() {
    return Positioned.fill(
      child: Container(
        color: Colors.black.withOpacity(0.6),
        child: const Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              CircularProgressIndicator(color: Colors.white),
              SizedBox(height: 20),
              Text(
                'Applying new settings...',
                style: TextStyle(color: Colors.white, fontSize: 18, decoration: TextDecoration.none),
              ),
            ],
          ),
        ),
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
      child: Opacity(
        opacity: _isServerConnected ? 1.0 : 0.5,
        child: GestureDetector(
          // onTap: () => _onModeSelected(index),
          onTap: _isServerConnected ? () => _onModeSelected(index) : null,
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
      ),
    );
  }

  Widget _buildViewButton(
      double left,
      double top,
      String label,
      String activeIconPath,
      String inactiveIconPath,
      bool isActive,
      {VoidCallback? onPressed}
      ) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    final Color color = isActive ? Colors.grey : Colors.black.withOpacity(0.6);
    final String iconToDisplay = isActive ? activeIconPath : inactiveIconPath;

    return Positioned(
      left: left * widthScale,
      top: top * heightScale,
      child: GestureDetector(
        onTap: onPressed,
        child: Container(
          width: 220 * widthScale,
          height: 175 * heightScale,
          // This allows the image to fill the entire container.
          decoration: BoxDecoration(
            color: color,
            borderRadius: BorderRadius.circular(15),
            border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.5),
            image: label.isEmpty // Conditionally add the image to the decoration
                ? DecorationImage(
              image: AssetImage(iconToDisplay),
              fit: BoxFit.cover, // This makes the image fill the space
            )
                : null,
          ),
          // Use a child only if the label is NOT empty (for the 'Setting' button)
          child: label.isNotEmpty
              ? Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Expanded(
                  flex: 3,
                  child: Image.asset(iconToDisplay, fit: BoxFit.contain)
              ),
              SizedBox(height: 8 * heightScale),
              Text(
                  label,
                  textAlign: TextAlign.center,
                  style: TextStyle(
                      fontFamily: 'NotoSans',
                      fontWeight: FontWeight.w700,
                      fontSize: 26 * heightScale,
                      color: Colors.white
                  )
              ),
            ],
          )
              : null, // If the label is empty, the container has no child.
        ),
      ),
    );
  }

  // A dedicated builder for the custom zoom buttons ---
  Widget _buildZoomButton({
    required String iconPath,
    required VoidCallback? onPressed,
  }) {
    final screenHeight = MediaQuery.of(context).size.height;
    final heightScale = screenHeight / 1080.0;
    final bool isEnabled = onPressed != null;

    return GestureDetector(
      onTap: onPressed,
      child: Opacity(
        opacity: isEnabled ? 1.0 : 0.5,
        child: Container(
          width: 100 * heightScale,  // Make the button wider
          height: 80 * heightScale,
          decoration: BoxDecoration(
            borderRadius: BorderRadius.circular(15 * heightScale), // Less rounded corners
            image: DecorationImage(
              // This assumes you have a metallic texture image
              image: AssetImage('assets/new_icons/metal_texture.png'),
              fit: BoxFit.cover,
            ),
            boxShadow: [ // Add a subtle shadow to give it depth
              BoxShadow(
                color: Colors.black.withOpacity(0.4),
                blurRadius: 3,
                offset: Offset(0, 2),
              ),
            ],
          ),
          // Use a Padding to control the icon size inside the button
          child: Padding(
            padding: EdgeInsets.all(15.0 * heightScale), // Adjust padding to make icon larger/smaller
            child: Image.asset(
              iconPath,
              // Make the icon white to stand out on the metallic background
              color: Colors.white,
              colorBlendMode: BlendMode.srcIn,
            ),
          ),
        ),
      ),
    );
  }


  //  WIDGET METHOD for +/- buttons ---
  Widget _buildIconBottomBarButton(String iconPath, VoidCallback? onPressed) {
    final screenHeight = MediaQuery.of(context).size.height;
    final screenWidth = MediaQuery.of(context).size.width;
    final heightScale = screenHeight / 1080.0;
    final widthScale = screenWidth / 1920.0; // Use widthScale for width
    final bool isEnabled = onPressed != null;

    // Define height and width separately to create a rectangle.
    final double buttonHeight = 80 * heightScale; // Keep the height consistent with other buttons
    final double buttonWidth = 120 * widthScale;  // Make the width larger. ADJUST THIS VALUE to your liking.

    return GestureDetector(
      onTap: onPressed,
      child: Opacity(
        opacity: isEnabled ? 1.0 : 0.5,
        child: Container(
          width: buttonWidth,  
          height: buttonHeight, 
          decoration: BoxDecoration(
            image: DecorationImage(
              image: AssetImage(iconPath),
              fit: BoxFit.cover,
            ),
            borderRadius: BorderRadius.circular(15 * heightScale),
          ),
        ),
      ),
    );
  }


  //  dedicated builder for buttons with a custom image background ---
  Widget _buildImageBottomBarButton({
    required String label,
    required String iconPath,
    required String backgroundImagePath, // The path to the button's background image
    required VoidCallback? onPressed,
  }) {
    final screenHeight = MediaQuery.of(context).size.height;
    final screenWidth = MediaQuery.of(context).size.width;
    final heightScale = screenHeight / 1080.0;
    final widthScale = screenWidth / 1920.0;
    final bool isEnabled = onPressed != null;

    // Define the size for this specific button. Adjust as needed.
    final double buttonWidth = 220 * widthScale;
    final double buttonHeight = 80 * heightScale;

    return GestureDetector(
      onTap: onPressed,
      child: Opacity(
        opacity: isEnabled ? 1.0 : 0.5,
        child: Container(
          width: buttonWidth,
          height: buttonHeight,
          decoration: BoxDecoration(
            // Use the provided image as the background
            image: DecorationImage(
              image: AssetImage(backgroundImagePath),
              fit: BoxFit.fill, // Stretch the image to fill the container
            ),
          ),
          // The content (icon and text) is placed on top of the background image
          child: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Image.asset(iconPath, height: 36 * heightScale),
              const SizedBox(width: 12),
              Text(
                label,
                style: TextStyle(
                  fontFamily: 'NotoSans',
                  fontWeight: FontWeight.w700,
                  fontSize: 36 * heightScale,
                  color: Colors.white,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // A dedicated builder for the "Attack Permitted" red overlay ---
  Widget _buildDangerOverlay() {
    // This overlay should only be visible when permission has been requested AND granted.
    bool isVisible = _permissionRequestIsActive && _permissionHasBeenGranted;

    if (!isVisible) {
      return const SizedBox.shrink(); // Return an empty widget if not visible
    }

    return Positioned.fill(
      child: IgnorePointer( // IgnorePointer prevents this overlay from blocking touches
        child: Container(
          // Use a red color with low opacity to create the tint effect
          color: Colors.red.withOpacity(0.3),
        ),
      ),
    );
  }

  Widget _buildBottomBar() {
    final screenWidth = MediaQuery.of(context).size.width;
    final widthScale = screenWidth / 1920.0;
    final screenHeight = MediaQuery.of(context).size.height;
    final heightScale = screenHeight / 1080.0;

    return LayoutBuilder(
      builder: (context, constraints) {
        String permissionLabel;
        String permissionBackground;
        VoidCallback? permissionOnPressed;

        if (_permissionRequestIsActive) {
          if (_permissionHasBeenGranted) {
            // State 3: Permission has been granted by the user
            permissionLabel = "Attack Permitted";
            permissionBackground = ICON_PATH_PERMISSION_GREEN;
            permissionOnPressed = null; // Button is disabled
          } else {
            // State 2: Server is requesting permission
            permissionLabel = "Permission to Attack";
            permissionBackground = ICON_PATH_PERMISSION_RED;
            permissionOnPressed = _isServerConnected ? _onPermissionPressed : null; // Button is enabled
          }
        } else {
          // State 1: Idle / No request from server
          permissionLabel = "Request Pending";
          permissionBackground = ICON_PATH_PERMISSION_BLUE;
          permissionOnPressed = null; // Button is disabled
        }

        //  Swap the order of the first two buttons ---
        final List<Widget> leftCluster = [
          // 1. START/STOP button is now first
          _buildWideBottomBarButton(
            label: _isModeActive ? "STOP" : "START",
            iconPath: _isModeActive ? ICON_PATH_STOP : ICON_PATH_START,
            backgroundImagePath: ICON_PATH_PERMISSION_GREEN, // Use the green background
            onPressed: _isServerConnected ? _onStartStopPressed : null,
          ),
          SizedBox(width: 12 * widthScale),
          // 2. PERMISSION button is now second
          _buildPermissionButton(
            label: permissionLabel,
            backgroundImagePath: permissionBackground,
            onPressed: permissionOnPressed,
            // iconPath: '',
          ),
          SizedBox(width: 22 * widthScale),
          // 3. ZOOM IN button remains the same
          _buildIconBottomBarButton(
            ICON_PATH_PLUS,
            _isServerConnected ? () {
              setState(() {
                if (_currentZoomLevel < 5.0) _currentZoomLevel += 0.1;
                else _currentZoomLevel = 5.0;
                _transformationController.value = Matrix4.identity()..scale(_currentZoomLevel);
                _isUiZoomInPressed = true;
                Future.delayed(const Duration(milliseconds: 150), () => _isUiZoomInPressed = false);
              });
            } : null,
          ),
          SizedBox(width: 12 * widthScale),
          // 4. ZOOM OUT button remains the same
          _buildIconBottomBarButton(
            ICON_PATH_MINUS,
            _isServerConnected ? () {
              setState(() {
                if (_currentZoomLevel > 1.0) _currentZoomLevel -= 0.1;
                else _currentZoomLevel = 1.0;
                _transformationController.value = Matrix4.identity()..scale(_currentZoomLevel);
                _isUiZoomOutPressed = true;
                Future.delayed(const Duration(milliseconds: 150), () => _isUiZoomOutPressed = false);
              });
            } : null,
          ),
        ];

        final List<Widget> middleCluster = [
          Row(
            mainAxisSize: MainAxisSize.min,
            children: [
              Image.asset(ICON_PATH_WIND_W, height: 40 * heightScale),
              SizedBox(width: 8 * widthScale),
              Text(
                (_gamepadConnected ? _pendingLateralWindSpeed : _lateralWindSpeed).toStringAsFixed(1),
                style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w600, fontSize: 60 * heightScale, color: Colors.white),
              ),
            ],
          ),
          SizedBox(width: 40 * widthScale),
          Row(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.baseline,
            textBaseline: TextBaseline.alphabetic,
            children: [
              const Text("60", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 60, color: Colors.white)),
              SizedBox(width: 8 * widthScale),
              const Text("M", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 37, color: Colors.white)),
            ],
          ),
          SizedBox(width: 20 * widthScale),
          Image.asset(_getWifiIconPath(), height: 40),
        ];

        final List<Widget> rightCluster = [
          _buildImageBottomBarButton(
            label: "EXIT",
            iconPath: ICON_PATH_EXIT,
            backgroundImagePath: ICON_PATH_EXIT_BACKGROUND,
            onPressed: () async {
              final proceed = await _showExitDialog();
              if (proceed) SystemNavigator.pop();
            },
          ),
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

  Widget _buildWideBottomBarButton({
    required String label,
    required String iconPath,
    required String backgroundImagePath, // Now takes an image path instead of colors
    required VoidCallback? onPressed,
  }) {
    final screenHeight = MediaQuery.of(context).size.height;
    final screenWidth = MediaQuery.of(context).size.width;
    final heightScale = screenHeight / 1080.0;
    final widthScale = screenWidth / 1920.0;
    final bool isEnabled = onPressed != null;

    // Use a wider width for this specific button
    final double buttonWidth = 400 * widthScale; // Increased width
    final double buttonHeight = 78 * heightScale;

    return GestureDetector(
      onTap: onPressed,
      child: Opacity(
        opacity: isEnabled ? 1.0 : 0.5,
        child: Container(
          width: buttonWidth,
          height: buttonHeight,
          decoration: BoxDecoration(
            // Use the provided image as the background
            image: DecorationImage(
              image: AssetImage(backgroundImagePath),
              fit: BoxFit.fill,
            ),
          ),
          // The content (icon and text) is placed on top of the background image
          child: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Image.asset(iconPath, height: 36 * heightScale),
              const SizedBox(width: 12),
              Text(
                label,
                style: TextStyle(
                  fontFamily: 'NotoSans',
                  fontWeight: FontWeight.w700,
                  fontSize: 36 * heightScale,
                  color: Colors.white,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  // Make the onPressed parameter nullable and add the  textColor parameter
  Widget _buildBottomBarButton(
      String label,
      String? iconPath,
      List<Color> gradientColors,
      VoidCallback? onPressed,
      {Color textColor = Colors.white}
      ) {
    final screenHeight = MediaQuery.of(context).size.height;
    final heightScale = screenHeight / 1080.0;
    final bool isEnabled = onPressed != null;

    return GestureDetector(
      onTap: onPressed,
      child: Opacity(
        opacity: isEnabled ? 1.0 : 0.5,
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
                Text(
                    label,
                    style: TextStyle(
                        fontFamily: 'NotoSans',
                        fontWeight: FontWeight.w700,
                        fontSize: 30 * heightScale,
                        color: textColor
                    )
                ),
            ],
          ),
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

    if (_isSwitchingCamera) {
      setState(() {
        _isSwitchingCamera = false; // 2. Unlock the button
      });
    }

    switch (call.method) {
      case 'onStreamReady':
        setState(() { _isGStreamerLoading = false; _gstreamerHasError = false; });
        break;
      case 'onStreamError':
        final String? error = call.arguments?['error'];
        setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "Stream error: ${"" ?? 'Unknown native error'}"; });
        break;
    }
  }

 
  void _retryStream() {
    if (_currentCameraIndex == -1) {
      print("Retry failed: No camera index selected.");
      return;
    }
    print("Retrying stream via full native reset for camera index: $_currentCameraIndex");

    if (_gstreamerChannel != null && _isGStreamerReady) {
      setState(() {
        _isGStreamerLoading = true;
        _gstreamerHasError = false;
        _errorMessage = null;
      });

      // Start the timeout timer for the retry attempt
      _streamTimeoutTimer?.cancel();
      _streamTimeoutTimer = Timer(const Duration(seconds: 8), () {
        if (mounted && _isGStreamerLoading) {
          setState(() {
            _gstreamerHasError = true;
            _errorMessage = "Connection timed out.";
            _isGStreamerLoading = false;
          });
        }
      });

      try {
        final String url = _cameraUrls[_currentCameraIndex];
        // Call the new native method
        _gstreamerChannel!.invokeMethod('resetAndRestartStream', {'url': url});
      } catch (e) {
        _streamTimeoutTimer?.cancel();
        if (mounted) {
          setState(() {
            _gstreamerHasError = true;
            _errorMessage = "Failed to retry stream: ${e.toString()}";
            _isGStreamerLoading = false;
          });
        }
      }
    } else {
      // Fallback if the channel isn't ready for some reason, do a full widget rebuild
      _switchCamera(_currentCameraIndex);
    }
  }

  Future<void> _loadSettingsAndInitialize() async {
    _robotIpAddress = await _settingsService.loadIpAddress();
    _cameraUrls = await _settingsService.loadCameraUrls();
    _videoQuality = await _settingsService.loadVideoQuality(); // <-- ADD THIS LINE
    _bitrateMode = await _settingsService.loadBitrateMode();
    _targetBitrate = await _settingsService.loadVideoBitrate();

    if (mounted) {
      _connectToStatusServer();
      _connectCommandServer(); 
      _switchCamera(0);
      _startCommandTimer();
      setState(() => _isLoading = false);
    }
  }

  Future<void> _sendCommandPacket(UserCommand command) async {
    if (_commandSocket != null) {
      try {
        _commandSocket!.add(command.toBytes());
        await _commandSocket!.flush();
      } catch (e) {
        print("Error sending command packet: $e");
        // The socket's own error handler will trigger the reconnect logic.
      }
    }
  }

  Future<void> _sendTouchPacket(TouchCoord coord) async {
    if (_robotIpAddress.isEmpty || _touchSocket == null) return;
    try {
      const int TOUCH_PORT = 65433;
      _touchSocket!.send(coord.toBytes(), InternetAddress(_robotIpAddress), TOUCH_PORT);
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
    if (_isSwitchingCamera) {
      print("Camera switch already in progress. Ignoring request.");
      return;
    }
    if (_cameraUrls.isEmpty || index < 0 || index >= _cameraUrls.length) return;
    if (index == _currentCameraIndex && !_gstreamerHasError) return;

    // 1. Immediately set the loading and switching state.
    setState(() {
      _isSwitchingCamera = true;
      _isGStreamerLoading = true;
      _currentCameraIndex = index;
      _gstreamerHasError = false; // Clear any previous error
      _errorMessage = null;
    });

    // 2. ALWAYS start the timeout timer. This is our safety net.
    _streamTimeoutTimer?.cancel(); // Cancel any previous timer
    _streamTimeoutTimer = Timer(const Duration(seconds: 8), () {
      // If, after 8 seconds, we are still in a loading state...
      if (mounted && _isGStreamerLoading) {
        print("Stream connection timed out.");
        // ...force the UI into an error state.
        setState(() {
          _gstreamerHasError = true;
          _errorMessage = "Connection timed out.";
          _isGStreamerLoading = false;
          _isSwitchingCamera = false; // Unlock the camera switch flag
        });
      }
    });

    // 3. Decide whether to create a view or change the stream on the existing one.
    if (_gstreamerChannel != null && _isGStreamerReady) {
      try {
        final String url = _cameraUrls[index];
        await _gstreamerChannel!.invokeMethod('changeStream', {'url': url});
      } catch (e) {
        _streamTimeoutTimer?.cancel(); // Stop the timer if the call fails instantly
        if (mounted) {
          setState(() {
            _gstreamerHasError = true;
            _errorMessage = "Failed to switch camera: ${e.toString()}";
            _isGStreamerLoading = false;
            _isSwitchingCamera = false;
          });
        }
      }
    } else {
      setState(() {
        _gstreamerViewKey = UniqueKey();
      });
    }
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
          const SizedBox(height: 30),
          ElevatedButton.icon(
            icon: const Icon(Icons.refresh),
            label: const Text('Retry'),
            onPressed: _retryStream,
            style: ElevatedButton.styleFrom(foregroundColor: Colors.white, backgroundColor: Colors.blueGrey, padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 15), textStyle: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          ),
        ])),
      );
    }
    return const SizedBox.shrink();
  }

  Future<void> _navigateToSettings() async {
    _commandTimer?.cancel();

    setState(() {
      _isNavigatingAway = true;
    });

    final bool? settingsChanged = await Navigator.push<bool>(
      context,
      MaterialPageRoute(builder: (context) => const SettingsMenuPage()),
    );

    // Set this flag back to false immediately so the view starts rebuilding
    setState(() {
      _isNavigatingAway = false;
    });

    if (settingsChanged == true && mounted) {
      // 1. Show the loading overlay
      setState(() {
        _isReinitializing = true;
      });

      // 2. Await for the entire reconnection process to finish
      await _reloadSettingsAndReconnect();

      // 3. Hide the loading overlay
      if (mounted) {
        setState(() {
          _isReinitializing = false;
        });
      }
    } else {
      // If no changes, just restart the timer
      _startCommandTimer();
    }
  }

  Future<void> _reloadSettingsAndReconnect() async {
    _robotIpAddress = await _settingsService.loadIpAddress();
    _cameraUrls = await _settingsService.loadCameraUrls();

    _videoQuality = await _settingsService.loadVideoQuality();

    _bitrateMode = await _settingsService.loadBitrateMode();
    _targetBitrate = await _settingsService.loadVideoBitrate();

    _handleDisconnect();
    _handleCommandDisconnect();

    // Re-establish connections to the (potentially new) IP address
    _connectToStatusServer();
    _connectCommandServer();

    if (mounted) {
      _switchCamera(_currentCameraIndex);
    }

    // Finally, restart the command timer.
    _startCommandTimer();
  }
}