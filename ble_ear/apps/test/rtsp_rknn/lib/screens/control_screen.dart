import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_vlc_player/flutter_vlc_player.dart';
import 'package:provider/provider.dart';
import 'package:rtsp_rknn/screens/settings_screen.dart';
import 'package:rtsp_rknn/services/robot_service.dart';
import 'package:rtsp_rknn/state/app_state.dart';

class ControlScreen extends StatefulWidget {
  @override
  _ControlScreenState createState() => _ControlScreenState();
}

class _ControlScreenState extends State<ControlScreen> {
  late VlcPlayerController _videoPlayerController;
  late RobotService _robotService;
  String? _currentlyPlayingUrl;

  Timer? _connectionTimer;
  bool _isAttemptingConnection = false;

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
    {'label': 'Auto Attack', 'cmd': 'CMD_AUTO_ATTACK'},
    {'label': 'Manual Attack', 'cmd': 'CMD_MANU_ATTACK'}
  ];

  @override
  void initState() {
    super.initState();
    final appState = Provider.of<AppState>(context, listen: false);
    _currentlyPlayingUrl = appState.currentCameraUrl;

    _videoPlayerController = VlcPlayerController.network(
      _currentlyPlayingUrl!,
      hwAcc: HwAcc.full,
      autoPlay: false,
      options: VlcPlayerOptions(),
    );

    _robotService = RobotService(ipAddress: appState.robotIpAddress);
    _robotService.connect();

    appState.addListener(_onAppStateChanged);
    _videoPlayerController.addListener(_onPlayerStateChanged);

    // Use a post-frame callback to ensure the widget is built before starting
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _initiateConnectionAttempt();
    });
  }

  void _initiateConnectionAttempt() {
    if (_isAttemptingConnection || !mounted) return;

    setState(() {
      _isAttemptingConnection = true;
    });

    _connectionTimer?.cancel();
    _videoPlayerController.play();

    _connectionTimer = Timer(const Duration(seconds: 10), () {
      if (mounted && !_videoPlayerController.value.isPlaying) {
        setState(() {
          _isAttemptingConnection = false;
        });
      }
    });
  }

  void _onPlayerStateChanged() {
    if (_videoPlayerController.value.isPlaying && mounted) {
      _connectionTimer?.cancel();
      if (_isAttemptingConnection) {
        setState(() {
          _isAttemptingConnection = false;
        });
      }
    }
  }

  void _onAppStateChanged() {
    final appState = Provider.of<AppState>(context, listen: false);

    if (_robotService.ipAddress != appState.robotIpAddress) {
      _robotService.dispose();
      _robotService = RobotService(ipAddress: appState.robotIpAddress);
      _robotService.connect();
    }

    if (_currentlyPlayingUrl != appState.currentCameraUrl) {
      _currentlyPlayingUrl = appState.currentCameraUrl;
      if (_videoPlayerController.value.isInitialized) {
        _videoPlayerController.setMediaFromNetwork(appState.currentCameraUrl);
        _initiateConnectionAttempt();
      }
    }
  }

  @override
  void dispose() {
    _connectionTimer?.cancel();
    _videoPlayerController.removeListener(_onPlayerStateChanged);
    Provider.of<AppState>(context, listen: false).removeListener(_onAppStateChanged);
    _videoPlayerController.dispose();
    _robotService.dispose();
    super.dispose();
  }

  // --- WIDGETS ---

  Widget _buildVideoPlayerWidget() {
    if (_isAttemptingConnection) {
      return const Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            CircularProgressIndicator(),
            SizedBox(height: 16),
            Text('Connecting...', style: TextStyle(color: Colors.white)),
          ],
        ),
      );
    }

    if (!_videoPlayerController.value.isPlaying && !_videoPlayerController.value.isBuffering) {
      return const Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(Icons.videocam_off_outlined, color: Colors.grey, size: 48),
            SizedBox(height: 8),
            Text('Camera Not Available', style: TextStyle(color: Colors.white, fontSize: 18)),
          ],
        ),
      );
    }

    return VlcPlayer(
      controller: _videoPlayerController,
      aspectRatio: 16 / 9,
      placeholder: const Center(child: CircularProgressIndicator()),
    );
  }

  @override
  Widget build(BuildContext context) {
    final appState = Provider.of<AppState>(context);
    final screenHeight = MediaQuery.of(context).size.height;
    final screenWidth = MediaQuery.of(context).size.width;

    return Scaffold(
      body: SafeArea(
        child: Column(
          children: [
            // --- TOP SECTION (Takes 75% of screen height) ---
            SizedBox(
              height: screenHeight * 0.75 - MediaQuery.of(context).padding.top,
              child: Row(
                children: [
                  // 1. Left-side Scrollable Command List
                  Container(
                    width: screenWidth * 0.25,
                    color: Theme.of(context).scaffoldBackgroundColor,
                    child: ListView.builder(
                      padding: const EdgeInsets.symmetric(vertical: 8.0, horizontal: 4.0),
                      itemCount: _commands.length,
                      itemBuilder: (context, index) {
                        final command = _commands[index];
                        final isManualAttack = command['cmd'] == 'CMD_MANU_ATTACK';
                        return _commandButton(
                          label: command['label']!,
                          onPressed: isManualAttack
                              ? () => _showAttackConfirmationDialog(command['cmd']!)
                              : () => _robotService.sendCommand(command['cmd']!),
                        );
                      },
                    ),
                  ),

                  // 2. Right-side Video Area
                  Expanded(
                    child: Container(
                      color: Colors.black,
                      child: Column(
                        children: [
                          _buildCameraSwitcher(appState),
                          Expanded(child: _buildVideoPlayerWidget()),
                        ],
                      ),
                    ),
                  ),
                ],
              ),
            ),

            // --- BOTTOM CONTROL BAR (Takes remaining space) ---
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

  // --- HELPER WIDGETS ---

  Widget _buildCameraSwitcher(AppState appState) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4.0, horizontal: 8.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          // Each button is wrapped in Expanded to prevent overflow
          Expanded(child: _cameraSelectButton(0, "Cam 1", appState)),
          const SizedBox(width: 8),
          Expanded(child: _cameraSelectButton(1, "Cam 2", appState)),
          const SizedBox(width: 8),
          Expanded(child: _cameraSelectButton(2, "Cam 3", appState)),
        ],
      ),
    );
  }

  Widget _buildBottomBar() {
    return Row(
      children: [
        _bottomBarButton(label: "Return", command: "CMD_RETUR"),
        const SizedBox(width: 8),
        _bottomBarButton(label: "Attack", command: "CMD_START", color: Colors.orange),
        const SizedBox(width: 8),
        _bottomBarButton(label: "STOP", command: "CMD_STOP", color: Colors.blue),
        const SizedBox(width: 8),
        _bottomBarButton(icon: Icons.add, command: "CMD_ZOOM_IN"),
        const SizedBox(width: 8),
        _bottomBarButton(icon: Icons.remove, command: "CMD_ZOOM_OUT"),
        const SizedBox(width: 8),
        _bottomBarButton(
          icon: Icons.settings,
          onPressed: () {
            Navigator.push(context, MaterialPageRoute(builder: (context) => SettingsScreen()))
                .then((_) => _initiateConnectionAttempt());
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
          style: ElevatedButton.styleFrom(
            textStyle: const TextStyle(fontSize: 12), // Smaller font to prevent wrapping
            padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 4),
          ),
          child: Text(label, textAlign: TextAlign.center),
        ),
      ),
    );
  }

  // Generic helper for creating rectangular buttons on the bottom bar
  Widget _bottomBarButton({String? label, IconData? icon, String? command, Color? color, VoidCallback? onPressed}) {
    return Expanded(
      child: ElevatedButton(
        style: ElevatedButton.styleFrom(
          backgroundColor: color,
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8.0)), // Rectangular shape
          padding: const EdgeInsets.symmetric(vertical: 12),
        ),
        onPressed: onPressed ?? () => _robotService.sendCommand(command!),
        child: icon != null ? Icon(icon) : Text(label!, style: const TextStyle(fontSize: 12)),
      ),
    );
  }

  Widget _cameraSelectButton(int index, String label, AppState appState) {
    return ElevatedButton(
      onPressed: () => appState.selectCamera(index),
      child: Text(label),
      style: ElevatedButton.styleFrom(
        backgroundColor: appState.selectedCameraIndex == index ? Theme.of(context).primaryColor : Colors.grey.shade600,
      ),
    );
  }

  void _showAttackConfirmationDialog(String command) {
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
            },
            child: const Text("OK"),
          ),
        ],
      ),
    );
  }
}