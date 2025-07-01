import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter_vlc_player/flutter_vlc_player.dart';
import 'package:google_fonts/google_fonts.dart';

// Make sure you have a file named 'icon_constants.dart' in your lib/ folder
// with all the icon paths.
import 'icon_constants.dart';

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
      title: 'Robot Control App',
      theme: ThemeData(
        primarySwatch: Colors.blue,
        textTheme: GoogleFonts.rajdhaniTextTheme(),
      ),
      home: const HomePage(),
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
    'rtsp://192.168.0.158:8554/cam1',
    'rtsp://192.168.0.158:8554/cam2',
  ];
  int _currentCameraIndex = -1;
  String? _errorMessage;

  // --- UI State ---
  int _activeLeftButtonIndex = 0;
  int _activeRightButtonIndex = 0;
  bool _isStarted = false;

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

  Future<void> _switchCamera(int index) async {
    if (index == _currentCameraIndex && _vlcPlayerController != null) return;
    final VlcPlayerController? oldController = _vlcPlayerController;

    setState(() {
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
        advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(150)]),
        video: VlcVideoOptions(
            [VlcVideoOptions.dropLateFrames(true), VlcVideoOptions.skipFrames(true)]),
        extras: ['--h264-fps=60', '--no-audio'],
      ),
    );

    newController.addListener(() {
      if (!mounted) return;
      if (newController.value.hasError &&
          _errorMessage != newController.value.errorDescription) {
        setState(() => _errorMessage = newController.value.errorDescription);
      }
    });

    if (mounted) {
      setState(() => _vlcPlayerController = newController);
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
    return Scaffold(
      backgroundColor: Colors.black,
      body: Stack(
        fit: StackFit.expand,
        children: [
          // Layer 1: Video Player Background
          Positioned.fill(child: buildPlayerWidget()),

          // Layer 2: UI Controls Overlay
          // Left Side Buttons
          Positioned(
            left: 15,
            top: 20, // Padding from top
            bottom: 80, // Padding from bottom to avoid the bottom bar
            width: 120, // Give the list view a defined width
            child: ListView(
              children: [
                _buildSideButton(0, _activeLeftButtonIndex == 0, ICON_PATH_DRIVING_ACTIVE, ICON_PATH_DRIVING_INACTIVE, "Driving", () => _onLeftButtonPressed(0, 'CMD_MODE_DRIVE')),
                const SizedBox(height: 20),
                _buildSideButton(1, _activeLeftButtonIndex == 1, ICON_PATH_PETROL_ACTIVE, ICON_PATH_PETROL_INACTIVE, "Petrol", () => _onLeftButtonPressed(1, 'CMD_MODE_PATROL')),
                const SizedBox(height: 20),
                _buildSideButton(2, _activeLeftButtonIndex == 2, ICON_PATH_RECON_ACTIVE, ICON_PATH_RECON_INACTIVE, "Recon", () => _onLeftButtonPressed(2, 'CMD_MODE_RECON')),
                const SizedBox(height: 20),
                _buildSideButton(3, _activeLeftButtonIndex == 3, ICON_PATH_MANUAL_ATTACK_ACTIVE, ICON_PATH_MANUAL_ATTACK_INACTIVE, "Manual Attack", () => _onLeftButtonPressed(3, 'CMD_MANU_ATTACK')),
                const SizedBox(height: 20),
                _buildSideButton(4, _activeLeftButtonIndex == 4, ICON_PATH_DRONE_ACTIVE, ICON_PATH_DRONE_INACTIVE, "Drone", () => _onLeftButtonPressed(4, 'CMD_MODE_DRONE')),
                const SizedBox(height: 20),
                _buildSideButton(5, _activeLeftButtonIndex == 5, ICON_PATH_RETURN_ACTIVE, ICON_PATH_RETURN_INACTIVE, "Return", () => _onLeftButtonPressed(5, 'CMD_RETURN')),
              ],
            ),
          ),

          // --- FIXED: Right Side Buttons are now in a scrollable ListView ---
          Positioned(
            right: 15,
            top: 20,  // Padding from top
            bottom: 80, // Padding from bottom
            width: 120, // Give the list view a defined width
            child: ListView(
              children: [
                _buildSideButton(0, _activeRightButtonIndex == 0, ICON_PATH_ATTACK_VIEW_ACTIVE, ICON_PATH_ATTACK_VIEW_INACTIVE, "Attack View", () => _onRightButtonPressed(0)),
                const SizedBox(height: 20),
                _buildSideButton(1, _activeRightButtonIndex == 1, ICON_PATH_TOP_VIEW_ACTIVE, ICON_PATH_TOP_VIEW_INACTIVE, "Top View", () => _onRightButtonPressed(1)),
                const SizedBox(height: 20),
                _buildSideButton(2, _activeRightButtonIndex == 2, ICON_PATH_3D_VIEW_ACTIVE, ICON_PATH_3D_VIEW_INACTIVE, "3D View", () => _onRightButtonPressed(2)),
                const SizedBox(height: 120), // Fixed gap
                _buildSideButton(-1, false, ICON_PATH_SETTINGS, ICON_PATH_SETTINGS, "Setting", () => _navigateToSettings()),
              ],
            ),
          ),

          // Bottom Control Bar (already scrollable, no changes needed)
          Positioned(
            bottom: 10,
            left: 15,
            right: 15,
            child: SingleChildScrollView(
              scrollDirection: Axis.horizontal,
              child: Row(
                children: [
                  _buildBottomButton("ATTACK", ICON_PATH_ATTACK, [const Color(0xffc32121), const Color(0xff831616)], () async {
                    final proceed = await _showConfirmationDialog(context, 'Confirm Attack', 'Do you want to proceed?');
                    if (proceed) _sendCommand('CMD_MODE_ATTACK');
                  }),
                  const SizedBox(width: 12),
                  _buildBottomButton(_isStarted ? "STOP" : "START", _isStarted ? ICON_PATH_STOP : ICON_PATH_START, [const Color(0xff25a625), const Color(0xff127812)], () {
                    setState(() => _isStarted = !_isStarted);
                    _sendCommand(_isStarted ? 'CMD_STOP' : 'CMD_START');
                  }),
                  const SizedBox(width: 12),
                  _buildBottomButton("", ICON_PATH_PLUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_IN')),
                  const SizedBox(width: 12),
                  _buildBottomButton("", ICON_PATH_MINUS, [const Color(0xffc0c0c0), const Color(0xffa0a0a0)], () => _sendCommand('CMD_ZOOM_OUT')),
                  const SizedBox(width: 12),
                  Image.asset(ICON_PATH_WIFI, height: 40, width: 40),
                  const SizedBox(width: 12),
                  _buildBottomButton("EXIT", ICON_PATH_EXIT, [const Color(0xff1e78c3), const Color(0xff12569b)], () => _sendCommand('CMD_EXIT')),
                ],
              ),
            ),
          )
        ],
      ),
    );
  }

  // --- UI HELPER FUNCTIONS & WIDGET BUILDERS ---

  void _onLeftButtonPressed(int index, String command) {
    setState(() => _activeLeftButtonIndex = index);
    _sendCommand(command);
  }

  void _onRightButtonPressed(int index) {
    setState(() => _activeRightButtonIndex = index);
    _switchCamera(index);
  }

  // --- FIXED: Player widget with Retry button ---
  Widget buildPlayerWidget() {
    if (_vlcPlayerController == null) {
      return const Center(child: CircularProgressIndicator(color: Colors.white));
    }
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
            ElevatedButton(
              onPressed: () => _switchCamera(_currentCameraIndex),
              style: ElevatedButton.styleFrom(backgroundColor: Colors.grey[700]),
              child: const Text('Retry', style: TextStyle(color: Colors.white)),
            ),
          ],
        ),
      );
    }
    return VlcPlayer(
      controller: _vlcPlayerController!,
      aspectRatio: 16 / 9,
      placeholder: const Center(child: CircularProgressIndicator(color: Colors.white)),
    );
  }

  Widget _buildSideButton(int index, bool isActive, String activeIcon,
      String inactiveIcon, String label, VoidCallback onPressed) {
    return GestureDetector(
      onTap: onPressed,
      child: Container(
        padding: const EdgeInsets.symmetric(vertical: 8, horizontal: 12),
        decoration: BoxDecoration(
            color: Colors.black.withOpacity(0.6),
            borderRadius: BorderRadius.circular(10),
            border: Border.all(
                color: isActive ? Colors.white : Colors.transparent,
                width: 1.5)),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Image.asset(isActive ? activeIcon : inactiveIcon,
                height: 40, width: 40),
            const SizedBox(height: 5),
            Text(label,
                style: const TextStyle(
                    color: Colors.white,
                    fontSize: 14,
                    fontWeight: FontWeight.bold)),
          ],
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
                      letterSpacing: 1.2)),
            ]
          ],
        ),
      ),
    );
  }
}