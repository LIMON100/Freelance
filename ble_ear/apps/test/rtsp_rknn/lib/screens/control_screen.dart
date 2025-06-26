// lib/screens/control_screen.dart
import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_vlc_player/flutter_vlc_player.dart';
import 'package:provider/provider.dart';

import 'package:rtsp_rknn/screens/settings_screen.dart';
import 'package:rtsp_rknn/state/app_state.dart';
import '../services/robot_service.dart';

class ControlScreen extends StatefulWidget {
  @override
  _ControlScreenState createState() => _ControlScreenState();
}

class _ControlScreenState extends State<ControlScreen> {
  late RobotService _robotService;

  final List<VlcPlayerController> _controllers = [];
  final Map<int, bool> _isConnecting = {0: false, 1: false, 2: false};
  final Map<int, Timer?> _connectionTimers = {0: null, 1: null, 2: null};

  // Maps to hold the last logged state to prevent spamming the console
  final Map<int, VlcPlayerValue> _lastLoggedState = {};

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

    _robotService = RobotService(ipAddress: appState.robotIpAddress);
    _robotService.connect();
    appState.addListener(_onAppStateChanged);

    for (int i = 0; i < appState.cameraUrls.length; i++) {
      var controller = VlcPlayerController.network(
        appState.cameraUrls[i],
        hwAcc: HwAcc.full,
        autoPlay: false,
        options: VlcPlayerOptions(
          advanced: VlcAdvancedOptions([
            VlcAdvancedOptions.networkCaching(300), // Slightly more cache for stability
          ]),
          rtp: VlcRtpOptions([
            VlcRtpOptions.rtpOverRtsp(true),
          ]),
        ),
      );
      // Main listener for UI updates
      controller.addListener(() {
        if (mounted) setState(() {});
        // --- NEW: Call the logger on every state change ---
        _logPlayerState(i, controller.value);
      });
      _controllers.add(controller);
    }
    _attemptToConnectAllStreams();
  }

  // --- NEW: Detailed Logger Function ---
  void _logPlayerState(int index, VlcPlayerValue state) {
    final lastState = _lastLoggedState[index];
    // Only print the log if a meaningful property has changed, to avoid spam.
    if (lastState == null ||
        lastState.isPlaying != state.isPlaying ||
        lastState.isBuffering != state.isBuffering ||
        lastState.errorDescription != state.errorDescription)
    {
      print('--- CAM ${index + 1} STATE ---');
      print('  URL: ${_controllers[index].dataSource}');
      print('  isInitialized: ${state.isInitialized}');
      print('  isPlaying: ${state.isPlaying}');
      print('  isBuffering: ${state.isBuffering}');
      print('  hasError: ${state.hasError}');
      if (state.errorDescription.isNotEmpty) {
        print('  ERROR: ${state.errorDescription}');
      }
      print('--------------------');
      _lastLoggedState[index] = state;
    }
  }

  void _attemptToConnectAllStreams() {
    for (int i = 0; i < _controllers.length; i++) {
      _initiateConnectionAttempt(i);
    }
  }

  void _initiateConnectionAttempt(int index) {
    if (_isConnecting[index] == true || !mounted) return;
    if (index >= _controllers.length) return;

    setState(() => _isConnecting[index] = true);
    _connectionTimers[index]?.cancel();

    Timer.periodic(const Duration(milliseconds: 500), (timer) {
      if (!mounted) {
        timer.cancel();
        return;
      }
      final controller = _controllers[index];
      if (controller.value.isInitialized) {
        controller.play();
        timer.cancel();

        // --- UPDATED: Increased timeout to 1 minute ---
        _connectionTimers[index] = Timer(const Duration(minutes: 1), () {
          if (mounted && !controller.value.isPlaying) {
            setState(() => _isConnecting[index] = false);
            print("--- CAM ${index+1}: Connection timed out after 1 minute. ---");
          }
        });
      }
    });

    _controllers[index].addListener(() {
      if (_controllers[index].value.isPlaying) {
        _connectionTimers[index]?.cancel();
        if (_isConnecting[index] == true) {
          setState(() => _isConnecting[index] = false);
        }
      }
    });
  }

  void _onAppStateChanged() {
    final appState = Provider.of<AppState>(context, listen: false);
    if (_robotService.ipAddress != appState.robotIpAddress) {
      _robotService.dispose();
      _robotService = RobotService(ipAddress: appState.robotIpAddress);
      _robotService.connect();
    }
  }

  @override
  void dispose() {
    for (var controller in _controllers) {
      if (controller.value.isInitialized) {
        controller.dispose();
      }
    }
    for (var timer in _connectionTimers.values) {
      timer?.cancel();
    }
    _robotService.dispose();
    super.dispose();
  }

  // The rest of the file (build method and helpers) remains the same
  // ... (All the `build...` methods are correct from the previous version)

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
                          Expanded(
                            child: Stack(
                              children: List.generate(_controllers.length, (index) {
                                return Visibility(
                                  visible: appState.selectedCameraIndex == index,
                                  maintainState: true,
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

  Widget _buildVideoPlayerWidget(int index) {
    if (index >= _controllers.length) return SizedBox.shrink();
    final controller = _controllers[index];
    if (_isConnecting[index] == true) {
      return const Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [CircularProgressIndicator(), SizedBox(height: 16), Text('Connecting...', style: TextStyle(color: Colors.white))]));
    }
    if (!controller.value.isPlaying && !controller.value.isBuffering) {
      return const Center(child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [Icon(Icons.videocam_off_outlined, color: Colors.grey, size: 48), SizedBox(height: 8), Text('Camera Not Available', style: TextStyle(color: Colors.white, fontSize: 18))]));
    }
    return VlcPlayer(controller: controller, aspectRatio: 16 / 9, placeholder: const Center(child: CircularProgressIndicator()));
  }

  void _selectAndAttemptConnection(int index, AppState appState) {
    appState.selectCamera(index);
    if (index >= _controllers.length) return;
    final controller = _controllers[index];
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
            Navigator.pushReplacement(
              context,
              MaterialPageRoute(builder: (context) => SettingsScreen()),
            ).then((_) {
              // When returning from settings, reload the control screen to apply changes
              Navigator.pushReplacement(
                context,
                MaterialPageRoute(builder: (context) => ControlScreen()),
              );
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