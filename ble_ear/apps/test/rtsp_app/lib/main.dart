// import 'dart:io';
// import 'package:flutter/material.dart';
// import 'package:flutter_vlc_player/flutter_vlc_player.dart';
// import 'package:google_fonts/google_fonts.dart';
//
// import 'icon_constants.dart';
// import 'splash_screen.dart';
// import 'settings_menu_page.dart';
//
// // --- CONFIGURATION FOR ROBOT CONNECTION ---
// const String ROBOT_IP_ADDRESS = '192.168.0.158';
// const int ROBOT_COMMAND_PORT = 65432;
//
// // --- MAIN APP ENTRY POINT ---
// void main() {
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
//         textTheme: GoogleFonts.rajdhaniTextTheme(),
//       ),
//       home: const SplashScreen(),
//     );
//   }
// }
//
// // --- HOME PAGE WIDGET ---
// // --- HOME PAGE WIDGET ---
// class HomePage extends StatefulWidget {
//   const HomePage({super.key});
//
//   @override
//   State<HomePage> createState() => _HomePageState();
// }
//
// class _HomePageState extends State<HomePage> {
//   // All state variables and backend logic functions remain the same...
//   VlcPlayerController? _vlcPlayerController;
//   List<String> _cameraUrls = [
//     'rtsp://192.168.0.158:8554/cam0',
//     'rtsp://192.168.0.158:8554/cam0',
//     'rtsp://192.168.0.158:8554/cam0',
//   ];
//   int _currentCameraIndex = -1;
//   String? _errorMessage;
//   int _activeLeftButtonIndex = 0;
//   int _activeRightButtonIndex = 0;
//   bool _isStarted = false;
//   bool _isForwardPressed = false;
//   bool _isBackPressed = false;
//   bool _isLeftPressed = false;
//   bool _isRightPressed = false;
//   bool _isAutoAttackMode = false;
//
//   @override
//   void initState() {
//     super.initState();
//     _switchCamera(0);
//   }
//
//   @override
//   void dispose() {
//     _vlcPlayerController?.dispose();
//     super.dispose();
//   }
//
//   Future<void> _sendCommand(String command) async {
//     try {
//       final socket = await Socket.connect(ROBOT_IP_ADDRESS, ROBOT_COMMAND_PORT,
//           timeout: const Duration(seconds: 5));
//       socket.write(command);
//       await socket.flush();
//       socket.close();
//       print('Command sent: $command');
//     } catch (e) {
//       print('Error sending command: $e');
//       ScaffoldMessenger.of(context).showSnackBar(
//         SnackBar(
//             content: const Text('Error: Could not connect to robot.'),
//             backgroundColor: Colors.red),
//       );
//     }
//   }
//
//   // --- RESTORED STABLE CAMERA SWITCHING LOGIC ---
//   Future<void> _switchCamera(int index) async {
//     final VlcPlayerController? oldController = _vlcPlayerController;
//
//     setState(() {
//       if (index == _currentCameraIndex && _vlcPlayerController != null) {
//       } else if (index == _currentCameraIndex) {
//         return;
//       }
//       _currentCameraIndex = index;
//       _vlcPlayerController = null;
//       _errorMessage = null;
//     });
//
//     await oldController?.dispose();
//     await Future.delayed(const Duration(milliseconds: 200));
//
//     final newController = VlcPlayerController.network(
//       _cameraUrls[index],
//       hwAcc: HwAcc.disabled,
//       autoPlay: true,
//       options: VlcPlayerOptions(
//         // advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(150)]),
//         advanced: VlcAdvancedOptions([
//           VlcAdvancedOptions.networkCaching(120),
//           VlcAdvancedOptions.clockJitter(0),
//           VlcAdvancedOptions.fileCaching(30),
//           VlcAdvancedOptions.liveCaching(30),
//           VlcAdvancedOptions.clockSynchronization(1),
//         ]),
//         video: VlcVideoOptions([
//           VlcVideoOptions.dropLateFrames(true),
//           VlcVideoOptions.skipFrames(true),
//         ]),
//         extras: ['--h264-fps=60', '--no-audio'],
//       ),
//     );
//
//     newController.addListener(() {
//       if (!mounted) return;
//       final state = newController.value;
//       if (state.hasError && _errorMessage != state.errorDescription) {
//         setState(() {
//           _errorMessage = state.errorDescription ?? 'An Unknown Error Occurred!';
//         });
//       }
//     });
//
//     if (mounted) {
//       setState(() {
//         _vlcPlayerController = newController;
//       });
//     }
//   }
//
//   Future<void> _navigateToSettings() async {
//     final newUrls = await Navigator.push<List<String>>(
//       context,
//       MaterialPageRoute(
//           builder: (context) => SettingsMenuPage(cameraUrls: _cameraUrls)),
//     );
//     if (newUrls != null) {
//       setState(() => _cameraUrls = newUrls);
//       _switchCamera(_currentCameraIndex);
//     }
//   }
//
//   // --- ADDED DIALOG FUNCTIONALITY ---
//   Future<bool> _showCustomConfirmationDialog({
//     required BuildContext context,
//     required String iconPath,
//     required String title,
//     required Color titleColor,
//     required String content,
//   }) async {
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
//                     Text(title, style: GoogleFonts.notoSans(fontSize: 28, fontWeight: FontWeight.bold, color: titleColor)),
//                   ],
//                 ),
//                 const SizedBox(height: 24),
//                 Text(content, textAlign: TextAlign.center, style: GoogleFonts.notoSans(fontSize: 22, color: Colors.black87)),
//                 const SizedBox(height: 32),
//                 Row(
//                   mainAxisAlignment: MainAxisAlignment.spaceEvenly,
//                   children: [
//                     TextButton(onPressed: () => Navigator.of(dialogContext).pop(false), child: Text('Cancel', style: GoogleFonts.notoSans(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.grey.shade700))),
//                     SizedBox(height: 40, child: VerticalDivider(color: Colors.grey.shade400, thickness: 1)),
//                     TextButton(onPressed: () => Navigator.of(dialogContext).pop(true), child: Text('OK', style: GoogleFonts.notoSans(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.blue.shade700))),
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
//     if (_isAutoAttackMode) {
//       final proceed = await _showCustomConfirmationDialog(
//         context: context,
//         iconPath: ICON_PATH_AUTO_ATTACK_ACTIVE,
//         title: "DANGERS",
//         titleColor: Colors.red,
//         content: 'Are you sure you want to stop\n"Auto Attack" mode?',
//       );
//       if (proceed) {
//         _sendCommand('CMD_MANU_ATTACK');
//         setState(() => _isAutoAttackMode = false);
//       }
//     } else {
//       final proceed = await _showCustomConfirmationDialog(
//         context: context,
//         iconPath: ICON_PATH_MANUAL_ATTACK_ACTIVE,
//         title: "DANGERS",
//         titleColor: Colors.red,
//         content: 'Are you sure you want to start\n"Auto Attack" mode?',
//       );
//       if (proceed) {
//         _sendCommand('CMD_AUTO_ATTACK');
//         setState(() => _isAutoAttackMode = true);
//       }
//     }
//   }
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
//           Positioned.fill(child: buildPlayerWidget()),
//
//           _buildSideButton(30, 35, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE, _activeLeftButtonIndex == 0, () => _onLeftButtonPressed(0, 'CMD_MODE_DRIVE')),
//           _buildSideButton(30, 185, "Petrol", ICON_PATH_PETROL_INACTIVE, ICON_PATH_PETROL_ACTIVE, _activeLeftButtonIndex == 1, () => _onLeftButtonPressed(1, 'CMD_MODE_PATROL')),
//           _buildSideButton(30, 335, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE, _activeLeftButtonIndex == 2, () => _onLeftButtonPressed(2, 'CMD_MODE_RECON')),
//
//           _buildSideButton(
//               30, 485,
//               _isAutoAttackMode ? "Auto Attack" : "Manual Attack",
//               _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_INACTIVE : ICON_PATH_MANUAL_ATTACK_INACTIVE,
//               _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_ACTIVE : ICON_PATH_MANUAL_ATTACK_ACTIVE,
//               _activeLeftButtonIndex == 3,
//                   () {
//                 setState(() => _activeLeftButtonIndex = 3);
//                 _handleManualAutoAttackToggle();
//               }
//           ),
//
//           _buildSideButton(30, 635, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE, _activeLeftButtonIndex == 4, () => _onLeftButtonPressed(4, 'CMD_MODE_DRONE')),
//           _buildSideButton(30, 785, "Return", ICON_PATH_RETURN_INACTIVE, ICON_PATH_RETURN_ACTIVE, _activeLeftButtonIndex == 5, () => _onLeftButtonPressed(5, 'CMD_RETUR')),
//
//           // Right buttons now switch cameras again
//           _buildSideButton(1690, 30, "Cam 1", ICON_PATH_ATTACK_VIEW_INACTIVE, ICON_PATH_ATTACK_VIEW_ACTIVE, _activeRightButtonIndex == 0, () => _onRightButtonPressed(0)),
//           _buildSideButton(1690, 250, "Cam 2", ICON_PATH_TOP_VIEW_INACTIVE, ICON_PATH_TOP_VIEW_ACTIVE, _activeRightButtonIndex == 1, () => _onRightButtonPressed(1)),
//           _buildSideButton(1690, 470, "Cam 3", ICON_PATH_3D_VIEW_INACTIVE, ICON_PATH_3D_VIEW_ACTIVE, _activeRightButtonIndex == 2, () => _onRightButtonPressed(2)),
//
//           _buildSideButton(1690, 720, "Setting", ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, false, () => _navigateToSettings()),
//
//           _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE, 'CMD_FORWARD', () => setState(() => _isForwardPressed = true), () => setState(() => _isForwardPressed = false)),
//           _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE, 'CMD_BACKWARD', () => setState(() => _isBackPressed = true), () => setState(() => _isBackPressed = false)),
//           _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE, 'CMD_LEFT', () => setState(() => _isLeftPressed = true), () => setState(() => _isLeftPressed = false)),
//           _buildDirectionalButton(1560, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE, 'CMD_RIGHT', () => setState(() => _isRightPressed = true), () => setState(() => _isRightPressed = false)),
//
//           Align(
//             alignment: Alignment.bottomCenter,
//             child: Padding(
//               padding: EdgeInsets.only(bottom: 20 * heightScale, left: 30 * widthScale, right: 30 * widthScale),
//               child: _buildBottomBar(),
//             ),
//           ),
//         ],
//       ),
//     );
//   }
//
//   void _onLeftButtonPressed(int index, String command) {
//     setState(() => _activeLeftButtonIndex = index);
//     _sendCommand(command);
//   }
//
//   // Restored right button logic to switch cameras
//   void _onRightButtonPressed(int index) {
//     setState(() => _activeRightButtonIndex = index);
//     _switchCamera(index);
//   }
//
//   // --- RESTORED STABLE PLAYER WIDGET ---
//   Widget buildPlayerWidget() {
//     if (_errorMessage != null) {
//       return Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
//         Padding(padding: const EdgeInsets.symmetric(horizontal: 24.0),
//             child: Text('Error: $_errorMessage', textAlign: TextAlign.center, style: const TextStyle(color: Colors.red, fontSize: 16, fontWeight: FontWeight.bold))),
//         const SizedBox(height: 20),
//         ElevatedButton(onPressed: () => _switchCamera(_currentCameraIndex), style: ElevatedButton.styleFrom(backgroundColor: Colors.grey[700]), child: const Text('Retry', style: TextStyle(color: Colors.white))),
//       ],
//       ));
//     }
//     if (_vlcPlayerController == null) {
//       return const Center(child: CircularProgressIndicator(color: Colors.white));
//     }
//     return VlcPlayer(controller: _vlcPlayerController!, aspectRatio: 16 / 9, placeholder: const Center(child: CircularProgressIndicator(color: Colors.white)));
//   }
//
//   // The rest of the helper methods can remain the same
//   Widget _buildSideButton(double left, double top, String label, String inactiveIcon, String activeIcon, bool isActive, VoidCallback onPressed) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTap: onPressed,
//         child: Container(
//           width: 200 * widthScale,
//           height: 120 * heightScale,
//           padding: const EdgeInsets.all(8.0),
//           decoration: BoxDecoration(
//             color: Colors.black.withOpacity(0.6),
//             borderRadius: BorderRadius.circular(10),
//             border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.0),
//           ),
//           child: FittedBox(
//             fit: BoxFit.contain,
//             child: Column(
//               mainAxisAlignment: MainAxisAlignment.center,
//               children: [
//                 Image.asset(isActive ? activeIcon : inactiveIcon, height: 50),
//                 const SizedBox(height: 10),
//                 Text(label, textAlign: TextAlign.center, style: GoogleFonts.notoSans(fontSize: 24, fontWeight: FontWeight.bold, color: const Color(0xFFFFFFFF))),
//               ],
//             ),
//           ),
//         ),
//       ),
//     );
//   }
//
//   Widget _buildDirectionalButton(double left, double top, bool isPressed, String inactiveIcon, String activeIcon, String command, VoidCallback onPress, VoidCallback onRelease) {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final screenHeight = MediaQuery.of(context).size.height;
//     final widthScale = screenWidth / 1920.0;
//     final heightScale = screenHeight / 1080.0;
//
//     return Positioned(
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTapDown: (_) {
//           onPress();
//           _sendCommand(command);
//         },
//         onTapUp: (_) {
//           onRelease();
//           _sendCommand('CMD_STOP');
//         },
//         onTapCancel: () => onRelease(),
//         child: Image.asset(isPressed ? activeIcon : inactiveIcon, height: 100 * heightScale, width: 100 * widthScale, fit: BoxFit.contain),
//       ),
//     );
//   }
//
//   // --- FIX: REWRITTEN BOTTOM BAR FOR RESPONSIVENESS ---
//   Widget _buildBottomBar() {
//     final screenWidth = MediaQuery.of(context).size.width;
//     final widthScale = screenWidth / 1920.0;
//
//     return Row(
//       mainAxisAlignment: MainAxisAlignment.spaceBetween,
//       crossAxisAlignment: CrossAxisAlignment.center,
//       children: [
//         // --- Left Cluster of Buttons ---
//         // This Row ensures the left buttons stay grouped together.//         Row(
//           mainAxisSize: MainAxisSize.min,
//           children: [
//             _buildBottomBarButton("ATTACK", ICON_PATH_ATTACK, [const Color(0xffc32121), const Color(0xff831616)],
//                     () async {
//                   final proceed = await _showCustomConfirmationDialog(
//                     context: context,
//                     iconPath: ICON_PATH_ATTACK,
//                     title: 'DANGERS',
//                     titleColor: Colors.red,
//                     content: 'Do you want to proceed the command?',
//                   );
//                   if (proceed) _sendCommand('CMD_MODE_ATTACK');
//                 }
//             ),
//             SizedBox(width: 12 * widthScale),
//             _buildBottomBarButton(_isStarted ? "STOP" : "START", _isStarted ? ICON_PATH_STOP : ICON_PATH_START, [const Color(0xff25a625), const Color(0xff127812)], () { setState(() => _isStarted = !_isStarted); _sendCommand(_isStarted ? 'CMD_STOP' : 'CMD_START'); }),
//             SizedBox(width: 12 * widthScale),
//             _buildBottomBarButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_IN')),
//             SizedBox(width: 12 * widthScale),
//             _buildBottomBarButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_OUT')),
//           ],
//         ),
//
//         // --- Middle Status Cluster ---
//         // Flexible allows this section to shrink, and FittedBox scales its content down.
//         Flexible(
//           child: FittedBox(
//             fit: BoxFit.scaleDown, // Ensures it doesn't scale up, only down
//             child: Row(
//               mainAxisAlignment: MainAxisAlignment.center,
//               children: [
//                 const SizedBox(width: 20), // Add spacing
//                 Text("0", style: GoogleFonts.notoSans(fontSize: 80, fontWeight: FontWeight.bold, color: Colors.white, height: 1.0)),
//                 const SizedBox(width: 8),
//                 Padding(
//                   padding: const EdgeInsets.only(top: 30.0), // Adjust alignment
//                   child: Text("Km/h", style: GoogleFonts.notoSans(fontSize: 36, fontWeight: FontWeight.w500, color: Colors.white)),
//                 ),
//                 const SizedBox(width: 20),
//                 Image.asset(ICON_PATH_WIFI, height: 40),
//                 const SizedBox(width: 20), // Add spacing
//               ],
//             ),
//           ),
//         ),
//
//         // --- Right Cluster of Buttons ---
//         Row(
//           mainAxisSize: MainAxisSize.min,
//           children: [
//             _buildBottomBarButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)],
//                     () async {
//                   final proceed = await _showCustomConfirmationDialog(
//                     context: context,
//                     iconPath: ICON_PATH_EXIT,
//                     title: 'Information',
//                     titleColor: Colors.blue.shade700,
//                     content: 'Do you want to finish?',
//                   );
//                   if (proceed) _sendCommand('CMD_EXIT');
//                 }
//             ),
//           ],
//         )
//       ],
//     );
//   }
//
//   Widget _buildBottomBarButton(String label, String iconPath,
//       List<Color> gradientColors, VoidCallback onPressed) {
//
//     final screenHeight = MediaQuery.of(context).size.height;
//     final heightScale = screenHeight / 1080.0;
//
//     return GestureDetector(
//       onTap: onPressed,
//       child: Container(
//         height: 80 * heightScale,
//         padding: EdgeInsets.symmetric(horizontal: label.isEmpty ? 25 : 35),
//         decoration: BoxDecoration(
//           gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//           borderRadius: BorderRadius.circular(40 * heightScale),
//         ),
//         child: Row(
//           mainAxisAlignment: MainAxisAlignment.center,
//           crossAxisAlignment: CrossAxisAlignment.center,
//           children: [
//             Image.asset(iconPath, height: 36 * heightScale),
//             if (label.isNotEmpty) ...[
//               const SizedBox(width: 12),
//               Text(label, style: GoogleFonts.notoSans(fontSize: 36 * heightScale, fontWeight: FontWeight.bold, color: Colors.white)),
//             ]
//           ],
//         ),
//       ),
//     );
//   }
// }









import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter_vlc_player/flutter_vlc_player.dart';
import 'package:google_fonts/google_fonts.dart';

import 'icon_constants.dart';
import 'splash_screen.dart';
import 'settings_menu_page.dart';

// --- CONFIGURATION FOR ROBOT CONNECTION ---
const String ROBOT_IP_ADDRESS = '192.168.0.158';
const int ROBOT_COMMAND_PORT = 65432;

// --- MAIN APP ENTRY POINT ---
void main() {
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
  // All state variables and backend logic functions remain the same.
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
  bool _isStarted = false;
  bool _isForwardPressed = false;
  bool _isBackPressed = false;
  bool _isLeftPressed = false;
  bool _isRightPressed = false;
  bool _isAutoAttackMode = false;

  @override
  void initState() {
    super.initState();
    _switchCamera(0);
  }

  @override
  void dispose() {
    _vlcPlayerController?.dispose();
    super.dispose();
  }

  // --- All backend logic functions remain unchanged ---
  Future<void> _sendCommand(String command) async {
    try {
      final socket = await Socket.connect(ROBOT_IP_ADDRESS, ROBOT_COMMAND_PORT, timeout: const Duration(seconds: 5));
      socket.write(command);
      await socket.flush();
      socket.close();
      print('Command sent: $command');
    } catch (e) {
      print('Error sending command: $e');
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: const Text('Error: Could not connect to robot.'), backgroundColor: Colors.red));
    }
  }

  Future<void> _switchCamera(int index) async {
    final VlcPlayerController? oldController = _vlcPlayerController;
    setState(() {
      if (index == _currentCameraIndex && _vlcPlayerController != null) {} else if (index == _currentCameraIndex) { return; }
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
          VlcAdvancedOptions.networkCaching(120),
          VlcAdvancedOptions.clockJitter(0),
          VlcAdvancedOptions.fileCaching(30),
          VlcAdvancedOptions.liveCaching(30),
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
        setState(() { _errorMessage = state.errorDescription ?? 'An Unknown Error Occurred!'; });
      }
    });
    if (mounted) {
      setState(() { _vlcPlayerController = newController; });
    }
  }

  Future<void> _navigateToSettings() async {
    final newUrls = await Navigator.push<List<String>>(context, MaterialPageRoute(builder: (context) => SettingsMenuPage(cameraUrls: _cameraUrls)));
    if (newUrls != null) {
      setState(() => _cameraUrls = newUrls);
      _switchCamera(_currentCameraIndex);
    }
  }

  Future<bool> _showCustomConfirmationDialog({ required BuildContext context, required String iconPath, required String title, required Color titleColor, required String content}) async {
    return await showDialog<bool>(context: context, barrierDismissible: false, builder: (BuildContext dialogContext) {
      return Dialog(shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20.0)),
        child: Container(width: 500, padding: const EdgeInsets.all(24.0),
          child: Column(mainAxisSize: MainAxisSize.min, children: [
            Row(children: [ Image.asset(iconPath, height: 40, width: 40), const SizedBox(width: 16), Text(title, style: GoogleFonts.notoSans(fontSize: 28, fontWeight: FontWeight.bold, color: titleColor))]),
            const SizedBox(height: 24), Text(content, textAlign: TextAlign.center, style: GoogleFonts.notoSans(fontSize: 22, color: Colors.black87)), const SizedBox(height: 32),
            Row(mainAxisAlignment: MainAxisAlignment.spaceEvenly, children: [
              TextButton(onPressed: () => Navigator.of(dialogContext).pop(false), child: Text('Cancel', style: GoogleFonts.notoSans(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.grey.shade700))),
              SizedBox(height: 40, child: VerticalDivider(color: Colors.grey.shade400, thickness: 1)),
              TextButton(onPressed: () => Navigator.of(dialogContext).pop(true), child: Text('OK', style: GoogleFonts.notoSans(fontSize: 24, fontWeight: FontWeight.bold, color: Colors.blue.shade700))),
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
      if (proceed) { _sendCommand('CMD_MANU_ATTACK'); setState(() => _isAutoAttackMode = false); }
    } else {
      final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_MANUAL_ATTACK_ACTIVE, title: "DANGERS", titleColor: Colors.red, content: 'Are you sure you want to start\n"Auto Attack" mode?');
      if (proceed) { _sendCommand('CMD_AUTO_ATTACK'); setState(() => _isAutoAttackMode = true); }
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
          // All other side and directional buttons remain the same...
          _buildSideButton(30, 35, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE, _activeLeftButtonIndex == 0, () => _onLeftButtonPressed(0, 'CMD_MODE_DRIVE')),
          _buildSideButton(30, 185, "Petrol", ICON_PATH_PETROL_INACTIVE, ICON_PATH_PETROL_ACTIVE, _activeLeftButtonIndex == 1, () => _onLeftButtonPressed(1, 'CMD_MODE_PATROL')),
          _buildSideButton(30, 335, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE, _activeLeftButtonIndex == 2, () => _onLeftButtonPressed(2, 'CMD_MODE_RECON')),
          _buildSideButton(30, 485, _isAutoAttackMode ? "Auto Attack" : "Manual Attack", _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_INACTIVE : ICON_PATH_MANUAL_ATTACK_INACTIVE, _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_ACTIVE : ICON_PATH_MANUAL_ATTACK_ACTIVE, _activeLeftButtonIndex == 3, () { setState(() => _activeLeftButtonIndex = 3); _handleManualAutoAttackToggle(); }),
          _buildSideButton(30, 635, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE, _activeLeftButtonIndex == 4, () => _onLeftButtonPressed(4, 'CMD_MODE_DRONE')),
          _buildSideButton(30, 785, "Return", ICON_PATH_RETURN_INACTIVE, ICON_PATH_RETURN_ACTIVE, _activeLeftButtonIndex == 5, () => _onLeftButtonPressed(5, 'CMD_RETUR')),
          _buildSideButton(1690, 30, "Attack View", ICON_PATH_ATTACK_VIEW_INACTIVE, ICON_PATH_ATTACK_VIEW_ACTIVE, _activeRightButtonIndex == 0, () => _onRightButtonPressed(0)),
          _buildSideButton(1690, 250, "Top View", ICON_PATH_TOP_VIEW_INACTIVE, ICON_PATH_TOP_VIEW_ACTIVE, _activeRightButtonIndex == 1, () => _onRightButtonPressed(1)),
          _buildSideButton(1690, 470, "3d View", ICON_PATH_3D_VIEW_INACTIVE, ICON_PATH_3D_VIEW_ACTIVE, _activeRightButtonIndex == 2, () => _onRightButtonPressed(2)),
          _buildSideButton(1690, 720, "Setting", ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, false, () => _navigateToSettings()),
          _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE, 'CMD_FORWARD', () => setState(() => _isForwardPressed = true), () => setState(() => _isForwardPressed = false)),
          _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE, 'CMD_BACKWARD', () => setState(() => _isBackPressed = true), () => setState(() => _isBackPressed = false)),
          _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE, 'CMD_LEFT', () => setState(() => _isLeftPressed = true), () => setState(() => _isLeftPressed = false)),
          _buildDirectionalButton(1560, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE, 'CMD_RIGHT', () => setState(() => _isRightPressed = true), () => setState(() => _isRightPressed = false)),

          // --- FIX: This now uses a scrollable, robust layout ---
          Align(
            alignment: Alignment.bottomCenter,
            child: Padding(
              padding: EdgeInsets.only(bottom: 20 * heightScale),
              child: _buildBottomBar(),
            ),
          ),
        ],
      ),
    );
  }

  void _onLeftButtonPressed(int index, String command) {
    setState(() => _activeLeftButtonIndex = index);
    _sendCommand(command);
  }

  void _onRightButtonPressed(int index) {
    setState(() => _activeRightButtonIndex = index);
    _switchCamera(index);
  }

  Widget buildPlayerWidget() {
    if (_errorMessage != null) {
      return Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
        Padding(padding: const EdgeInsets.symmetric(horizontal: 24.0),
            child: Text('Error: $_errorMessage', textAlign: TextAlign.center, style: const TextStyle(color: Colors.red, fontSize: 16, fontWeight: FontWeight.bold))),
        const SizedBox(height: 20),
        ElevatedButton(onPressed: () => _switchCamera(_currentCameraIndex), style: ElevatedButton.styleFrom(backgroundColor: Colors.grey[700]), child: const Text('Retry', style: TextStyle(color: Colors.white))),
      ],
      ));
    }
    if (_vlcPlayerController == null) {
      return const Center(child: CircularProgressIndicator(color: Colors.white));
    }
    return VlcPlayer(controller: _vlcPlayerController!, aspectRatio: 16 / 9, placeholder: const Center(child: CircularProgressIndicator(color: Colors.white)));
  }

  Widget _buildSideButton(double left, double top, String label, String inactiveIcon, String activeIcon, bool isActive, VoidCallback onPressed) {
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
          padding: const EdgeInsets.all(8.0),
          decoration: BoxDecoration(color: Colors.black.withOpacity(0.6), borderRadius: BorderRadius.circular(10), border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.0)),
          child: FittedBox(
            fit: BoxFit.contain,
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Image.asset(isActive ? activeIcon : inactiveIcon, height: 50),
                const SizedBox(height: 10),
                Text(label, textAlign: TextAlign.center, style: GoogleFonts.notoSans(fontSize: 24, fontWeight: FontWeight.bold, color: const Color(0xFFFFFFFF))),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildDirectionalButton(double left, double top, bool isPressed, String inactiveIcon, String activeIcon, String command, VoidCallback onPress, VoidCallback onRelease) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    return Positioned(
      left: left * widthScale,
      top: top * heightScale,
      child: GestureDetector(
        onTapDown: (_) { onPress(); _sendCommand(command); },
        onTapUp: (_) { onRelease(); _sendCommand('CMD_STOP'); },
        onTapCancel: () => onRelease(),
        child: Image.asset(isPressed ? activeIcon : inactiveIcon, height: 100 * heightScale, width: 100 * widthScale, fit: BoxFit.contain),
      ),
    );
  }

  // --- FIX: This is the new, simple, and robust scrollable bottom bar ---
  Widget _buildBottomBar() {
    return SingleChildScrollView(
      scrollDirection: Axis.horizontal,
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16.0), // Add padding so it doesn't touch the edges
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            _buildBottomBarButton("ATTACK", ICON_PATH_ATTACK, [const Color(0xffc32121), const Color(0xff831616)],
                    () async {
                  final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_ATTACK, title: 'DANGERS', titleColor: Colors.red, content: 'Do you want to proceed the command?');
                  if (proceed) _sendCommand('CMD_MODE_ATTACK');
                }
            ),
            const SizedBox(width: 12),
            _buildBottomBarButton(_isStarted ? "STOP" : "START", _isStarted ? ICON_PATH_STOP : ICON_PATH_START, [const Color(0xff25a625), const Color(0xff127812)], () { setState(() => _isStarted = !_isStarted); _sendCommand(_isStarted ? 'CMD_STOP' : 'CMD_START'); }),
            const SizedBox(width: 12),
            _buildBottomBarButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_IN')),
            const SizedBox(width: 12),
            _buildBottomBarButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_OUT')),
            const SizedBox(width: 24),
            Row(
              crossAxisAlignment: CrossAxisAlignment.end,
              children: [
                Text("0", style: GoogleFonts.notoSans(fontSize: 80, fontWeight: FontWeight.bold, color: Colors.white, height: 1.0)),
                Padding(
                  padding: const EdgeInsets.only(bottom: 12.0),
                  child: Text("Km/h", style: GoogleFonts.notoSans(fontSize: 36, fontWeight: FontWeight.w500, color: Colors.white)),
                ),
              ],
            ),
            const SizedBox(width: 20),
            Image.asset(ICON_PATH_WIFI, height: 40),
            const SizedBox(width: 24),
            _buildBottomBarButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)],
                    () async {
                  final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_EXIT, title: 'Information', titleColor: Colors.blue.shade700, content: 'Do you want to finish?');
                  if (proceed) _sendCommand('CMD_EXIT');
                }
            ),
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
        padding: EdgeInsets.symmetric(horizontal: label.isEmpty ? 25 : 35),
        decoration: BoxDecoration(
          gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
          borderRadius: BorderRadius.circular(40 * heightScale),
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
