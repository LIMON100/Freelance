// import 'dart:io';
// import 'package:flutter/material.dart';
// import 'package:flutter/services.dart';
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
//   WidgetsFlutterBinding.ensureInitialized();
//
//   // 3. Set the full-screen mode and orientation right at the start
//   SystemChrome.setEnabledSystemUIMode(SystemUiMode.immersiveSticky);
//   SystemChrome.setPreferredOrientations([
//     DeviceOrientation.landscapeLeft,
//     DeviceOrientation.landscapeRight,
//   ]);
//
//
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
//   // All state variables and backend logic functions remain the same.
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
//   bool _isAttackModeOn = false; // NEW: Add this variable to track attack state
//   final List<Color> _attackInactiveColors = [const Color(0xffc32121), const Color(0xff831616)];
//   final List<Color> _attackActiveColors = [const Color(0xFF424242), const Color(0xFF212121)]; // Dark Gray gradient
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
//   // --- All backend logic functions remain unchanged ---
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
//       ScaffoldMessenger.of(context).showSnackBar(SnackBar(
//           content: const Text('Error: Could not connect to robot.'),
//           backgroundColor: Colors.red)
//       );
//     }
//   }
//
//   Future<void> _switchCamera(int index) async {
//     final VlcPlayerController? oldController = _vlcPlayerController;
//     setState(() {
//       if (index == _currentCameraIndex && _vlcPlayerController != null) {} else
//       if (index == _currentCameraIndex) {
//         return;
//       }
//       _currentCameraIndex = index;
//       _vlcPlayerController = null;
//       _errorMessage = null;
//     });
//     await oldController?.dispose();
//     await Future.delayed(const Duration(milliseconds: 200));
//     // final newController = VlcPlayerController.network(_cameraUrls[index], hwAcc: HwAcc.disabled, autoPlay: true, options: VlcPlayerOptions(advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(150)]), video: VlcVideoOptions([VlcVideoOptions.dropLateFrames(true), VlcVideoOptions.skipFrames(true)]), extras: ['--h264-fps=60', '--no-audio']));
//     final newController = VlcPlayerController.network(
//       _cameraUrls[index],
//       hwAcc: HwAcc.disabled,
//       autoPlay: true,
//       options: VlcPlayerOptions(
//         // advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(150)]),
//         advanced: VlcAdvancedOptions([
//           VlcAdvancedOptions.networkCaching(50),
//           VlcAdvancedOptions.clockJitter(0),
//           VlcAdvancedOptions.fileCaching(0),
//           VlcAdvancedOptions.liveCaching(0),
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
//           _errorMessage =
//               state.errorDescription ?? 'An Unknown Error Occurred!';
//         });
//       }
//     });
//     if (mounted) {
//       setState(() {
//         _vlcPlayerController = newController;
//       });
//     }
//   }
//
//   Future<void> _navigateToSettings() async {
//     final newUrls = await Navigator.push<List<String>>(context,
//         MaterialPageRoute(
//             builder: (context) => SettingsMenuPage(cameraUrls: _cameraUrls)));
//     if (newUrls != null) {
//       setState(() => _cameraUrls = newUrls);
//       _switchCamera(_currentCameraIndex);
//     }
//   }
//
//   Future<bool> _showCustomConfirmationDialog(
//       { required BuildContext context, required String iconPath, required String title, required Color titleColor, required String content}) async {
//     return await showDialog<bool>(
//       context: context,
//       barrierDismissible: false,
//       builder: (BuildContext dialogContext) {
//         return Dialog(shape: RoundedRectangleBorder(
//             borderRadius: BorderRadius.circular(20.0)),
//           child: Container(width: 500, padding: const EdgeInsets.all(24.0),
//             child: Column(mainAxisSize: MainAxisSize.min, children: [
//               Row(children: [
//                 Image.asset(iconPath, height: 40, width: 40),
//                 const SizedBox(width: 16),
//                 Text(title, style: GoogleFonts.notoSans(fontSize: 28,
//                     fontWeight: FontWeight.bold,
//                     color: titleColor))
//               ]),
//               const SizedBox(height: 24),
//               Text(content, textAlign: TextAlign.center,
//                   style: GoogleFonts.notoSans(
//                       fontSize: 22, color: Colors.black87)),
//               const SizedBox(height: 32),
//               Row(mainAxisAlignment: MainAxisAlignment.spaceEvenly, children: [
//                 TextButton(
//                     onPressed: () => Navigator.of(dialogContext).pop(false),
//                     child: Text('Cancel', style: GoogleFonts.notoSans(
//                         fontSize: 24, fontWeight: FontWeight.bold, color: Colors
//                         .grey.shade700))),
//                 SizedBox(height: 40,
//                     child: VerticalDivider(
//                         color: Colors.grey.shade400, thickness: 1)),
//                 TextButton(
//                     onPressed: () => Navigator.of(dialogContext).pop(true),
//                     child: Text('OK', style: GoogleFonts.notoSans(
//                         fontSize: 24, fontWeight: FontWeight.bold, color: Colors
//                         .blue.shade700))),
//               ],
//               ),
//             ],
//             ),
//           ),
//         );
//       },
//     ) ?? false;
//   }
//
//   Future<void> _handleManualAutoAttackToggle() async {
//     if (_isAutoAttackMode) {
//       final proceed = await _showCustomConfirmationDialog(context: context,
//           iconPath: ICON_PATH_AUTO_ATTACK_ACTIVE,
//           title: "DANGERS",
//           titleColor: Colors.red,
//           content: 'Are you sure you want to stop\n"Auto Attack" mode?');
//       if (proceed) {
//         _sendCommand('CMD_MANU_ATTACK');
//         setState(() => _isAutoAttackMode = false);
//       }
//     } else {
//       final proceed = await _showCustomConfirmationDialog(context: context,
//           iconPath: ICON_PATH_MANUAL_ATTACK_ACTIVE,
//           title: "DANGERS",
//           titleColor: Colors.red,
//           content: 'Are you sure you want to start\n"Auto Attack" mode?');
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
//           // --- Left Side Buttons (Now use the dedicated left button widget) ---
//           _buildLeftButton(30, 35, "Driving", ICON_PATH_DRIVING_INACTIVE, ICON_PATH_DRIVING_ACTIVE, _activeLeftButtonIndex == 0, () => _onLeftButtonPressed(0, 'CMD_MODE_DRIVE')),
//           _buildLeftButton(30, 185, "Petrol", ICON_PATH_PETROL_INACTIVE, ICON_PATH_PETROL_ACTIVE, _activeLeftButtonIndex == 1, () => _onLeftButtonPressed(1, 'CMD_MODE_PATROL')),
//           _buildLeftButton(30, 335, "Recon", ICON_PATH_RECON_INACTIVE, ICON_PATH_RECON_ACTIVE, _activeLeftButtonIndex == 2, () => _onLeftButtonPressed(2, 'CMD_MODE_RECON')),
//           _buildLeftButton(30, 485, _isAutoAttackMode ? "Auto Attack" : "Manual Attack", _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_INACTIVE : ICON_PATH_MANUAL_ATTACK_INACTIVE, _isAutoAttackMode ? ICON_PATH_AUTO_ATTACK_ACTIVE : ICON_PATH_MANUAL_ATTACK_ACTIVE, _activeLeftButtonIndex == 3, () { setState(() => _activeLeftButtonIndex = 3); _handleManualAutoAttackToggle(); }),
//           _buildLeftButton(30, 635, "Drone", ICON_PATH_DRONE_INACTIVE, ICON_PATH_DRONE_ACTIVE, _activeLeftButtonIndex == 4, () => _onLeftButtonPressed(4, 'CMD_MODE_DRONE')),
//           _buildLeftButton(30, 785, "Return", ICON_PATH_RETURN_INACTIVE, ICON_PATH_RETURN_ACTIVE, _activeLeftButtonIndex == 5, () => _onLeftButtonPressed(5, 'CMD_RETUR')),
//
//           // --- Right Side Buttons (Now use the new, larger right button widget) ---
//           _buildRightButton(1690, 30, "Attack View", ICON_PATH_ATTACK_VIEW_INACTIVE, ICON_PATH_ATTACK_VIEW_ACTIVE, _activeRightButtonIndex == 0, () => _onRightButtonPressed(0)),
//           _buildRightButton(1690, 250, "Top View", ICON_PATH_TOP_VIEW_INACTIVE, ICON_PATH_TOP_VIEW_ACTIVE, _activeRightButtonIndex == 1, () => _onRightButtonPressed(1)),
//           _buildRightButton(1690, 470, "3D View", ICON_PATH_3D_VIEW_INACTIVE, ICON_PATH_3D_VIEW_ACTIVE, _activeRightButtonIndex == 2, () => _onRightButtonPressed(2)),
//           _buildRightButton(1690, 720, "Setting", ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, false, () => _navigateToSettings()),
//
//           // The rest of the layout remains the same
//           _buildDirectionalButton(890, 30, _isForwardPressed, ICON_PATH_FORWARD_INACTIVE, ICON_PATH_FORWARD_ACTIVE, 'CMD_FORWARD', () => setState(() => _isForwardPressed = true), () => setState(() => _isForwardPressed = false)),
//           _buildDirectionalButton(890, 820, _isBackPressed, ICON_PATH_BACKWARD_INACTIVE, ICON_PATH_BACKWARD_ACTIVE, 'CMD_BACKWARD', () => setState(() => _isBackPressed = true), () => setState(() => _isBackPressed = false)),
//           _buildDirectionalButton(260, 405, _isLeftPressed, ICON_PATH_LEFT_INACTIVE, ICON_PATH_LEFT_ACTIVE, 'CMD_LEFT', () => setState(() => _isLeftPressed = true), () => setState(() => _isLeftPressed = false)),
//           _buildDirectionalButton(1560, 405, _isRightPressed, ICON_PATH_RIGHT_INACTIVE, ICON_PATH_RIGHT_ACTIVE, 'CMD_RIGHT', () => setState(() => _isRightPressed = true), () => setState(() => _isRightPressed = false)),
//
//           // Align(
//           //   alignment: Alignment.bottomCenter,
//           //   child: Padding(
//           //     padding: EdgeInsets.only(bottom: 20 * heightScale, left: 30 * widthScale, right: 30 * widthScale),
//           //     child: _buildBottomBar(),
//           //   ),
//           // ),
//           Align(
//             alignment: Alignment.bottomCenter,
//             child: Container(
//               // The container for the black bar background
//               margin: const EdgeInsets.only(bottom: 5), // Margin from the screen edge
//               padding: const EdgeInsets.symmetric(horizontal: 15, vertical: 4), // Padding inside the bar
//               decoration: BoxDecoration(
//                 color: Colors.black.withOpacity(0.6), // Semi-transparent black
//                 borderRadius: BorderRadius.circular(40), // Fully rounded ends
//               ),
//               child: _buildBottomBar(), // Your existing Row of buttons goes here
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
//   void _onRightButtonPressed(int index) {
//     setState(() => _activeRightButtonIndex = index);
//     _switchCamera(index);
//   }
//
//   Widget buildPlayerWidget() {
//     if (_errorMessage != null) {
//       return Center(
//           child: Column(
//             mainAxisAlignment: MainAxisAlignment.center,
//             children: [
//               Padding(
//                   padding: const EdgeInsets.symmetric(horizontal: 24.0),
//                   child: Text('Error: $_errorMessage',
//                       textAlign: TextAlign.center,
//                       style: const TextStyle(
//                           color: Colors.red,
//                           fontSize: 16,
//                           fontWeight: FontWeight.bold))),
//               const SizedBox(height: 20),
//               ElevatedButton(
//                   onPressed: () => _switchCamera(_currentCameraIndex),
//                   style:
//                   ElevatedButton.styleFrom(backgroundColor: Colors.grey[700]),
//                   child: const Text('Retry', style: TextStyle(color: Colors.white))),
//             ],
//           ));
//     }
//     if (_vlcPlayerController == null) {
//       return const Center(child: CircularProgressIndicator(color: Colors.white));
//     }
//     // No aspectRatio here allows the player to fill the space.
//     // We added the aspect ratio hint to the VLC options instead.
//     return VlcPlayer(
//         controller: _vlcPlayerController!,
//         placeholder: const Center(child: CircularProgressIndicator(color: Colors.white)), aspectRatio: 16/12,);
//   }
//
//
//   Widget _buildLeftButton(double left, double top, String label, String inactiveIcon, String activeIcon, bool isActive, VoidCallback onPressed) {
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
//           padding: EdgeInsets.symmetric(vertical: 8.0 * heightScale),
//           decoration: BoxDecoration(
//             color: Colors.black.withOpacity(0.6),
//             borderRadius: BorderRadius.circular(10),
//             border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.0),
//           ),
//           child: Column(
//             mainAxisAlignment: MainAxisAlignment.center,
//             crossAxisAlignment: CrossAxisAlignment.center,
//             children: [
//               Expanded(flex: 2, child: Image.asset(isActive ? activeIcon : inactiveIcon, fit: BoxFit.contain)),
//               SizedBox(height: 5 * heightScale),
//               Text(label, textAlign: TextAlign.center, style: GoogleFonts.notoSans(fontSize: 24 * heightScale, fontWeight: FontWeight.bold, color: const Color(0xFFFFFFFF))),
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
//     return Positioned(
//       // The `left` coordinate is the starting edge, not the center.
//       // This was the main error.
//       left: left * widthScale,
//       top: top * heightScale,
//       child: GestureDetector(
//         onTap: onPressed,
//         child: Container(
//           width: buttonWidth * widthScale,
//           height: buttonHeight * heightScale,
//           padding: EdgeInsets.symmetric(vertical: 10.0 * heightScale),
//           decoration: BoxDecoration(
//             color: Colors.black.withOpacity(0.6),
//             borderRadius: BorderRadius.circular(15),
//             border: Border.all(color: isActive ? Colors.white : Colors.transparent, width: 2.5),
//           ),
//           child: Column(
//             mainAxisAlignment: MainAxisAlignment.center,
//             crossAxisAlignment: CrossAxisAlignment.center,
//             children: [
//               Expanded(
//                 flex: 3,
//                 child: Image.asset(isActive ? activeIcon : inactiveIcon, fit: BoxFit.contain),
//               ),
//               SizedBox(height: 8 * heightScale),
//               Text(label, textAlign: TextAlign.center, style: GoogleFonts.notoSans(fontSize: 26 * heightScale, fontWeight: FontWeight.bold, color: const Color(0xFFFFFFFF))),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//
//   Widget _buildDirectionalButton(double left, double top, bool isPressed,
//       String inactiveIcon, String activeIcon, String command,
//       VoidCallback onPress, VoidCallback onRelease) {
//     final screenWidth = MediaQuery
//         .of(context)
//         .size
//         .width;
//     final screenHeight = MediaQuery
//         .of(context)
//         .size
//         .height;
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
//         child: Image.asset(
//             isPressed ? activeIcon : inactiveIcon, height: 100 * heightScale,
//             width: 100 * widthScale,
//             fit: BoxFit.contain),
//       ),
//     );
//   }
//
//
//   // --- FIX: FINAL ROBUST BOTTOM BAR WITH PERFECT TEXT ALIGNMENT ---
//   // Widget _buildBottomBar() {
//   //   final screenWidth = MediaQuery.of(context).size.width;
//   //   final widthScale = screenWidth / 1920.0;
//   //
//   //   return Row(
//   //     mainAxisAlignment: MainAxisAlignment.center,
//   //     crossAxisAlignment: CrossAxisAlignment.center,
//   //     children: [
//   //       _buildBottomBarButton(
//   //         "ATTACK",
//   //         _isAttackModeOn ? ICON_PATH_ATTACK_of : ICON_PATH_ATTACK,
//   //         _isAttackModeOn ? _attackActiveColors : _attackInactiveColors, // Conditionally choose the gradient
//   //             () async {
//   //           // Logic remains the same
//   //           if (!_isAttackModeOn) {
//   //             final proceed = await _showCustomConfirmationDialog(
//   //                 context: context,
//   //                 iconPath: ICON_PATH_ATTACK,
//   //                 title: 'DANGERS',
//   //                 titleColor: Colors.red,
//   //                 content: 'Do you want to proceed the command?');
//   //             if (proceed) {
//   //               _sendCommand('CMD_MODE_ATTACK');
//   //               setState(() {
//   //                 _isAttackModeOn = true;
//   //               });
//   //             }
//   //           } else {
//   //             _sendCommand('CMD_STOP_ATTACK');
//   //             setState(() {
//   //               _isAttackModeOn = false;
//   //             });
//   //           }
//   //         },
//   //       ),
//   //
//   //       SizedBox(width: 12 * widthScale),
//   //       // _buildBottomBarButton(_isStarted ? "STOP" : "START", _isStarted ? ICON_PATH_STOP : ICON_PATH_START, [const Color(0xff25a625), const Color(0xff127812)], () { setState(() => _isStarted = !_isStarted); _sendCommand(_isStarted ? 'CMD_STOP' : 'CMD_START'); }),
//   //
//   //       _buildWideBottomBarButton(_isStarted ? "STOP" : "START", _isStarted ? ICON_PATH_STOP : ICON_PATH_START, [const Color(0xff25a625), const Color(0xff127812)], () { setState(() => _isStarted = !_isStarted); _sendCommand(_isStarted ? 'CMD_STOP' : 'CMD_START'); }),
//   //       SizedBox(width: 60 * widthScale),
//   //       _buildBottomBarButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_IN')),
//   //       SizedBox(width: 12 * widthScale),
//   //       _buildBottomBarButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_OUT')),
//   //
//   //       const Spacer(),
//   //       SizedBox(width: 45 * widthScale),
//   //
//   //       Row(
//   //         mainAxisSize: MainAxisSize.min,
//   //         crossAxisAlignment: CrossAxisAlignment.baseline,
//   //         textBaseline: TextBaseline.alphabetic,
//   //         children: [
//   //           Text("0", style: GoogleFonts.notoSans(fontSize: 80, fontWeight: FontWeight.bold, color: Colors.white)),
//   //           SizedBox(width: 8 * widthScale),
//   //           Text("Km/h", style: GoogleFonts.notoSans(fontSize: 36, fontWeight: FontWeight.w500, color: Colors.white)),
//   //         ],
//   //       ),
//   //       SizedBox(width: 45 * widthScale),
//   //       Image.asset(ICON_PATH_WIFI, height: 40),
//   //
//   //       const Spacer(),
//   //
//   //       _buildBottomBarButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)],
//   //               () async {
//   //             final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_EXIT, title: 'Information', titleColor: Colors.blue.shade700, content: 'Do you want to finish?');
//   //             if (proceed) _sendCommand('CMD_EXIT');
//   //           }
//   //       ),
//   //     ],
//   //   );
//   // }
//
//
//   // --- FINAL, ROBUST, AND CONDITIONALLY SCROLLABLE BOTTOM BAR ---
//   Widget _buildBottomBar() {
//     // LayoutBuilder gives us the available screen width at render time.
//     return LayoutBuilder(
//       builder: (context, constraints) {
//         final screenWidth = MediaQuery.of(context).size.width;
//         final widthScale = screenWidth / 1920.0;
//
//         // Let's define a breakpoint. If the screen is narrower than this, we'll scroll.
//         // 1200 logical pixels is a good tablet breakpoint.
//         const double scrollBreakpoint = 1200.0;
//
//         // Define the list of widgets once to avoid code duplication.
//         final List<Widget> leftCluster = [
//           _buildBottomBarButton(
//             "ATTACK",
//             _isAttackModeOn ? ICON_PATH_ATTACK_of : ICON_PATH_ATTACK,
//             _isAttackModeOn ? _attackActiveColors : _attackInactiveColors,
//                 () async {
//               if (!_isAttackModeOn) {
//                 final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_ATTACK, title: 'DANGERS', titleColor: Colors.red, content: 'Do you want to proceed the command?');
//                 if (proceed) {
//                   _sendCommand('CMD_MODE_ATTACK');
//                   setState(() => _isAttackModeOn = true);
//                 }
//               } else {
//                 _sendCommand('CMD_STOP_ATTACK');
//                 setState(() => _isAttackModeOn = false);
//               }
//             },
//           ),
//           SizedBox(width: 12 * widthScale),
//           _buildWideBottomBarButton(_isStarted ? "STOP" : "START", _isStarted ? ICON_PATH_STOP : ICON_PATH_START, [const Color(0xff25a625), const Color(0xff127812)], () { setState(() => _isStarted = !_isStarted); _sendCommand(_isStarted ? 'CMD_STOP' : 'CMD_START'); }),
//           SizedBox(width: 50 * widthScale),
//           _buildBottomBarButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_IN')),
//           SizedBox(width: 12 * widthScale),
//           _buildBottomBarButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_OUT')),
//         ];
//
//         final List<Widget> middleCluster = [
//           Row(
//             mainAxisSize: MainAxisSize.min,
//             crossAxisAlignment: CrossAxisAlignment.baseline,
//             textBaseline: TextBaseline.alphabetic,
//             children: [
//               Text("0", style: GoogleFonts.notoSans(fontSize: 80, fontWeight: FontWeight.bold, color: Colors.white)),
//               SizedBox(width: 8 * widthScale),
//               Text("Km/h", style: GoogleFonts.notoSans(fontSize: 36, fontWeight: FontWeight.w500, color: Colors.white)),
//             ],
//           ),
//           SizedBox(width: 60 * widthScale),
//           Image.asset(ICON_PATH_WIFI, height: 40),
//         ];
//
//         final List<Widget> rightCluster = [
//           _buildBottomBarButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)], () async {
//             final proceed = await _showCustomConfirmationDialog(context: context, iconPath: ICON_PATH_EXIT, title: 'Information', titleColor: Colors.blue.shade700, content: 'Do you want to finish?');
//             if (proceed) _sendCommand('CMD_EXIT');
//           }),
//         ];
//
//         // --- THE CONDITIONAL LOGIC ---
//         if (constraints.maxWidth < scrollBreakpoint) {
//           // --- SMALL SCREEN: Render inside a SingleChildScrollView ---
//           return SingleChildScrollView(
//             scrollDirection: Axis.horizontal,
//             physics: const ClampingScrollPhysics(),
//             child: Row(
//               children: [
//                 ...leftCluster,
//                 // Use a large SizedBox for spacing inside the scrollable area
//                 SizedBox(width: 100 * widthScale),
//                 ...middleCluster,
//                 SizedBox(width: 100 * widthScale),
//                 ...rightCluster,
//               ],
//             ),
//           );
//         } else {
//           // --- LARGE SCREEN: Render in a Row with Spacers ---
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
//       // This Container enforces a minimum width, making it as wide as the ATTACK button.
//       child: Container(
//         constraints: BoxConstraints(
//           minWidth: 220 * widthScale, // Enforce a minimum width
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
//             Text(label, style: GoogleFonts.notoSans(fontSize: 36 * heightScale, fontWeight: FontWeight.bold, color: Colors.white)),
//           ],
//         ),
//       ),
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
//         // Increased horizontal padding makes the buttons wider
//         padding: EdgeInsets.symmetric(horizontal: label.isEmpty ? 30 : 45),
//         decoration: BoxDecoration(
//           gradient: LinearGradient(colors: gradientColors, begin: Alignment.topCenter, end: Alignment.bottomCenter),
//           // Reduced border radius makes them more rectangular
//           borderRadius: BorderRadius.circular(25 * heightScale),
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
//
//
