// OLd fully functional 04_10

// import 'dart:async';
// import 'dart:io';
// import 'dart:typed_data';
// import 'package:flutter/foundation.dart';
// import 'package:flutter/gestures.dart';
// import 'package:flutter/material.dart';
// import 'package:flutter/services.dart';
// import 'package:rtest1/settings_service.dart';
// import 'TouchCoord.dart';
// import 'UserCommand.dart';
// import 'icon_constants.dart';
// import 'splash_screen.dart';
// import 'settings_menu_page.dart';
//
// // --- CONFIGURATION FOR ROBOT CONNECTION ---
// const String ROBOT_IP_ADDRESS = '192.168.0.158';
// const int ROBOT_COMMAND_PORT = 65432;
//
// // --- MAIN APP ENTRY POINT ---
// void main() async {
//   WidgetsFlutterBinding.ensureInitialized();
//   SystemChrome.setEnabledSystemUIMode(SystemUiMode.manual, overlays: []);
//   SystemChrome.setPreferredOrientations([
//     DeviceOrientation.landscapeLeft,
//     DeviceOrientation.landscapeRight,
//   ]);
//   runApp(const MyApp());
// }
//
// class MyApp extends StatelessWidget {
//   const MyApp({super.key});
//
//   @override
//   Widget build(BuildContext context) {
//     return MaterialApp(
//       debugShowCheckedModeBanner: false,
//       title: '',
//       theme: ThemeData(
//         primarySwatch: Colors.blue,
//       ),
//       home: const SplashScreen(),
//     );
//   }
// }
//
// // --- HOME PAGE WIDGET ---
// class HomePage extends StatefulWidget {
//   const HomePage({super.key});
//
//   @override
//   State<HomePage> createState() => _HomePageState();
// }
//
// class _HomePageState extends State<HomePage> {
//   String? _errorMessage;
//   int _activeLeftButtonIndex = -1; //0
//   int _activeRightButtonIndex = 0;
//   bool _isAutoAttackMode = false;
//   bool _isAttackModeOn = false;
//   final List<Color> _attackInactiveColors = [const Color(0xffc32121), const Color(0xff831616)];
//   final List<Color> _attackActiveColors = [const Color(0xFF424242), const Color(0xFF212121)];
//   UserCommand _currentCommand = UserCommand();
//   bool _isForwardPressed = false;
//   bool _isBackPressed = false;
//   bool _isLeftPressed = false;
//   bool _isRightPressed = false;
//   bool _isStarted = false;
//   bool _isGunTriggerPressed = false;
//   final GlobalKey _playerKey = GlobalKey();
//   static const platform = MethodChannel('com.yourcompany/gamepad');
//   bool _gamepadConnected = false;
//   Timer? _commandTimer;
//   Map<String, double> _gamepadAxisValues = {};
//   Uint8List? _lastSentPacket;
//   final SettingsService _settingsService = SettingsService();
//   bool _isLoading = true;
//   String _robotIpAddress = '';
//   List<String> _cameraUrls = [];
//   int _currentCameraIndex = -1;
//   Key _gstreamerViewKey = UniqueKey();
//   bool _isGStreamerReady = false;
//   bool _isGStreamerLoading = true;
//   bool _gstreamerHasError = false;
//   MethodChannel? _gstreamerChannel;
//   bool _isGStreamerInitialized = false;
//   bool _novalidurl = false;
//   Timer? _streamTimeoutTimer;
//   bool _isManualTouchModeEnabled = false;
//
//
//   @override
//   void initState() {
//     super.initState();
//     _loadSettingsAndInitialize();
//     platform.setMethodCallHandler(_handleGamepadEvent);
//   }
//
//   @override
//   void dispose() {
//     _commandTimer?.cancel();
//     // _gstreamerChannel?.invokeMethod('stopStream');
//     _stopGStreamer();
//     super.dispose();
//   }
//
//   void _stopGStreamer() {
//     if (_gstreamerChannel != null && _isGStreamerReady) {
//       print("DEBUG_DISPOSE: Invoking GStreamer stopStream.");
//       try {
//         _gstreamerChannel!.invokeMethod('stopStream').catchError((e) {
//           print("DEBUG_DISPOSE: Error invoking stopStream: $e");
//         });
//       } catch (e) {
//         print("DEBUG_DISPOSE: Exception invoking stopStream: $e");
//       }
//     }
//
//     setState(() {
//       _isGStreamerReady = false;
//       _isGStreamerLoading = true;
//       _gstreamerHasError = false;
//       _errorMessage = null;
//       _gstreamerViewKey = UniqueKey(); // Invalidate key to force rebuild on next load
//       _isGStreamerInitialized = false; // Reset initialization flag
//     });
//     print("DEBUG_DISPOSE: GStreamer state reset.");
//   }
//
//
//   Future<void> _loadSettingsAndInitialize() async {
//     _robotIpAddress = await _settingsService.loadIpAddress();
//     _cameraUrls = await _settingsService.loadCameraUrls();
//
//     if (mounted) {
//       _switchCamera(0);
//       _startCommandTimer();
//       setState(() {
//         _isLoading = false;
//       });
//     }
//   }
//
//   Future<void> _handleGStreamerMessages(MethodCall call) async {
//     if (!mounted) return;
//
//     _streamTimeoutTimer?.cancel(); // Cancel timeout on any message from native
//
//     switch (call.method) {
//       case 'onStreamReady':
//         print("GSTREAMER_CALLBACK: Stream is ready and playing.");
//         setState(() {
//           _isGStreamerLoading = false;
//           _gstreamerHasError = false;
//           _novalidurl = false;
//           _errorMessage = null;
//         });
//         break;
//
//       case 'onStreamError':
//         final String? error = call.arguments['error'];
//         print("GSTREAMER_CALLBACK: Received stream error: $error");
//         setState(() {
//           _isGStreamerLoading = false;
//           _gstreamerHasError = true;
//           _novalidurl = true;
//           _errorMessage = "Stream error";
//         });
//         break;
//     }
//   }
//
//   void _onGStreamerPlatformViewCreated(int id) {
//     _gstreamerChannel = MethodChannel('gstreamer_channel_$id');
//     _gstreamerChannel!.setMethodCallHandler(_handleGStreamerMessages); // Set the handler
//
//     setState(() {
//       _isGStreamerReady = true;
//       _isGStreamerLoading = false;
//       _gstreamerHasError = false;
//       _errorMessage = null;
//     });
//     _playCurrentCameraStream();
//   }
//
//
//   Future<void> _playCurrentCameraStream() async {
//     _streamTimeoutTimer?.cancel();
//
//     if (_cameraUrls.isEmpty || _currentCameraIndex < 0 || _currentCameraIndex >= _cameraUrls.length) {
//       print("ERROR: No valid camera URL configured.");
//       if (mounted) {
//         setState(() {
//           _isGStreamerLoading = false;
//           _gstreamerHasError = true;
//           _novalidurl = true;
//           _errorMessage = "No valid camera URL. Please check settings.";
//         });
//       }
//       return;
//     }
//
//     if (_gstreamerChannel == null || !_isGStreamerReady) {
//       print("ERROR: GStreamer channel is not ready.");
//       if (mounted) {
//         setState(() {
//           _isGStreamerLoading = false;
//           _gstreamerHasError = true;
//           _novalidurl = true;
//           _errorMessage = "GStreamer channel not ready.";
//         });
//       }
//       return;
//     }
//
//     print("INFO: Attempting to start GStreamer stream...");
//     setState(() {
//       _isGStreamerLoading = true;
//       _gstreamerHasError = false;
//       _novalidurl = false;
//       _errorMessage = null;
//     });
//
//     const timeoutDuration = Duration(seconds: 8); // Increased timeout slightly
//     _streamTimeoutTimer = Timer(timeoutDuration, () {
//       if (mounted && _isGStreamerLoading) {
//         print("ERROR: GStreamer stream timed out after $timeoutDuration.");
//         setState(() {
//           _gstreamerHasError = true;
//           _novalidurl = true;
//           _errorMessage = "Connection timed out. Check network.";
//           _isGStreamerLoading = false;
//         });
//       }
//     });
//
//     try {
//       final String url = _cameraUrls[_currentCameraIndex];
//       // The pipeline is now created in the native code, we just send the URL
//       await _gstreamerChannel!.invokeMethod('startStream', {'url': url});
//     } on PlatformException catch (e) {
//       print("GStreamer start failed (PlatformException): ${e.message}");
//       _streamTimeoutTimer?.cancel();
//       if (mounted) {
//         setState(() {
//           _gstreamerHasError = true;
//           _novalidurl = true;
//           _errorMessage = "Platform error: ${e.message}";
//           _isGStreamerLoading = false;
//         });
//       }
//     } catch (e) {
//       print("GStreamer start failed (Unknown Error): $e");
//       _streamTimeoutTimer?.cancel();
//       if (mounted) {
//         setState(() {
//           _gstreamerHasError = true;
//           _novalidurl = true;
//           _errorMessage = "An unknown error occurred.";
//           _isGStreamerLoading = false;
//         });
//       }
//     }
//   }
//
//
//   Future<void> _handleGamepadEvent(MethodCall call) async {
//     if (!mounted) return;
//     if (!_gamepadConnected && call.method != "onMotionEvent") {
//       setState(() => _gamepadConnected = true);
//     }
//     if (call.method == "onMotionEvent") {
//       _gamepadAxisValues = Map<String, double>.from(call.arguments);
//     }
//     if (call.method == "onButtonDown") {
//       final String button = call.arguments['button'];
//       if (button == 'KEYCODE_BUTTON_A') {
//         setState(() => _currentCommand.commandId = 0);
//       }
//       if (button == 'KEYCODE_BUTTON_X') {
//         setState(() => _currentCommand.gunPermission = true);
//       }
//       if (button == 'KEYCODE_BUTTON_START') {
//         _handleManualAutoAttackToggle();
//       }
//     }
//     if (call.method == "onButtonUp") {
//       final String button = call.arguments['button'];
//       if (button == 'KEYCODE_BUTTON_X') {
//         setState(() => _currentCommand.gunPermission = false);
//       }
//     }
//   }
//
//   void _startCommandTimer() {
//     _commandTimer = Timer.periodic(const Duration(milliseconds: 100), (timer) {
//       if (_gamepadConnected) {
//         double horizontalValue = _gamepadAxisValues['AXIS_X'] ?? _gamepadAxisValues['AXIS_Z'] ?? _gamepadAxisValues['AXIS_HAT_X'] ?? 0.0;
//         _currentCommand.turnAngle = (horizontalValue * 100).round();
//         if (_currentCommand.turnAngle.abs() < 15) _currentCommand.turnAngle = 0;
//
//         double verticalValue = _gamepadAxisValues['AXIS_Y'] ?? _gamepadAxisValues['AXIS_RZ'] ?? _gamepadAxisValues['AXIS_HAT_Y'] ?? 0.0;
//         _currentCommand.moveSpeed = (verticalValue * -100).round();
//         if (_currentCommand.moveSpeed.abs() < 15) _currentCommand.moveSpeed = 0;
//
//         double triggerValue = _gamepadAxisValues['AXIS_RTRIGGER'] ?? _gamepadAxisValues['AXIS_LTRIGGER'] ?? 0.0;
//         _currentCommand.gunTrigger = triggerValue > 0.5;
//       } else {
//         _currentCommand.moveSpeed = 0;
//         if (_isForwardPressed) _currentCommand.moveSpeed = 100;
//         else if (_isBackPressed) _currentCommand.moveSpeed = -100;
//
//         _currentCommand.turnAngle = 0;
//         if (_isLeftPressed) _currentCommand.turnAngle = -100;
//         else if (_isRightPressed) _currentCommand.turnAngle = 100;
//
//         _currentCommand.gunTrigger = _isGunTriggerPressed;
//       }
//
//       _sendCommandPacket(_currentCommand);
//
//       if (_currentCommand.touchX != -1.0) {
//         _currentCommand.touchX = -1.0;
//         _currentCommand.touchY = -1.0;
//       }
//     });
//   }
//
//   Future<void> _sendCommandPacket(UserCommand command) async {
//     if (_robotIpAddress.isEmpty || ROBOT_COMMAND_PORT == 0) {
//       print("Error: Robot IP address or port is not configured.");
//       return;
//     }
//     try {
//       final socket = await Socket.connect(_robotIpAddress, ROBOT_COMMAND_PORT, timeout: const Duration(milliseconds: 150));
//       socket.add(command.toBytes());
//       await socket.flush();
//       socket.close();
//     } catch (e) {
//       print('Error sending command packet: $e');
//     }
//   }
//
//
//   Future<void> _sendTouchPacket(TouchCoord coord) async {
//     try {
//       // The port for touch commands
//       const int TOUCH_PORT = 65433;
//
//       // Create a UDP socket, send the data, and immediately close the socket.
//       // This is extremely fast and avoids connection state issues.
//       final socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
//       socket.send(coord.toBytes(), InternetAddress(_robotIpAddress), TOUCH_PORT);
//       socket.close();
//
//       print('Sent Touch Packet (UDP): X=${coord.x}, Y=${coord.y}');
//     } catch (e) {
//       print('Error sending touch UDP packet: $e');
//     }
//   }
//
//   void _onLeftButtonPressed(int index, int commandId) {
//     setState(() {
//       _activeLeftButtonIndex = index;
//       _currentCommand.commandId = commandId;
//       _isManualTouchModeEnabled = false;
//     });
//   }
//
//   void _onRightButtonPressed(int index) {
//     setState(() => _activeRightButtonIndex = index);
//     _switchCamera(index);
//   }
//
//
//   Future<void> _switchCamera(int index) async {
//     if (_cameraUrls.isEmpty || index < 0 || index >= _cameraUrls.length) {
//       print("Warning: Camera index out of bounds or no camera URLs configured.");
//       setState(() {
//         _currentCameraIndex = -1;
//         _isGStreamerLoading = false; // Not loading if no URL
//         _gstreamerHasError = true;
//         _novalidurl = true;
//         _errorMessage = "No cameras configured.";
//         _isGStreamerReady = false;
//       });
//       return;
//     }
//
//     if (index == _currentCameraIndex && !_gstreamerHasError) {
//       return; // Don't reload if it's the same and not in an error state
//     }
//
//     // Call stopStream on the existing channel before creating a new view
//     if (_gstreamerChannel != null && _isGStreamerReady) {
//       try {
//         await _gstreamerChannel!.invokeMethod('stopStream');
//       } catch (e) {
//         print("Error stopping previous stream: $e");
//       }
//     }
//
//     setState(() {
//       _currentCameraIndex = index;
//       _gstreamerViewKey = UniqueKey(); // This forces the AndroidView to be recreated
//       _isGStreamerReady = false;
//       _isGStreamerLoading = true;
//       _gstreamerHasError = false;
//       _novalidurl = false;
//       _errorMessage = null;
//     });
//   }
//
//   Future<void> _navigateToSettings() async {
//     final bool? settingsChanged = await Navigator.push<bool>(
//       context,
//       MaterialPageRoute(
//         builder: (context) => const SettingsMenuPage(),
//       ),
//     );
//
//     if (settingsChanged == true && mounted) {
//       print("Settings changed, reloading...");
//       _commandTimer?.cancel();
//       await _loadSettingsAndInitialize();
//     }
//   }
//
//   Future<bool> _showCustomConfirmationDialog(
//       {required BuildContext context, required String iconPath, required String title, required Color titleColor, required String content}) async {
//     return await showDialog<bool>(
//       context: context,
//       barrierDismissible: false,
//       builder: (BuildContext dialogContext) {
//         return Dialog(
//           shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20.0)),
//           child: Container(
//             width: 500,
//             padding: const EdgeInsets.all(24.0),
//             child: Column(
//               mainAxisSize: MainAxisSize.min,
//               children: [
//                 Row(
//                   children: [
//                     Image.asset(iconPath, height: 40, width: 40),
//                     const SizedBox(width: 16),
//                     Text(title,
//                       // style: GoogleFonts.notoSans(fontSize: 28, fontWeight: FontWeight.bold, color: titleColor)
//                       style: TextStyle(
//                         fontFamily: 'NotoSans',
//                         fontWeight: FontWeight.w700, // For Medium
//                         // or FontWeight.w700 for Bold
//                         fontSize: 28,
//                         color: titleColor,
//                       ),
//                     ),
//                   ],
//                 ),
//                 const SizedBox(height: 24),
//                 Text(content, textAlign: TextAlign.center,
//                   // style: GoogleFonts.notoSans(fontSize: 22, color: Colors.black87)
//                   style: TextStyle(
//                     fontFamily: 'NotoSans',
//                     fontWeight: FontWeight.w700, // For Medium
//                     // or FontWeight.w700 for Bold
//                     fontSize: 22,
//                     color: Colors.black87,
//                   ),
//                 ),
//                 const SizedBox(height: 32),
//                 Row(
//                   mainAxisAlignment: MainAxisAlignment.spaceEvenly,
//                   children: [
//                     TextButton(
//                       onPressed: () => Navigator.of(dialogContext).pop(false),
//                       child: Text('Cancel',
//                         // style: GoogleFonts.notoSans(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.grey.shade700)
//                         style: TextStyle(
//                           fontFamily: 'NotoSans',
//                           fontWeight: FontWeight.w700, // For Medium
//                           // or FontWeight.w700 for Bold
//                           fontSize: 23,
//                           color: Colors.grey.shade700,
//                         ),
//
//                       ),
//                     ),
//                     SizedBox(height: 40, child: VerticalDivider(color: Colors.grey.shade400, thickness: 1)),
//                     TextButton(
//                       onPressed: () => Navigator.of(dialogContext).pop(true),
//                       child: Text('OK',
//                         // style: GoogleFonts.notoSans(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.blue.shade700)
//                         style: TextStyle(
//                           fontFamily: 'NotoSans',
//                           fontWeight: FontWeight.w700, // For Medium
//                           // or FontWeight.w700 for Bold
//                           fontSize: 23,
//                           color: Colors.blue.shade700,
//                         ),
//                       ),
//                     ),
//                   ],
//                 ),
//               ],
//             ),
//           ),
//         );
//       },
//     ) ?? false;
//   }
//
//   Future<void> _handleManualAutoAttackToggle() async {
//     if (_isAutoAttackMode) { // Currently in Auto Attack, switching OFF
//       final proceed = await _showCustomConfirmationDialog(
//           context: context,
//           iconPath: ICON_PATH_AUTO_ATTACK_ACTIVE,
//           title: "DANGERS",
//           titleColor: Colors.red,
//           content: 'Are you sure you want to stop\n"Auto Attack" mode?');
//       if (proceed) {
//         setState(() {
//           _isAutoAttackMode = false;
//           _currentCommand.commandId = 0; // Assuming 0 is a neutral/stop command
//           // When Auto Attack is turned off, return the selector to the default state (e.g., Driving)
//           _activeLeftButtonIndex = -1; // Set to the index for "Driving"
//           _isManualTouchModeEnabled = false;
//         });
//       }
//     } else { // Currently NOT in Auto Attack, switching ON to Manual Attack
//       final proceed = await _showCustomConfirmationDialog(
//           context: context,
//           iconPath: ICON_PATH_MANUAL_ATTACK_ACTIVE,
//           title: "DANGERS",
//           titleColor: Colors.red,
//           content: 'Are you sure you want to start\n"Auto Attack" mode?');
//       if (proceed) {
//         setState(() {
//           _isAutoAttackMode = true;
//           _currentCommand.commandId = 4; // Command ID for Manual/Auto Attack
//           // *** ADD THIS LINE ***
//           // When entering Manual/Auto Attack, highlight the Manual Attack button visually
//           _activeLeftButtonIndex = 3; // Set to the index for "Manual Attack"
//         });
//       }
//     }
//   }
//
//   Widget buildPlayerWidgetGestureDetector() {
//     return GestureDetector(
//       key: _playerKey,
//       behavior: HitTestBehavior.opaque, // Ensure GestureDetector captures all touch events
//       // onTapDown: (_) {
//       //   print("DEBUG_TOUCH: Touch DOWN detected on video stream area."); // New debug print for touch down
//       // },
//       onTapUp: (details) {
//         if (_isManualTouchModeEnabled) {
//           if (_activeLeftButtonIndex == 3) {
//             // print("DEBUG_TOUCH: Manual Attack mode detected (_activeLeftButtonIndex == 3).");
//             final RenderBox? renderBox = _playerKey.currentContext
//                 ?.findRenderObject() as RenderBox?;
//             if (renderBox == null) {
//               print(
//                   "DEBUG_TOUCH: ERROR: Could not get RenderBox for _playerKey.");
//               return;
//             }
//             final Offset localPosition = renderBox.globalToLocal(
//                 details.globalPosition);
//             final double x = localPosition.dx / renderBox.size.width;
//             final double y = localPosition.dy / renderBox.size.height;
//             if (x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0) {
//               setState(() {
//                 _currentCommand.touchX = x;
//                 _currentCommand.touchY = y;
//               });
//               _sendTouchPacket(TouchCoord()
//                 ..x = x
//                 ..y = y);
//             } else {
//               print("DEBUG_TOUCH: Coords out of bounds. Not processing touch.");
//             }
//           } else {
//             print(
//                 "DEBUG_TOUCH: Not in Manual Attack mode (_activeLeftButtonIndex != 3). Touch ignored.");
//           }
//         }
//       },
//       child: Container(color: Colors.transparent),
//     );
//   }
//
//   Widget _buildStreamOverlay() {
//     if (_isGStreamerLoading) {
//       return Container(
//         color: Colors.black.withOpacity(0.5),
//         child: const Center(
//           child: Column(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               CircularProgressIndicator(color: Colors.white),
//               SizedBox(height: 20),
//               Text(
//                 'Connecting to stream...',
//                 style: TextStyle(color: Colors.white, fontSize: 18),
//               ),
//             ],
//           ),
//         ),
//       );
//     }
//
//     if (_novalidurl && _gstreamerHasError) {
//       return Container(
//         color: Colors.black.withOpacity(0.7),
//         child: Center(
//           child: Column(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               const Icon(Icons.error_outline, color: Colors.redAccent, size: 70),
//               const SizedBox(height: 20),
//               Padding(
//                 padding: const EdgeInsets.symmetric(horizontal: 40.0),
//                 child: Text(
//                   _errorMessage ?? 'Stream failed to load.',
//                   textAlign: TextAlign.center,
//                   style: const TextStyle(color: Colors.white, fontSize: 20),
//                 ),
//               ),
//               const SizedBox(height: 30),
//               ElevatedButton.icon(
//                 icon: const Icon(Icons.refresh),
//                 label: const Text('Retry'),
//                 onPressed: () {
//                   if (_currentCameraIndex != -1) {
//                     _switchCamera(_currentCameraIndex);
//                   }
//                 },
//                 style: ElevatedButton.styleFrom(
//                   foregroundColor: Colors.white,
//                   backgroundColor: Colors.blueGrey,
//                   padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 15),
//                   textStyle: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
//                 ),
//               ),
//             ],
//           ),
//         ),
//       );
//     }
//
//     return const SizedBox.shrink();
//   }
//
//
//   @override
//   Widget build(BuildContext context) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     return Scaffold(
//       backgroundColor: Colors.black,
//       body: Stack(
//         children: [
//           Positioned.fill(child: buildPlayerWidgetGestureDetector()),
//           Positioned.fill(
//             child: IgnorePointer(
//               child: KeyedSubtree(
//                 key: _gstreamerViewKey,
//                 child: AndroidView(
//                   viewType: 'gstreamer_view',
//                   onPlatformViewCreated: _onGStreamerPlatformViewCreated,
//                   creationParamsCodec: const StandardMessageCodec(),
//                 ),
//               ),
//             ),
//           ),
//
//           Positioned(child: _buildStreamOverlay()),
//
//           _buildLeftButton(
//               30, 35, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE,
//               _activeLeftButtonIndex != -1 && _activeLeftButtonIndex == 0,
//                   () => _onLeftButtonPressed(0, 1)
//           ),
//           _buildLeftButton(
//               30, 185, "Petrol", ICON_PATH_PETROL_INACTIVE, ICON_PATH_PETROL_ACTIVE,
//               _activeLeftButtonIndex != -1 && _activeLeftButtonIndex == 1,
//                   () => _onLeftButtonPressed(1, 2)
//           ),
//           _buildLeftButton(
//               30, 335, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE,
//               _activeLeftButtonIndex != -1 && _activeLeftButtonIndex == 2,
//                   () => _onLeftButtonPressed(2, 2)
//           ),
//
//           _buildLeftButton(
//               30, 485, _isAutoAttackMode ? "Auto Attack" : "Manual Attack",
//               _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_INACTIVE : ICON_PATH_MANUAL_ATTACK_INACTIVE,
//               _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_ACTIVE : ICON_PATH_MANUAL_ATTACK_ACTIVE,
//               _activeLeftButtonIndex == 3,
//                   () {
//                 // When this button is pressed, ensure _activeLeftButtonIndex is set to 3
//                 setState(() {
//                   _activeLeftButtonIndex = 3;
//                   _isManualTouchModeEnabled = true; // <-- ADD THIS LINE to enable touch mode
//                 });
//                 _handleManualAutoAttackToggle();
//               }
//           ),
//
//           _buildLeftButton(
//               30, 635, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE,
//               _activeLeftButtonIndex != -1 && _activeLeftButtonIndex == 4,
//                   () => _onLeftButtonPressed(4, 3)
//           ),
//           _buildLeftButton(
//               30, 785, "Return", ICON_PATH_RETURN_INACTIVE, ICON_PATH_RETURN_ACTIVE,
//               _activeLeftButtonIndex != -1 && _activeLeftButtonIndex == 5,
//                   () => _onLeftButtonPressed(5, 0)
//           ),
//
//           // --- RIGHT BUTTONS ---
//           _buildRightButton(1690, 30, "Attack View", ICON_PATH_ATTACK_VIEW_INACTIVE, ICON_PATH_ATTACK_VIEW_ACTIVE, _activeRightButtonIndex == 0, () => _onRightButtonPressed(0)),
//           _buildRightButton(1690, 250, "Top View", ICON_PATH_TOP_VIEW_INACTIVE, ICON_PATH_TOP_VIEW_ACTIVE, _activeRightButtonIndex == 1, () => _onRightButtonPressed(1)),
//           _buildRightButton(1690, 470, "3D View", ICON_PATH_3D_VIEW_INACTIVE, ICON_PATH_3D_VIEW_ACTIVE, _activeRightButtonIndex == 2, () => _onRightButtonPressed(2)),
//           _buildRightButton(1690, 720, "Setting", ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, false, () => _navigateToSettings()),
//
//           // --- DIRECTIONAL BUTTONS --- (No changes needed here)
//           _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE, () => setState(() => _isForwardPressed = true), () => setState(() => _isForwardPressed = false)),
//           _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE, () => setState(() => _isBackPressed = true), () => setState(() => _isBackPressed = false)),
//           _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE, () => setState(() => _isLeftPressed = true), () => setState(() => _isLeftPressed = false)),
//           _buildDirectionalButton(1540, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE, () => setState(() => _isRightPressed = true), () => setState(() => _isRightPressed = false)),
//           Align(
//             alignment: Alignment.bottomCenter,
//             child: Container(
//               margin: const EdgeInsets.only(bottom: 20, left: 20, right: 20),
//               decoration: BoxDecoration(color: Colors.black.withOpacity(0.6), borderRadius: BorderRadius.circular(50)),
//               child: _buildBottomBar(),
//             ),
//           ),
//         ],
//       ),
//     );
//   }
//
//   Widget _buildLeftButton(double left, double top, String label, String inactiveIcon, String activeIcon, bool isActive, VoidCallback onPressed) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     // *** THIS IS THE ONLY CHANGE NEEDED ***
//     // The background color is now determined by the 'isActive' state, not a temporary press.
//     final containerColor = isActive ? Color(0xFFc32121) : Colors.black.withOpacity(0.6);
//
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTap: onPressed, // Use the simple onTap
//         child: Container(
//           width: 200 * widthScale,
//           height: 120 * heightScale,
//           padding: EdgeInsets.symmetric(vertical: 8.0 * heightScale),
//           decoration: BoxDecoration(
//             color: containerColor, // <-- APPLY THE COLOR HERE
//             borderRadius: BorderRadius.circular(10),
//             border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.0),
//           ),
//           child: Column(
//             mainAxisAlignment: MainAxisAlignment.center,
//             crossAxisAlignment: CrossAxisAlignment.center,
//             children: [
//               Expanded(flex: 2, child: Image.asset(isActive ? activeIcon : inactiveIcon, fit: BoxFit.contain)),
//               SizedBox(height: 5 * heightScale),
//               Text(label, textAlign: TextAlign.center,
//                 style: TextStyle(
//                   fontFamily: 'NotoSans',
//                   fontWeight: FontWeight.bold,
//                   fontSize: 24 * heightScale,
//                   color: Colors.white,
//                 ),
//               ),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   Widget _buildRightButton(double left, double top, String label, String inactiveIcon, String activeIcon, bool isActive, VoidCallback onPressed) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     const double buttonWidth = 220;
//     const double buttonHeight = 175;
//
//     // The background color is determined by the 'isActive' state.
//     final containerColor = isActive ? Color(0xFFc32121) : Colors.black.withOpacity(0.6);
//
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTap: onPressed, // Use the simple onTap
//         child: Container(
//           width: buttonWidth * widthScale,
//           height: buttonHeight * heightScale,
//           padding: EdgeInsets.symmetric(vertical: 10.0 * heightScale),
//           decoration: BoxDecoration(
//             color: containerColor,
//             borderRadius: BorderRadius.circular(15),
//             border: Border.all(
//               color: isActive ? Colors.white : Colors.transparent,
//               width: 2.5,
//             ),
//           ),
//           child: Column(
//             mainAxisAlignment: MainAxisAlignment.center,
//             crossAxisAlignment: CrossAxisAlignment.center,
//             children: [
//               Expanded(
//                 flex: 3,
//                 child: Image.asset(isActive ? activeIcon : inactiveIcon,
//                     fit: BoxFit.contain),
//               ),
//               SizedBox(height: 8 * heightScale),
//               Text(
//                 label,
//                 textAlign: TextAlign.center,
//                 style: TextStyle(
//                   fontFamily: 'NotoSans',
//                   fontWeight: FontWeight.w700,
//                   fontSize: 26 * heightScale,
//                   color: Colors.white,
//                 ),
//               ),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   Widget _buildDirectionalButton(double left, double top, bool isPressed, String inactiveIcon, String activeIcon, VoidCallback onPress, VoidCallback onRelease) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTapDown: (_) => onPress(),
//         onTapUp: (_) => onRelease(),
//         onTapCancel: () => onRelease(),
//         child: Image.asset(
//           isPressed ? activeIcon : inactiveIcon,
//           height: 100 * heightScale,
//           width: 100 * widthScale,
//           fit: BoxFit.contain,
//         ),
//       ),
//     );
//   }
//
//   Widget _buildBottomBar() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     return LayoutBuilder(
//       builder: (context, constraints) {
//         const double scrollBreakpoint = 1200.0;
//
//         final List<Widget> leftCluster = [
//           _buildBottomBarButton(
//             "ATTACK",
//             _isAttackModeOn ? ICON_PATH_ATTACK_of : ICON_PATH_ATTACK,
//             _isAttackModeOn ? _attackActiveColors : _attackInactiveColors,
//                 () async {
//               if (!_isAttackModeOn) {
//                 final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_ATTACK, title: 'DANGERS', titleColor: Colors.red, content: 'Do you want to proceed the command?');
//                 if (proceed) {
//                   setState(() {
//                     _isAttackModeOn = true;
//                     _currentCommand.commandId = 4;
//                     _activeLeftButtonIndex = 3;
//                   });
//                 }
//               } else {
//                 setState(() {
//                   _isAttackModeOn = false;
//                   _currentCommand.commandId = 0;
//                   _activeLeftButtonIndex = -1;
//                 });
//               }
//             },
//           ),
//           SizedBox(width: 12 * widthScale),
//           _buildWideBottomBarButton(
//               _isStarted ? "STOP" : "START",
//               _isStarted ? ICON_PATH_STOP : ICON_PATH_START,
//               [const Color(0xff25a625), const Color(0xff127812)],
//                   () {
//                 setState(() {
//                   _isStarted = !_isStarted;
//                   _isGunTriggerPressed = _isStarted;
//                   if (!_isStarted) {
//                     _currentCommand.moveSpeed = 0;
//                     _currentCommand.turnAngle = 0;
//                     _currentCommand.gunTrigger = false;
//                     _isGunTriggerPressed = false;
//                     _isForwardPressed = false;
//                     _isBackPressed = false;
//                     _isLeftPressed = false;
//                     _isRightPressed = false;
//                   }
//                 });
//               }),
//           SizedBox(width: 12 * widthScale),
//           _buildBottomBarButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () {}),
//           SizedBox(width: 12 * widthScale),
//           _buildBottomBarButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () {}),
//         ];
//
//         final List<Widget> middleCluster = [
//           Row(
//             mainAxisSize: MainAxisSize.min,
//             crossAxisAlignment: CrossAxisAlignment.baseline,
//             textBaseline: TextBaseline.alphabetic,
//             children: [
//               Text("0",
//                 // style: GoogleFonts.notoSans(fontSize: 80, fontWeight: FontWeight.bold, color: Colors.white)
//                 style: TextStyle(
//                   fontFamily: 'NotoSans',
//                   fontWeight: FontWeight.w700, // For Medium
//                   // or FontWeight.w700 for Bold
//                   fontSize: 75,
//                   color: Colors.white,
//                 ),
//               ),
//               SizedBox(width: 8 * widthScale),
//               Text("Km/h",
//                 // style: GoogleFonts.notoSans(fontSize: 36, fontWeight: FontWeight.w500, color: Colors.white)
//                 style: TextStyle(
//                   fontFamily: 'NotoSans',
//                   fontWeight: FontWeight.w700, // For Medium
//                   // or FontWeight.w700 for Bold
//                   fontSize: 37,
//                   color: Colors.white,
//                 ),
//               ),
//             ],
//           ),
//           SizedBox(width: 20 * widthScale),
//           Image.asset(ICON_PATH_WIFI, height: 40),
//         ];
//
//         final List<Widget> rightCluster = [
//           _buildBottomBarButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)], () async {
//             final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_EXIT, title: 'Information', titleColor: Colors.blue.shade700, content: 'Do you want to finish?');
//             if (proceed) {
//               SystemNavigator.pop();
//             }
//           }),
//         ];
//
//         if (constraints.maxWidth < scrollBreakpoint) {
//           return SingleChildScrollView(
//             scrollDirection: Axis.horizontal,
//             physics: const ClampingScrollPhysics(),
//             child: Row(
//               children: [
//                 ...leftCluster,
//                 SizedBox(width: 100 * widthScale),
//                 ...middleCluster,
//                 SizedBox(width: 100 * widthScale),
//                 ...rightCluster,
//               ],
//             ),
//           );
//         } else {
//           return Row(
//             children: [
//               ...leftCluster,
//               const Spacer(),
//               ...middleCluster,
//               const Spacer(),
//               ...rightCluster,
//             ],
//           );
//         }
//       },
//     );
//   }
//
//   Widget _buildWideBottomBarButton(String label, String iconPath, List<Color> gradientColors, VoidCallback onPressed) {
//     final screenHeight = MediaQuery.of(context).size.height;
//     final screenWidth = MediaQuery.of(context).size.width;
//     final heightScale = screenHeight / 1080.0;
//     final widthScale = screenWidth / 1920.0;
//
//     return GestureDetector(
//       onTap: onPressed,
//       child: Container(
//         constraints: BoxConstraints(
//           minWidth: 220 * widthScale,
//         ),
//         height: 80 * heightScale,
//         padding: const EdgeInsets.symmetric(horizontal: 74),
//         decoration: BoxDecoration(
//           gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//           borderRadius: BorderRadius.circular(25 * heightScale),
//         ),
//         child: Row(
//           mainAxisAlignment: MainAxisAlignment.center,
//           crossAxisAlignment: CrossAxisAlignment.center,
//           children: [
//             Image.asset(iconPath, height: 36 * heightScale),
//             const SizedBox(width: 12),
//             Text(label,
//               // style: GoogleFonts.notoSans(fontSize: 36 * heightScale, fontWeight: FontWeight.bold, color: Colors.white)
//               style: TextStyle(
//                 fontFamily: 'NotoSans',
//                 fontWeight: FontWeight.w700, // For Medium
//                 // or FontWeight.w700 for Bold
//                 fontSize: 36 * heightScale,
//                 color: Colors.white,
//               ),
//             ),
//           ],
//         ),
//       ),
//     );
//   }
//
//   Widget _buildBottomBarButton(String label, String iconPath, List<Color> gradientColors, VoidCallback onPressed) {
//     final screenHeight = MediaQuery.of(context).size.height;
//     final heightScale = screenHeight / 1080.0;
//
//     return GestureDetector(
//       onTap: onPressed,
//       child: Container(
//         height: 80 * heightScale,
//         padding: EdgeInsets.symmetric(horizontal: label.isEmpty ? 30 : 45),
//         decoration: BoxDecoration(
//           gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//           borderRadius: BorderRadius.circular(25 * heightScale),
//         ),
//         child: Row(
//           mainAxisAlignment: MainAxisAlignment.center,
//           crossAxisAlignment: CrossAxisAlignment.center,
//           children: [
//             Image.asset(iconPath, height: 36 * heightScale),
//             if (label.isNotEmpty) ...[
//               const SizedBox(width: 12),
//               Text(label,
//                 // style: GoogleFonts.notoSans(fontSize: 36 * heightScale, fontWeight: FontWeight.bold, color: Colors.white)
//                 style: TextStyle(
//                   fontFamily: 'NotoSans',
//                   fontWeight: FontWeight.w700, // For Medium
//                   // or FontWeight.w700 for Bold
//                   fontSize: 36 * heightScale,
//                   color: Colors.white,
//                 ),
//               ),
//             ]
//           ],
//         ),
//       ),
//     );
//   }
// }


