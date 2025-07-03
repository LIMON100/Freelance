// import 'dart:io';
// import 'package:flutter/material.dart';
// import 'package:flutter_vlc_player/flutter_vlc_player.dart';
// import 'package:google_fonts/google_fonts.dart';
//
// import 'icon_constants.dart';
// import 'splash_screen.dart';
//
// // --- CONFIGURATION FOR ROBOT CONNECTION ---
// const String ROBOT_IP_ADDRESS = '192.168.0.158';
// const int ROBOT_COMMAND_PORT = 65432;
//
// // --- SETTINGS PAGE (Can be moved to its own file later) ---
// class SettingsPage extends StatefulWidget {
//   final List<String> cameraUrls;
//   const SettingsPage({super.key, required this.cameraUrls});
//   @override
//   State<SettingsPage> createState() => _SettingsPageState();
// }
//
// class _SettingsPageState extends State<SettingsPage> {
//   late List<TextEditingController> _urlControllers;
//   @override
//   void initState() {
//     super.initState();
//     _urlControllers =
//         widget.cameraUrls.map((url) => TextEditingController(text: url)).toList();
//   }
//   @override
//   void dispose() {
//     for (var controller in _urlControllers) {
//       controller.dispose();
//     }
//     super.dispose();
//   }
//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       appBar: AppBar(
//           title: const Text('Settings'),
//           backgroundColor: Colors.grey[200],
//           foregroundColor: Colors.black87),
//       body: ListView(
//         padding: const EdgeInsets.all(16.0),
//         children: [
//           for (int i = 0; i < _urlControllers.length; i++)
//             Padding(
//               padding: const EdgeInsets.only(bottom: 16.0),
//               child: Column(
//                 crossAxisAlignment: CrossAxisAlignment.start,
//                 children: [
//                   Text('Camera ${i + 1} URL',
//                       style:
//                       const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
//                   const SizedBox(height: 8),
//                   TextFormField(
//                     controller: _urlControllers[i],
//                     decoration: const InputDecoration(
//                         border: OutlineInputBorder(), hintText: 'e.g., rtsp://...'),
//                   ),
//                 ],
//               ),
//             ),
//           const SizedBox(height: 20),
//           ElevatedButton(
//             onPressed: () {
//               final newUrls =
//               _urlControllers.map((controller) => controller.text).toList();
//               Navigator.pop(context, newUrls);
//             },
//             style: ElevatedButton.styleFrom(
//               padding: const EdgeInsets.symmetric(vertical: 16.0),
//               backgroundColor: Theme.of(context).primaryColor,
//               foregroundColor: Colors.white,
//             ),
//             child: const Text('Save Changes'),
//           ),
//         ],
//       ),
//     );
//   }
// }
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
// class HomePage extends StatefulWidget {
//   const HomePage({super.key});
//
//   @override
//   State<HomePage> createState() => _HomePageState();
// }
//
// class _HomePageState extends State<HomePage> {
//   // --- Backend State ---
//   VlcPlayerController? _vlcPlayerController;
//   List<String> _cameraUrls = [
//     'rtsp://192.168.0.158:8554/cam0',
//     'rtsp://192.168.0.158:8554/cam0',
//     'rtsp://192.168.0.158:8554/cam0',
//   ];
//   int _currentCameraIndex = -1;
//   String? _errorMessage;
//
//   // --- UI State ---
//   int _activeLeftButtonIndex = 0;
//   int _activeRightButtonIndex = 0;
//   bool _isStarted = false;
//
//   bool _isForwardPressed = false;
//   bool _isBackPressed = false;
//   bool _isLeftPressed = false;
//   bool _isRightPressed = false;
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
//   // --- BACKEND LOGIC ---
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
//   // Future<void> _switchCamera(int index) async {
//   //   if (index == _currentCameraIndex && _vlcPlayerController != null) return;
//   //   final VlcPlayerController? oldController = _vlcPlayerController;
//   //
//   //   setState(() {
//   //     _currentCameraIndex = index;
//   //     _vlcPlayerController = null;
//   //     _errorMessage = null;
//   //   });
//   //
//   //   await oldController?.dispose();
//   //   await Future.delayed(const Duration(milliseconds: 200));
//   //
//   //   final newController = VlcPlayerController.network(
//   //     _cameraUrls[index],
//   //     hwAcc: HwAcc.disabled,
//   //     autoPlay: true,
//   //     options: VlcPlayerOptions(
//   //       advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(150)]),
//   //       video: VlcVideoOptions(
//   //           [VlcVideoOptions.dropLateFrames(true), VlcVideoOptions.skipFrames(true)]),
//   //       extras: ['--h264-fps=60', '--no-audio'],
//   //     ),
//   //   );
//   //
//   //   newController.addListener(() {
//   //     if (!mounted) return;
//   //     if (newController.value.hasError &&
//   //         _errorMessage != newController.value.errorDescription) {
//   //       setState(() => _errorMessage = newController.value.errorDescription);
//   //     }
//   //   });
//   //
//   //   if (mounted) {
//   //     setState(() => _vlcPlayerController = newController);
//   //   }
//   // }
//
//   Future<void> _switchCamera(int index, {bool force = false}) async {
//     // If not forcing a retry, and the camera is the same and valid, do nothing.
//     if (!force && index == _currentCameraIndex && _vlcPlayerController != null) {
//       return;
//     }
//
//     // Immediately dispose of any existing controller.
//     await _vlcPlayerController?.dispose();
//
//     // Set UI to loading state
//     setState(() {
//       _currentCameraIndex = index;
//       _vlcPlayerController = null;
//       _errorMessage = null;
//     });
//
//     // A small delay to ensure the native view is fully gone.
//     await Future.delayed(const Duration(milliseconds: 100));
//
//     // Create the new controller.
//     final newController = VlcPlayerController.network(
//       _cameraUrls[index],
//       hwAcc: HwAcc.disabled,
//       autoPlay: true,
//       options: VlcPlayerOptions(
//         advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(200)]), // Slightly increased caching for stability
//         video: VlcVideoOptions([VlcVideoOptions.dropLateFrames(true), VlcVideoOptions.skipFrames(true)]),
//         extras: ['--h264-fps=60', '--no-audio'],
//       ),
//     );
//
//     // Add a one-time listener to check for initialization success or failure.
//     void listener() {
//       if (!mounted) return;
//       final value = newController.value;
//
//       // If an error occurs during initialization
//       if (value.hasError) {
//         // Clean up the failed controller immediately
//         newController.removeListener(listener);
//         newController.dispose();
//         // Update the UI to show the error and ensure the controller state is null
//         setState(() {
//           _errorMessage = value.errorDescription ?? 'Unknown connection error.';
//           _vlcPlayerController = null;
//         });
//       }
//       // If the player is initialized and playing, we can remove the listener.
//       else if (value.isPlaying) {
//         newController.removeListener(listener);
//       }
//     }
//
//     newController.addListener(listener);
//
//     // Assign the new controller to the state. The UI will show a spinner.
//     // The listener above will handle updating the UI on error.
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
//       MaterialPageRoute(builder: (context) => SettingsPage(cameraUrls: _cameraUrls)),
//     );
//     if (newUrls != null) {
//       setState(() => _cameraUrls = newUrls);
//       _switchCamera(_currentCameraIndex);
//     }
//   }
//
//   Future<bool> _showConfirmationDialog(
//       BuildContext context, String title, String content) async {
//     return await showDialog<bool>(
//       context: context,
//       builder: (BuildContext context) => AlertDialog(
//         title: Text(title, style: Theme.of(context).textTheme.headlineSmall),
//         content: Text(content),
//         actions: <Widget>[
//           TextButton(
//               onPressed: () => Navigator.of(context).pop(false),
//               child: const Text('Cancel')),
//           TextButton(
//               onPressed: () => Navigator.of(context).pop(true),
//               child: const Text('OK')),
//         ],
//       ),
//     ) ??
//         false;
//   }
//
//   // --- UI BUILD METHOD ---
//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       backgroundColor: Colors.black,
//       body: Stack(
//         fit: StackFit.expand,
//         children: [
//           // Layer 1: Video Player Background
//           Positioned.fill(child: buildPlayerWidget()),
//
//           // Layer 2: UI Controls Overlay
//
//           // Left Side Buttons
//           Positioned(
//             left: 15, top: 20, bottom: 80, width: 120,
//             child: ListView(
//               children: [
//                 _buildSideButton(0, _activeLeftButtonIndex == 0, ICON_PATH_DRIVING_ACTIVE, ICON_PATH_DRIVING_INACTIVE, "Driving", () => _onLeftButtonPressed(0, 'CMD_MODE_DRIVE')),
//                 const SizedBox(height: 20),
//                 _buildSideButton(1, _activeLeftButtonIndex == 1, ICON_PATH_PETROL_ACTIVE, ICON_PATH_PETROL_INACTIVE, "Petrol", () => _onLeftButtonPressed(1, 'CMD_MODE_PATROL')),
//                 const SizedBox(height: 20),
//                 _buildSideButton(2, _activeLeftButtonIndex == 2, ICON_PATH_RECON_ACTIVE, ICON_PATH_RECON_INACTIVE, "Recon", () => _onLeftButtonPressed(2, 'CMD_MODE_RECON')),
//                 const SizedBox(height: 20),
//                 _buildSideButton(3, _activeLeftButtonIndex == 3, ICON_PATH_MANUAL_ATTACK_ACTIVE, ICON_PATH_MANUAL_ATTACK_INACTIVE, "Manual Attack", () => _onLeftButtonPressed(3, 'CMD_MANU_ATTACK')),
//                 const SizedBox(height: 20),
//                 _buildSideButton(4, _activeLeftButtonIndex == 4, ICON_PATH_DRONE_ACTIVE, ICON_PATH_DRONE_INACTIVE, "Drone", () => _onLeftButtonPressed(4, 'CMD_MODE_DRONE')),
//                 const SizedBox(height: 20),
//                 _buildSideButton(5, _activeLeftButtonIndex == 5, ICON_PATH_RETURN_ACTIVE, ICON_PATH_RETURN_INACTIVE, "Return", () => _onLeftButtonPressed(5, 'CMD_RETURN')),
//               ],
//             ),
//           ),
//
//           // Right Side Buttons
//           Positioned(
//             right: 15, top: 20, bottom: 80, width: 120,
//             child: ListView(
//               children: [
//                 _buildSideButton(0, _activeRightButtonIndex == 0, ICON_PATH_ATTACK_VIEW_ACTIVE, ICON_PATH_ATTACK_VIEW_INACTIVE, "Attack View", () => _onRightButtonPressed(0)),
//                 const SizedBox(height: 20),
//                 _buildSideButton(1, _activeRightButtonIndex == 1, ICON_PATH_TOP_VIEW_ACTIVE, ICON_PATH_TOP_VIEW_INACTIVE, "Top View", () => _onRightButtonPressed(1)),
//                 const SizedBox(height: 20),
//                 _buildSideButton(2, _activeRightButtonIndex == 2, ICON_PATH_3D_VIEW_ACTIVE, ICON_PATH_3D_VIEW_INACTIVE, "3D View", () => _onRightButtonPressed(2)),
//                 const SizedBox(height: 120),
//                 _buildSideButton(-1, false, ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, "Setting", () => _navigateToSettings()),
//               ],
//             ),
//           ),
//
//           // --- NEW: Directional Control Buttons ---
//           _buildDirectionalButton(
//             alignment: Alignment.topCenter,
//             padding: const EdgeInsets.only(top: 20),
//             isPressed: _isForwardPressed,
//             activeIcon: ICON_PATH_FORWARD_ACTIVE,
//             inactiveIcon: ICON_PATH_FORWARD_INACTIVE,
//             command: 'CMD_FORWARD',
//             onPress: () => setState(() => _isForwardPressed = true),
//             onRelease: () => setState(() => _isForwardPressed = false),
//           ),
//           _buildDirectionalButton(
//             alignment: Alignment.bottomCenter,
//             padding: const EdgeInsets.only(bottom: 80), // Pushes it above the bottom bar
//             isPressed: _isBackPressed,
//             activeIcon: ICON_PATH_BACKWARD_ACTIVE,
//             inactiveIcon: ICON_PATH_BACKWARD_INACTIVE,
//             command: 'CMD_BACKWARD',
//             onPress: () => setState(() => _isBackPressed = true),
//             onRelease: () => setState(() => _isBackPressed = false),
//           ),
//           _buildDirectionalButton(
//             alignment: Alignment.centerLeft,
//             padding: const EdgeInsets.only(left: 140), // Pushes it away from the side buttons
//             isPressed: _isLeftPressed,
//             activeIcon: ICON_PATH_LEFT_ACTIVE,
//             inactiveIcon: ICON_PATH_LEFT_INACTIVE,
//             command: 'CMD_LEFT',
//             onPress: () => setState(() => _isLeftPressed = true),
//             onRelease: () => setState(() => _isLeftPressed = false),
//           ),
//           _buildDirectionalButton(
//             alignment: Alignment.centerRight,
//             padding: const EdgeInsets.only(right: 140), // Pushes it away from the side buttons
//             isPressed: _isRightPressed,
//             activeIcon: ICON_PATH_RIGHT_ACTIVE,
//             inactiveIcon: ICON_PATH_RIGHT_INACTIVE,
//             command: 'CMD_RIGHT',
//             onPress: () => setState(() => _isRightPressed = true),
//             onRelease: () => setState(() => _isRightPressed = false),
//           ),
//
//           // Bottom Control Bar (already scrollable, no changes needed)
//           Positioned(
//             bottom: 10,
//             left: 15,
//             right: 15,
//             child: SingleChildScrollView(
//               scrollDirection: Axis.horizontal,
//               child: Row(
//                 children: [
//                   _buildBottomButton("ATTACK", ICON_PATH_ATTACK, [const Color(0xffc32121), const Color(0xff831616)], () async {
//                     final proceed = await _showConfirmationDialog(context, 'Confirm Attack', 'Do you want to proceed?');
//                     if (proceed) _sendCommand('CMD_MODE_ATTACK');
//                   }),
//                   const SizedBox(width: 12),
//                   _buildBottomButton(_isStarted ? "STOP" : "START", _isStarted ? ICON_PATH_STOP : ICON_PATH_START, [const Color(0xff25a625), const Color(0xff127812)], () {
//                     setState(() => _isStarted = !_isStarted);
//                     _sendCommand(_isStarted ? 'CMD_STOP' : 'CMD_START');
//                   }),
//                   const SizedBox(width: 12),
//                   _buildBottomButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_IN')),
//                   const SizedBox(width: 12),
//                   _buildBottomButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_OUT')),
//                   const SizedBox(width: 12),
//                   Image.asset(ICON_PATH_WIFI, height: 40, width: 40),
//                   const SizedBox(width: 12),
//                   _buildBottomButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)], () => _sendCommand('CMD_EXIT')),
//                 ],
//               ),
//             ),
//           )
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
//   void _onRightButtonPressed(int index) {
//     setState(() => _activeRightButtonIndex = index);
//     _switchCamera(index);
//   }
//
//   // --- FIXED: Player widget with Retry button ---
//   // Widget buildPlayerWidget() {
//   //   if (_vlcPlayerController == null) {
//   //     return const Center(child: CircularProgressIndicator(color: Colors.white));
//   //   }
//   //   if (_errorMessage != null) {
//   //     return Center(
//   //       child: Column(
//   //         mainAxisAlignment: MainAxisAlignment.center,
//   //         children: [
//   //           Padding(
//   //             padding: const EdgeInsets.symmetric(horizontal: 24.0),
//   //             child: Text(
//   //               'Error: $_errorMessage',
//   //               textAlign: TextAlign.center,
//   //               style: const TextStyle(
//   //                   color: Colors.red,
//   //                   fontSize: 16,
//   //                   fontWeight: FontWeight.bold),
//   //             ),
//   //           ),
//   //           const SizedBox(height: 20),
//   //           ElevatedButton(
//   //             onPressed: () => _switchCamera(_currentCameraIndex),
//   //             style: ElevatedButton.styleFrom(backgroundColor: Colors.grey[700]),
//   //             child: const Text('Retry', style: TextStyle(color: Colors.white)),
//   //           ),
//   //         ],
//   //       ),
//   //     );
//   //   }
//   //   return VlcPlayer(
//   //     controller: _vlcPlayerController!,
//   //     aspectRatio: 16 / 9,
//   //     placeholder: const Center(child: CircularProgressIndicator(color: Colors.white)),
//   //   );
//   // }
//
//   Widget buildPlayerWidget() {
//     // If there is an error message, show the error UI.
//     // The controller is guaranteed to be null in this state now.
//     if (_errorMessage != null) {
//       return Center(
//         child: Column(
//           mainAxisAlignment: MainAxisAlignment.center,
//           children: [
//             Padding(
//               padding: const EdgeInsets.symmetric(horizontal: 24.0),
//               child: Text(
//                 'Error: $_errorMessage',
//                 textAlign: TextAlign.center,
//                 style: const TextStyle(
//                     color: Colors.red,
//                     fontSize: 16,
//                     fontWeight: FontWeight.bold),
//               ),
//             ),
//             const SizedBox(height: 20),
//             // Call _switchCamera. The `force` parameter is no longer needed
//             // because the logic now correctly handles retries.
//             ElevatedButton(
//               onPressed: () => _switchCamera(_currentCameraIndex),
//               style: ElevatedButton.styleFrom(backgroundColor: Colors.grey[700]),
//               child: const Text('Retry', style: TextStyle(color: Colors.white)),
//             ),
//           ],
//         ),
//       );
//     }
//
//     // If the controller is null and there is no error, we are loading.
//     if (_vlcPlayerController == null) {
//       return const Center(child: CircularProgressIndicator(color: Colors.white));
//     }
//
//     // Otherwise, show the player.
//     return VlcPlayer(
//       controller: _vlcPlayerController!,
//       aspectRatio: 16 / 9,
//       placeholder: const Center(child: CircularProgressIndicator(color: Colors.white)),
//     );
//   }
//
//
//   Widget _buildDirectionalButton({
//     required Alignment alignment,
//     required EdgeInsets padding,
//     required bool isPressed,
//     required String activeIcon,
//     required String inactiveIcon,
//     required String command,
//     required VoidCallback onPress,
//     required VoidCallback onRelease,
//   }) {
//     return Padding(
//       padding: padding,
//       child: Align(
//         alignment: alignment,
//         child: GestureDetector(
//           onTapDown: (_) {
//             onPress();
//             _sendCommand(command);
//           },
//           onTapUp: (_) => onRelease(),
//           onTapCancel: () => onRelease(),
//           child: Image.asset(
//             isPressed ? activeIcon : inactiveIcon,
//             height: 60,
//             width: 60,
//           ),
//         ),
//       ),
//     );
//   }
//
//
//   Widget _buildSideButton(int index, bool isActive, String activeIcon,
//       String inactiveIcon, String label, VoidCallback onPressed) {
//     return GestureDetector(
//       onTap: onPressed,
//       child: Container(
//         padding: const EdgeInsets.symmetric(vertical: 8, horizontal: 12),
//         decoration: BoxDecoration(
//             color: Colors.black.withOpacity(0.6),
//             borderRadius: BorderRadius.circular(10),
//             border: Border.all(
//                 color: isActive ? Colors.white : Colors.transparent,
//                 width: 1.5)),
//         child: Column(
//           mainAxisSize: MainAxisSize.min,
//           children: [
//             Image.asset(isActive ? activeIcon : inactiveIcon,
//                 height: 40, width: 40),
//             const SizedBox(height: 5),
//             Text(label,
//                 style: const TextStyle(
//                     color: Colors.white,
//                     fontSize: 14,
//                     fontWeight: FontWeight.bold)),
//           ],
//         ),
//       ),
//     );
//   }
//
//   Widget _buildBottomButton(String label, String iconPath,
//       List<Color> gradientColors, VoidCallback onPressed) {
//     return GestureDetector(
//       onTap: onPressed,
//       child: Container(
//         padding: EdgeInsets.symmetric(
//             horizontal: label.isNotEmpty ? 16 : 12, vertical: 10),
//         decoration: BoxDecoration(
//             gradient: LinearGradient(
//                 colors: gradientColors,
//                 begin: Alignment.topCenter,
//                 end: Alignment.bottomCenter),
//             borderRadius: BorderRadius.circular(30),
//             boxShadow: [
//               BoxShadow(
//                   color: Colors.black.withOpacity(0.5),
//                   blurRadius: 5,
//                   offset: const Offset(0, 3))
//             ]),
//         child: Row(
//           mainAxisSize: MainAxisSize.min,
//           children: [
//             Image.asset(iconPath, height: 24, width: 24),
//             if (label.isNotEmpty) ...[
//               const SizedBox(width: 8),
//               Text(label,
//                   style: const TextStyle(
//                       color: Colors.white,
//                       fontSize: 18,
//                       fontWeight: FontWeight.bold,
//                       letterSpacing: 1.2),
//               ),
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

