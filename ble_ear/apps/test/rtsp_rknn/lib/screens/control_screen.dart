// import 'dart:async';
// import 'package:flutter/material.dart';
// import 'package:flutter_vlc_player/flutter_vlc_player.dart';
// import 'package:provider/provider.dart';
//
// // --- CORRECTED PACKAGE IMPORTS ---
// import 'package:rtsp_rknn/screens/settings_screen.dart';
// import 'package:rtsp_rknn/services/robot_service.dart';
// import 'package:rtsp_rknn/state/app_state.dart';
//
// class ControlScreen extends StatefulWidget {
//   @override
//   _ControlScreenState createState() => _ControlScreenState();
// }
//
// class _ControlScreenState extends State<ControlScreen> {
//   late VlcPlayerController _videoPlayerController;
//   late RobotService _robotService;
//   String? _currentlyPlayingUrl;
//
//   Timer? _connectionTimer;
//   bool _isAttemptingConnection = false;
//
//   // --- NEW/UPDATED State flags for toggling buttons ---
//   // false = Show "Start", true = Show "Stop"
//   bool _isStarted = false;
//   // false = Show "Auto Attack", true = Show "Manual Attack" option
//   bool _showManualAttackButton = false;
//
//   final List<Map<String, String>> _commands = [
//     {'label': 'Driving', 'cmd': 'CMD_MODE_DRIVE'},
//     {'label': 'Patrol', 'cmd': 'CMD_MODE_PATROL'},
//     {'label': 'Recon', 'cmd': 'CMD_MODE_RECON'},
//     {'label': 'Drone', 'cmd': 'CMD_MODE_DRONE'},
//     {'label': 'Forward', 'cmd': 'CMD_FORWARD'},
//     {'label': 'Backward', 'cmd': 'CMD_BACKWARD'},
//     {'label': 'Left', 'cmd': 'CMD_LEFT'},
//     {'label': 'Right', 'cmd': 'CMD_RIGHT'},
//     {'label': 'Turn Left', 'cmd': 'CMD_TURN_LEFT'},
//     {'label': 'Turn Right', 'cmd': 'CMD_TURN_RIGHT'},
//     {'label': 'Scan', 'cmd': 'CMD_SCAN'},
//     {'label': 'Front 3D', 'cmd': 'CMD_FRONT_3D'},
//     {'label': 'Top View', 'cmd': 'CMD_TOP_VIEW'},
//   ];
//
//   @override
//   void initState() {
//     super.initState();
//     final appState = Provider.of<AppState>(context, listen: false);
//     _currentlyPlayingUrl = appState.currentCameraUrl;
//     _videoPlayerController = VlcPlayerController.network(_currentlyPlayingUrl!, hwAcc: HwAcc.full, autoPlay: false, options: VlcPlayerOptions());
//     _robotService = RobotService(ipAddress: appState.robotIpAddress);
//     _robotService.connect();
//     appState.addListener(_onAppStateChanged);
//     _videoPlayerController.addListener(_onPlayerStateChanged);
//     WidgetsBinding.instance.addPostFrameCallback((_) => _initiateConnectionAttempt());
//   }
//
//   void _initiateConnectionAttempt() {
//     if (_isAttemptingConnection || !mounted) return;
//     setState(() => _isAttemptingConnection = true);
//     _connectionTimer?.cancel();
//     _videoPlayerController.play();
//     _connectionTimer = Timer(const Duration(seconds: 10), () {
//       if (mounted && !_videoPlayerController.value.isPlaying) {
//         setState(() => _isAttemptingConnection = false);
//       }
//     });
//   }
//
//   void _onPlayerStateChanged() {
//     if (_videoPlayerController.value.isPlaying && mounted && _isAttemptingConnection) {
//       _connectionTimer?.cancel();
//       setState(() => _isAttemptingConnection = false);
//     }
//   }
//
//   void _onAppStateChanged() {
//     final appState = Provider.of<AppState>(context, listen: false);
//     if (_robotService.ipAddress != appState.robotIpAddress) {
//       _robotService.dispose();
//       _robotService = RobotService(ipAddress: appState.robotIpAddress);
//       _robotService.connect();
//     }
//     if (_currentlyPlayingUrl != appState.currentCameraUrl) {
//       _currentlyPlayingUrl = appState.currentCameraUrl;
//       if (_videoPlayerController.value.isInitialized) {
//         _videoPlayerController.setMediaFromNetwork(appState.currentCameraUrl);
//         _initiateConnectionAttempt();
//       }
//     }
//   }
//
//   @override
//   void dispose() {
//     _connectionTimer?.cancel();
//     _videoPlayerController.removeListener(_onPlayerStateChanged);
//     Provider.of<AppState>(context, listen: false).removeListener(_onAppStateChanged);
//     _videoPlayerController.dispose();
//     _robotService.dispose();
//     super.dispose();
//   }
//
//   @override
//   Widget build(BuildContext context) {
//     final appState = Provider.of<AppState>(context);
//     final screenHeight = MediaQuery.of(context).size.height;
//
//     return Scaffold(
//       body: SafeArea(
//         child: Column(
//           children: [
//             SizedBox(
//               height: screenHeight * 0.75 - MediaQuery.of(context).padding.top,
//               child: Row(
//                 children: [
//                   _buildLeftCommandPanel(),
//                   Expanded(
//                     child: Container(
//                       color: Colors.black,
//                       child: Column(
//                         children: [
//                           _buildCameraSwitcher(appState),
//                           Expanded(child: _buildVideoPlayerWidget()),
//                         ],
//                       ),
//                     ),
//                   ),
//                 ],
//               ),
//             ),
//             Expanded(
//               child: Container(
//                 padding: const EdgeInsets.symmetric(horizontal: 8.0, vertical: 8.0),
//                 color: Theme.of(context).bottomAppBarTheme.color,
//                 child: _buildBottomBar(),
//               ),
//             ),
//           ],
//         ),
//       ),
//     );
//   }
//
//   // --- HELPER WIDGETS ---
//
//   Widget _buildLeftCommandPanel() {
//     return Container(
//       width: MediaQuery.of(context).size.width * 0.25,
//       color: Theme.of(context).scaffoldBackgroundColor,
//       child: SingleChildScrollView(
//         padding: const EdgeInsets.symmetric(vertical: 8.0, horizontal: 4.0),
//         child: Column(
//           children: [
//             ..._commands.map((command) => _commandButton(
//               label: command['label']!,
//               onPressed: () => _robotService.sendCommand(command['cmd']!),
//             )).toList(),
//             _showManualAttackButton
//                 ? _commandButton(
//               label: "Manual Attack",
//               onPressed: () {
//                 _showAttackConfirmationDialog("CMD_MANU_ATTACK", () {
//                   setState(() => _showManualAttackButton = false);
//                 });
//               },
//             )
//                 : _commandButton(
//               label: "Auto Attack",
//               onPressed: () {
//                 _robotService.sendCommand("CMD_AUTO_ATTACK");
//                 setState(() => _showManualAttackButton = true);
//               },
//             ),
//           ],
//         ),
//       ),
//     );
//   }
//
//   // UPDATED: Implements the new button logic as requested
//   Widget _buildBottomBar() {
//     return Row(
//       children: [
//         _bottomBarButton(label: "Return", command: "CMD_RETUR"),
//         const SizedBox(width: 8),
//         // Dedicated "Attack" button
//         _bottomBarButton(label: "Attack", command: "CMD_AUTO_ATTACK", color: Colors.orange),
//         const SizedBox(width: 8),
//         // Toggling "Start/Stop" button
//         _isStarted
//             ? _bottomBarButton(
//           label: "STOP",
//           color: Colors.blue,
//           onPressed: () {
//             _robotService.sendCommand("CMD_STOP");
//             setState(() => _isStarted = false);
//           },
//         )
//             : _bottomBarButton(
//           label: "Start",
//           onPressed: () {
//             _robotService.sendCommand("CMD_START");
//             setState(() => _isStarted = true);
//           },
//         ),
//         const SizedBox(width: 8),
//         _bottomBarButton(icon: Icons.add, command: "CMD_ZOOM_IN"),
//         const SizedBox(width: 8),
//         _bottomBarButton(icon: Icons.remove, command: "CMD_ZOOM_OUT"),
//         const SizedBox(width: 8),
//         _bottomBarButton(
//           icon: Icons.settings,
//           onPressed: () => Navigator.push(context, MaterialPageRoute(builder: (context) => SettingsScreen()))
//               .then((_) => _initiateConnectionAttempt()),
//         ),
//         const SizedBox(width: 8),
//         _bottomBarButton(label: "EXIT", command: "CMD_EXIT"),
//       ],
//     );
//   }
//
//   Widget _buildVideoPlayerWidget() {
//     if (_isAttemptingConnection) {
//       return const Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [CircularProgressIndicator(), SizedBox(height: 16), Text('Connecting...', style: TextStyle(color: Colors.white))]));
//     }
//     if (!_videoPlayerController.value.isPlaying && !_videoPlayerController.value.isBuffering) {
//       return const Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [Icon(Icons.videocam_off_outlined, color: Colors.grey, size: 48), SizedBox(height: 8), Text('Camera Not Available', style: TextStyle(color: Colors.white, fontSize: 18))]));
//     }
//     return VlcPlayer(controller: _videoPlayerController, aspectRatio: 16 / 9, placeholder: const Center(child: CircularProgressIndicator()));
//   }
//
//   Widget _commandButton({required String label, required VoidCallback onPressed}) {
//     return Padding(
//       padding: const EdgeInsets.symmetric(vertical: 2.0),
//       child: SizedBox(
//         width: double.infinity,
//         child: ElevatedButton(
//           onPressed: onPressed,
//           style: ElevatedButton.styleFrom(shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8.0)), textStyle: const TextStyle(fontSize: 12), padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 4)),
//           child: Text(label, textAlign: TextAlign.center),
//         ),
//       ),
//     );
//   }
//
//   Widget _bottomBarButton({String? label, IconData? icon, String? command, Color? color, VoidCallback? onPressed}) {
//     return Expanded(
//       child: ElevatedButton(
//         style: ElevatedButton.styleFrom(backgroundColor: color, shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8.0)), padding: const EdgeInsets.symmetric(vertical: 12)),
//         onPressed: onPressed ?? () => _robotService.sendCommand(command!),
//         child: icon != null ? Icon(icon) : Text(label!, style: const TextStyle(fontSize: 12)),
//       ),
//     );
//   }
//
//   Widget _cameraSelectButton(int index, String label, AppState appState) {
//     return Expanded(
//       child: ElevatedButton(
//         onPressed: () => appState.selectCamera(index),
//         child: Text(label),
//         style: ElevatedButton.styleFrom(backgroundColor: appState.selectedCameraIndex == index ? Theme.of(context).primaryColor : Colors.grey.shade600),
//       ),
//     );
//   }
//
//   void _showAttackConfirmationDialog(String command, [VoidCallback? onConfirmed]) {
//     showDialog(
//       context: context,
//       builder: (ctx) => AlertDialog(
//         title: const Text("Start Manual Attack"),
//         content: const Text("Do you want to proceed with the command?"),
//         actions: <Widget>[
//           TextButton(onPressed: () => Navigator.of(ctx).pop(), child: const Text("Cancel")),
//           TextButton(
//             onPressed: () {
//               _robotService.sendCommand(command);
//               Navigator.of(ctx).pop();
//               onConfirmed?.call();
//             },
//             child: const Text("OK"),
//           ),
//         ],
//       ),
//     );
//   }
//
//   Widget _buildCameraSwitcher(AppState appState) {
//     return Padding(
//       padding: const EdgeInsets.symmetric(vertical: 4.0, horizontal: 8.0),
//       child: Row(
//         mainAxisAlignment: MainAxisAlignment.center,
//         children: [
//           _cameraSelectButton(0, "Cam 1", appState),
//           const SizedBox(width: 8),
//           _cameraSelectButton(1, "Cam 2", appState),
//           const SizedBox(width: 8),
//           _cameraSelectButton(2, "Cam 3", appState),
//         ],
//       ),
//     );
//   }
// }
//
//
//