//09_17
// import 'dart:async';
// import 'dart:io';
// import 'package:flutter/material.dart';
// import 'package:flutter/services.dart';
// import 'package:rtest1/settings_service.dart';
// import 'CommandIds.dart';
// import 'TouchCoord.dart';
// import 'UserCommand.dart';
// import 'icon_constants.dart';
// import 'splash_screen.dart';
// import 'settings_menu_page.dart';
//
// void main() async {
//   WidgetsFlutterBinding.ensureInitialized();
//   SystemChrome.setEnabledSystemUIMode(SystemUiMode.manual, overlays: []);
//   SystemChrome.setPreferredOrientations([
//     DeviceOrientation.landscapeLeft,
//     DeviceOrientation.landscapeRight,
//   ]);
//   runApp(const MyApp());
// }
//
// class MyApp extends StatelessWidget {
//   const MyApp({super.key});
//
//   @override
//   Widget build(BuildContext context) {
//     return MaterialApp(
//       debugShowCheckedModeBanner: false,
//       title: '',
//       theme: ThemeData(primarySwatch: Colors.blue),
//       home: const SplashScreen(),
//     );
//   }
// }
//
// // --- HOME PAGE WIDGET ---
// class HomePage extends StatefulWidget {
//   const HomePage({super.key});
//
//   @override
//   State<HomePage> createState() => _HomePageState();
// }
//
//
// class _HomePageState extends State<HomePage> {
//   // --- STATE MANAGEMENT ---
//   UserCommand _currentCommand = UserCommand();
//   bool _isLoading = true;
//   Timer? _commandTimer;
//
//   // NEW: State variables for the new UI logic
//   int _selectedModeIndex = -1;
//   bool _isModeActive = false;
//   bool _isPermissionToAttackOn = false;
//
//   // On-screen button press states
//   bool _isForwardPressed = false;
//   bool _isBackPressed = false;
//   bool _isLeftPressed = false;
//   bool _isRightPressed = false;
//
//   // Gamepad State
//   static const platform = MethodChannel('com.yourcompany/gamepad');
//   bool _gamepadConnected = false;
//   Map<String, double> _gamepadAxisValues = {};
//   bool _isZoomInPressed = false;
//   bool _isZoomOutPressed = false;
//
//
//   // GStreamer State
//   String? _errorMessage;
//   final SettingsService _settingsService = SettingsService();
//   String _robotIpAddress = '';
//   List<String> _cameraUrls = [];
//   int _currentCameraIndex = 0;
//   Key _gstreamerViewKey = UniqueKey();
//   bool _isGStreamerReady = false;
//   bool _isGStreamerLoading = true;
//   bool _gstreamerHasError = false;
//   MethodChannel? _gstreamerChannel;
//   Timer? _streamTimeoutTimer;
//
//   final Map<int, int> _buttonIndexToCommandId = {
//     0: CommandIds.DRIVING,
//     1: CommandIds.RECON,
//     2: CommandIds.MANUAL_ATTACK,
//     3: CommandIds.AUTO_ATTACK,
//     4: CommandIds.DRONE,
//   };
//
//   final Map<int, Color> _buttonActiveColor = {
//     0: const Color(0xff25a625), // Green for Driving
//     1: const Color(0xff25a625), // CHANGED: Green for Recon
//     2: const Color(0xffc32121), // Red for Manual Attack
//     3: const Color(0xffc32121), // Red for Auto Attack
//     4: const Color(0xff25a625), // CHANGED: Green for Drone
//   };
//
//   final List<Color> _permissionDisabledColors = [const Color(0xFF424242), const Color(0xFF212121)]; // Black/Grey
//   final List<Color> _permissionOffColors = [const Color(0xffc32121), const Color(0xff831616)];      // Red
//   final List<Color> _permissionOnColors = [const Color(0xff6b0000), const Color(0xff520000)];
//
//   @override
//   void initState() {
//     super.initState();
//     _loadSettingsAndInitialize();
//     platform.setMethodCallHandler(_handleGamepadEvent);
//   }
//
//   @override
//   void dispose() {
//     _commandTimer?.cancel();
//     _stopGStreamer();
//     super.dispose();
//   }
//
//   // --- LOGIC METHODS ---
//
//   void _onModeSelected(int index) {
//     setState(() {
//       // CHANGED: If a mode is currently active, do nothing, as per client request.
//       if (_isModeActive) {
//         ScaffoldMessenger.of(context).showSnackBar(
//           const SnackBar(
//             content: Text('Please press STOP before selecting a new mode.'),
//             duration: Duration(seconds: 2),
//           ),
//         );
//         return;
//       }
//       _selectedModeIndex = index;
//     });
//   }
//
//   void _onStartStopPressed() {
//     setState(() {
//       if (_isModeActive) {
//         _stopCurrentMode();
//       } else {
//         if (_selectedModeIndex != -1) {
//           _isModeActive = true;
//           _currentCommand.command_id = _buttonIndexToCommandId[_selectedModeIndex] ?? CommandIds.IDLE;
//         }
//       }
//     });
//   }
//
//   void _stopCurrentMode() {
//     _isModeActive = false;
//     _selectedModeIndex = -1;
//     _currentCommand.command_id = CommandIds.IDLE;
//     _isForwardPressed = false;
//     _isBackPressed = false;
//     _isLeftPressed = false;
//     _isRightPressed = false;
//   }
//
//   void _onPermissionPressed() {
//     setState(() {
//       _isPermissionToAttackOn = !_isPermissionToAttackOn;
//       _currentCommand.attack_permission = _isPermissionToAttackOn;
//     });
//   }
//
//   Future<void> _handleGamepadEvent(MethodCall call) async {
//     if (!mounted) return;
//     if (!_gamepadConnected) setState(() => _gamepadConnected = true);
//
//     if (call.method == "onMotionEvent") {
//       setState(() => _gamepadAxisValues = Map<String, double>.from(call.arguments));
//     } else if (call.method == "onButtonDown") {
//       final String button = call.arguments['button'];
//       setState(() {
//         switch (button) {
//           case 'KEYCODE_BUTTON_B': // Start
//             _onStartStopPressed();
//             break;
//           case 'KEYCODE_BUTTON_A': // Stop
//             if (_isModeActive) _onStartStopPressed();
//             break;
//           case 'KEYCODE_BUTTON_X': // Permission
//             _onPermissionPressed();
//             break;
//           case 'KEYCODE_BUTTON_L1':
//             _isZoomInPressed = true;
//             break;
//           case 'KEYCODE_BUTTON_L2':
//             _isZoomOutPressed = true;
//             break;
//         }
//       });
//     } else if (call.method == "onButtonUp") {
//       final String button = call.arguments['button'];
//       setState(() {
//         switch (button) {
//           case 'KEYCODE_BUTTON_L1':
//             _isZoomInPressed = false;
//             break;
//           case 'KEYCODE_BUTTON_L2':
//             _isZoomOutPressed = false;
//             break;
//         }
//       });
//     }
//   }
//
//   void _startCommandTimer() {
//     _commandTimer = Timer.periodic(const Duration(milliseconds: 100), (timer) {
//       if (_gamepadConnected) {
//         _currentCommand.move_speed = ((_gamepadAxisValues['AXIS_Y'] ?? 0.0) * -100).round();
//         _currentCommand.turn_angle = ((_gamepadAxisValues['AXIS_X'] ?? 0.0) * 100).round();
//         _currentCommand.tilt_speed = ((_gamepadAxisValues['AXIS_RZ'] ?? 0.0) * -100).round();
//         _currentCommand.pan_speed = ((_gamepadAxisValues['AXIS_Z'] ?? 0.0) * 100).round();
//
//         // gun_trigger is removed as per client request
//
//         _currentCommand.zoom_command = 0; // Default to no zoom
//         if (_isZoomInPressed) {
//           _currentCommand.zoom_command = 1; // Zoom In
//         } else if (_isZoomOutPressed || (_gamepadAxisValues['AXIS_BRAKE'] ?? 0.0) > 0.5) {
//           _currentCommand.zoom_command = -1; // Zoom Out (digital or analog)
//         }
//
//       } else {
//         _currentCommand.move_speed = 0;
//         if (_isForwardPressed) _currentCommand.move_speed = 100;
//         if (_isBackPressed) _currentCommand.move_speed = -100;
//         _currentCommand.turn_angle = 0;
//         if (_isLeftPressed) _currentCommand.turn_angle = -100;
//         if (_isRightPressed) _currentCommand.turn_angle = 100;
//
//         _currentCommand.pan_speed = 0;
//         _currentCommand.tilt_speed = 0;
//         _currentCommand.zoom_command = 0;
//       }
//
//       _sendCommandPacket(_currentCommand);
//
//       if (_currentCommand.touch_x != -1.0) {
//         _currentCommand.touch_x = -1.0;
//         _currentCommand.touch_y = -1.0;
//       }
//     });
//   }
//
//   // --- UI WIDGETS ---
//
//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       backgroundColor: Colors.black,
//       body: _isLoading
//           ? const Center(child: CircularProgressIndicator())
//           : Stack(
//         children: [
//           Positioned.fill(
//             child: KeyedSubtree(
//               key: _gstreamerViewKey,
//               child: AndroidView(
//                 viewType: 'gstreamer_view',
//                 onPlatformViewCreated: _onGStreamerPlatformViewCreated,
//               ),
//             ),
//           ),
//
//           if (!_isGStreamerLoading && !_gstreamerHasError)
//             Positioned.fill(child: _buildTouchDetector()),
//
//           Positioned.fill(child: _buildStreamOverlay()),
//           // Positioned.fill(child: _buildTouchDetector()),
//
//           _buildModeButton(0, 30, 30, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE),
//           _buildModeButton(1, 30, 214, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE),
//           _buildModeButton(2, 30, 398, "Manual Attack", ICON_PATH_MANUAL_ATTACK_INACTIVE, ICON_PATH_MANUAL_ATTACK_ACTIVE),
//           _buildModeButton(3, 30, 582, "Auto Attack", ICON_PATH_AUTO_ATTACK_INACTIVE, ICON_PATH_AUTO_ATTACK_ACTIVE),
//           _buildModeButton(4, 30, 766, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE),
//
//           _buildViewButton(1690, 30, "Attack View", ICON_PATH_ATTACK_VIEW_ACTIVE, true),
//           _buildViewButton(1690, 720, "Setting", ICON_PATH_SETTINGS, false, onPressed: _navigateToSettings),
//
//           _buildDirectionalControls(),
//           Align(
//             alignment: Alignment.bottomCenter,
//             child: Container(
//               margin: const EdgeInsets.only(bottom: 20, left: 20, right: 20),
//               decoration: BoxDecoration(color: Colors.black.withOpacity(0.6), borderRadius: BorderRadius.circular(50)),
//               child: _buildBottomBar(),
//             ),
//           ),
//         ],
//       ),
//     );
//   }
//
//   Widget _buildModeButton(int index, double left, double top, String label, String inactiveIcon, String activeIcon) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     final bool isSelected = _selectedModeIndex == index;
//     final Color color = isSelected ? (_buttonActiveColor[index] ?? Colors.grey) : Colors.black.withOpacity(0.6);
//     final String icon = isSelected ? activeIcon : inactiveIcon;
//
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTap: () => _onModeSelected(index),
//         child: Container(
//           width: 200 * widthScale,
//           height: 120 * heightScale,
//           padding: EdgeInsets.symmetric(vertical: 4.0 * heightScale),
//           decoration: BoxDecoration(
//             color: color,
//             borderRadius: BorderRadius.circular(9),
//             border: Border.all(color: isSelected ? Colors.white : Colors.transparent, width: 2.0),
//           ),
//           child: Column(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               Expanded(flex: 2, child: Image.asset(icon, fit: BoxFit.contain)),
//               SizedBox(height: 5 * heightScale),
//               Text(label, textAlign: TextAlign.center, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.bold, fontSize: 25 * heightScale, color: Colors.white)),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   Widget _buildViewButton(double left, double top, String label, String iconPath, bool isActive, {VoidCallback? onPressed}) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     final Color color = isActive ? const Color(0xFFc32121) : Colors.black.withOpacity(0.6);
//
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTap: onPressed,
//         child: Container(
//           width: 220 * widthScale,
//           height: 175 * heightScale,
//           padding: EdgeInsets.symmetric(vertical: 10.0 * heightScale),
//           decoration: BoxDecoration(
//             color: color,
//             borderRadius: BorderRadius.circular(15),
//             border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.5),
//           ),
//           child: Column(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               Expanded(flex: 3, child: Image.asset(iconPath, fit: BoxFit.contain)),
//               SizedBox(height: 8 * heightScale),
//               Text(label, textAlign: TextAlign.center, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 26 * heightScale, color: Colors.white)),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   Widget _buildBottomBar() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     return LayoutBuilder(
//       builder: (context, constraints) {
//
//         List<Color> permissionButtonColors;
//         bool isCombatModeActive = _isModeActive && (_selectedModeIndex == 2 || _selectedModeIndex == 3);
//
//         if (isCombatModeActive) {
//           // If a combat mode is active, the button is either ON or OFF
//           permissionButtonColors = _isPermissionToAttackOn ? _permissionOnColors : _permissionOffColors;
//         } else {
//           // If IDLE or in a non-combat mode, the button is DISABLED
//           permissionButtonColors = _permissionDisabledColors;
//         }
//
//         final List<Widget> leftCluster = [
//           _buildBottomBarButton(
//             "PERMISSION TO ATTACK",
//             null,
//             _isPermissionToAttackOn ? [const Color(0xffc32121), const Color(0xff831616)] : [const Color(0xFF424242), const Color(0xFF212121)],
//             _onPermissionPressed,
//           ),
//
//           SizedBox(width: 12 * widthScale),
//           _buildWideBottomBarButton(
//             _isModeActive ? "STOP" : "START",
//             _isModeActive ? ICON_PATH_STOP : ICON_PATH_START,
//             [const Color(0xff25a625), const Color(0xff127812)],
//             _onStartStopPressed,
//           ),
//           SizedBox(width: 12 * widthScale),
//           _buildBottomBarButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () {}),
//           SizedBox(width: 12 * widthScale),
//           _buildBottomBarButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () {}),
//         ];
//
//         final List<Widget> middleCluster = [
//           Row(
//             mainAxisSize: MainAxisSize.min,
//             crossAxisAlignment: CrossAxisAlignment.baseline,
//             textBaseline: TextBaseline.alphabetic,
//             children: [
//               const Text("0", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 75, color: Colors.white)),
//               SizedBox(width: 8 * widthScale),
//               const Text("Km/h", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 37, color: Colors.white)),
//             ],
//           ),
//           SizedBox(width: 20 * widthScale),
//           Image.asset(ICON_PATH_WIFI, height: 40),
//         ];
//
//         final List<Widget> rightCluster = [
//           _buildBottomBarButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)], () async {
//             final proceed = await _showExitDialog();
//             if (proceed) SystemNavigator.pop();
//           }),
//         ];
//
//         return Row(
//           children: [
//             ...leftCluster,
//             const Spacer(),
//             ...middleCluster,
//             const Spacer(),
//             ...rightCluster,
//           ],
//         );
//       },
//     );
//   }
//
//   Widget _buildWideBottomBarButton(String label, String iconPath, List<Color> gradientColors, VoidCallback onPressed) {
//     final screenHeight = MediaQuery.of(context).size.height;
//     final screenWidth = MediaQuery.of(context).size.width;
//     final heightScale = screenHeight / 1080.0;
//     final widthScale = screenWidth / 1920.0;
//
//     return GestureDetector(
//       onTap: onPressed,
//       child: Container(
//         constraints: BoxConstraints(minWidth: 220 * widthScale),
//         height: 80 * heightScale,
//         padding: const EdgeInsets.symmetric(horizontal: 74),
//         decoration: BoxDecoration(
//           gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//           borderRadius: BorderRadius.circular(25 * heightScale),
//         ),
//         child: Row(
//           mainAxisAlignment: MainAxisAlignment.center,
//           children: [
//             Image.asset(iconPath, height: 36 * heightScale),
//             const SizedBox(width: 12),
//             Text(label, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 36 * heightScale, color: Colors.white)),
//           ],
//         ),
//       ),
//     );
//   }
//
//   Widget _buildBottomBarButton(String label, String? iconPath, List<Color> gradientColors, VoidCallback onPressed) {
//     final screenHeight = MediaQuery.of(context).size.height;
//     final heightScale = screenHeight / 1080.0;
//
//     return GestureDetector(
//       onTap: onPressed,
//       child: Container(
//         height: 80 * heightScale,
//         padding: const EdgeInsets.symmetric(horizontal: 30),
//         decoration: BoxDecoration(
//           gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//           borderRadius: BorderRadius.circular(25 * heightScale),
//         ),
//         child: Row(
//           mainAxisAlignment: MainAxisAlignment.center,
//           children: [
//             if (iconPath != null && iconPath.isNotEmpty) ...[
//               Image.asset(iconPath, height: 36 * heightScale),
//               if (label.isNotEmpty) const SizedBox(width: 12),
//             ],
//             if (label.isNotEmpty)
//               Text(label, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 30 * heightScale, color: Colors.white)),
//           ],
//         ),
//       ),
//     );
//   }
//
//   Widget _buildDirectionalControls() {
//     return Stack(
//       children: [
//         _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE, () => setState(() => _isForwardPressed = true), () => setState(() => _isForwardPressed = false)),
//         _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE, () => setState(() => _isBackPressed = true), () => setState(() => _isBackPressed = false)),
//         _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE, () => setState(() => _isLeftPressed = true), () => setState(() => _isLeftPressed = false)),
//         _buildDirectionalButton(1540, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE, () => setState(() => _isRightPressed = true), () => setState(() => _isRightPressed = false)),
//       ],
//     );
//   }
//
//   Widget _buildDirectionalButton(double left, double top, bool isPressed, String inactiveIcon, String activeIcon, VoidCallback onPress, VoidCallback onRelease) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTapDown: (_) => onPress(),
//         onTapUp: (_) => onRelease(),
//         onTapCancel: () => onRelease(),
//         child: Image.asset(isPressed ? activeIcon : inactiveIcon, height: 100 * heightScale, width: 100 * widthScale, fit: BoxFit.contain),
//       ),
//     );
//   }
//
//   Widget _buildTouchDetector() {
//     return GestureDetector(
//       key: const GlobalObjectKey('_playerKey'),
//       behavior: HitTestBehavior.opaque,
//       onTapUp: (details) {
//         if (_isModeActive && _selectedModeIndex == 2) { // 2 corresponds to Manual Attack
//           final RenderBox? renderBox = context.findRenderObject() as RenderBox?;
//           if (renderBox != null) {
//             final Offset localPosition = renderBox.globalToLocal(details.globalPosition);
//             final double x = localPosition.dx / renderBox.size.width;
//             final double y = localPosition.dy / renderBox.size.height;
//             if (x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0) {
//               _sendTouchPacket(TouchCoord()..x = x..y = y);
//             }
//           }
//         }
//       },
//       child: Container(color: Colors.transparent),
//     );
//   }
//
//   Future<bool> _showExitDialog() async {
//     return await showDialog<bool>(
//       context: context,
//       builder: (context) => AlertDialog(
//         title: const Text('Confirm Exit'),
//         content: const Text('Do you want to close the application?'),
//         actions: [
//           TextButton(onPressed: () => Navigator.of(context).pop(false), child: const Text('Cancel')),
//           TextButton(onPressed: () => Navigator.of(context).pop(true), child: const Text('Exit')),
//         ],
//       ),
//     ) ?? false;
//   }
//
//   Future<void> _handleGStreamerMessages(MethodCall call) async {
//     if (!mounted) return;
//     _streamTimeoutTimer?.cancel();
//     switch (call.method) {
//       case 'onStreamReady':
//         setState(() { _isGStreamerLoading = false; _gstreamerHasError = false; });
//         break;
//       case 'onStreamError':
//         final String? error = call.arguments?['error'];
//         setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "Stream error: ${error ?? 'Unknown native error'}"; });
//         break;
//     }
//   }
//
//   void _retryStream() {
//     if (_currentCameraIndex == -1) {
//       print("Retry failed: No camera index selected.");
//       return;
//     }
//
//     print("Retrying stream for camera index: $_currentCameraIndex");
//
//     setState(() {
//       _gstreamerViewKey = UniqueKey(); // This is the MOST IMPORTANT line. It forces the widget to be destroyed and recreated.
//       _isGStreamerReady = false;
//       _isGStreamerLoading = true; // Show the loading spinner again
//       _gstreamerHasError = false; // Clear the error state
//       _errorMessage = null;
//     });
//
//   }
//
//   Future<void> _loadSettingsAndInitialize() async {
//     _robotIpAddress = await _settingsService.loadIpAddress();
//     _cameraUrls = await _settingsService.loadCameraUrls();
//     if (mounted) {
//       _switchCamera(0);
//       _startCommandTimer();
//       setState(() => _isLoading = false);
//     }
//   }
//
//   Future<void> _sendCommandPacket(UserCommand command) async {
//     if (_robotIpAddress.isEmpty) return;
//     try {
//       final socket = await Socket.connect(_robotIpAddress, 65432, timeout: const Duration(milliseconds: 150));
//       socket.add(command.toBytes());
//       await socket.flush();
//       socket.close();
//     } catch (e) {}
//   }
//
//   Future<void> _sendTouchPacket(TouchCoord coord) async {
//     if (_robotIpAddress.isEmpty) return;
//     try {
//       const int TOUCH_PORT = 65433;
//       final socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
//       socket.send(coord.toBytes(), InternetAddress(_robotIpAddress), TOUCH_PORT);
//       socket.close();
//       print('Sent Touch Packet (UDP): X=${coord.x}, Y=${coord.y}');
//     } catch (e) {
//       print('Error sending touch UDP packet: $e');
//     }
//   }
//
//   void _stopGStreamer() {
//     if (_gstreamerChannel != null && _isGStreamerReady) {
//       _gstreamerChannel!.invokeMethod('stopStream').catchError((e) => print("Error stopping stream: $e"));
//     }
//     _isGStreamerReady = false;
//   }
//
//   Future<void> _switchCamera(int index) async {
//     if (_cameraUrls.isEmpty || index < 0 || index >= _cameraUrls.length) return;
//     if (index == _currentCameraIndex && !_gstreamerHasError) return;
//     if (_gstreamerChannel != null && _isGStreamerReady) {
//       try { await _gstreamerChannel!.invokeMethod('stopStream'); } catch (e) {}
//     }
//     setState(() {
//       _currentCameraIndex = index;
//       _gstreamerViewKey = UniqueKey();
//       _isGStreamerReady = false;
//       _isGStreamerLoading = true;
//       _gstreamerHasError = false;
//       _errorMessage = null;
//     });
//   }
//
//
//   void _onGStreamerPlatformViewCreated(int id) {
//     _gstreamerChannel = MethodChannel('gstreamer_channel_$id');
//     _gstreamerChannel!.setMethodCallHandler(_handleGStreamerMessages);
//     setState(() {
//       _isGStreamerReady = true;
//       _isGStreamerLoading = false;
//     });
//     _playCurrentCameraStream();
//   }
//
//   Future<void> _playCurrentCameraStream() async {
//     _streamTimeoutTimer?.cancel();
//     if (_cameraUrls.isEmpty || _currentCameraIndex < 0 || _currentCameraIndex >= _cameraUrls.length) {
//       if (mounted) setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "No valid camera URL."; });
//       return;
//     }
//     if (_gstreamerChannel == null || !_isGStreamerReady) {
//       if (mounted) setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "GStreamer channel not ready."; });
//       return;
//     }
//     setState(() { _isGStreamerLoading = true; _gstreamerHasError = false; _errorMessage = null; });
//     _streamTimeoutTimer = Timer(const Duration(seconds: 8), () {
//       if (mounted && _isGStreamerLoading) {
//         setState(() { _gstreamerHasError = true; _errorMessage = "Connection timed out."; _isGStreamerLoading = false; });
//       }
//     });
//     try {
//       final String url = _cameraUrls[_currentCameraIndex];
//       await _gstreamerChannel!.invokeMethod('startStream', {'url': url});
//     } catch (e) {
//       _streamTimeoutTimer?.cancel();
//       if (mounted) {
//         setState(() { _gstreamerHasError = true; _errorMessage = "Failed to start stream: ${e.toString()}"; _isGStreamerLoading = false; });
//       }
//     }
//   }
//
//   Widget _buildStreamOverlay() {
//     if (_isGStreamerLoading) {
//       return Container(
//         color: Colors.black.withOpacity(0.5),
//         child: const Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [CircularProgressIndicator(color: Colors.white), SizedBox(height: 20), Text('Connecting to stream...', style: TextStyle(color: Colors.white, fontSize: 18))])),
//       );
//     }
//     if (_gstreamerHasError) {
//       return Container(
//         color: Colors.black.withOpacity(0.7),
//         child: Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
//           const Icon(Icons.error_outline, color: Colors.redAccent, size: 70),
//           const SizedBox(height: 20),
//           Padding(padding: const EdgeInsets.symmetric(horizontal: 40.0), child: Text(_errorMessage ?? 'Stream failed to load.', textAlign: TextAlign.center, style: const TextStyle(color: Colors.white, fontSize: 20))),
//           const SizedBox(height: 30),
//           ElevatedButton.icon(
//             icon: const Icon(Icons.refresh),
//             label: const Text('Retry'),
//             onPressed: () { if (_currentCameraIndex != -1) _switchCamera(_currentCameraIndex); },
//             style: ElevatedButton.styleFrom(foregroundColor: Colors.white, backgroundColor: Colors.blueGrey, padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 15), textStyle: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
//           ),
//         ])),
//       );
//     }
//     return const SizedBox.shrink();
//   }
//
//   Future<void> _navigateToSettings() async {
//     final bool? settingsChanged = await Navigator.push<bool>(context, MaterialPageRoute(builder: (context) => const SettingsMenuPage()));
//     if (settingsChanged == true && mounted) {
//       _commandTimer?.cancel();
//       await _loadSettingsAndInitialize();
//     }
//   }
// }
//
//
//
//