// --- CONFIGURATION FOR ROBOT CONNECTION ---
const String ROBOT_IP_ADDRESS = '192.168.0.158';
const int ROBOT_COMMAND_PORT = 65432;

// --- SETTINGS PAGE (Can be moved to its own file later) ---
class SettingsPage extends StatefulWidget {
  final List<String> cameraUrls;
  const SettingsPage({super.key, required this.cameraUrls});
  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  late List<TextEditingController> _urlControllers;
  @override
  void initState() {
    super.initState();
    _urlControllers =
        widget.cameraUrls.map((url) => TextEditingController(text: url)).toList();
  }
  @override
  void dispose() {
    for (var controller in _urlControllers) {
      controller.dispose();
    }
    super.dispose();
  }
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
          title: const Text('Settings'),
          backgroundColor: Colors.grey[200],
          foregroundColor: Colors.black87),
      body: ListView(
        padding: const EdgeInsets.all(16.0),
        children: [
          for (int i = 0; i < _urlControllers.length; i++)
            Padding(
              padding: const EdgeInsets.only(bottom: 16.0),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('Camera ${i + 1} URL',
                      style:
                      const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                  const SizedBox(height: 8),
                  TextFormField(
                    controller: _urlControllers[i],
                    decoration: const InputDecoration(
                        border: OutlineInputBorder(), hintText: 'e.g., rtsp://...'),
                  ),
                ],
              ),
            ),
          const SizedBox(height: 20),
          ElevatedButton(
            onPressed: () {
              final newUrls =
              _urlControllers.map((controller) => controller.text).toList();
              Navigator.pop(context, newUrls);
            },
            style: ElevatedButton.styleFrom(
              padding: const EdgeInsets.symmetric(vertical: 16.0),
              backgroundColor: Theme.of(context).primaryColor,
              foregroundColor: Colors.white,
            ),
            child: const Text('Save Changes'),
          ),
        ],
      ),
    );
  }
}

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
  // --- Backend State ---
  VlcPlayerController? _vlcPlayerController;
  List<String> _cameraUrls = [
    'rtsp://192.168.0.158:8554/cam0',
    'rtsp://192.168.0.158:8554/cam0',
    'rtsp://192.168.0.158:8554/cam0',
  ];
  int _currentCameraIndex = -1;
  String? _errorMessage;

  // --- UI State ---
  int _activeLeftButtonIndex = 0;
  int _activeRightButtonIndex = 0;
  bool _isStarted = false;

  bool _isForwardPressed = false;
  bool _isBackPressed = false;
  bool _isLeftPressed = false;
  bool _isRightPressed = false;

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

  // --- BACKEND LOGIC ---
  Future<void> _sendCommand(String command) async {
    try {
      final socket = await Socket.connect(ROBOT_IP_ADDRESS, ROBOT_COMMAND_PORT,
          timeout: const Duration(seconds: 5));
      socket.write(command);
      await socket.flush();
      socket.close();
      print('Command sent: $command');
    } catch (e) {
      print('Error sending command: $e');
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
            content: const Text('Error: Could not connect to robot.'),
            backgroundColor: Colors.red),
      );
    }
  }

  // Future<void> _switchCamera(int index) async {
  //   if (index == _currentCameraIndex && _vlcPlayerController != null) return;
  //   final VlcPlayerController? oldController = _vlcPlayerController;
  //
  //   setState(() {
  //     _currentCameraIndex = index;
  //     _vlcPlayerController = null;
  //     _errorMessage = null;
  //   });
  //
  //   await oldController?.dispose();
  //   await Future.delayed(const Duration(milliseconds: 200));
  //
  //   final newController = VlcPlayerController.network(
  //     _cameraUrls[index],
  //     hwAcc: HwAcc.disabled,
  //     autoPlay: true,
  //     options: VlcPlayerOptions(
  //       advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(150)]),
  //       video: VlcVideoOptions(
  //           [VlcVideoOptions.dropLateFrames(true), VlcVideoOptions.skipFrames(true)]),
  //       extras: ['--h264-fps=60', '--no-audio'],
  //     ),
  //   );
  //
  //   newController.addListener(() {
  //     if (!mounted) return;
  //     if (newController.value.hasError &&
  //         _errorMessage != newController.value.errorDescription) {
  //       setState(() => _errorMessage = newController.value.errorDescription);
  //     }
  //   });
  //
  //   if (mounted) {
  //     setState(() => _vlcPlayerController = newController);
  //   }
  // }

  Future<void> _switchCamera(int index, {bool force = false}) async {
    // If not forcing a retry, and the camera is the same and valid, do nothing.
    if (!force && index == _currentCameraIndex && _vlcPlayerController != null) {
      return;
    }

    // Immediately dispose of any existing controller.
    await _vlcPlayerController?.dispose();

    // Set UI to loading state
    setState(() {
      _currentCameraIndex = index;
      _vlcPlayerController = null;
      _errorMessage = null;
    });

    // A small delay to ensure the native view is fully gone.
    await Future.delayed(const Duration(milliseconds: 100));

    // Create the new controller.
    final newController = VlcPlayerController.network(
      _cameraUrls[index],
      hwAcc: HwAcc.disabled,
      autoPlay: true,
      options: VlcPlayerOptions(
        advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(200)]), // Slightly increased caching for stability
        video: VlcVideoOptions([VlcVideoOptions.dropLateFrames(true), VlcVideoOptions.skipFrames(true)]),
        extras: ['--h264-fps=60', '--no-audio'],
      ),
    );

    // Add a one-time listener to check for initialization success or failure.
    void listener() {
      if (!mounted) return;
      final value = newController.value;

      // If an error occurs during initialization
      if (value.hasError) {
        // Clean up the failed controller immediately
        newController.removeListener(listener);
        newController.dispose();
        // Update the UI to show the error and ensure the controller state is null
        setState(() {
          _errorMessage = value.errorDescription ?? 'Unknown connection error.';
          _vlcPlayerController = null;
        });
      }
      // If the player is initialized and playing, we can remove the listener.
      else if (value.isPlaying) {
        newController.removeListener(listener);
      }
    }

    newController.addListener(listener);

    // Assign the new controller to the state. The UI will show a spinner.
    // The listener above will handle updating the UI on error.
    if (mounted) {
      setState(() {
        _vlcPlayerController = newController;
      });
    }
  }

  Future<void> _navigateToSettings() async {
    final newUrls = await Navigator.push<List<String>>(
      context,
      MaterialPageRoute(builder: (context) => SettingsPage(cameraUrls: _cameraUrls)),
    );
    if (newUrls != null) {
      setState(() => _cameraUrls = newUrls);
      _switchCamera(_currentCameraIndex);
    }
  }

  Future<bool> _showConfirmationDialog(
      BuildContext context, String title, String content) async {
    return await showDialog<bool>(
      context: context,
      builder: (BuildContext context) => AlertDialog(
        title: Text(title, style: Theme.of(context).textTheme.headlineSmall),
        content: Text(content),
        actions: <Widget>[
          TextButton(
              onPressed: () => Navigator.of(context).pop(false),
              child: const Text('Cancel')),
          TextButton(
              onPressed: () => Navigator.of(context).pop(true),
              child: const Text('OK')),
        ],
      ),
    ) ??
        false;
  }

  // --- UI BUILD METHOD ---
  @override
  Widget build(BuildContext context) {
    // For absolute positioning, we need the screen size.
    // The design is based on 1920x1080.
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    return Scaffold(
      backgroundColor: Colors.black,
      body: Stack(
        children: [
          // Layer 1: Video Player Background
          Positioned.fill(child: buildPlayerWidget()),

          // --- Layer 2: All UI Controls Positioned Absolutely ---

          // --- Left Side Buttons ---
          _buildSideButton(30, 35, 90, 65, 130, 122, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE, _activeLeftButtonIndex == 0, () => _onLeftButtonPressed(0, 'CMD_MODE_DRIVE')),
          _buildSideButton(30, 185, 90, 215, 130, 272, "Petrol", ICON_PATH_PETROL_INACTIVE, ICON_PATH_PETROL_ACTIVE, _activeLeftButtonIndex == 1, () => _onLeftButtonPressed(1, 'CMD_MODE_PATROL')),
          _buildSideButton(30, 335, 90, 365, 130, 422, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE, _activeLeftButtonIndex == 2, () => _onLeftButtonPressed(2, 'CMD_MODE_RECON')),
          _buildSideButton(30, 485, 90, 515, 130, 572, "Manual Attack", ICON_PATH_MANUAL_ATTACK_INACTIVE, ICON_PATH_MANUAL_ATTACK_ACTIVE, _activeLeftButtonIndex == 3, () => _onLeftButtonPressed(3, 'CMD_MANU_ATTACK')),
          _buildSideButton(30, 635, 90, 665, 130, 722, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE, _activeLeftButtonIndex == 4, () => _onLeftButtonPressed(4, 'CMD_MODE_DRONE')),
          _buildSideButton(30, 785, 90, 815, 130, 872, "Return", ICON_PATH_RETURN_INACTIVE, ICON_PATH_RETURN_ACTIVE, _activeLeftButtonIndex == 5, () => _onLeftButtonPressed(5, 'CMD_RETUR')),

          // --- Right Side Buttons ---
          _buildSideButton(1690, 30, 1740, 60, 1790, 177, "Attack View", ICON_PATH_ATTACK_VIEW_INACTIVE, ICON_PATH_ATTACK_VIEW_ACTIVE, _activeRightButtonIndex == 0, () => _onRightButtonPressed(0, 'CMD_ATTACK_VIEW')),
          _buildSideButton(1690, 250, 1740, 280, 1790, 397, "Top View", ICON_PATH_TOP_VIEW_INACTIVE, ICON_PATH_TOP_VIEW_ACTIVE, _activeRightButtonIndex == 1, () => _onRightButtonPressed(1, 'CMD_TOP_VIEW')),
          _buildSideButton(1690, 470, 1740, 500, 1790, 617, "3D View", ICON_PATH_3D_VIEW_INACTIVE, ICON_PATH_3D_VIEW_ACTIVE, _activeRightButtonIndex == 2, () => _onRightButtonPressed(2, 'CMD_FRONT_3D')),
          _buildSideButton(1690, 720, 1740, 750, 1790, 867, "Setting", ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, false, () => _navigateToSettings()),

          // --- Directional Arrow Buttons ---
          _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE, 'CMD_FORWARD', () => setState(() => _isForwardPressed = true), () => setState(() => _isForwardPressed = false)),
          _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE, 'CMD_BACKWARD', () => setState(() => _isBackPressed = true), () => setState(() => _isBackPressed = false)),
          _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE, 'CMD_LEFT', () => setState(() => _isLeftPressed = true), () => setState(() => _isLeftPressed = false)),
          _buildDirectionalButton(1560, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE, 'CMD_RIGHT', () => setState(() => _isRightPressed = true), () => setState(() => _isRightPressed = false)),

          // --- Bottom Bar ---
          _buildBottomBarButton(30, 970, 128, 1000, "ATTACK", ICON_PATH_ATTACK, [const Color(0xffc32121), const Color(0xff831616)], () => _sendCommand('CMD_MODE_ATTACK')),
          _buildBottomBarButton(198, 970, 288, 996, _isStarted ? "STOP" : "START", _isStarted ? ICON_PATH_STOP : ICON_PATH_START, [const Color(0xff25a625), const Color(0xff127812)], () { setState(() => _isStarted = !_isStarted); _sendCommand(_isStarted ? 'CMD_STOP' : 'CMD_START'); }),
          _buildBottomBarButton(450, 970, 0, 0, "", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_IN')),
          _buildBottomBarButton(576, 970, 0, 0, "", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_OUT')),
          Positioned(left: 930 * widthScale, top: 978 * heightScale, child: Image.asset(ICON_PATH_WIFI, height: 40 * heightScale, width: 40 * widthScale)),
          _buildBottomBarButton(1690, 970, 1769, 986, "EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)], () => _sendCommand('CMD_EXIT')),

          // --- Bottom Status Labels ---
          _buildStatusLabel(1392, 951, "0", 80, true, widthScale, heightScale),
          _buildStatusLabel(1400, 995, "Km/h", 36, false, widthScale, heightScale),
        ],
      ),
    );
  }

  // --- UI STATE HANDLERS ---
  void _onLeftButtonPressed(int index, String command) {
    setState(() => _activeLeftButtonIndex = index);
    _sendCommand(command);
  }

  void _onRightButtonPressed(int index, String command) {
    setState(() => _activeRightButtonIndex = index);
    // Note: Right buttons in this design don't switch cameras, they send commands.
    _sendCommand(command);
  }

  // --- FIXED: Player widget with Retry button ---
  // Widget buildPlayerWidget() {
  //   if (_vlcPlayerController == null) {
  //     return const Center(child: CircularProgressIndicator(color: Colors.white));
  //   }
  //   if (_errorMessage != null) {
  //     return Center(
  //       child: Column(
  //         mainAxisAlignment: MainAxisAlignment.center,
  //         children: [
  //           Padding(
  //             padding: const EdgeInsets.symmetric(horizontal: 24.0),
  //             child: Text(
  //               'Error: $_errorMessage',
  //               textAlign: TextAlign.center,
  //               style: const TextStyle(
  //                   color: Colors.red,
  //                   fontSize: 16,
  //                   fontWeight: FontWeight.bold),
  //             ),
  //           ),
  //           const SizedBox(height: 20),
  //           ElevatedButton(
  //             onPressed: () => _switchCamera(_currentCameraIndex),
  //             style: ElevatedButton.styleFrom(backgroundColor: Colors.grey[700]),
  //             child: const Text('Retry', style: TextStyle(color: Colors.white)),
  //           ),
  //         ],
  //       ),
  //     );
  //   }
  //   return VlcPlayer(
  //     controller: _vlcPlayerController!,
  //     aspectRatio: 16 / 9,
  //     placeholder: const Center(child: CircularProgressIndicator(color: Colors.white)),
  //   );
  // }

  Widget buildPlayerWidget() {
    // If there is an error message, show the error UI.
    // The controller is guaranteed to be null in this state now.
    if (_errorMessage != null) {
      return Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 24.0),
              child: Text(
                'Error: $_errorMessage',
                textAlign: TextAlign.center,
                style: const TextStyle(
                    color: Colors.red,
                    fontSize: 16,
                    fontWeight: FontWeight.bold),
              ),
            ),
            const SizedBox(height: 20),
            // Call _switchCamera. The `force` parameter is no longer needed
            // because the logic now correctly handles retries.
            ElevatedButton(
              onPressed: () => _switchCamera(_currentCameraIndex),
              style: ElevatedButton.styleFrom(backgroundColor: Colors.grey[700]),
              child: const Text('Retry', style: TextStyle(color: Colors.white)),
            ),
          ],
        ),
      );
    }

    // If the controller is null and there is no error, we are loading.
    if (_vlcPlayerController == null) {
      return const Center(child: CircularProgressIndicator(color: Colors.white));
    }

    // Otherwise, show the player.
    return VlcPlayer(
      controller: _vlcPlayerController!,
      aspectRatio: 16 / 9,
      placeholder: const Center(child: CircularProgressIndicator(color: Colors.white)),
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
        onTapDown: (_) {
          onPress();
          _sendCommand(command); // Send command on press
        },
        onTapUp: (_) {
          onRelease();
          _sendCommand('CMD_STOP'); // Send a general stop command on release
        },
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


  Widget _buildSideButton(double left, double top, double iconLeft, double iconTop, double textLeft, double textTop, String label, String inactiveIcon, String activeIcon, bool isActive, VoidCallback onPressed) {
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
          decoration: BoxDecoration(
            color: Colors.black.withOpacity(0.6),
            borderRadius: BorderRadius.circular(10),
            border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.0),
          ),
          child: Stack(
            children: [
              Positioned(
                left: (iconLeft - left) * widthScale,
                top: (iconTop - top) * heightScale,
                child: Image.asset(isActive ? activeIcon : inactiveIcon, height: 50 * heightScale),
              ),
              Positioned(
                left: (textLeft - left) * widthScale,
                top: (textTop - top) * heightScale,
                child: Text(
                  label,
                  style: GoogleFonts.notoSans(
                    fontSize: 24 * heightScale,
                    fontWeight: FontWeight.bold,
                    color: const Color(0xFFFFFFFF),
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildBottomBarButton(double left, double top, double textLeft, double textTop, String label, String iconPath, List<Color> gradientColors, VoidCallback onPressed) {
    final screenWidth = MediaQuery.of(context).size.width;
    final screenHeight = MediaQuery.of(context).size.height;
    final widthScale = screenWidth / 1920.0;
    final heightScale = screenHeight / 1080.0;

    // Use a Stack to layer the icon and text over the container
    return Positioned(
      left: left * widthScale,
      top: top * heightScale,
      child: GestureDetector(
        onTap: onPressed,
        child: Container(
          width: (textLeft > 0 ? (textLeft - left) * 2 : 100) * widthScale, // Estimate width
          height: 80 * heightScale,
          decoration: BoxDecoration(
            gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
            borderRadius: BorderRadius.circular(40 * heightScale),
          ),
          child: Stack(
            alignment: Alignment.center,
            children: [
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Image.asset(iconPath, height: 36 * heightScale),
                  if(label.isNotEmpty) SizedBox(width: 8 * widthScale),
                  if(label.isNotEmpty)
                    Text(
                      label,
                      style: GoogleFonts.notoSans(
                        fontSize: 36 * heightScale,
                        fontWeight: FontWeight.bold,
                        color: Colors.white,
                      ),
                    )
                ],
              )
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildStatusLabel(double left, double top, String text, double fontSize, bool isBold, double widthScale, double heightScale) {
    return Positioned(
      left: left * widthScale,
      top: top * heightScale,
      child: Text(
        text,
        textAlign: isBold ? TextAlign.right : TextAlign.left,
        style: GoogleFonts.notoSans(
          fontSize: fontSize * heightScale,
          fontWeight: isBold ? FontWeight.bold : FontWeight.w500, // w500 is Medium
          color: Colors.white,
        ),
      ),
    );
  }


  Widget _buildBottomButton(String label, String iconPath,
      List<Color> gradientColors, VoidCallback onPressed) {
    return GestureDetector(
      onTap: onPressed,
      child: Container(
        padding: EdgeInsets.symmetric(
            horizontal: label.isNotEmpty ? 16 : 12, vertical: 10),
        decoration: BoxDecoration(
            gradient: LinearGradient(
                colors: gradientColors,
                begin: Alignment.topCenter,
                end: Alignment.bottomCenter),
            borderRadius: BorderRadius.circular(30),
            boxShadow: [
              BoxShadow(
                  color: Colors.black.withOpacity(0.5),
                  blurRadius: 5,
                  offset: const Offset(0, 3))
            ]),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Image.asset(iconPath, height: 24, width: 24),
            if (label.isNotEmpty) ...[
              const SizedBox(width: 8),
              Text(label,
                style: const TextStyle(
                    color: Colors.white,
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                    letterSpacing: 1.2),
              ),
            ]
          ],
        ),
      ),
    );
  }
}
