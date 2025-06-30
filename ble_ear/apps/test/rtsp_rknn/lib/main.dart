import 'package:flutter/material.dart';
import 'package:flutter_vlc_player/flutter_vlc_player.dart';

// SettingsPage remains the same...
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
    _urlControllers = widget.cameraUrls.map((url) => TextEditingController(text: url)).toList();
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
      appBar: AppBar(title: const Text('Settings'), backgroundColor: Colors.grey[200], foregroundColor: Colors.black87),
      body: ListView(
        padding: const EdgeInsets.all(16.0),
        children: [
          for (int i = 0; i < _urlControllers.length; i++)
            Padding(
              padding: const EdgeInsets.only(bottom: 16.0),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('Camera ${i + 1} URL', style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                  const SizedBox(height: 8),
                  TextFormField(
                    controller: _urlControllers[i],
                    decoration: const InputDecoration(border: OutlineInputBorder(), hintText: 'e.g., rtsp://...'),
                  ),
                ],
              ),
            ),
          const SizedBox(height: 20),
          ElevatedButton(
            onPressed: () {
              final newUrls = _urlControllers.map((controller) => controller.text).toList();
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
        visualDensity: VisualDensity.adaptivePlatformDensity,
        elevatedButtonTheme: ElevatedButtonThemeData(
          style: ButtonStyle(
            foregroundColor: MaterialStateProperty.all<Color>(Colors.black87),
          ),
        ),
      ),
      home: const HomePage(),
    );
  }
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  VlcPlayerController? _vlcPlayerController;

  List<String> _cameraUrls = [
    'rtsp://192.168.0.158:8554/cam0',
    'rtsp://192.168.0.158:8554/cam0',
    'rtsp://192.168.0.158:8554/cam0',
  ];
  int _currentCameraIndex = -1; // Start at -1 to ensure first switch runs

  bool _isStarted = false;
  bool _isAutoAttack = false;
  String? _errorMessage;

  @override
  void initState() {
    super.initState();
    // Initialize with the first camera URL
    _switchCamera(0);
  }

  // --- REFACTORED AND CORRECTED CAMERA SWITCHING LOGIC ---
  Future<void> _switchCamera(int index) async {
    // If the selected camera is already active, do nothing.
    if (_vlcPlayerController != null && index == _currentCameraIndex) {
      return;
    }

    // Store the old controller so we can dispose of it cleanly.
    final VlcPlayerController? oldController = _vlcPlayerController;

    // **STEP 1: Remove the player from the UI immediately.**
    // This is crucial to start the teardown process of the native view.
    setState(() {
      _currentCameraIndex = index;
      _vlcPlayerController = null; // Show a loading spinner
      _errorMessage = null;
    });

    // **STEP 2: Dispose of the old controller.**
    await oldController?.dispose();

    // **STEP 3 (THE CRITICAL FIX): Wait for a brief moment.**
    // This gives the native side time to fully release all resources (sockets, surfaces)
    // and prevents the race condition that causes both errors.
    await Future.delayed(const Duration(milliseconds: 200));

    // STEP 4: Create the new controller only after the system is clean.
    final newController = VlcPlayerController.network(
      _cameraUrls[index],
      hwAcc: HwAcc.disabled,
      autoPlay: true,
      options: VlcPlayerOptions(
        advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(150)]),
        video: VlcVideoOptions([
          VlcVideoOptions.dropLateFrames(true),
          VlcVideoOptions.skipFrames(true),
        ]),
        extras: ['--h264-fps=60', '--aspect-ratio=16:9', '--no-audio'],
      ),
    );

    // Add a listener for the new controller.
    newController.addListener(() {
      if (!mounted) return;
      final state = newController.value;
      // Update the error message only if it has changed, to avoid extra rebuilds.
      if (state.hasError && _errorMessage != state.errorDescription) {
        setState(() {
          _errorMessage = state.errorDescription;
        });
      }
    });

    // STEP 5: Assign the new controller to the state to display it.
    if (mounted) {
      setState(() {
        _vlcPlayerController = newController;
      });
    }
  }


  @override
  void dispose() {
    _vlcPlayerController?.dispose();
    super.dispose();
  }

  void _sendCommand(String command) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Command sent: $command')),
    );
  }

  void _navigateToSettings() async {
    final newUrls = await Navigator.push<List<String>>(
      context,
      MaterialPageRoute(
        builder: (context) => SettingsPage(cameraUrls: _cameraUrls),
      ),
    );

    if (newUrls != null) {
      bool hasUrlChanged = false;
      for (int i = 0; i < _cameraUrls.length; i++) {
        if (_cameraUrls[i] != newUrls[i]) {
          hasUrlChanged = true;
          break;
        }
      }

      if (hasUrlChanged) {
        setState(() {
          _cameraUrls = newUrls;
        });
        // Re-initialize the player with the potentially updated URL
        _switchCamera(_currentCameraIndex);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Robot Control App'),
        elevation: 0,
        backgroundColor: Colors.grey[200],
        foregroundColor: Colors.black87,
      ),
      body: Column(
        children: [
          Expanded(
            flex: 7,
            child: Row(
              children: [
                Container(
                  width: MediaQuery.of(context).size.width * 0.30,
                  color: Colors.lightGreen[100],
                  child: ListView(
                    padding: const EdgeInsets.all(4.0),
                    children: [
                      _buildCommandButton('Driving', () => _sendCommand('CMD_MODE_DRIVE')),
                      _buildCommandButton('Patrol', () => _sendCommand('CMD_MODE_PATROL')),
                      _buildCommandButton('Recon', () => _sendCommand('CMD_MODE_RECON')),
                      _buildCommandButton('Drone', () => _sendCommand('CMD_MODE_DRONE')),
                      _buildCommandButton('Return', () => _sendCommand('CMD_RETURN')),
                      _buildCommandButton('Forward', () => _sendCommand('CMD_FORWARD')),
                      _buildCommandButton('Backward', () => _sendCommand('CMD_BACKWARD')),
                      _buildCommandButton('Left', () => _sendCommand('CMD_LEFT')),
                      _buildCommandButton('Right', () => _sendCommand('CMD_RIGHT')),
                      _buildCommandButton('Turn Left', () => _sendCommand('CMD_TURN_LEFT')),
                      _buildCommandButton('Turn Right', () => _sendCommand('CMD_TURN_RIGHT')),
                      _buildCommandButton('Scan', () => _sendCommand('CMD_SCAN')),
                    ],
                  ),
                ),
                Expanded(
                  child: Column(
                    children: [
                      Padding(
                        padding: const EdgeInsets.fromLTRB(8, 8, 8, 0),
                        child: Row(
                          children: [
                            for (int i = 0; i < 3; i++)
                              Expanded(
                                child: Padding(
                                  padding: const EdgeInsets.symmetric(horizontal: 4.0),
                                  child: ElevatedButton(
                                    onPressed: () => _switchCamera(i),
                                    style: ElevatedButton.styleFrom(
                                      backgroundColor: _currentCameraIndex == i ? Theme.of(context).primaryColor : Colors.grey[300],
                                      foregroundColor: _currentCameraIndex == i ? Colors.white : Colors.black87,
                                    ),
                                    child: Text('Cam ${i + 1}'),
                                  ),
                                ),
                              ),
                          ],
                        ),
                      ),
                      Expanded(
                        child: Container(
                          margin: const EdgeInsets.all(8.0),
                          color: Colors.black,
                          child: _errorMessage != null
                              ? Center(
                            child: Padding(
                              padding: const EdgeInsets.all(8.0),
                              child: Text(
                                'Error: $_errorMessage',
                                textAlign: TextAlign.center,
                                style: const TextStyle(color: Colors.red, fontSize: 16),
                              ),
                            ),
                          )
                          // This now correctly shows the placeholder during the switch
                              : _vlcPlayerController == null
                              ? const Center(child: CircularProgressIndicator())
                              : VlcPlayer(
                            controller: _vlcPlayerController!,
                            placeholder: const Center(child: CircularProgressIndicator()), aspectRatio: 1/1,
                          ),
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
          Expanded(
            flex: 3,
            child: Container(
              color: Colors.grey[200],
              child: Center(
                child: SingleChildScrollView(
                  scrollDirection: Axis.horizontal,
                  padding: const EdgeInsets.symmetric(horizontal: 4.0),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      _buildBottomButton(
                        _isAutoAttack ? 'Auto Attack' : 'Attack',
                        _isAutoAttack ? Colors.redAccent : Colors.orange,
                            () {
                          setState(() => _isAutoAttack = !_isAutoAttack);
                          _sendCommand(_isAutoAttack ? 'CMD_AUTO_ATTACK' : 'CMD_ATTACK');
                        },
                      ),
                      _buildBottomButton(
                        _isStarted ? 'Stop' : 'Start',
                        _isStarted ? Colors.green : Colors.grey[300],
                            () {
                          setState(() => _isStarted = !_isStarted);
                          _sendCommand(_isStarted ? 'CMD_STOP' : 'CMD_START');
                        },
                      ),
                      _buildIconBottomButton(Icons.add, () => _sendCommand('CMD_ZOOM_IN')),
                      _buildIconBottomButton(Icons.remove, () => _sendCommand('CMD_ZOOM_OUT')),
                      _buildBottomButton('Settings', Colors.grey[400], _navigateToSettings),
                      _buildBottomButton('EXIT', Colors.blue, () => _sendCommand('CMD_EXIT')),
                    ],
                  ),
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildCommandButton(String label, VoidCallback onPressed) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2.0),
      child: ElevatedButton(onPressed: onPressed, style: ElevatedButton.styleFrom(minimumSize: const Size(0, 50), backgroundColor: Colors.grey[300], shape: const BeveledRectangleBorder(borderRadius: BorderRadius.zero), padding: const EdgeInsets.symmetric(horizontal: 4.0)), child: SizedBox(width: double.infinity, child: Text(label, textAlign: TextAlign.center, style: const TextStyle(fontSize: 14, fontWeight: FontWeight.normal)))),
    );
  }
  Widget _buildIconBottomButton(IconData icon, VoidCallback onPressed) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 4.0),
      child: ElevatedButton(onPressed: onPressed, style: ElevatedButton.styleFrom(minimumSize: const Size(60, 50), backgroundColor: Colors.grey[300], shape: const BeveledRectangleBorder(borderRadius: BorderRadius.zero)), child: Icon(icon, size: 30.0)),
    );
  }
  Widget _buildBottomButton(String label, Color? color, VoidCallback onPressed) {
    final textColor = (label == 'EXIT' || (label == 'Auto Attack')) ? Colors.white : Colors.black87;
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 4.0),
      child: ElevatedButton(onPressed: onPressed, style: ElevatedButton.styleFrom(minimumSize: const Size(60, 50), backgroundColor: color, foregroundColor: textColor, shape: const BeveledRectangleBorder(borderRadius: BorderRadius.zero)), child: Text(label, textAlign: TextAlign.center, style: const TextStyle(fontSize: 14, fontWeight: FontWeight.normal))),
    );
  }
}