//09_19
import 'dart:async';
// import 'dart:io';
// import 'package:flutter/material.dart';
// import 'package:flutter/services.dart';
// import 'package:rtest1/settings_service.dart';
// import 'CommandIds.dart';
// import 'ServerStatus.dart';
// import 'StatusPacket.dart';
// import 'TouchCoord.dart';
// import 'UserCommand.dart';
// import 'icon_constants.dart';
// import 'splash_screen.dart';
// import 'settings_menu_page.dart';
// import 'package:flutter_joystick/flutter_joystick.dart';
// import 'DrivingCommand.dart';
//
// void main() async {
//   WidgetsFlutterBinding.ensureInitialized();
//   SystemChrome.setEnabledSystemUIMode(SystemUiMode.manual, overlays: []);
//   SystemChrome.setPreferredOrientations([
//     DeviceOrientation.landscapeLeft,
//     DeviceOrientation.landscapeRight,
//   ]);
//   runApp(const MyApp());
// }
//
// class MyApp extends StatelessWidget {
//   const MyApp({super.key});
//
//   @override
//   Widget build(BuildContext context) {
//     return MaterialApp(
//       debugShowCheckedModeBanner: false,
//       title: '',
//       theme: ThemeData(primarySwatch: Colors.blue),
//       home: const SplashScreen(),
//     );
//   }
// }
//
// // --- HOME PAGE WIDGET ---
// class HomePage extends StatefulWidget {
//   const HomePage({super.key});
//
//   @override
//   State<HomePage> createState() => _HomePageState();
// }
//
//
// class _HomePageState extends State<HomePage> {
//   // --- STATE MANAGEMENT ---
//   UserCommand _currentCommand = UserCommand();
//   bool _isLoading = true;
//   Timer? _commandTimer;
//
//   // NEW: State variables for the new UI logic
//   int _selectedModeIndex = -1;
//   bool _isModeActive = false;
//   bool _isPermissionToAttackOn = false;
//
//   // On-screen button press states
//   bool _isForwardPressed = false;
//   bool _isBackPressed = false;
//   bool _isLeftPressed = false;
//   bool _isRightPressed = false;
//
//   // Gamepad State
//   static const platform = MethodChannel('com.yourcompany/gamepad');
//   bool _gamepadConnected = false;
//   Map<String, double> _gamepadAxisValues = {};
//   bool _isZoomInPressed = false;
//   bool _isZoomOutPressed = false;
//
//   // GStreamer State
//   String? _errorMessage;
//   final SettingsService _settingsService = SettingsService();
//   String _robotIpAddress = '';
//   List<String> _cameraUrls = [];
//   int _currentCameraIndex = 0;
//   Key _gstreamerViewKey = UniqueKey();
//   bool _isGStreamerReady = false;
//   bool _isGStreamerLoading = true;
//   bool _gstreamerHasError = false;
//   MethodChannel? _gstreamerChannel;
//   Timer? _streamTimeoutTimer;
//
//   double _currentZoomLevel = 1.0;
//   double _lateralWindSpeed = 0.0;
//   int _windDirectionIndex = 0;
//
//   final TransformationController _transformationController = TransformationController();
//
//   Socket? _statusSocket;
//   StreamSubscription? _statusSocketSubscription;
//   bool _isServerConnected = false;
//
//   ServerStatus _serverStatus = ServerStatus.disconnected();
//
//
//   final Map<int, int> _buttonIndexToCommandId = {
//     0: CommandIds.DRIVING,
//     1: CommandIds.RECON,
//     2: CommandIds.MANUAL_ATTACK,
//     3: CommandIds.AUTO_ATTACK,
//     4: CommandIds.DRONE,
//   };
//
//   final Map<int, Color> _buttonActiveColor = {
//     0: const Color(0xff25a625), // Green for Driving
//     1: const Color(0xff25a625), // CHANGED: Green for Recon
//     2: const Color(0xffc32121), // Red for Manual Attack
//     3: const Color(0xffc32121), // Red for Auto Attack
//     4: const Color(0xff25a625), // CHANGED: Green for Drone
//   };
//
//   final List<Color> _permissionDisabledColors = [const Color(0xFF424242), const Color(0xFF212121)]; // Black/Grey
//   final List<Color> _permissionOffColors = [const Color(0xffc32121), const Color(0xff831616)];      // Red
//   final List<Color> _permissionOnColors = [const Color(0xff6b0000), const Color(0xff520000)];
//
//   @override
//   void initState() {
//     super.initState();
//     _loadSettingsAndInitialize();
//     platform.setMethodCallHandler(_handleGamepadEvent);
//   }
//
//   // NEED THIS for wind icon changes
//   // @override
//   // void initState() {
//   //   super.initState();
//   //   _loadSettingsAndInitialize();
//   //   platform.setMethodCallHandler(_handleGamepadEvent);
//   //
//   //   // --- TEMPORARY: Simulate receiving wind data (speed AND direction) ---
//   //   Timer.periodic(const Duration(seconds: 2), (timer) {
//   //     if (mounted) {
//   //       setState(() {
//   //         // Generate a random wind speed between -5.0 and 5.0
//   //         _lateralWindSpeed = (DateTime.now().second % 100) / 10.0 - 5.0;
//   //
//   //         // Cycle through the 8 wind directions every 16 seconds (2 seconds per direction)
//   //         _windDirectionIndex = (DateTime.now().second ~/ 2) % 8;
//   //       });
//   //     } else {
//   //       timer.cancel();
//   //     }
//   //   });
//   // }
//
//   @override
//   void dispose() {
//     _commandTimer?.cancel();
//     _stopGStreamer();
//     _transformationController.dispose();
//     _statusSocketSubscription?.cancel();
//     _statusSocket?.destroy();
//     super.dispose();
//   }
//
//
//   Future<void> _connectToStatusServer() async {
//     if (_robotIpAddress.isEmpty) return;
//
//     // Disconnect if already connected
//     await _statusSocketSubscription?.cancel();
//     _statusSocket?.destroy();
//
//     try {
//       const int STATUS_PORT = 65435;
//       _statusSocket = await Socket.connect(_robotIpAddress, STATUS_PORT, timeout: const Duration(seconds: 5));
//       setState(() {
//         _isServerConnected = true;
//       });
//       print("Connected to status server!");
//
//       _statusSocketSubscription = _statusSocket!.listen(
//             (Uint8List data) {
//           // This is where we receive the StatusPacket
//           try {
//             final status = StatusPacket.fromBytes(data);
//             // Update the UI with the new data from the server
//             if (mounted) {
//               setState(() {
//                 _lateralWindSpeed = status.lateralWindSpeed;
//                 _windDirectionIndex = status.windDirectionIndex;
//                 // You can also update other UI elements based on:
//                 // status.currentModeId
//                 // status.isRtspRunning
//               });
//             }
//           } catch (e) {
//             print("Error parsing status packet: $e");
//           }
//         },
//         onError: (error) {
//           print("Status socket error: $error");
//           setState(() => _isServerConnected = false);
//           _reconnectStatusServer();
//         },
//         onDone: () {
//           print("Status server disconnected.");
//           setState(() => _isServerConnected = false);
//           _reconnectStatusServer();
//         },
//         cancelOnError: true,
//       );
//     } catch (e) {
//       print("Failed to connect to status server: $e");
//       setState(() => _isServerConnected = false);
//       _reconnectStatusServer();
//     }
//   }
//
//   void _reconnectStatusServer() {
//     // Simple reconnect logic: try again after a delay
//     Future.delayed(const Duration(seconds: 5), () {
//       if (mounted && !_isServerConnected) {
//         print("Attempting to reconnect to status server...");
//         _connectToStatusServer();
//       }
//     });
//   }
//
//   // --- LOGIC METHODS ---
//
//   void _onModeSelected(int index) {
//     setState(() {
//       // CHANGED: If a mode is currently active, do nothing, as per client request.
//       if (_isModeActive) {
//         ScaffoldMessenger.of(context).showSnackBar(
//           const SnackBar(
//             content: Text('Please press STOP before selecting a new mode.'),
//             duration: Duration(seconds: 2),
//           ),
//         );
//         return;
//       }
//       _selectedModeIndex = index;
//     });
//   }
//
//   void _onStartStopPressed() {
//     setState(() {
//       if (_isModeActive) {
//         _stopCurrentMode();
//       } else {
//         if (_selectedModeIndex != -1) {
//           _isModeActive = true;
//           _currentCommand.command_id = _buttonIndexToCommandId[_selectedModeIndex] ?? CommandIds.IDLE;
//         }
//       }
//     });
//   }
//
//   void _stopCurrentMode() {
//     _isModeActive = false;
//     _selectedModeIndex = -1;
//     _currentCommand.command_id = CommandIds.IDLE;
//     _isForwardPressed = false;
//     _isBackPressed = false;
//     _isLeftPressed = false;
//     _isRightPressed = false;
//   }
//
//   void _onPermissionPressed() {
//     setState(() {
//       _isPermissionToAttackOn = !_isPermissionToAttackOn;
//       _currentCommand.attack_permission = _isPermissionToAttackOn;
//     });
//   }
//
//   Future<void> _handleGamepadEvent(MethodCall call) async {
//     if (!mounted) return;
//     if (!_gamepadConnected) setState(() => _gamepadConnected = true);
//
//     if (call.method == "onMotionEvent") {
//       setState(() => _gamepadAxisValues = Map<String, double>.from(call.arguments));
//     } else if (call.method == "onButtonDown") {
//       final String button = call.arguments['button'];
//       setState(() {
//         switch (button) {
//           case 'KEYCODE_BUTTON_B': // Start
//             _onStartStopPressed();
//             break;
//           case 'KEYCODE_BUTTON_A': // Stop
//             if (_isModeActive) _onStartStopPressed();
//             break;
//           case 'KEYCODE_BUTTON_X': // Permission
//             _onPermissionPressed();
//             break;
//           case 'KEYCODE_BUTTON_L1':
//             _isZoomInPressed = true;
//             break;
//           case 'KEYCODE_BUTTON_L2':
//             _isZoomOutPressed = true;
//             break;
//         }
//       });
//     } else if (call.method == "onButtonUp") {
//       final String button = call.arguments['button'];
//       setState(() {
//         switch (button) {
//           case 'KEYCODE_BUTTON_L1':
//             _isZoomInPressed = false;
//             break;
//           case 'KEYCODE_BUTTON_L2':
//             _isZoomOutPressed = false;
//             break;
//         }
//       });
//     }
//   }
//
//   // void _startCommandTimer() {
//   //   _commandTimer = Timer.periodic(const Duration(milliseconds: 100), (timer) {
//   //     if (_gamepadConnected) {
//   //       _currentCommand.move_speed = ((_gamepadAxisValues['AXIS_Y'] ?? 0.0) * -100).round();
//   //       _currentCommand.turn_angle = ((_gamepadAxisValues['AXIS_X'] ?? 0.0) * 100).round();
//   //       _currentCommand.tilt_speed = ((_gamepadAxisValues['AXIS_RZ'] ?? 0.0) * -100).round();
//   //       _currentCommand.pan_speed = ((_gamepadAxisValues['AXIS_Z'] ?? 0.0) * 100).round();
//   //
//   //
//   //       _currentCommand.zoom_command = 0; // Default to no zoom
//   //       if (_isZoomInPressed) {
//   //         _currentCommand.zoom_command = 1; // Zoom In
//   //       } else if (_isZoomOutPressed || (_gamepadAxisValues['AXIS_BRAKE'] ?? 0.0) > 0.5) {
//   //         _currentCommand.zoom_command = -1; // Zoom Out (digital or analog)
//   //       }
//   //
//   //     } else {
//   //       _currentCommand.move_speed = 0;
//   //       if (_isForwardPressed) _currentCommand.move_speed = 100;
//   //       if (_isBackPressed) _currentCommand.move_speed = -100;
//   //       _currentCommand.turn_angle = 0;
//   //       if (_isLeftPressed) _currentCommand.turn_angle = -100;
//   //       if (_isRightPressed) _currentCommand.turn_angle = 100;
//   //
//   //       _currentCommand.pan_speed = 0;
//   //       _currentCommand.tilt_speed = 0;
//   //       _currentCommand.zoom_command = 0;
//   //     }
//   //
//   //     _sendCommandPacket(_currentCommand);
//   //
//   //     if (_currentCommand.touch_x != -1.0) {
//   //       _currentCommand.touch_x = -1.0;
//   //       _currentCommand.touch_y = -1.0;
//   //     }
//   //   });
//   // }
//
//   // Inside the _HomePageState class
//
//   void _startCommandTimer() {
//     _commandTimer = Timer.periodic(const Duration(milliseconds: 100), (timer) {
//       // Create a new, separate command object for driving
//       DrivingCommand drivingCommand = DrivingCommand();
//
//       if (_gamepadConnected) {
//         // Populate the driving command from the gamepad
//         drivingCommand.move_speed = ((_gamepadAxisValues['AXIS_Y'] ?? 0.0) * -100).round();
//         drivingCommand.turn_angle = ((_gamepadAxisValues['AXIS_X'] ?? 0.0) * 100).round();
//
//         // Populate the main state command from the gamepad
//         _currentCommand.tilt_speed = ((_gamepadAxisValues['AXIS_RZ'] ?? 0.0) * -100).round();
//         _currentCommand.pan_speed = ((_gamepadAxisValues['AXIS_Z'] ?? 0.0) * 100).round();
//
//         _currentCommand.zoom_command = 0;
//         if (_isZoomInPressed) {
//           _currentCommand.zoom_command = 1;
//         } else if (_isZoomOutPressed || (_gamepadAxisValues['AXIS_BRAKE'] ?? 0.0) > 0.5) {
//           _currentCommand.zoom_command = -1;
//         }
//
//       } else {
//         // Populate the driving command from the on-screen joystick/buttons
//         drivingCommand.move_speed = 0;
//         if (_isForwardPressed) drivingCommand.move_speed = 100;
//         if (_isBackPressed) drivingCommand.move_speed = -100;
//
//         drivingCommand.turn_angle = 0;
//         if (_isLeftPressed) drivingCommand.turn_angle = -100;
//         if (_isRightPressed) drivingCommand.turn_angle = 100;
//
//         // The main state command's pan/tilt are already being set by the joystick listener
//       }
//
//       // --- SEND BOTH PACKETS ---
//       _sendCommandPacket(_currentCommand); // Sends the state command (TCP)
//       _sendDrivingPacket(drivingCommand); // Sends the driving command (UDP)
//
//       // Reset touch coordinates after sending
//       if (_currentCommand.touch_x != -1.0) {
//         _currentCommand.touch_x = -1.0;
//         _currentCommand.touch_y = -1.0;
//       }
//     });
//   }
//
//   Future<void> _sendDrivingPacket(DrivingCommand command) async {
//     if (_robotIpAddress.isEmpty) return;
//     try {
//       const int DRIVING_PORT = 65434; // The new port for driving
//       final socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
//       socket.send(command.toBytes(), InternetAddress(_robotIpAddress), DRIVING_PORT);
//       socket.close();
//     } catch (e) {
//     }
//   }
//
//   // Widget to display the current zoom level
//   Widget _buildZoomDisplay() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     return Positioned(
//       // Positioned according to the client's reference image (x=1360, y=30)
//       top: 30 * widthScale,
//       right: (1920 - 1360 - 200) * widthScale, // Approximate right position based on image
//       child: Container(
//         padding: EdgeInsets.symmetric(horizontal: 20 * widthScale, vertical: 10 * widthScale),
//         decoration: BoxDecoration(
//           color: Colors.black.withOpacity(0.5),
//           borderRadius: BorderRadius.circular(10),
//         ),
//         child: Text(
//           '${_currentZoomLevel.toStringAsFixed(1)} x', // Formats to one decimal place, e.g., "1.3 x"
//           style: TextStyle(
//             fontFamily: 'NotoSans',
//             fontSize: 60 * widthScale, // Scaled font size
//             fontWeight: FontWeight.w600, // Medium weight
//             color: Colors.white,
//           ),
//         ),
//       ),
//     );
//   }
//
//   // Widget to display the lateral wind indicator (UPDATED)
//   Widget _buildWindIndicator() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     // Create a list of all the wind icon paths in order
//     final List<String> windIcons = [
//       ICON_PATH_WIND_N,
//       ICON_PATH_WIND_NE,
//       ICON_PATH_WIND_E,
//       ICON_PATH_WIND_SE,
//       ICON_PATH_WIND_S,
//       ICON_PATH_WIND_SW,
//       ICON_PATH_WIND_W,
//       ICON_PATH_WIND_NW,
//     ];
//
//     // Select the current icon based on the state variable
//     // with a check to prevent out-of-bounds errors.
//     final String currentWindIcon = (_windDirectionIndex >= 0 && _windDirectionIndex < windIcons.length)
//         ? windIcons[_windDirectionIndex]
//         : windIcons[0]; // Default to North if index is invalid
//
//     return Positioned(
//       top: 40 * widthScale,
//       left: 280 * widthScale,
//       child: Row(
//         children: [
//           // Use an Image.asset widget to display the dynamic icon
//           Image.asset(
//             currentWindIcon,
//             width: 40 * widthScale,
//             height: 40 * widthScale,
//           ),
//           SizedBox(width: 10 * widthScale),
//           Text(
//             _lateralWindSpeed.toStringAsFixed(1),
//             style: TextStyle(
//               fontFamily: 'NotoSans',
//               fontSize: 60 * widthScale,
//               fontWeight: FontWeight.w600,
//               color: Colors.white,
//             ),
//           ),
//         ],
//       ),
//     );
//   }
//
//   Widget _buildMovementJoystick() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     return Positioned(
//       left: 260 * widthScale,
//       bottom: 220 * widthScale,
//       child: Opacity(
//         opacity: _isServerConnected ? 1.0 : 0.5,
//         child: Joystick(
//           mode: JoystickMode.vertical,
//           stick: const CircleAvatar(
//             radius: 30,
//             backgroundColor: Colors.blue,
//           ),
//           base: Container(
//             width: 150,
//             height: 150,
//             decoration: BoxDecoration(
//               color: Colors.grey,
//               shape: BoxShape.circle,
//               border: Border.all(color: Colors.white38, width: 2),
//             ),
//           ),
//           // --- THIS IS THE FIX ---
//           // The listener is always active, but the logic inside is conditional.
//           listener: (details) {
//             if (!_isServerConnected) return; // Do nothing if disconnected
//             setState(() {
//               _isForwardPressed = details.y < -0.5;
//               _isBackPressed = details.y > 0.5;
//               if (details.y.abs() <= 0.5) {
//                 _isForwardPressed = false;
//                 _isBackPressed = false;
//               }
//               _isLeftPressed = false;
//               _isRightPressed = false;
//             });
//           },
//         ),
//       ),
//     );
//   }
//
//   // Joystick for Camera Pan/Tilt (Right Side - HORIZONTAL ONLY)
//   // Joystick for Robot Turn (Right Side - HORIZONTAL ONLY)
//   Widget _buildPanTiltJoystick() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     return Positioned(
//       right: 260 * widthScale,
//       bottom: 220 * widthScale,
//       child: Opacity(
//         opacity: _isServerConnected ? 1.0 : 0.5,
//         child: Joystick(
//           mode: JoystickMode.horizontal,
//           stick: const CircleAvatar(
//             radius: 30,
//             backgroundColor: Colors.blue,
//           ),
//           base: Container(
//             width: 150,
//             height: 150,
//             decoration: BoxDecoration(
//               color: Colors.grey,
//               shape: BoxShape.circle,
//               border: Border.all(color: Colors.white38, width: 2),
//             ),
//           ),
//           // --- THIS IS THE FIX ---
//           // The listener now sets the boolean flags for turning,
//           // just like the old arrow buttons.
//           listener: (details) {
//             if (!_isServerConnected) return; // Do nothing if disconnected
//             setState(() {
//               _isLeftPressed = details.x < -0.5;
//               _isRightPressed = details.x > 0.5;
//
//               // If the joystick is centered, clear the flags.
//               if (details.x.abs() <= 0.5) {
//                 _isLeftPressed = false;
//                 _isRightPressed = false;
//               }
//             });
//           },
//         ),
//       ),
//     );
//   }
//   // --- NEW, NON-BLOCKING WIDGET to display connection status ---
//   Widget _buildConnectionStatusBanner() {
//     if (_isServerConnected) {
//       return const SizedBox.shrink();
//     }
//
//     return Positioned(
//       top: 0,
//       left: 0,
//       right: 0,
//       child: AnimatedSwitcher(
//         duration: const Duration(milliseconds: 300),
//         child: _isServerConnected
//             ? Container( // Connected State
//           key: const ValueKey('connected'),
//           padding: const EdgeInsets.all(8.0),
//           color: Colors.green.withOpacity(0.8),
//           child: const Row(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               Icon(Icons.check_circle, color: Colors.white, size: 16),
//               SizedBox(width: 8),
//               Text(
//                 'Connected',
//                 style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
//               ),
//             ],
//           ),
//         )
//             : Container( // Disconnected State
//           key: const ValueKey('disconnected'),
//           padding: const EdgeInsets.all(8.0),
//           color: Colors.red.withOpacity(0.8),
//           child: Row(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               const Icon(Icons.error, color: Colors.white, size: 16),
//               const SizedBox(width: 8),
//               Text(
//                 'Connection Lost - Attempting to reconnect, please check server',
//                 style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
//               ),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       backgroundColor: Colors.black,
//       body: _isLoading
//           ? const Center(child: CircularProgressIndicator())
//           : Stack(
//         children: [
//           // Positioned.fill(
//           //   child: KeyedSubtree(
//           //     key: _gstreamerViewKey,
//           //     child: AndroidView(
//           //       viewType: 'gstreamer_view',
//           //       onPlatformViewCreated: _onGStreamerPlatformViewCreated,
//           //     ),
//           //   ),
//           // ),
//           Positioned.fill(
//             child: InteractiveViewer(
//               transformationController: _transformationController,
//               minScale: 1.0, // User cannot pinch-to-zoom out
//               maxScale: 5.0, // User can pinch-to-zoom in up to 5x
//               panEnabled: false, // Disable panning with finger, only joysticks control it
//               child: KeyedSubtree(
//                 key: _gstreamerViewKey,
//                 child: AndroidView(
//                   viewType: 'gstreamer_view',
//                   onPlatformViewCreated: _onGStreamerPlatformViewCreated,
//                 ),
//               ),
//             ),
//           ),
//
//           if (!_isGStreamerLoading && !_gstreamerHasError)
//             Positioned.fill(child: _buildTouchDetector()),
//
//           Positioned.fill(child: _buildStreamOverlay()),
//
//           _buildWindIndicator(),
//           _buildZoomDisplay(),
//
//           _buildConnectionStatusBanner(),
//
//           _buildModeButton(0, 30, 30, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE),
//           _buildModeButton(1, 30, 214, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE),
//           _buildModeButton(2, 30, 398, "Manual Attack", ICON_PATH_MANUAL_ATTACK_INACTIVE, ICON_PATH_MANUAL_ATTACK_ACTIVE),
//           _buildModeButton(3, 30, 582, "Auto Attack", ICON_PATH_AUTO_ATTACK_INACTIVE, ICON_PATH_AUTO_ATTACK_ACTIVE),
//           _buildModeButton(4, 30, 766, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE),
//
//           _buildViewButton(
//             1690, 30, "Day View",
//             ICON_PATH_DAY_VIEW_ACTIVE,   // Pass the ACTIVE icon
//             ICON_PATH_DAY_VIEW_INACTIVE, // Pass the INACTIVE icon
//             _currentCameraIndex == 0,    // This button is active if camera index is 0
//             onPressed: () => _switchCamera(0),
//           ),
//
//           // Night View Button (IR Camera)
//           _buildViewButton(
//             1690, 214, "Night View",
//             ICON_PATH_NIGHT_VIEW_ACTIVE,   // Pass the ACTIVE icon
//             ICON_PATH_NIGHT_VIEW_INACTIVE, // Pass the INACTIVE icon
//             _currentCameraIndex == 1,      // This button is active if camera index is 1
//             onPressed: () => _switchCamera(1),
//           ),
//
//           _buildViewButton(
//               1690, 720, "Setting",
//               ICON_PATH_SETTINGS, // Active icon
//               ICON_PATH_SETTINGS, // Inactive icon (the same)
//               false,              // Never shows the "active" red background
//               onPressed: _navigateToSettings
//           ),
//
//           // _buildDirectionalControls(),
//           _buildMovementJoystick(),
//           _buildPanTiltJoystick(),
//
//           Align(
//             alignment: Alignment.bottomCenter,
//             child: Container(
//               margin: const EdgeInsets.only(bottom: 20, left: 20, right: 20),
//               decoration: BoxDecoration(color: Colors.black.withOpacity(0.6), borderRadius: BorderRadius.circular(50)),
//               child: _buildBottomBar(),
//             ),
//           ),
//         ],
//       ),
//     );
//   }
//
//   Widget _buildModeButton(int index, double left, double top, String label, String inactiveIcon, String activeIcon) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     final bool isSelected = _selectedModeIndex == index;
//     final Color color = isSelected ? (_buttonActiveColor[index] ?? Colors.grey) : Colors.black.withOpacity(0.6);
//     final String icon = isSelected ? activeIcon : inactiveIcon;
//
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: Opacity(
//         opacity: _isServerConnected ? 1.0 : 0.5,
//         child: GestureDetector(
//           // onTap: () => _onModeSelected(index),
//           onTap: _isServerConnected ? () => _onModeSelected(index) : null,
//           child: Container(
//             width: 200 * widthScale,
//             height: 120 * heightScale,
//             padding: EdgeInsets.symmetric(vertical: 4.0 * heightScale),
//             decoration: BoxDecoration(
//               color: color,
//               borderRadius: BorderRadius.circular(9),
//               border: Border.all(color: isSelected ? Colors.white : Colors.transparent, width: 2.0),
//             ),
//             child: Column(
//               mainAxisAlignment: MainAxisAlignment.center,
//               children: [
//                 Expanded(flex: 2, child: Image.asset(icon, fit: BoxFit.contain)),
//                 SizedBox(height: 5 * heightScale),
//                 Text(label, textAlign: TextAlign.center, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.bold, fontSize: 25 * heightScale, color: Colors.white)),
//               ],
//             ),
//           ),
//         ),
//       ),
//     );
//   }
//
//   // Widget _buildViewButton(double left, double top, String label, String iconPath, bool isActive, {VoidCallback? onPressed}) {
//   //   final screenWidth = MediaQuery.of(context).size.width;
//   //   final screenHeight = MediaQuery.of(context).size.height;
//   //   final widthScale = screenWidth / 1920.0;
//   //   final heightScale = screenHeight / 1080.0;
//   //
//   //   final Color color = isActive ? const Color(0xFFc32121) : Colors.black.withOpacity(0.6);
//   //
//   //   return Positioned(
//   //     left: left * widthScale,
//   //     top: top * heightScale,
//   //     child: GestureDetector(
//   //       onTap: onPressed,
//   //       child: Container(
//   //         width: 220 * widthScale,
//   //         height: 175 * heightScale,
//   //         padding: EdgeInsets.symmetric(vertical: 10.0 * heightScale),
//   //         decoration: BoxDecoration(
//   //           color: color,
//   //           borderRadius: BorderRadius.circular(15),
//   //           border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.5),
//   //         ),
//   //         child: Column(
//   //           mainAxisAlignment: MainAxisAlignment.center,
//   //           children: [
//   //             Expanded(flex: 3, child: Image.asset(iconPath, fit: BoxFit.contain)),
//   //             SizedBox(height: 8 * heightScale),
//   //             Text(label, textAlign: TextAlign.center, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 26 * heightScale, color: Colors.white)),
//   //           ],
//   //         ),
//   //       ),
//   //     ),
//   //   );
//   // }
//
//   Widget _buildViewButton(
//       double left,
//       double top,
//       String label,
//       String activeIconPath,   // <-- Changed parameter name for clarity
//       String inactiveIconPath, // <-- NEW parameter
//       bool isActive,
//       {VoidCallback? onPressed}
//       ) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     // --- THIS IS THE FIX ---
//     // The background color is determined by the active state.
//     final Color color = isActive ? Colors.grey : Colors.black.withOpacity(0.6);
//     // The icon path is ALSO determined by the active state.
//     final String iconToDisplay = isActive ? activeIconPath : inactiveIconPath;
//
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTap: onPressed,
//         child: Container(
//           width: 220 * widthScale,
//           height: 175 * heightScale,
//           padding: EdgeInsets.symmetric(vertical: 10.0 * heightScale),
//           decoration: BoxDecoration(
//             color: color,
//             borderRadius: BorderRadius.circular(15),
//             border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.5),
//           ),
//           child: Column(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               Expanded(
//                   flex: 3,
//                   // Use the dynamically selected icon path
//                   child: Image.asset(iconToDisplay, fit: BoxFit.contain)
//               ),
//               SizedBox(height: 8 * heightScale),
//               Text(
//                   label,
//                   textAlign: TextAlign.center,
//                   style: TextStyle(
//                       fontFamily: 'NotoSans',
//                       fontWeight: FontWeight.w700,
//                       fontSize: 26 * heightScale,
//                       color: Colors.white
//                   )
//               ),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   // Widget _buildBottomBar() {
//   //   final screenWidth = MediaQuery.of(context).size.width;
//   //   final widthScale = screenWidth / 1920.0;
//   //
//   //   return LayoutBuilder(
//   //     builder: (context, constraints) {
//   //
//   //       List<Color> permissionButtonColors;
//   //       bool isCombatModeActive = _isModeActive && (_selectedModeIndex == 2 || _selectedModeIndex == 3);
//   //
//   //       if (isCombatModeActive) {
//   //         // If a combat mode is active, the button is either ON or OFF
//   //         permissionButtonColors = _isPermissionToAttackOn ? _permissionOnColors : _permissionOffColors;
//   //       } else {
//   //         // If IDLE or in a non-combat mode, the button is DISABLED
//   //         permissionButtonColors = _permissionDisabledColors;
//   //       }
//   //
//   //       final List<Widget> leftCluster = [
//   //         _buildBottomBarButton(
//   //           "PERMISSION TO ATTACK",
//   //           null,
//   //           _isPermissionToAttackOn ? [const Color(0xffc32121), const Color(0xff831616)] : [const Color(0xFF424242), const Color(0xFF212121)],
//   //           _onPermissionPressed,
//   //         ),
//   //
//   //         SizedBox(width: 12 * widthScale),
//   //         _buildWideBottomBarButton(
//   //           _isModeActive ? "STOP" : "START",
//   //           _isModeActive ? ICON_PATH_STOP : ICON_PATH_START,
//   //           [const Color(0xff25a625), const Color(0xff127812)],
//   //           _onStartStopPressed,
//   //         ),
//   //
//   //         SizedBox(width: 12 * widthScale),
//   //
//   //         _buildBottomBarButton(
//   //             "",
//   //             ICON_PATH_PLUS,
//   //             [const Color(0xffc0c0c0), const Color(0xffa0a0a0)],
//   //                 () {
//   //               // --- NEW ZOOM IN LOGIC ---
//   //               setState(() {
//   //                 // Update the text display variable
//   //                 if (_currentZoomLevel < 5.0) {
//   //                   _currentZoomLevel += 0.1;
//   //                 } else {
//   //                   _currentZoomLevel = 5.0;
//   //                 }
//   //
//   //                 // Programmatically update the InteractiveViewer's scale
//   //                 _transformationController.value = Matrix4.identity()..scale(_currentZoomLevel);
//   //               });
//   //             }
//   //         ),
//   //         SizedBox(width: 12 * widthScale),
//   //         _buildBottomBarButton(
//   //             "",
//   //             ICON_PATH_MINUS,
//   //             [const Color(0xffc0c0c0), const Color(0xffa0a0a0)],
//   //                 () {
//   //               // --- NEW ZOOM OUT LOGIC ---
//   //               setState(() {
//   //                 // Update the text display variable
//   //                 if (_currentZoomLevel > 1.0) {
//   //                   _currentZoomLevel -= 0.1;
//   //                 } else {
//   //                   _currentZoomLevel = 1.0;
//   //                 }
//   //
//   //                 // Programmatically update the InteractiveViewer's scale
//   //                 _transformationController.value = Matrix4.identity()..scale(_currentZoomLevel);
//   //               });
//   //             }
//   //         ),
//   //       ];
//   //
//   //       final List<Widget> middleCluster = [
//   //         Row(
//   //           mainAxisSize: MainAxisSize.min,
//   //           crossAxisAlignment: CrossAxisAlignment.baseline,
//   //           textBaseline: TextBaseline.alphabetic,
//   //           children: [
//   //             const Text("0", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 75, color: Colors.white)),
//   //             SizedBox(width: 8 * widthScale),
//   //             const Text("Km/h", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 37, color: Colors.white)),
//   //           ],
//   //         ),
//   //         SizedBox(width: 20 * widthScale),
//   //         Image.asset(ICON_PATH_WIFI, height: 40),
//   //       ];
//   //
//   //       final List<Widget> rightCluster = [
//   //         _buildBottomBarButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)], () async {
//   //           final proceed = await _showExitDialog();
//   //           if (proceed) SystemNavigator.pop();
//   //         }),
//   //       ];
//   //
//   //       return Row(
//   //         children: [
//   //           ...leftCluster,
//   //           const Spacer(),
//   //           ...middleCluster,
//   //           const Spacer(),
//   //           ...rightCluster,
//   //         ],
//   //       );
//   //     },
//   //   );
//   // }
//
//   Widget _buildBottomBar() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     return LayoutBuilder(
//       builder: (context, constraints) {
//         List<Color> permissionButtonColors;
//         bool isCombatModeActive = _isModeActive && (_selectedModeIndex == 2 || _selectedModeIndex == 3);
//
//         if (isCombatModeActive) {
//           permissionButtonColors = _isPermissionToAttackOn ? _permissionOnColors : _permissionOffColors;
//         } else {
//           permissionButtonColors = _permissionDisabledColors;
//         }
//
//         final List<Widget> leftCluster = [
//           _buildBottomBarButton(
//             "PERMISSION TO ATTACK",
//             null,
//             // permissionButtonColors,
//             _isPermissionToAttackOn ? [const Color(0xffc32121), const Color(0xff831616)] : [const Color(0xFF424242), const Color(0xFF212121)],
//             _isServerConnected ? _onPermissionPressed : null,
//           ),
//           SizedBox(width: 12 * widthScale),
//           _buildWideBottomBarButton(
//             _isModeActive ? "STOP" : "START",
//             _isModeActive ? ICON_PATH_STOP : ICON_PATH_START,
//             [const Color(0xff25a625), const Color(0xff127812)],
//             _isServerConnected ? _onStartStopPressed : null,
//           ),
//           SizedBox(width: 12 * widthScale),
//           _buildBottomBarButton(
//             "",
//             ICON_PATH_PLUS,
//             [const Color(0xffc0c0c0), const Color(0xffa0a0a0)],
//             _isServerConnected ? () {
//               setState(() {
//                 if (_currentZoomLevel < 5.0) _currentZoomLevel += 0.1;
//                 else _currentZoomLevel = 5.0;
//                 _transformationController.value = Matrix4.identity()..scale(_currentZoomLevel);
//               });
//             } : null,
//           ),
//           SizedBox(width: 12 * widthScale),
//           _buildBottomBarButton(
//             "",
//             ICON_PATH_MINUS,
//             [const Color(0xffc0c0c0), const Color(0xffa0a0a0)],
//             _isServerConnected ? () {
//               setState(() {
//                 if (_currentZoomLevel > 1.0) _currentZoomLevel -= 0.1;
//                 else _currentZoomLevel = 1.0;
//                 _transformationController.value = Matrix4.identity()..scale(_currentZoomLevel);
//               });
//             } : null,
//           ),
//         ];
//
//         final List<Widget> middleCluster = [
//           Row(
//             mainAxisSize: MainAxisSize.min,
//             crossAxisAlignment: CrossAxisAlignment.baseline,
//             textBaseline: TextBaseline.alphabetic,
//             children: [
//               const Text("0", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 75, color: Colors.white)),
//               SizedBox(width: 8 * widthScale),
//               const Text("Km/h", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 37, color: Colors.white)),
//             ],
//           ),
//           SizedBox(width: 20 * widthScale),
//           Image.asset(ICON_PATH_WIFI, height: 40),
//         ];
//
//         final List<Widget> rightCluster = [
//           _buildBottomBarButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)], () async {
//             final proceed = await _showExitDialog();
//             if (proceed) SystemNavigator.pop();
//           }),
//         ];
//
//         return Row(
//           children: [
//             ...leftCluster,
//             const Spacer(),
//             ...middleCluster,
//             const Spacer(),
//             ...rightCluster,
//           ],
//         );
//       },
//     );
//   }
//
//   // Widget _buildWideBottomBarButton(String label, String iconPath, List<Color> gradientColors, VoidCallback onPressed) {
//   //   final screenHeight = MediaQuery.of(context).size.height;
//   //   final screenWidth = MediaQuery.of(context).size.width;
//   //   final heightScale = screenHeight / 1080.0;
//   //   final widthScale = screenWidth / 1920.0;
//   //
//   //   return GestureDetector(
//   //     onTap: onPressed,
//   //     child: Container(
//   //       constraints: BoxConstraints(minWidth: 220 * widthScale),
//   //       height: 80 * heightScale,
//   //       padding: const EdgeInsets.symmetric(horizontal: 74),
//   //       decoration: BoxDecoration(
//   //         gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//   //         borderRadius: BorderRadius.circular(25 * heightScale),
//   //       ),
//   //       child: Row(
//   //         mainAxisAlignment: MainAxisAlignment.center,
//   //         children: [
//   //           Image.asset(iconPath, height: 36 * heightScale),
//   //           const SizedBox(width: 12),
//   //           Text(label, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 36 * heightScale, color: Colors.white)),
//   //         ],
//   //       ),
//   //     ),
//   //   );
//   // }
//
//
//
//   // Widget _buildBottomBarButton(String label, String? iconPath, List<Color> gradientColors, VoidCallback onPressed) {
//   //   final screenHeight = MediaQuery.of(context).size.height;
//   //   final heightScale = screenHeight / 1080.0;
//   //
//   //   return GestureDetector(
//   //     onTap: onPressed,
//   //     child: Container(
//   //       height: 80 * heightScale,
//   //       padding: const EdgeInsets.symmetric(horizontal: 30),
//   //       decoration: BoxDecoration(
//   //         gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//   //         borderRadius: BorderRadius.circular(25 * heightScale),
//   //       ),
//   //       child: Row(
//   //         mainAxisAlignment: MainAxisAlignment.center,
//   //         children: [
//   //           if (iconPath != null && iconPath.isNotEmpty) ...[
//   //             Image.asset(iconPath, height: 36 * heightScale),
//   //             if (label.isNotEmpty) const SizedBox(width: 12),
//   //           ],
//   //           if (label.isNotEmpty)
//   //             Text(label, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 30 * heightScale, color: Colors.white)),
//   //         ],
//   //       ),
//   //     ),
//   //   );
//   // }
//
//   // Make the onPressed parameter nullable by adding '?'
//   Widget _buildWideBottomBarButton(String label, String iconPath, List<Color> gradientColors, VoidCallback? onPressed) {
//     final screenHeight = MediaQuery.of(context).size.height;
//     final screenWidth = MediaQuery.of(context).size.width;
//     final heightScale = screenHeight / 1080.0;
//     final widthScale = screenWidth / 1920.0;
//     final bool isEnabled = onPressed != null;
//
//     return GestureDetector(
//       onTap: onPressed,
//       child: Opacity(
//         opacity: isEnabled ? 1.0 : 0.5,
//         child: Container(
//           constraints: BoxConstraints(minWidth: 220 * widthScale),
//           height: 80 * heightScale,
//           padding: const EdgeInsets.symmetric(horizontal: 74),
//           decoration: BoxDecoration(
//             gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//             borderRadius: BorderRadius.circular(25 * heightScale),
//           ),
//           child: Row(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               Image.asset(iconPath, height: 36 * heightScale),
//               const SizedBox(width: 12),
//               Text(label, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 36 * heightScale, color: Colors.white)),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   // Make the onPressed parameter nullable by adding '?'
//   Widget _buildBottomBarButton(String label, String? iconPath, List<Color> gradientColors, VoidCallback? onPressed) {
//     final screenHeight = MediaQuery.of(context).size.height;
//     final heightScale = screenHeight / 1080.0;
//     final bool isEnabled = onPressed != null;
//
//     return GestureDetector(
//       onTap: onPressed,
//       child: Opacity(
//         opacity: isEnabled ? 1.0 : 0.5,
//         child: Container(
//           height: 80 * heightScale,
//           padding: const EdgeInsets.symmetric(horizontal: 30),
//           decoration: BoxDecoration(
//             gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//             borderRadius: BorderRadius.circular(25 * heightScale),
//           ),
//           child: Row(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               if (iconPath != null && iconPath.isNotEmpty) ...[
//                 Image.asset(iconPath, height: 36 * heightScale),
//                 if (label.isNotEmpty) const SizedBox(width: 12),
//               ],
//               if (label.isNotEmpty)
//                 Text(label, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 30 * heightScale, color: Colors.white)),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   Widget _buildDirectionalControls() {
//     return Stack(
//       children: [
//         _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE, () => setState(() => _isForwardPressed = true), () => setState(() => _isForwardPressed = false)),
//         _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE, () => setState(() => _isBackPressed = true), () => setState(() => _isBackPressed = false)),
//         _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE, () => setState(() => _isLeftPressed = true), () => setState(() => _isLeftPressed = false)),
//         _buildDirectionalButton(1540, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE, () => setState(() => _isRightPressed = true), () => setState(() => _isRightPressed = false)),
//       ],
//     );
//   }
//
//   Widget _buildDirectionalButton(double left, double top, bool isPressed, String inactiveIcon, String activeIcon, VoidCallback onPress, VoidCallback onRelease) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTapDown: (_) => onPress(),
//         onTapUp: (_) => onRelease(),
//         onTapCancel: () => onRelease(),
//         child: Image.asset(isPressed ? activeIcon : inactiveIcon, height: 100 * heightScale, width: 100 * widthScale, fit: BoxFit.contain),
//       ),
//     );
//   }
//
//   Widget _buildTouchDetector() {
//     return GestureDetector(
//       key: const GlobalObjectKey('_playerKey'),
//       behavior: HitTestBehavior.opaque,
//       onTapUp: (details) {
//         if (_isModeActive && _selectedModeIndex == 2) { // 2 corresponds to Manual Attack
//           final RenderBox? renderBox = context.findRenderObject() as RenderBox?;
//           if (renderBox != null) {
//             final Offset localPosition = renderBox.globalToLocal(details.globalPosition);
//             final double x = localPosition.dx / renderBox.size.width;
//             final double y = localPosition.dy / renderBox.size.height;
//             if (x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0) {
//               _sendTouchPacket(TouchCoord()..x = x..y = y);
//             }
//           }
//         }
//       },
//       child: Container(color: Colors.transparent),
//     );
//   }
//
//   Future<bool> _showExitDialog() async {
//     return await showDialog<bool>(
//       context: context,
//       builder: (context) => AlertDialog(
//         title: const Text('Confirm Exit'),
//         content: const Text('Do you want to close the application?'),
//         actions: [
//           TextButton(onPressed: () => Navigator.of(context).pop(false), child: const Text('Cancel')),
//           TextButton(onPressed: () => Navigator.of(context).pop(true), child: const Text('Exit')),
//         ],
//       ),
//     ) ?? false;
//   }
//
//   Future<void> _handleGStreamerMessages(MethodCall call) async {
//     if (!mounted) return;
//     _streamTimeoutTimer?.cancel();
//     switch (call.method) {
//       case 'onStreamReady':
//         setState(() { _isGStreamerLoading = false; _gstreamerHasError = false; });
//         break;
//       case 'onStreamError':
//         final String? error = call.arguments?['error'];
//         setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "Stream error: ${"" ?? 'Unknown native error'}"; });
//         break;
//     }
//   }
//
//   void _retryStream() {
//     if (_currentCameraIndex == -1) {
//       print("Retry failed: No camera index selected.");
//       return;
//     }
//     print("Retrying stream for camera index: $_currentCameraIndex");
//
//     setState(() {
//       _gstreamerViewKey = UniqueKey(); // This is the MOST IMPORTANT line. It forces the widget to be destroyed and recreated.
//       _isGStreamerReady = false;
//       _isGStreamerLoading = true; // Show the loading spinner again
//       _gstreamerHasError = false; // Clear the error state
//       _errorMessage = null;
//     });
//   }
//
//   // Future<void> _loadSettingsAndInitialize() async {
//   //   _robotIpAddress = await _settingsService.loadIpAddress();
//   //   _cameraUrls = await _settingsService.loadCameraUrls();
//   //   if (mounted) {
//   //     _switchCamera(0);
//   //     _startCommandTimer();
//   //     setState(() => _isLoading = false);
//   //   }
//   // }
//
//   Future<void> _loadSettingsAndInitialize() async {
//     _robotIpAddress = await _settingsService.loadIpAddress();
//     _cameraUrls = await _settingsService.loadCameraUrls();
//     if (mounted) {
//       _connectToStatusServer(); // <-- ADD THIS CALL
//       _switchCamera(0);
//       _startCommandTimer();
//       setState(() => _isLoading = false);
//     }
//   }
//
//   Future<void> _sendCommandPacket(UserCommand command) async {
//     if (_robotIpAddress.isEmpty) return;
//     try {
//       final socket = await Socket.connect(_robotIpAddress, 65432, timeout: const Duration(milliseconds: 150));
//       socket.add(command.toBytes());
//       await socket.flush();
//       socket.close();
//     } catch (e) {}
//   }
//
//   Future<void> _sendTouchPacket(TouchCoord coord) async {
//     if (_robotIpAddress.isEmpty) return;
//     try {
//       const int TOUCH_PORT = 65433;
//       final socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
//       socket.send(coord.toBytes(), InternetAddress(_robotIpAddress), TOUCH_PORT);
//       socket.close();
//       print('Sent Touch Packet (UDP): X=${coord.x}, Y=${coord.y}');
//     } catch (e) {
//       print('Error sending touch UDP packet: $e');
//     }
//   }
//
//   void _stopGStreamer() {
//     if (_gstreamerChannel != null && _isGStreamerReady) {
//       _gstreamerChannel!.invokeMethod('stopStream').catchError((e) => print("Error stopping stream: $e"));
//     }
//     _isGStreamerReady = false;
//   }
//
//   Future<void> _switchCamera(int index) async {
//     if (_cameraUrls.isEmpty || index < 0 || index >= _cameraUrls.length) return;
//     if (index == _currentCameraIndex && !_gstreamerHasError) return;
//     if (_gstreamerChannel != null && _isGStreamerReady) {
//       try { await _gstreamerChannel!.invokeMethod('stopStream'); } catch (e) {}
//     }
//     setState(() {
//       _currentCameraIndex = index;
//       _gstreamerViewKey = UniqueKey();
//       _isGStreamerReady = false;
//       _isGStreamerLoading = true;
//       _gstreamerHasError = false;
//       _errorMessage = null;
//     });
//   }
//
//
//   void _onGStreamerPlatformViewCreated(int id) {
//     _gstreamerChannel = MethodChannel('gstreamer_channel_$id');
//     _gstreamerChannel!.setMethodCallHandler(_handleGStreamerMessages);
//     setState(() {
//       _isGStreamerReady = true;
//       _isGStreamerLoading = false;
//     });
//     _playCurrentCameraStream();
//   }
//
//   Future<void> _playCurrentCameraStream() async {
//     _streamTimeoutTimer?.cancel();
//     if (_cameraUrls.isEmpty || _currentCameraIndex < 0 || _currentCameraIndex >= _cameraUrls.length) {
//       if (mounted) setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "No valid camera URL."; });
//       return;
//     }
//     if (_gstreamerChannel == null || !_isGStreamerReady) {
//       if (mounted) setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "GStreamer channel not ready."; });
//       return;
//     }
//     setState(() { _isGStreamerLoading = true; _gstreamerHasError = false; _errorMessage = null; });
//     _streamTimeoutTimer = Timer(const Duration(seconds: 8), () {
//       if (mounted && _isGStreamerLoading) {
//         setState(() { _gstreamerHasError = true; _errorMessage = "Connection timed out."; _isGStreamerLoading = false; });
//       }
//     });
//     try {
//       final String url = _cameraUrls[_currentCameraIndex];
//       await _gstreamerChannel!.invokeMethod('startStream', {'url': url});
//     } catch (e) {
//       _streamTimeoutTimer?.cancel();
//       if (mounted) {
//         setState(() { _gstreamerHasError = true; _errorMessage = "Failed to start stream: ${e.toString()}"; _isGStreamerLoading = false; });
//       }
//     }
//   }
//
//   Widget _buildStreamOverlay() {
//     if (_isGStreamerLoading) {
//       return Container(
//         color: Colors.black.withOpacity(0.5),
//         child: const Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [CircularProgressIndicator(color: Colors.white), SizedBox(height: 20), Text('Connecting to stream...', style: TextStyle(color: Colors.white, fontSize: 18))])),
//       );
//     }
//     if (_gstreamerHasError) {
//       return Container(
//         color: Colors.black.withOpacity(0.7),
//         child: Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
//           const Icon(Icons.error_outline, color: Colors.redAccent, size: 70),
//           const SizedBox(height: 20),
//           Padding(padding: const EdgeInsets.symmetric(horizontal: 40.0), child: Text(_errorMessage ?? 'Stream failed to load.', textAlign: TextAlign.center, style: const TextStyle(color: Colors.white, fontSize: 20))),
//           const SizedBox(height: 30),
//           ElevatedButton.icon(
//             icon: const Icon(Icons.refresh),
//             label: const Text('Retry'),
//             onPressed: () { if (_currentCameraIndex != -1) _switchCamera(_currentCameraIndex); },
//             style: ElevatedButton.styleFrom(foregroundColor: Colors.white, backgroundColor: Colors.blueGrey, padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 15), textStyle: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
//           ),
//         ])),
//       );
//     }
//     return const SizedBox.shrink();
//   }
//
//   Future<void> _navigateToSettings() async {
//     final bool? settingsChanged = await Navigator.push<bool>(context, MaterialPageRoute(builder: (context) => const SettingsMenuPage()));
//     if (settingsChanged == true && mounted) {
//       _commandTimer?.cancel();
//       await _loadSettingsAndInitialize();
//     }
//   }
// }



//09_23
import 'dart:async';
// import 'dart:io';
// import 'package:flutter/material.dart';
// import 'package:flutter/services.dart';
// import 'package:rtest1/settings_service.dart';
// import 'CommandIds.dart';
// import 'ServerStatus.dart';
// import 'StatusPacket.dart';
// import 'TouchCoord.dart';
// import 'UserCommand.dart';
// import 'icon_constants.dart';
// import 'splash_screen.dart';
// import 'settings_menu_page.dart';
// import 'package:flutter_joystick/flutter_joystick.dart';
// import 'DrivingCommand.dart';
//
// void main() async {
//   WidgetsFlutterBinding.ensureInitialized();
//   SystemChrome.setEnabledSystemUIMode(SystemUiMode.manual, overlays: []);
//   SystemChrome.setPreferredOrientations([
//     DeviceOrientation.landscapeLeft,
//     DeviceOrientation.landscapeRight,
//   ]);
//   runApp(const MyApp());
// }
//
// class MyApp extends StatelessWidget {
//   const MyApp({super.key});
//
//   @override
//   Widget build(BuildContext context) {
//     return MaterialApp(
//       debugShowCheckedModeBanner: false,
//       title: '',
//       theme: ThemeData(primarySwatch: Colors.blue),
//       home: const SplashScreen(),
//     );
//   }
// }
//
// // --- HOME PAGE WIDGET ---
// class HomePage extends StatefulWidget {
//   const HomePage({super.key});
//
//   @override
//   State<HomePage> createState() => _HomePageState();
// }
//
//
// class _HomePageState extends State<HomePage> {
//   // --- STATE MANAGEMENT ---
//   UserCommand _currentCommand = UserCommand();
//   bool _isLoading = true;
//   Timer? _commandTimer;
//
//   // State variables for the UI logic
//   int _selectedModeIndex = -1;
//   bool _isModeActive = false;
//   bool _isPermissionToAttackOn = false;
//
//   // On-screen button press states
//   bool _isForwardPressed = false;
//   bool _isBackPressed = false;
//   bool _isLeftPressed = false;
//   bool _isRightPressed = false;
//
//   // Gamepad State
//   static const platform = MethodChannel('com.yourcompany/gamepad');
//   bool _gamepadConnected = false;
//   Map<String, double> _gamepadAxisValues = {};
//   bool _isZoomInPressed = false;
//   bool _isZoomOutPressed = false;
//
//   // GStreamer State
//   String? _errorMessage;
//   final SettingsService _settingsService = SettingsService();
//   String _robotIpAddress = '';
//   List<String> _cameraUrls = [];
//   int _currentCameraIndex = 0;
//   Key _gstreamerViewKey = UniqueKey();
//   bool _isGStreamerReady = false;
//   bool _isGStreamerLoading = true;
//   bool _gstreamerHasError = false;
//   MethodChannel? _gstreamerChannel;
//   Timer? _streamTimeoutTimer;
//
//   double _currentZoomLevel = 1.0;
//   double _lateralWindSpeed = 0.0;
//   int _windDirectionIndex = 0;
//   double _pendingLateralWindSpeed = 0.0;
//
//   final TransformationController _transformationController = TransformationController();
//
//   Socket? _statusSocket;
//   StreamSubscription? _statusSocketSubscription;
//   bool _isServerConnected = false;
//
//   ServerStatus _serverStatus = ServerStatus.disconnected();
//
//   bool _isUiZoomInPressed = false;
//   bool _isUiZoomOutPressed = false;
//   int _confirmedServerModeId = CommandIds.IDLE;
//
//
//   final Map<int, int> _buttonIndexToCommandId = {
//     0: CommandIds.DRIVING,
//     1: CommandIds.RECON,
//     2: CommandIds.MANUAL_ATTACK,
//     3: CommandIds.AUTO_ATTACK,
//     4: CommandIds.DRONE,
//   };
//
//   final Map<int, Color> _buttonActiveColor = {
//     0: const Color(0xff25a625), // Green for Driving
//     1: const Color(0xff25a625), // CHANGED: Green for Recon
//     2: const Color(0xffc32121), // Red for Manual Attack
//     3: const Color(0xffc32121), // Red for Auto Attack
//     4: const Color(0xff25a625), // CHANGED: Green for Drone
//   };
//
//   final List<Color> _permissionDisabledColors = [const Color(0xFF424242), const Color(0xFF212121)]; // Black/Grey
//   final List<Color> _permissionOffColors = [const Color(0xffc32121), const Color(0xff831616)];      // Red
//   final List<Color> _permissionOnColors = [const Color(0xff6b0000), const Color(0xff520000)];
//
//   @override
//   void initState() {
//     super.initState();
//     _loadSettingsAndInitialize();
//     platform.setMethodCallHandler(_handleGamepadEvent);
//   }
//
//   // NEED THIS for wind icon changes
//   // @override
//   // void initState() {
//   //   super.initState();
//   //   _loadSettingsAndInitialize();
//   //   platform.setMethodCallHandler(_handleGamepadEvent);
//   //
//   //   // --- TEMPORARY: Simulate receiving wind data (speed AND direction) ---
//   //   Timer.periodic(const Duration(seconds: 2), (timer) {
//   //     if (mounted) {
//   //       setState(() {
//   //         // Generate a random wind speed between -5.0 and 5.0
//   //         _lateralWindSpeed = (DateTime.now().second % 100) / 10.0 - 5.0;
//   //
//   //         // Cycle through the 8 wind directions every 16 seconds (2 seconds per direction)
//   //         _windDirectionIndex = (DateTime.now().second ~/ 2) % 8;
//   //       });
//   //     } else {
//   //       timer.cancel();
//   //     }
//   //   });
//   // }
//
//   @override
//   void dispose() {
//     _commandTimer?.cancel();
//     _stopGStreamer();
//     _transformationController.dispose();
//     _statusSocketSubscription?.cancel();
//     _statusSocket?.destroy();
//     super.dispose();
//   }
//
//
//   Future<void> _connectToStatusServer() async {
//     if (_robotIpAddress.isEmpty) return;
//
//     // Disconnect if already connected
//     await _statusSocketSubscription?.cancel();
//     _statusSocket?.destroy();
//
//     try {
//       const int STATUS_PORT = 65435;
//       _statusSocket = await Socket.connect(_robotIpAddress, STATUS_PORT, timeout: const Duration(seconds: 5));
//       setState(() {
//         _isServerConnected = true;
//       });
//       print("Connected to status server!");
//
//       _statusSocketSubscription = _statusSocket!.listen(
//             (Uint8List data) {
//           try {
//             final status = StatusPacket.fromBytes(data);
//             // Update the UI with the new data from the server
//             if (mounted) {
//               setState(() {
//                 _lateralWindSpeed = status.lateralWindSpeed;
//                 _windDirectionIndex = status.windDirectionIndex;
//
//                 _confirmedServerModeId = status.currentModeId;
//               });
//             }
//           } catch (e) {
//             print("Error parsing status packet: $e");
//           }
//         },
//         onError: (error) {
//           print("Status socket error: $error");
//           setState(() => _isServerConnected = false);
//           _reconnectStatusServer();
//         },
//         onDone: () {
//           print("Status server disconnected.");
//           setState(() => _isServerConnected = false);
//           _reconnectStatusServer();
//         },
//         cancelOnError: true,
//       );
//     } catch (e) {
//       print("Failed to connect to status server: $e");
//       setState(() => _isServerConnected = false);
//       _reconnectStatusServer();
//     }
//   }
//
//   void _reconnectStatusServer() {
//     // Simple reconnect logic: try again after a delay
//     Future.delayed(const Duration(seconds: 5), () {
//       if (mounted && !_isServerConnected) {
//         print("Attempting to reconnect to status server...");
//         _connectToStatusServer();
//       }
//     });
//   }
//
//   // --- LOGIC METHODS ---
//
//   void _onModeSelected(int index) {
//     setState(() {
//       // CHANGED: If a mode is currently active, do nothing, as per client request.
//       if (_isModeActive) {
//         ScaffoldMessenger.of(context).showSnackBar(
//           const SnackBar(
//             content: Text('Please press STOP before selecting a new mode.'),
//             duration: Duration(seconds: 2),
//           ),
//         );
//         return;
//       }
//       _selectedModeIndex = index;
//     });
//   }
//
//   void _onStartStopPressed() {
//     setState(() {
//       if (_isModeActive) {
//         _stopCurrentMode();
//       } else {
//         if (_selectedModeIndex != -1) {
//           _isModeActive = true;
//           _currentCommand.command_id = _buttonIndexToCommandId[_selectedModeIndex] ?? CommandIds.IDLE;
//         }
//       }
//     });
//   }
//
//   // void _stopCurrentMode() {
//   //   _isModeActive = false;
//   //   _selectedModeIndex = -1;
//   //   _currentCommand.command_id = CommandIds.IDLE;
//   //   _isForwardPressed = false;
//   //   _isBackPressed = false;
//   //   _isLeftPressed = false;
//   //   _isRightPressed = false;
//   // }
//
//   void _stopCurrentMode() {
//     _isModeActive = false;
//     _selectedModeIndex = -1;
//     _currentCommand.command_id = CommandIds.IDLE;
//     _isForwardPressed = false;
//     _isBackPressed = false;
//     _isLeftPressed = false;
//     _isRightPressed = false;
//
//     // --- THIS IS THE FIX ---
//     // Automatically reset the attack permission when stopping any mode.
//     _isPermissionToAttackOn = false;
//     _currentCommand.attack_permission = false;
//   }
//
//   void _onPermissionPressed() {
//     setState(() {
//       _isPermissionToAttackOn = !_isPermissionToAttackOn;
//       _currentCommand.attack_permission = _isPermissionToAttackOn;
//     });
//   }
//
//   // Future<void> _handleGamepadEvent(MethodCall call) async {
//   //   if (!mounted) return;
//   //   if (!_gamepadConnected) setState(() => _gamepadConnected = true);
//   //
//   //   if (call.method == "onMotionEvent") {
//   //     setState(() => _gamepadAxisValues = Map<String, double>.from(call.arguments));
//   //   } else if (call.method == "onButtonDown") {
//   //     final String button = call.arguments['button'];
//   //     setState(() {
//   //       switch (button) {
//   //         case 'KEYCODE_BUTTON_B': // Start
//   //           _onStartStopPressed();
//   //           break;
//   //         case 'KEYCODE_BUTTON_A': // Stop
//   //           if (_isModeActive) _onStartStopPressed();
//   //           break;
//   //         case 'KEYCODE_BUTTON_X': // Permission
//   //           _onPermissionPressed();
//   //           break;
//   //         case 'KEYCODE_BUTTON_L1':
//   //           _isZoomInPressed = true;
//   //           break;
//   //         case 'KEYCODE_BUTTON_L2':
//   //           _isZoomOutPressed = true;
//   //           break;
//   //       }
//   //     });
//   //   } else if (call.method == "onButtonUp") {
//   //     final String button = call.arguments['button'];
//   //     setState(() {
//   //       switch (button) {
//   //         case 'KEYCODE_BUTTON_L1':
//   //           _isZoomInPressed = false;
//   //           break;
//   //         case 'KEYCODE_BUTTON_L2':
//   //           _isZoomOutPressed = false;
//   //           break;
//   //       }
//   //     });
//   //   }
//   // }
//
//
//   Future<void> _handleGamepadEvent(MethodCall call) async {
//     if (!mounted) return;
//     if (!_gamepadConnected) {
//       setState(() {
//         _gamepadConnected = true;
//         _pendingLateralWindSpeed = _lateralWindSpeed; // Initialize pending value
//       });
//     }
//
//     if (call.method == "onMotionEvent") {
//       final newAxisValues = Map<String, double>.from(call.arguments);
//
//       // ---: Handle D-Pad for Wind Speed Adjustment ---
//       final double hatX = newAxisValues['AXIS_HAT_X'] ?? 0.0;
//       final double prevHatX = _gamepadAxisValues['AXIS_HAT_X'] ?? 0.0;
//
//       // Detect a press (transition from 0 to -1 or 1)
//       if (hatX != 0 && prevHatX == 0) {
//         setState(() {
//           if (hatX > 0.5) { // D-Pad Right
//             _pendingLateralWindSpeed += 0.1;
//           } else if (hatX < -0.5) { // D-Pad Left
//             _pendingLateralWindSpeed -= 0.1;
//           }
//         });
//       }
//
//       setState(() => _gamepadAxisValues = newAxisValues);
//
//     } else if (call.method == "onButtonDown") {
//       final String button = call.arguments['button'];
//       setState(() {
//         switch (button) {
//           case 'KEYCODE_BUTTON_B': // Start
//             _onStartStopPressed();
//             break;
//           case 'KEYCODE_BUTTON_A': // Stop
//             if (_isModeActive) _onStartStopPressed();
//             break;
//           case 'KEYCODE_BUTTON_X': // Dual purpose: Permission AND Confirm Wind
//           // --- NEW: Confirm Wind Speed Logic ---
//           // If the pending value is different, this press confirms the wind speed.
//             if ((_pendingLateralWindSpeed - _lateralWindSpeed).abs() > 0.01) {
//               _lateralWindSpeed = _pendingLateralWindSpeed;
//               ScaffoldMessenger.of(context).showSnackBar(
//                 SnackBar(
//                   content: Text('Wind speed set to ${_lateralWindSpeed.toStringAsFixed(1)}'),
//                   duration: const Duration(seconds: 2),
//                 ),
//               );
//             } else {
//               _onPermissionPressed();
//             }
//             break;
//           case 'KEYCODE_BUTTON_L1':
//             _isZoomInPressed = true;
//             break;
//           case 'KEYCODE_BUTTON_L2':
//             _isZoomOutPressed = true;
//             break;
//         }
//       });
//     } else if (call.method == "onButtonUp") {
//       final String button = call.arguments['button'];
//       setState(() {
//         switch (button) {
//           case 'KEYCODE_BUTTON_L1':
//             _isZoomInPressed = false;
//             break;
//           case 'KEYCODE_BUTTON_L2':
//             _isZoomOutPressed = false;
//             break;
//         }
//       });
//     }
//   }
//
//   void _startCommandTimer() {
//     _commandTimer = Timer.periodic(const Duration(milliseconds: 100), (timer) {
//       // Create a new, separate command object for driving
//       DrivingCommand drivingCommand = DrivingCommand();
//
//       // --- THIS IS THE NEW, UNIFIED LOGIC ---
//
//       // Check if the physical gamepad is actively being used.
//       // We consider it "active" if any of its main axes are moved beyond a small deadzone.
//       bool isGamepadActive = _gamepadConnected &&
//           ((_gamepadAxisValues['AXIS_X']?.abs() ?? 0) > 0.1 ||
//               (_gamepadAxisValues['AXIS_Y']?.abs() ?? 0) > 0.1 ||
//               (_gamepadAxisValues['AXIS_Z']?.abs() ?? 0) > 0.1 ||
//               (_gamepadAxisValues['AXIS_RZ']?.abs() ?? 0) > 0.1);
//
//       if (isGamepadActive) {
//         // Populate the driving command from the gamepad's left stick
//         drivingCommand.move_speed = ((_gamepadAxisValues['AXIS_Y'] ?? 0.0) * -100).round();
//         drivingCommand.turn_angle = ((_gamepadAxisValues['AXIS_X'] ?? 0.0) * 100).round();
//
//         // Populate the main state command from the gamepad's right stick
//         _currentCommand.tilt_speed = ((_gamepadAxisValues['AXIS_RZ'] ?? 0.0) * -100).round();
//         _currentCommand.pan_speed = ((_gamepadAxisValues['AXIS_Z'] ?? 0.0) * 100).round();
//
//         // Handle zoom from gamepad
//         _currentCommand.zoom_command = 0;
//         if (_isZoomInPressed) {
//           _currentCommand.zoom_command = 1;
//         } else if (_isZoomOutPressed || (_gamepadAxisValues['AXIS_BRAKE'] ?? 0.0) > 0.5) {
//           _currentCommand.zoom_command = -1;
//         }
//
//       } else {
//         // Populate the driving command from the left virtual joystick (via _currentCommand)
//         drivingCommand.move_speed = _currentCommand.move_speed;
//         drivingCommand.turn_angle = _currentCommand.turn_angle;
//
//         // The main state command's pan/tilt are already being set by the right virtual joystick's listener.
//         // We don't need to do anything extra here for pan/tilt.
//
//         // Reset zoom state if no gamepad is active
//         _currentCommand.zoom_command = 0;
//       }
//
//       // --- SEND BOTH PACKETS (This part is unchanged) ---
//       _sendCommandPacket(_currentCommand); // Sends the state command (TCP)
//       _sendDrivingPacket(drivingCommand); // Sends the driving command (UDP)
//
//       // Reset touch coordinates after sending
//       if (_currentCommand.touch_x != -1.0) {
//         _currentCommand.touch_x = -1.0;
//         _currentCommand.touch_y = -1.0;
//       }
//     });
//   }
//
//   Future<void> _sendDrivingPacket(DrivingCommand command) async {
//     if (_robotIpAddress.isEmpty) return;
//     try {
//       const int DRIVING_PORT = 65434; // The new port for driving
//       final socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
//       socket.send(command.toBytes(), InternetAddress(_robotIpAddress), DRIVING_PORT);
//       socket.close();
//     } catch (e) {
//       print(e);
//     }
//   }
//
//   Widget _buildModeStatusBanner() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     final Map<int, String> modeIdToText = {
//       CommandIds.DRIVING: "DRIVING MODE",
//       CommandIds.RECON: "RECON MODE",
//       CommandIds.MANUAL_ATTACK: "MANUAL MODE",
//       CommandIds.AUTO_ATTACK: "AUTO MODE",
//       CommandIds.DRONE: "DRONE MODE",
//     };
//
//     final String statusText = modeIdToText[_confirmedServerModeId] ?? "";
//
//     if (statusText.isEmpty) {
//       return const SizedBox.shrink();
//     }
//
//     // Use an Align widget for easy centering
//     return Align(
//       alignment: Alignment.topCenter,
//       child: Padding(
//         // Use padding to push it down from the top edge
//         padding: EdgeInsets.only(top: 40 * widthScale),
//         child: Container(
//           padding: EdgeInsets.symmetric(horizontal: 24 * widthScale, vertical: 12 * widthScale),
//           decoration: BoxDecoration(
//             color: Colors.black.withOpacity(0.6),
//             borderRadius: BorderRadius.circular(10),
//           ),
//           child: Text(
//             statusText,
//             style: TextStyle(
//               fontFamily: 'NotoSans',
//               fontSize: 40 * widthScale,
//               fontWeight: FontWeight.w600,
//               color: Colors.white,
//             ),
//           ),
//         ),
//       ),
//     );
//   }
//
//   // Widget to display the current zoom level
//   Widget _buildZoomDisplay() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     return Positioned(
//       // Positioned according to the client's reference image (x=1360, y=30)
//       top: 30 * widthScale,
//       right: (1920 - 1360 - 200) * widthScale, // Approximate right position based on image
//       child: Container(
//         padding: EdgeInsets.symmetric(horizontal: 20 * widthScale, vertical: 10 * widthScale),
//         decoration: BoxDecoration(
//           color: Colors.black.withOpacity(0.5),
//           borderRadius: BorderRadius.circular(10),
//         ),
//         child: Text(
//           '${_currentZoomLevel.toStringAsFixed(1)} x', // Formats to one decimal place, e.g., "1.3 x"
//           style: TextStyle(
//             fontFamily: 'NotoSans',
//             fontSize: 60 * widthScale, // Scaled font size
//             fontWeight: FontWeight.w600, // Medium weight
//             color: Colors.white,
//           ),
//         ),
//       ),
//     );
//   }
//
//   Widget _buildWindIndicator() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     final String currentWindIcon = ICON_PATH_WIND_W;
//
//     return Positioned(
//       top: 40 * widthScale,
//       left: 280 * widthScale,
//       child: Row(
//         children: [
//           Image.asset(
//             currentWindIcon,
//             width: 40 * widthScale,
//             height: 40 * widthScale,
//           ),
//           SizedBox(width: 10 * widthScale),
//           Text(
//             // _lateralWindSpeed.toStringAsFixed(1),
//             (_gamepadConnected ? _pendingLateralWindSpeed : _lateralWindSpeed).toStringAsFixed(1),
//             style: TextStyle(
//               fontFamily: 'NotoSans',
//               fontSize: 60 * widthScale,
//               fontWeight: FontWeight.w600,
//               color: Colors.white,
//             ),
//           ),
//         ],
//       ),
//     );
//   }
//
//   Widget _buildMovementJoystick() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     return Positioned(
//       left: 260 * widthScale,
//       bottom: 220 * widthScale,
//       child: Opacity(
//         opacity: _isServerConnected ? 1.0 : 0.5,
//         child: Joystick(
//           mode: JoystickMode.all,
//           stick: const CircleAvatar(
//             radius: 30,
//             backgroundColor: Colors.blue,
//           ),
//           base: Container(
//             width: 150,
//             height: 150,
//             decoration: BoxDecoration(
//               color: Colors.grey,
//               shape: BoxShape.circle,
//               border: Border.all(color: Colors.white38, width: 2),
//             ),
//           ),
//           listener: (details) {
//             if (!_isServerConnected) return; // Do nothing if disconnected
//             setState(() {
//               // Y-axis controls forward/backward speed
//               _currentCommand.move_speed = (details.y * -100).round();
//               // X-axis controls left/right turning
//               _currentCommand.turn_angle = (details.x * 100).round();
//             });
//           },
//         ),
//       ),
//     );
//   }
//
//   Widget _buildPanTiltJoystick() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     return Positioned(
//       right: 260 * widthScale,
//       bottom: 220 * widthScale,
//       child: Opacity(
//         opacity: _isServerConnected ? 1.0 : 0.5,
//         child: Joystick(
//           mode: JoystickMode.all,
//           stick: const CircleAvatar(
//             radius: 30,
//             backgroundColor: Colors.blue,
//           ),
//           base: Container(
//             width: 150,
//             height: 150,
//             decoration: BoxDecoration(
//               color: Colors.grey,
//               shape: BoxShape.circle,
//               border: Border.all(color: Colors.white38, width: 2),
//             ),
//           ),
//           listener: (details) {
//             if (!_isServerConnected) return; // Do nothing if disconnected
//             setState(() {
//               // X-axis controls pan (left/right) speed
//               _currentCommand.pan_speed = (details.x * 100).round();
//               // Y-axis controls tilt (up/down) speed
//               _currentCommand.tilt_speed = (details.y * -100).round(); // Y is often inverted for camera controls
//             });
//           },
//         ),
//       ),
//     );
//   }
//
//
//   Widget _buildConnectionStatusBanner() {
//     if (_isServerConnected) {
//       return const SizedBox.shrink();
//     }
//     return Positioned(
//       top: 0,
//       left: 0,
//       right: 0,
//       child: Container(
//         padding: const EdgeInsets.all(8.0),
//         color: Colors.red.withOpacity(0.8),
//         child: const Row(
//           mainAxisAlignment: MainAxisAlignment.center,
//           children: [
//             Icon(Icons.error, color: Colors.white, size: 16),
//             SizedBox(width: 8),
//             Text(
//               'Connection Lost - Attempting to reconnect...',
//               style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
//             ),
//           ],
//         ),
//       ),
//     );
//   }
//
//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       backgroundColor: Colors.black,
//       body: _isLoading
//           ? const Center(child: CircularProgressIndicator())
//           : Stack(
//         children: [
//           Positioned.fill(
//             child: InteractiveViewer(
//               transformationController: _transformationController,
//               minScale: 1.0, // User cannot pinch-to-zoom out
//               maxScale: 5.0, // User can pinch-to-zoom in up to 5x
//               panEnabled: false, // Disable panning with finger, only joysticks control it
//               child: KeyedSubtree(
//                 key: _gstreamerViewKey,
//                 child: AndroidView(
//                   viewType: 'gstreamer_view',
//                   onPlatformViewCreated: _onGStreamerPlatformViewCreated,
//                 ),
//               ),
//             ),
//           ),
//
//           if (!_isGStreamerLoading && !_gstreamerHasError)
//             Positioned.fill(child: _buildTouchDetector()),
//
//           Positioned.fill(child: _buildStreamOverlay()),
//
//           _buildWindIndicator(),
//           _buildZoomDisplay(),
//
//           _buildModeStatusBanner(),
//
//           _buildConnectionStatusBanner(),
//
//           _buildModeButton(0, 30, 30, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE),
//           _buildModeButton(1, 30, 214, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE),
//           _buildModeButton(2, 30, 398, "Manual Attack", ICON_PATH_MANUAL_ATTACK_INACTIVE, ICON_PATH_MANUAL_ATTACK_ACTIVE),
//           _buildModeButton(3, 30, 582, "Auto Attack", ICON_PATH_AUTO_ATTACK_INACTIVE, ICON_PATH_AUTO_ATTACK_ACTIVE),
//           _buildModeButton(4, 30, 766, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE),
//
//           _buildViewButton(
//             1690, 30, "Day View",
//             ICON_PATH_DAY_VIEW_ACTIVE,   // Pass the ACTIVE icon
//             ICON_PATH_DAY_VIEW_INACTIVE, // Pass the INACTIVE icon
//             _currentCameraIndex == 0,    // This button is active if camera index is 0
//             onPressed: () => _switchCamera(0),
//           ),
//
//           // Night View Button (IR Camera)
//           _buildViewButton(
//             1690, 214, "Night View",
//             ICON_PATH_NIGHT_VIEW_ACTIVE,   // Pass the ACTIVE icon
//             ICON_PATH_NIGHT_VIEW_INACTIVE, // Pass the INACTIVE icon
//             _currentCameraIndex == 1,      // This button is active if camera index is 1
//             onPressed: () => _switchCamera(1),
//           ),
//
//           _buildViewButton(
//               1690, 720, "Setting",
//               ICON_PATH_SETTINGS, // Active icon
//               ICON_PATH_SETTINGS, // Inactive icon (the same)
//               false,              // Never shows the "active" red background
//               onPressed: _navigateToSettings
//           ),
//
//           // _buildDirectionalControls(),
//           _buildMovementJoystick(),
//           _buildPanTiltJoystick(),
//
//           Align(
//             alignment: Alignment.bottomCenter,
//             child: Container(
//               margin: const EdgeInsets.only(bottom: 20, left: 20, right: 20),
//               decoration: BoxDecoration(color: Colors.black.withOpacity(0.6), borderRadius: BorderRadius.circular(50)),
//               child: _buildBottomBar(),
//             ),
//           ),
//
//         ],
//       ),
//     );
//   }
//
//   Widget _buildModeButton(int index, double left, double top, String label, String inactiveIcon, String activeIcon) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     final bool isSelected = _selectedModeIndex == index;
//     final Color color = isSelected ? (_buttonActiveColor[index] ?? Colors.grey) : Colors.black.withOpacity(0.6);
//     final String icon = isSelected ? activeIcon : inactiveIcon;
//
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: Opacity(
//         opacity: _isServerConnected ? 1.0 : 0.5,
//         child: GestureDetector(
//           // onTap: () => _onModeSelected(index),
//           onTap: _isServerConnected ? () => _onModeSelected(index) : null,
//           child: Container(
//             width: 200 * widthScale,
//             height: 120 * heightScale,
//             padding: EdgeInsets.symmetric(vertical: 4.0 * heightScale),
//             decoration: BoxDecoration(
//               color: color,
//               borderRadius: BorderRadius.circular(9),
//               border: Border.all(color: isSelected ? Colors.white : Colors.transparent, width: 2.0),
//             ),
//             child: Column(
//               mainAxisAlignment: MainAxisAlignment.center,
//               children: [
//                 Expanded(flex: 2, child: Image.asset(icon, fit: BoxFit.contain)),
//                 SizedBox(height: 5 * heightScale),
//                 Text(label, textAlign: TextAlign.center, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.bold, fontSize: 25 * heightScale, color: Colors.white)),
//               ],
//             ),
//           ),
//         ),
//       ),
//     );
//   }
//
//   Widget _buildViewButton(
//       double left,
//       double top,
//       String label,
//       String activeIconPath,
//       String inactiveIconPath,
//       bool isActive,
//       {VoidCallback? onPressed}
//       ) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     // --- THIS IS THE FIX ---
//     // The background color is determined by the active state.
//     final Color color = isActive ? Colors.grey : Colors.black.withOpacity(0.6);
//     // The icon path is ALSO determined by the active state.
//     final String iconToDisplay = isActive ? activeIconPath : inactiveIconPath;
//
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTap: onPressed,
//         child: Container(
//           width: 220 * widthScale,
//           height: 175 * heightScale,
//           padding: EdgeInsets.symmetric(vertical: 10.0 * heightScale),
//           decoration: BoxDecoration(
//             color: color,
//             borderRadius: BorderRadius.circular(15),
//             border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.5),
//           ),
//           child: Column(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               Expanded(
//                   flex: 3,
//                   // Use the dynamically selected icon path
//                   child: Image.asset(iconToDisplay, fit: BoxFit.contain)
//               ),
//               SizedBox(height: 8 * heightScale),
//               Text(
//                   label,
//                   textAlign: TextAlign.center,
//                   style: TextStyle(
//                       fontFamily: 'NotoSans',
//                       fontWeight: FontWeight.w700,
//                       fontSize: 26 * heightScale,
//                       color: Colors.white
//                   )
//               ),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   Widget _buildBottomBar() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     return LayoutBuilder(
//       builder: (context, constraints) {
//         List<Color> permissionButtonColors;
//         bool isCombatModeActive = _isModeActive && (_selectedModeIndex == 2 || _selectedModeIndex == 3);
//
//         if (isCombatModeActive) {
//           permissionButtonColors = _isPermissionToAttackOn ? _permissionOnColors : _permissionOffColors;
//         } else {
//           permissionButtonColors = _permissionDisabledColors;
//         }
//
//         final List<Widget> leftCluster = [
//           _buildBottomBarButton(
//             "PERMISSION TO ATTACK",
//             null,
//             // permissionButtonColors,
//             _isPermissionToAttackOn ? [const Color(0xffc32121), const Color(0xff831616)] : [const Color(0xFF424242), const Color(0xFF212121)],
//             _isServerConnected ? _onPermissionPressed : null,
//           ),
//           SizedBox(width: 12 * widthScale),
//           _buildWideBottomBarButton(
//             _isModeActive ? "STOP" : "START",
//             _isModeActive ? ICON_PATH_STOP : ICON_PATH_START,
//             [const Color(0xff25a625), const Color(0xff127812)],
//             _isServerConnected ? _onStartStopPressed : null,
//           ),
//           SizedBox(width: 12 * widthScale),
//           // _buildBottomBarButton(
//           //   "",
//           //   ICON_PATH_PLUS,
//           //   [const Color(0xffc0c0c0), const Color(0xffa0a0a0)],
//           //   _isServerConnected ? () {
//           //     setState(() {
//           //       if (_currentZoomLevel < 5.0) _currentZoomLevel += 0.1;
//           //       else _currentZoomLevel = 5.0;
//           //       _transformationController.value = Matrix4.identity()..scale(_currentZoomLevel);
//           //     });
//           //   } : null,
//           // ),
//           // SizedBox(width: 12 * widthScale),
//           // _buildBottomBarButton(
//           //   "",
//           //   ICON_PATH_MINUS,
//           //   [const Color(0xffc0c0c0), const Color(0xffa0a0a0)],
//           //   _isServerConnected ? () {
//           //     setState(() {
//           //       if (_currentZoomLevel > 1.0) _currentZoomLevel -= 0.1;
//           //       else _currentZoomLevel = 1.0;
//           //       _transformationController.value = Matrix4.identity()..scale(_currentZoomLevel);
//           //     });
//           //   } : null,
//           // ),
//
//           _buildBottomBarButton(
//             "",
//             ICON_PATH_PLUS,
//             [const Color(0xffc0c0c0), const Color(0xffa0a0a0)],
//             _isServerConnected ? () {
//               // --- UPDATED ZOOM IN LOGIC ---
//               setState(() {
//                 if (_currentZoomLevel < 5.0) _currentZoomLevel += 0.1;
//                 else _currentZoomLevel = 5.0;
//                 _transformationController.value = Matrix4.identity()..scale(_currentZoomLevel);
//
//                 // Set the flag for the command timer
//                 _isUiZoomInPressed = true;
//                 // Clear the flag after a moment so we only send one command per press
//                 Future.delayed(const Duration(milliseconds: 150), () => _isUiZoomInPressed = false);
//               });
//             } : null,
//           ),
//           SizedBox(width: 12 * widthScale),
//           _buildBottomBarButton(
//             "",
//             ICON_PATH_MINUS,
//             [const Color(0xffc0c0c0), const Color(0xffa0a0a0)],
//             _isServerConnected ? () {
//               // --- UPDATED ZOOM OUT LOGIC ---
//               setState(() {
//                 if (_currentZoomLevel > 1.0) _currentZoomLevel -= 0.1;
//                 else _currentZoomLevel = 1.0;
//                 _transformationController.value = Matrix4.identity()..scale(_currentZoomLevel);
//
//                 // Set the flag for the command timer
//                 _isUiZoomOutPressed = true;
//                 // Clear the flag after a moment
//                 Future.delayed(const Duration(milliseconds: 150), () => _isUiZoomOutPressed = false);
//               });
//             } : null,
//           ),
//         ];
//
//         final List<Widget> middleCluster = [
//           Row(
//             mainAxisSize: MainAxisSize.min,
//             crossAxisAlignment: CrossAxisAlignment.baseline,
//             textBaseline: TextBaseline.alphabetic,
//             children: [
//               const Text("0", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 75, color: Colors.white)),
//               SizedBox(width: 8 * widthScale),
//               const Text("Km/h", style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 37, color: Colors.white)),
//             ],
//           ),
//           SizedBox(width: 20 * widthScale),
//           Image.asset(ICON_PATH_WIFI, height: 40),
//         ];
//
//         final List<Widget> rightCluster = [
//           _buildBottomBarButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)], () async {
//             final proceed = await _showExitDialog();
//             if (proceed) SystemNavigator.pop();
//           }),
//         ];
//
//         return Row(
//           children: [
//             ...leftCluster,
//             const Spacer(),
//             ...middleCluster,
//             const Spacer(),
//             ...rightCluster,
//           ],
//         );
//       },
//     );
//   }
//
//   // Make the onPressed parameter nullable by adding '?'
//   Widget _buildWideBottomBarButton(String label, String iconPath, List<Color> gradientColors, VoidCallback? onPressed) {
//     final screenHeight = MediaQuery.of(context).size.height;
//     final screenWidth = MediaQuery.of(context).size.width;
//     final heightScale = screenHeight / 1080.0;
//     final widthScale = screenWidth / 1920.0;
//     final bool isEnabled = onPressed != null;
//
//     return GestureDetector(
//       onTap: onPressed,
//       child: Opacity(
//         opacity: isEnabled ? 1.0 : 0.5,
//         child: Container(
//           constraints: BoxConstraints(minWidth: 220 * widthScale),
//           height: 80 * heightScale,
//           padding: const EdgeInsets.symmetric(horizontal: 74),
//           decoration: BoxDecoration(
//             gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//             borderRadius: BorderRadius.circular(25 * heightScale),
//           ),
//           child: Row(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               Image.asset(iconPath, height: 36 * heightScale),
//               const SizedBox(width: 12),
//               Text(label, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 36 * heightScale, color: Colors.white)),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   // Make the onPressed parameter nullable by adding '?'
//   Widget _buildBottomBarButton(String label, String? iconPath, List<Color> gradientColors, VoidCallback? onPressed) {
//     final screenHeight = MediaQuery.of(context).size.height;
//     final heightScale = screenHeight / 1080.0;
//     final bool isEnabled = onPressed != null;
//
//     return GestureDetector(
//       onTap: onPressed,
//       child: Opacity(
//         opacity: isEnabled ? 1.0 : 0.5,
//         child: Container(
//           height: 80 * heightScale,
//           padding: const EdgeInsets.symmetric(horizontal: 30),
//           decoration: BoxDecoration(
//             gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//             borderRadius: BorderRadius.circular(25 * heightScale),
//           ),
//           child: Row(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               if (iconPath != null && iconPath.isNotEmpty) ...[
//                 Image.asset(iconPath, height: 36 * heightScale),
//                 if (label.isNotEmpty) const SizedBox(width: 12),
//               ],
//               if (label.isNotEmpty)
//                 Text(label, style: TextStyle(fontFamily: 'NotoSans', fontWeight: FontWeight.w700, fontSize: 30 * heightScale, color: Colors.white)),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   Widget _buildDirectionalControls() {
//     return Stack(
//       children: [
//         _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE, () => setState(() => _isForwardPressed = true), () => setState(() => _isForwardPressed = false)),
//         _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE, () => setState(() => _isBackPressed = true), () => setState(() => _isBackPressed = false)),
//         _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE, () => setState(() => _isLeftPressed = true), () => setState(() => _isLeftPressed = false)),
//         _buildDirectionalButton(1540, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE, () => setState(() => _isRightPressed = true), () => setState(() => _isRightPressed = false)),
//       ],
//     );
//   }
//
//   Widget _buildDirectionalButton(double left, double top, bool isPressed, String inactiveIcon, String activeIcon, VoidCallback onPress, VoidCallback onRelease) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTapDown: (_) => onPress(),
//         onTapUp: (_) => onRelease(),
//         onTapCancel: () => onRelease(),
//         child: Image.asset(isPressed ? activeIcon : inactiveIcon, height: 100 * heightScale, width: 100 * widthScale, fit: BoxFit.contain),
//       ),
//     );
//   }
//
//   Widget _buildTouchDetector() {
//     return GestureDetector(
//       key: const GlobalObjectKey('_playerKey'),
//       behavior: HitTestBehavior.opaque,
//       onTapUp: (details) {
//         if (_isModeActive && _selectedModeIndex == 2) { // 2 corresponds to Manual Attack
//           final RenderBox? renderBox = context.findRenderObject() as RenderBox?;
//           if (renderBox != null) {
//             final Offset localPosition = renderBox.globalToLocal(details.globalPosition);
//             final double x = localPosition.dx / renderBox.size.width;
//             final double y = localPosition.dy / renderBox.size.height;
//             if (x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0) {
//               _sendTouchPacket(TouchCoord()..x = x..y = y);
//             }
//           }
//         }
//       },
//       child: Container(color: Colors.transparent),
//     );
//   }
//
//   Future<bool> _showExitDialog() async {
//     return await showDialog<bool>(
//       context: context,
//       builder: (context) => AlertDialog(
//         title: const Text('Confirm Exit'),
//         content: const Text('Do you want to close the application?'),
//         actions: [
//           TextButton(onPressed: () => Navigator.of(context).pop(false), child: const Text('Cancel')),
//           TextButton(onPressed: () => Navigator.of(context).pop(true), child: const Text('Exit')),
//         ],
//       ),
//     ) ?? false;
//   }
//
//   Future<void> _handleGStreamerMessages(MethodCall call) async {
//     if (!mounted) return;
//     _streamTimeoutTimer?.cancel();
//     switch (call.method) {
//       case 'onStreamReady':
//         setState(() { _isGStreamerLoading = false; _gstreamerHasError = false; });
//         break;
//       case 'onStreamError':
//         final String? error = call.arguments?['error'];
//         setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "Stream error: ${"" ?? 'Unknown native error'}"; });
//         break;
//     }
//   }
//
//   void _retryStream() {
//     if (_currentCameraIndex == -1) {
//       print("Retry failed: No camera index selected.");
//       return;
//     }
//     print("Retrying stream for camera index: $_currentCameraIndex");
//
//     setState(() {
//       _gstreamerViewKey = UniqueKey(); // This is the MOST IMPORTANT line. It forces the widget to be destroyed and recreated.
//       _isGStreamerReady = false;
//       _isGStreamerLoading = true; // Show the loading spinner again
//       _gstreamerHasError = false; // Clear the error state
//       _errorMessage = null;
//     });
//   }
//
//
//   Future<void> _loadSettingsAndInitialize() async {
//     _robotIpAddress = await _settingsService.loadIpAddress();
//     _cameraUrls = await _settingsService.loadCameraUrls();
//     if (mounted) {
//       _connectToStatusServer(); // <-- ADD THIS CALL
//       _switchCamera(0);
//       _startCommandTimer();
//       setState(() => _isLoading = false);
//     }
//   }
//
//   Future<void> _sendCommandPacket(UserCommand command) async {
//     if (_robotIpAddress.isEmpty) return;
//     try {
//       final socket = await Socket.connect(_robotIpAddress, 65432, timeout: const Duration(milliseconds: 150));
//       socket.add(command.toBytes());
//       await socket.flush();
//       socket.close();
//     } catch (e) {}
//   }
//
//   Future<void> _sendTouchPacket(TouchCoord coord) async {
//     if (_robotIpAddress.isEmpty) return;
//     try {
//       const int TOUCH_PORT = 65433;
//       final socket = await RawDatagramSocket.bind(InternetAddress.anyIPv4, 0);
//       socket.send(coord.toBytes(), InternetAddress(_robotIpAddress), TOUCH_PORT);
//       socket.close();
//       print('Sent Touch Packet (UDP): X=${coord.x}, Y=${coord.y}');
//     } catch (e) {
//       print('Error sending touch UDP packet: $e');
//     }
//   }
//
//   void _stopGStreamer() {
//     if (_gstreamerChannel != null && _isGStreamerReady) {
//       _gstreamerChannel!.invokeMethod('stopStream').catchError((e) => print("Error stopping stream: $e"));
//     }
//     _isGStreamerReady = false;
//   }
//
//   Future<void> _switchCamera(int index) async {
//     if (_cameraUrls.isEmpty || index < 0 || index >= _cameraUrls.length) return;
//     if (index == _currentCameraIndex && !_gstreamerHasError) return;
//     if (_gstreamerChannel != null && _isGStreamerReady) {
//       try { await _gstreamerChannel!.invokeMethod('stopStream'); } catch (e) {}
//     }
//     setState(() {
//       _currentCameraIndex = index;
//       _gstreamerViewKey = UniqueKey();
//       _isGStreamerReady = false;
//       _isGStreamerLoading = true;
//       _gstreamerHasError = false;
//       _errorMessage = null;
//     });
//   }
//
//
//   void _onGStreamerPlatformViewCreated(int id) {
//     _gstreamerChannel = MethodChannel('gstreamer_channel_$id');
//     _gstreamerChannel!.setMethodCallHandler(_handleGStreamerMessages);
//     setState(() {
//       _isGStreamerReady = true;
//       _isGStreamerLoading = false;
//     });
//     _playCurrentCameraStream();
//   }
//
//   Future<void> _playCurrentCameraStream() async {
//     _streamTimeoutTimer?.cancel();
//     if (_cameraUrls.isEmpty || _currentCameraIndex < 0 || _currentCameraIndex >= _cameraUrls.length) {
//       if (mounted) setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "No valid camera URL."; });
//       return;
//     }
//     if (_gstreamerChannel == null || !_isGStreamerReady) {
//       if (mounted) setState(() { _isGStreamerLoading = false; _gstreamerHasError = true; _errorMessage = "GStreamer channel not ready."; });
//       return;
//     }
//     setState(() { _isGStreamerLoading = true; _gstreamerHasError = false; _errorMessage = null; });
//     _streamTimeoutTimer = Timer(const Duration(seconds: 8), () {
//       if (mounted && _isGStreamerLoading) {
//         setState(() { _gstreamerHasError = true; _errorMessage = "Connection timed out."; _isGStreamerLoading = false; });
//       }
//     });
//     try {
//       final String url = _cameraUrls[_currentCameraIndex];
//       await _gstreamerChannel!.invokeMethod('startStream', {'url': url});
//     } catch (e) {
//       _streamTimeoutTimer?.cancel();
//       if (mounted) {
//         setState(() { _gstreamerHasError = true; _errorMessage = "Failed to start stream: ${e.toString()}"; _isGStreamerLoading = false; });
//       }
//     }
//   }
//
//   Widget _buildStreamOverlay() {
//     if (_isGStreamerLoading) {
//       return Container(
//         color: Colors.black.withOpacity(0.5),
//         child: const Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [CircularProgressIndicator(color: Colors.white), SizedBox(height: 20), Text('Connecting to stream...', style: TextStyle(color: Colors.white, fontSize: 18))])),
//       );
//     }
//     if (_gstreamerHasError) {
//       return Container(
//         color: Colors.black.withOpacity(0.7),
//         child: Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
//           const Icon(Icons.error_outline, color: Colors.redAccent, size: 70),
//           const SizedBox(height: 20),
//           Padding(padding: const EdgeInsets.symmetric(horizontal: 40.0), child: Text(_errorMessage ?? 'Stream failed to load.', textAlign: TextAlign.center, style: const TextStyle(color: Colors.white, fontSize: 20))),
//           const SizedBox(height: 30),
//           ElevatedButton.icon(
//             icon: const Icon(Icons.refresh),
//             label: const Text('Retry'),
//             onPressed: () { if (_currentCameraIndex != -1) _switchCamera(_currentCameraIndex); },
//             style: ElevatedButton.styleFrom(foregroundColor: Colors.white, backgroundColor: Colors.blueGrey, padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 15), textStyle: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
//           ),
//         ])),
//       );
//     }
//     return const SizedBox.shrink();
//   }
//
//   Future<void> _navigateToSettings() async {
//     final bool? settingsChanged = await Navigator.push<bool>(context, MaterialPageRoute(builder: (context) => const SettingsMenuPage()));
//     if (settingsChanged == true && mounted) {
//       _commandTimer?.cancel();
//       await _loadSettingsAndInitialize();
//     }
//   }
// }