import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_vlc_player/flutter_vlc_player.dart';
import 'package:provider/provider.dart';

// --- Using your correct package name ---
import 'package:rtsp_rknn/screens/settings_screen.dart';
import 'package:rtsp_rknn/services/robot_service.dart';
import 'package:rtsp_rknn/state/app_state.dart';

class ControlScreen extends StatefulWidget {
  @override
  _ControlScreenState createState() => _ControlScreenState();
}

class _ControlScreenState extends State<ControlScreen> {
  late RobotService _robotService;

  // --- NEW ARCHITECTURE: Manage three controllers ---
  final List<VlcPlayerController> _controllers = [];
  final Map<int, bool> _isConnecting = {0: false, 1: false, 2: false};
  final Map<int, Timer?> _connectionTimers = {0: null, 1: null, 2: null};

  bool _isStarted = false;
  bool _showManualAttackButton = false;

  final List<Map<String, String>> _commands = [
    {'label': 'Driving', 'cmd': 'CMD_MODE_DRIVE'},
    {'label': 'Patrol', 'cmd': 'CMD_MODE_PATROL'},
    {'label': 'Recon', 'cmd': 'CMD_MODE_RECON'},
    {'label': 'Drone', 'cmd': 'CMD_MODE_DRONE'},
    {'label': 'Forward', 'cmd': 'CMD_FORWARD'},
    {'label': 'Backward', 'cmd': 'CMD_BACKWARD'},
    {'label': 'Left', 'cmd': 'CMD_LEFT'},
    {'label': 'Right', 'cmd': 'CMD_RIGHT'},
    {'label': 'Turn Left', 'cmd': 'CMD_TURN_LEFT'},
    {'label': 'Turn Right', 'cmd': 'CMD_TURN_RIGHT'},
    {'label': 'Scan', 'cmd': 'CMD_SCAN'},
    {'label': 'Front 3D', 'cmd': 'CMD_FRONT_3D'},
    {'label': 'Top View', 'cmd': 'CMD_TOP_VIEW'},
  ];

