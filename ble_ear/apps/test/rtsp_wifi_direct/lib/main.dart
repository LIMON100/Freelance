import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_vlc_player/flutter_vlc_player.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:strwd/settings_service.dart';

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
  VlcPlayerController? _vlcPlayerController;
  String _robotIpAddress = ''; // Initialize as empty
  List<String> _cameraUrls = []; // Initialize as empty
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
  final GlobalKey _playerKey = GlobalKey();


  static const platform = MethodChannel('com.yourcompany/gamepad');
  bool _gamepadConnected = false;
  // Command & Control State
  Timer? _commandTimer;
  Timer? _joystickIdleTimer;
  Map<String, double> _gamepadAxisValues = {};
  Uint8List? _lastSentPacket;
  final SettingsService _settingsService = SettingsService();
  bool _isLoading = true; // NEW: Loading state flag


  @override
  void initState() {
    super.initState();
    _loadSettingsAndInitialize();
    // _switchCamera(0);
    // platform.setMethodCallHandler(_handleGamepadEvent);
    // _startCommandTimer();
  }

  @override
  void dispose() {
    _commandTimer?.cancel();
    _vlcPlayerController?.dispose();
    super.dispose();
  }

  Future<void> _loadSettingsAndInitialize() async {
    // Load settings from storage
    _robotIpAddress = await _settingsService.loadIpAddress();
    _cameraUrls = await _settingsService.loadCameraUrls();

    if (mounted) {
      // Once settings are loaded, initialize the app
      _switchCamera(0);
      platform.setMethodCallHandler(_handleGamepadEvent);
      _startCommandTimer();

      // Finally, remove the loading screen
      setState(() {
        _isLoading = false;
      });
    }
  }

  // --- GAMEPAD HANDLER: ONLY UPDATES STATE ---
  Future<void> _handleGamepadEvent(MethodCall call) async {
    if (!mounted) return;
    if (!_gamepadConnected) {
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
      // 1. Prioritize Gamepad Input if connected
      if (_gamepadConnected) {
        double horizontalValue = _gamepadAxisValues['AXIS_X'] ?? _gamepadAxisValues['AXIS_Z'] ?? _gamepadAxisValues['AXIS_HAT_X'] ?? 0.0;
        _currentCommand.turnAngle = (horizontalValue * 100).round();
        if (_currentCommand.turnAngle.abs() < 15) _currentCommand.turnAngle = 0;

        double verticalValue = _gamepadAxisValues['AXIS_Y'] ?? _gamepadAxisValues['AXIS_RZ'] ?? _gamepadAxisValues['AXIS_HAT_Y'] ?? 0.0;
        _currentCommand.moveSpeed = (verticalValue * -100).round();
        if (_currentCommand.moveSpeed.abs() < 15) _currentCommand.moveSpeed = 0;

        double triggerValue = _gamepadAxisValues['AXIS_RTRIGGER'] ?? _gamepadAxisValues['AXIS_LTRIGGER'] ?? 0.0;
        _currentCommand.gunTrigger = triggerValue > 0.5;
      }
      // 2. Fall back to On-Screen Buttons if no gamepad
      else {
        if (_isForwardPressed) _currentCommand.moveSpeed = 100;
        else if (_isBackPressed) _currentCommand.moveSpeed = -100;
        else _currentCommand.moveSpeed = 0;

        if (_isLeftPressed) _currentCommand.turnAngle = -100;
        else if (_isRightPressed) _currentCommand.turnAngle = 100;
        else _currentCommand.turnAngle = 0;

        _currentCommand.gunTrigger = _isGunTriggerPressed;
      }

      // 3. Send the single, consolidated command packet
      _sendCommandPacket(_currentCommand);

      // --- THE FIX: Reset one-time touch coordinates AFTER they've been sent ---
      if (_currentCommand.touchX != -1.0) {
        _currentCommand.touchX = -1.0;
        _currentCommand.touchY = -1.0;
      }
    });
  }

  // --- The actual network sending functions ---
  Future<void> _sendCommandPacket(UserCommand command) async {
    try {
      final socket = await Socket.connect(_robotIpAddress, ROBOT_COMMAND_PORT, timeout: const Duration(milliseconds: 50));
      socket.add(command.toBytes());
      await socket.flush();
      socket.close();
    } catch (e) {
        print(e);
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


  // Button handlers now ONLY update state. The timer does the sending.
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
    // Navigate and wait for a boolean result
    final bool? settingsChanged = await Navigator.push<bool>(
      context,
      MaterialPageRoute(
        // We no longer need to pass any data TO the settings page
        builder: (context) => const SettingsMenuPage(),
      ),
    );

    // If settings were saved, reload them.
    if (settingsChanged == true && mounted) {
      print("Settings changed, reloading...");
      // Stop the current command timer to prevent using the old IP
      _commandTimer?.cancel();

      // Reload all settings and re-initialize the app's connections
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

  // After retrun from MANUAL ATTACK it goes ID 1
  // Future<void> _handleManualAutoAttackToggle() async {
  //   if (_isAutoAttackMode) {
  //     final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_AUTO_ATTACK_ACTIVE, title: "DANGERS", titleColor: Colors.red, content: 'Are you sure you want to stop\n"Auto Attack" mode?');
  //     if (proceed) {
  //       setState(() {
  //         _isAutoAttackMode = false;
  //         _currentCommand.commandId = 1; // Revert to Move state
  //       });
  //     }
  //   } else {
  //     final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_MANUAL_ATTACK_ACTIVE, title: "DANGERS", titleColor: Colors.red, content: 'Are you sure you want to start\n"Auto Attack" mode?');
  //     if (proceed) {
  //       setState(() {
  //         _isAutoAttackMode = true;
  //         _currentCommand.commandId = 4; // 4 = Attack
  //       });
  //     }
  //   }
  // }

  Future<void> _handleManualAutoAttackToggle() async {
    if (_isAutoAttackMode) {
      final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_AUTO_ATTACK_ACTIVE, title: "DANGERS", titleColor: Colors.red, content: 'Are you sure you want to stop\n"Auto Attack" mode?');
      if (proceed) {
        setState(() {
          _isAutoAttackMode = false;
          _currentCommand.commandId = 0;
          _activeLeftButtonIndex = 0;
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

          _buildLeftButton(30, 35, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE, _activeLeftButtonIndex == 0, () => _onLeftButtonPressed(0, 1)),
          _buildLeftButton(30, 185, "Petrol", ICON_PATH_PETROL_INACTIVE, ICON_PATH_PETROL_ACTIVE, _activeLeftButtonIndex == 1, () => _onLeftButtonPressed(1, 2)),
          _buildLeftButton(30, 335, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE, _activeLeftButtonIndex == 2, () => _onLeftButtonPressed(2, 2)),
          _buildLeftButton(30, 485, _isAutoAttackMode ? "Auto Attack" : "Manual Attack", _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_INACTIVE : ICON_PATH_MANUAL_ATTACK_INACTIVE, _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_ACTIVE : ICON_PATH_MANUAL_ATTACK_ACTIVE, _activeLeftButtonIndex == 3, () { setState(() => _activeLeftButtonIndex = 3); _handleManualAutoAttackToggle(); }),
          _buildLeftButton(30, 635, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE, _activeLeftButtonIndex == 4, () => _onLeftButtonPressed(4, 3)),
          _buildLeftButton(30, 785, "Return", ICON_PATH_RETURN_INACTIVE, ICON_PATH_RETURN_ACTIVE, _activeLeftButtonIndex == 5, () => _onLeftButtonPressed(5, 0)),

          _buildRightButton(1690, 30, "Attack View", ICON_PATH_ATTACK_VIEW_INACTIVE, ICON_PATH_ATTACK_VIEW_ACTIVE, _activeRightButtonIndex == 0, () => _onRightButtonPressed(0)),
          _buildRightButton(1690, 250, "Top View", ICON_PATH_TOP_VIEW_INACTIVE, ICON_PATH_TOP_VIEW_ACTIVE, _activeRightButtonIndex == 1, () => _onRightButtonPressed(1)),
          _buildRightButton(1690, 470, "3D View", ICON_PATH_3D_VIEW_INACTIVE, ICON_PATH_3D_VIEW_ACTIVE, _activeRightButtonIndex == 2, () => _onRightButtonPressed(2)),
          _buildRightButton(1690, 720, "Setting", ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, false, () => _navigateToSettings()),

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
                  child:
                  const Text('Retry', style: TextStyle(color: Colors.white))),
            ],
          ));
    }
    if (_vlcPlayerController == null) {
      return const Center(child: CircularProgressIndicator(color: Colors.white));
    }

    return GestureDetector(
      key: _playerKey,
      onTapUp: (details) {
        // Only process taps if in Manual Attack mode (index 3).
        if (_activeLeftButtonIndex == 3) {
          final RenderBox? box =
          _playerKey.currentContext?.findRenderObject() as RenderBox?;
          if (box == null) return;

          final Offset localPosition = box.globalToLocal(details.globalPosition);
          final double x = localPosition.dx / box.size.width;
          final double y = localPosition.dy / box.size.height;

          if (x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0) {
            setState(() {
              _currentCommand.touchX = x;
              _currentCommand.touchY = y;
              _sendCommandPacket(_currentCommand);
            });

          }
        }
      },
      child: Stack(
        fit: StackFit.expand,
        children: [
          SizedBox.expand(
            child: VlcPlayer(
              controller: _vlcPlayerController!,
              placeholder: const Center(
                  child: CircularProgressIndicator(color: Colors.white)), aspectRatio: 16/12,
            ),
          ),
          // A transparent container ensures the GestureDetector is always hit.
          Container(color: Colors.transparent),
        ],
      ),
    );
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
          // After retrun from ATTACK it goes ID 1
          // _buildBottomBarButton(
          //   "ATTACK",
          //   _isAttackModeOn ? ICON_PATH_ATTACK_of : ICON_PATH_ATTACK,
          //   _isAttackModeOn ? _attackActiveColors : _attackInactiveColors,
          //       () async {
          //     if (!_isAttackModeOn) {
          //       final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_ATTACK, title: 'DANGERS', titleColor: Colors.red, content: 'Do you want to proceed the command?');
          //       if (proceed) {
          //         setState(() {
          //           _isAttackModeOn = true;
          //           _currentCommand.commandId = 4; // attack
          //         });
          //       }
          //     } else {
          //       setState(() {
          //         _isAttackModeOn = false;
          //         _currentCommand.commandId = 1; // back to move mode
          //       });
          //     }
          //   },
          // ),
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
                    _currentCommand.commandId = 4; // 4 = attack
                  });
                }
              } else {
                setState(() {
                  _isAttackModeOn = false;
                  // --- THE FIX: Revert to Idle state, not Move state ---
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
                  _isGunTriggerPressed = _isStarted;
                });
              }
          ),
          SizedBox(width: 12 * widthScale),
          _buildBottomBarButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () { /* Zoom logic here */ }),
          SizedBox(width: 12 * widthScale),
          _buildBottomBarButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () { /* Zoom logic here */ }),
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