  @override
  void initState() {
    super.initState();
    final appState = Provider.of<AppState>(context, listen: false);

    // Initialize the command service
    _robotService = RobotService(ipAddress: appState.robotIpAddress);
    _robotService.connect();
    appState.addListener(_onAppStateChanged);

    // Create a controller for each camera URL
    for (int i = 0; i < appState.cameraUrls.length; i++) {
      var controller = VlcPlayerController.network(
        appState.cameraUrls[i],
        hwAcc: HwAcc.full,
        autoPlay: false,
        options: VlcPlayerOptions(),
      );
      // Add a listener that just rebuilds the UI when the player state changes
      controller.addListener(() {
        if (mounted) setState(() {});
      });
      _controllers.add(controller);
    }

    // On startup, attempt to connect to all three streams
    _attemptToConnectAllStreams();
  }

  // --- NEW: Robust Connection Logic ---

  void _attemptToConnectAllStreams() {
    for (int i = 0; i < _controllers.length; i++) {
      _initiateConnectionAttempt(i);
    }
  }

  // This function can be called at any time to try connecting a specific camera
  void _initiateConnectionAttempt(int index) {
    if (_isConnecting[index] == true || !mounted) return;

    setState(() => _isConnecting[index] = true);
    _connectionTimers[index]?.cancel();

    // This Timer is the robust fix for the "uninitialized" error.
    // It will try to play the video every 500ms until the controller is ready.
    Timer.periodic(const Duration(milliseconds: 500), (timer) {
      if (!mounted) {
        timer.cancel();
        return;
      }
      final controller = _controllers[index];
      // If controller is initialized, try to play and stop this retry timer.
      if (controller.value.isInitialized) {
        controller.play();
        timer.cancel(); // Stop retrying

        // Start the 10-second timeout timer
        _connectionTimers[index] = Timer(const Duration(seconds: 10), () {
          if (mounted && !controller.value.isPlaying) {
            setState(() => _isConnecting[index] = false);
          }
        });
      }
    });

    // Also check for successful playback to cancel the "Connecting..." state
    _controllers[index].addListener(() {
      if (_controllers[index].value.isPlaying) {
        _connectionTimers[index]?.cancel();
        if (_isConnecting[index] == true) {
          setState(() => _isConnecting[index] = false);
        }
      }
    });
  }

  // This listener now only handles IP changes for the command service
  void _onAppStateChanged() {
    final appState = Provider.of<AppState>(context, listen: false);
    if (_robotService.ipAddress != appState.robotIpAddress) {
      _robotService.dispose();
      _robotService = RobotService(ipAddress: appState.robotIpAddress);
      _robotService.connect();
    }
    // Note: We no longer need to handle camera URL changes here because the
    // controllers are re-created when the settings page is saved and we return.
  }

  @override
  void dispose() {
    for (var controller in _controllers) {
      controller.dispose();
    }
    for (var timer in _connectionTimers.values) {
      timer?.cancel();
    }
    _robotService.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final appState = Provider.of<AppState>(context);
    final screenHeight = MediaQuery.of(context).size.height;
    return Scaffold(
      body: SafeArea(
        child: Column(
          children: [
            SizedBox(
              height: screenHeight * 0.75 - MediaQuery.of(context).padding.top,
              child: Row(
                children: [
                  _buildLeftCommandPanel(),
                  Expanded(
                    child: Container(
                      color: Colors.black,
                      child: Column(
                        children: [
                          _buildCameraSwitcher(appState),
                          // --- NEW: Stack to hold all video players ---
                          Expanded(
                            child: Stack(
                              children: List.generate(_controllers.length, (index) {
                                return Visibility(
                                  // Show only the player for the selected camera index
                                  visible: appState.selectedCameraIndex == index,
                                  maintainState: true, // Keep state of hidden players
                                  child: _buildVideoPlayerWidget(index),
                                );
                              }),
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                ],
              ),
            ),
            Expanded(
              child: Container(
                padding: const EdgeInsets.symmetric(horizontal: 8.0, vertical: 8.0),
                color: Theme.of(context).bottomAppBarTheme.color,
                child: _buildBottomBar(),
              ),
            ),
          ],
        ),
      ),
    );
  }

  // --- HELPER WIDGETS (Updated) ---

  // This widget now builds the UI for a specific camera index
  Widget _buildVideoPlayerWidget(int index) {
    final controller = _controllers[index];
    if (_isConnecting[index] == true) {
      return const Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [CircularProgressIndicator(), SizedBox(height: 16), Text('Connecting...', style: TextStyle(color: Colors.white))]));
    }
    if (!controller.value.isPlaying && !controller.value.isBuffering) {
      return const Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [Icon(Icons.videocam_off_outlined, color: Colors.grey, size: 48), SizedBox(height: 8), Text('Camera Not Available', style: TextStyle(color: Colors.white, fontSize: 18))]));
    }
    return VlcPlayer(controller: controller, aspectRatio: 16 / 9, placeholder: const Center(child: CircularProgressIndicator()));
  }

  // --- NEW: Smart camera selection ---
  void _selectAndAttemptConnection(int index, AppState appState) {
    appState.selectCamera(index); // Switch the view immediately
    final controller = _controllers[index];
    // If the selected camera isn't playing, try to connect again.
    if (!controller.value.isPlaying) {
      _initiateConnectionAttempt(index);
    }
  }

  Widget _buildCameraSwitcher(AppState appState) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4.0, horizontal: 8.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: List.generate(3, (index) {
          return Expanded(
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 4.0),
              child: ElevatedButton(
                onPressed: () => _selectAndAttemptConnection(index, appState),
                child: Text('Cam ${index + 1}'),
                style: ElevatedButton.styleFrom(backgroundColor: appState.selectedCameraIndex == index ? Theme.of(context).primaryColor : Colors.grey.shade600),
              ),
            ),
          );
        }),
      ),
    );
  }

  // The rest of the build helpers are mostly the same
  // [Paste the helper widgets like _buildLeftCommandPanel, _buildBottomBar, etc. here]
  Widget _buildLeftCommandPanel() {
    return Container(
      width: MediaQuery.of(context).size.width * 0.25,
      color: Theme.of(context).scaffoldBackgroundColor,
      child: SingleChildScrollView(
        padding: const EdgeInsets.symmetric(vertical: 8.0, horizontal: 4.0),
        child: Column(
          children: [
            ..._commands.map((command) => _commandButton(
              label: command['label']!,
              onPressed: () => _robotService.sendCommand(command['cmd']!),
            )).toList(),
            _showManualAttackButton
                ? _commandButton(
              label: "Manual Attack",
              onPressed: () {
                _showAttackConfirmationDialog("CMD_MANU_ATTACK", () {
                  setState(() => _showManualAttackButton = false);
                });
              },
            )
                : _commandButton(
              label: "Auto Attack",
              onPressed: () {
                _robotService.sendCommand("CMD_AUTO_ATTACK");
                setState(() => _showManualAttackButton = true);
              },
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildBottomBar() {
    return Row(
      children: [
        _bottomBarButton(label: "Return", command: "CMD_RETUR"),
        const SizedBox(width: 8),
        _bottomBarButton(label: "Attack", command: "CMD_AUTO_ATTACK", color: Colors.orange),
        const SizedBox(width: 8),
        _isStarted
            ? _bottomBarButton(
          label: "STOP",
          color: Colors.blue,
          onPressed: () {
            _robotService.sendCommand("CMD_STOP");
            setState(() => _isStarted = false);
          },
        )
            : _bottomBarButton(
          label: "Start",
          onPressed: () {
            _robotService.sendCommand("CMD_START");
            setState(() => _isStarted = true);
          },
        ),
        const SizedBox(width: 8),
        _bottomBarButton(icon: Icons.add, command: "CMD_ZOOM_IN"),
        const SizedBox(width: 8),
        _bottomBarButton(icon: Icons.remove, command: "CMD_ZOOM_OUT"),
        const SizedBox(width: 8),
        _bottomBarButton(
          icon: Icons.settings,
          onPressed: () {
            // When returning from settings, we need to refresh the entire screen
            // to re-create the controllers with new URLs. A simple pop and push
            // is an effective way to do this.
            Navigator.push(context, MaterialPageRoute(builder: (context) => SettingsScreen()))
                .then((_) {
              Navigator.of(context).pop();
              Navigator.of(context).push(MaterialPageRoute(builder: (context) => ControlScreen()));
            });
          },
        ),
        const SizedBox(width: 8),
        _bottomBarButton(label: "EXIT", command: "CMD_EXIT"),
      ],
    );
  }

  Widget _commandButton({required String label, required VoidCallback onPressed}) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2.0),
      child: SizedBox(
        width: double.infinity,
        child: ElevatedButton(
          onPressed: onPressed,
          style: ElevatedButton.styleFrom(shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8.0)), textStyle: const TextStyle(fontSize: 12), padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 4)),
          child: Text(label, textAlign: TextAlign.center),
        ),
      ),
    );
  }

  Widget _bottomBarButton({String? label, IconData? icon, String? command, Color? color, VoidCallback? onPressed}) {
    return Expanded(
      child: ElevatedButton(
        style: ElevatedButton.styleFrom(backgroundColor: color, shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8.0)), padding: const EdgeInsets.symmetric(vertical: 12)),
        onPressed: onPressed ?? () => _robotService.sendCommand(command!),
        child: icon != null ? Icon(icon) : Text(label!, style: const TextStyle(fontSize: 12)),
      ),
    );
  }

  void _showAttackConfirmationDialog(String command, [VoidCallback? onConfirmed]) {
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text("Start Manual Attack"),
        content: const Text("Do you want to proceed with the command?"),
        actions: <Widget>[
          TextButton(onPressed: () => Navigator.of(ctx).pop(), child: const Text("Cancel")),
          TextButton(
            onPressed: () {
              _robotService.sendCommand(command);
              Navigator.of(ctx).pop();
              onConfirmed?.call();
            },
            child: const Text("OK"),
          ),
        ],
      ),
    );
  }
}