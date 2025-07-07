// import 'package:flutter/material.dart';
// import 'package:flutter_vlc_player/flutter_vlc_player.dart';
//
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
//       title: 'Robot Control App',
//       theme: ThemeData(
//         primarySwatch: Colors.blue,
//         visualDensity: VisualDensity.adaptivePlatformDensity,
//       ),
//       home: const HomePage(),
//     );
//   }
// }
//
// class HomePage extends StatefulWidget {
//   const HomePage({super.key});
//
//   @override
//   State<HomePage> createState() => _HomePageState();
// }
//
// class _HomePageState extends State<HomePage> {
//   VlcPlayerController? _vlcPlayerController;
//   final TextEditingController _urlController = TextEditingController(
//     text: 'rtsp://192.168.0.158:8554/cam0',
//   );
//   bool _isPlaying = false;
//   String? _errorMessage;
//
//   @override
//   void initState() {
//     super.initState();
//     _initializePlayer(_urlController.text);
//   }
//
//   void _initializePlayer(String url) {
//     _vlcPlayerController?.dispose();
//     // _vlcPlayerController = VlcPlayerController.network(
//     //   url,
//     //   hwAcc: HwAcc.auto,
//     //   autoPlay: true,
//     //   options: VlcPlayerOptions(
//     //     advanced: VlcAdvancedOptions([
//     //       VlcAdvancedOptions.networkCaching(10),
//     //     ]),
//     //     rtp: VlcRtpOptions([
//     //       VlcRtpOptions.rtpOverRtsp(false),
//     //     ]),
//     //   ),
//     // )
//
//     _vlcPlayerController =  VlcPlayerController.network(
//       url,
//       hwAcc: HwAcc.disabled,
//       autoPlay: true,
//       options: VlcPlayerOptions(
//           video: VlcVideoOptions([VlcVideoOptions.dropLateFrames(true),
//             VlcVideoOptions.skipFrames(false)],),
//           rtp: VlcRtpOptions([
//             VlcRtpOptions.rtpOverRtsp(true),
//             ":rtsp-tcp",
//           ]),
//           advanced: VlcAdvancedOptions([
//             VlcAdvancedOptions.networkCaching(100),
//             VlcAdvancedOptions.clockJitter(0),
//             VlcAdvancedOptions.fileCaching(30),
//             VlcAdvancedOptions.liveCaching(30),
//             VlcAdvancedOptions.clockSynchronization(1),
//           ]),
//           sout: VlcStreamOutputOptions([
//             VlcStreamOutputOptions.soutMuxCaching(0),
//           ]),
//           extras: ['--h264-fps=60']
//       ),
//      )..addListener(() {
//       final state = _vlcPlayerController!.value;
//       setState(() {
//         _isPlaying = state.isPlaying;
//         if (state.errorDescription != null && state.errorDescription!.isNotEmpty) {
//           _errorMessage = state.errorDescription;
//           _isPlaying = false;
//         } else {
//           _errorMessage = null;
//         }
//       });
//      }
//     );
//   }
//
//   @override
//   void dispose() {
//     _vlcPlayerController?.dispose();
//     _urlController.dispose();
//     super.dispose();
//   }
//
//   void _updateStreamUrl() async {
//     if (_vlcPlayerController != null) {
//       if (_vlcPlayerController!.value.isPlaying) {
//         await _vlcPlayerController!.stop();
//       }
//     }
//     setState(() {
//       _isPlaying = false;
//       _errorMessage = null;
//       _initializePlayer(_urlController.text);
//     });
//   }
//
//   void _sendCommand(String command) {
//     ScaffoldMessenger.of(context).showSnackBar(
//       SnackBar(content: Text('Command sent: $command')),
//     );
//   }
//
//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       appBar: AppBar(
//         title: const Text('Robot Control App'),
//         elevation: 2,
//       ),
//       body: Row(
//         children: [
//           // Left side: Command buttons (20% width) with scroll
//           Container(
//             width: MediaQuery.of(context).size.width * 0.20,
//             color: Colors.lightGreen[100],
//             child: ListView(
//               padding: const EdgeInsets.all(8.0),
//               children: [
//                 _buildCommandButton('Driving', () => _sendCommand('CMD_MODE_DRIVE')),
//                 _buildCommandButton('Petrol', () => _sendCommand('CMD_MODE_PATROL')),
//                 _buildCommandButton('Recon', () => _sendCommand('CMD_MODE_RECON')),
//                 _buildCommandButton('Manual Attack', () => _sendCommand('CMD_MANU_ATTACK')),
//                 _buildCommandButton('Auto Attack', () => _sendCommand('CMD_AUTO_ATTACK')),
//                 _buildCommandButton('Drone', () => _sendCommand('CMD_MODE_DRONE')),
//                 _buildCommandButton('Return', () => _sendCommand('CMD_RETURN')),
//               ],
//             ),
//           ),
//           // Right side: Camera view and bottom buttons
//           Expanded(
//             child: Column(
//               children: [
//                 // Camera view (flexible height with Expanded)
//                 Expanded(
//                   flex: 7,
//                   child: Container(
//                     margin: const EdgeInsets.all(8.0),
//                     child: _vlcPlayerController != null
//                         ? _errorMessage != null
//                         ? Center(
//                       child: Column(
//                         mainAxisAlignment: MainAxisAlignment.center,
//                         children: [
//                           Text(
//                             'Error: $_errorMessage',
//                             style: const TextStyle(color: Colors.red, fontSize: 16),
//                           ),
//                           const SizedBox(height: 10),
//                           ElevatedButton(
//                             onPressed: _updateStreamUrl,
//                             child: const Text('Retry'),
//                           ),
//                         ],
//                       ),
//                     )
//                         : VlcPlayer(
//                       controller: _vlcPlayerController!,
//                       aspectRatio: 16 / 9,
//                       placeholder: const Center(child: CircularProgressIndicator()),
//                     )
//                         : const Center(child: Text('Loading camera...')),
//                   ),
//                 ),
//                 // Bottom buttons (flexible height with Expanded)
//                 Expanded(
//                   flex: 3,
//                   child: Container(
//                     color: Colors.grey[200],
//                     padding: const EdgeInsets.all(8.0),
//                     child: Row(
//                       mainAxisAlignment: MainAxisAlignment.spaceEvenly,
//                       children: [
//                         Flexible(
//                           child: Padding(
//                             padding: const EdgeInsets.symmetric(horizontal: 4.0),
//                             child: ElevatedButton(
//                               onPressed: () => _sendCommand('CMD_ATTACK'),
//                               style: ElevatedButton.styleFrom(
//                                 backgroundColor: Colors.orange,
//                                 padding: const EdgeInsets.symmetric(vertical: 8),
//                               ),
//                               child: const Text('Attack'),
//                             ),
//                           ),
//                         ),
//                         Flexible(
//                           child: Padding(
//                             padding: const EdgeInsets.symmetric(horizontal: 4.0),
//                             child: ElevatedButton(
//                               onPressed: () => _sendCommand('CMD_STOP'),
//                               style: ElevatedButton.styleFrom(
//                                 padding: const EdgeInsets.symmetric(vertical: 8),
//                               ),
//                               child: const Text('STOP'),
//                             ),
//                           ),
//                         ),
//                         Flexible(
//                           child: Padding(
//                             padding: const EdgeInsets.symmetric(horizontal: 4.0),
//                             child: ElevatedButton(
//                               onPressed: () => _sendCommand('CMD_ZOOM_IN'),
//                               style: ElevatedButton.styleFrom(
//                                 padding: const EdgeInsets.symmetric(vertical: 8),
//                               ),
//                               child: const Text('+'),
//                             ),
//                           ),
//                         ),
//                         Flexible(
//                           child: Padding(
//                             padding: const EdgeInsets.symmetric(horizontal: 4.0),
//                             child: ElevatedButton(
//                               onPressed: () => _sendCommand('CMD_ZOOM_OUT'),
//                               style: ElevatedButton.styleFrom(
//                                 padding: const EdgeInsets.symmetric(vertical: 8),
//                               ),
//                               child: const Text('-'),
//                             ),
//                           ),
//                         ),
//                         Flexible(
//                           child: Padding(
//                             padding: const EdgeInsets.symmetric(horizontal: 4.0),
//                             child: ElevatedButton(
//                               onPressed: () => _sendCommand('CMD_EXIT'),
//                               style: ElevatedButton.styleFrom(
//                                 backgroundColor: Colors.blue,
//                                 padding: const EdgeInsets.symmetric(vertical: 8),
//                               ),
//                               child: const Text('EXIT'),
//                             ),
//                           ),
//                         ),
//                       ],
//                     ),
//                   ),
//                 ),
//               ],
//             ),
//           ),
//         ],
//       ),
//     );
//   }
//
//   Widget _buildCommandButton(String label, VoidCallback onPressed) {
//     return Padding(
//       padding: const EdgeInsets.symmetric(vertical: 4.0),
//       child: ElevatedButton(
//         onPressed: onPressed,
//         style: ElevatedButton.styleFrom(
//           minimumSize: const Size(double.infinity, 50),
//           shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
//         ),
//         child: Text(
//           label,
//           style: const TextStyle(fontSize: 16),
//         ),
//       ),
//     );
//   }
// }

// void _initializePlayer(String url) {
//   _vlcPlayerController?.dispose();
//   _vlcPlayerController = VlcPlayerController.network(
//     url,
//     hwAcc: HwAcc.disabled,
//     autoPlay: true,
//     options: VlcPlayerOptions(
//       video: VlcVideoOptions([
//         VlcVideoOptions.dropLateFrames(true),
//         VlcVideoOptions.skipFrames(false),
//       ]),
//       rtp: VlcRtpOptions([VlcRtpOptions.rtpOverRtsp(true), ":rtsp-tcp"]),
//       advanced: VlcAdvancedOptions([VlcAdvancedOptions.networkCaching(100)]),
//       extras: ['--h264-fps=60', '--aspect-ratio=16:9'],
//     ),
//   )..addListener(() {
//     if (!mounted) return;
//     final state = _vlcPlayerController!.value;
//     if (state.hasError) {
//       setState(() {
//         _errorMessage = state.errorDescription;
//       });
//     } else {
//       if (_errorMessage != null) {
//         setState(() {
//           _errorMessage = null;
//         });
//       }
//     }
//   });
//   setState(() {});
// }


// void _initializePlayer(String url) {
//   _vlcPlayerController?.dispose();
//   _vlcPlayerController  = VlcPlayerController.network(
//     url,
//     hwAcc: HwAcc.disabled,
//     autoPlay: true,
//     options: VlcPlayerOptions(
//         video: VlcVideoOptions([VlcVideoOptions.dropLateFrames(true),
//           VlcVideoOptions.skipFrames(false)],),
//         rtp: VlcRtpOptions([
//           VlcRtpOptions.rtpOverRtsp(true),
//           ":rtsp-tcp",
//         ]),
//         advanced: VlcAdvancedOptions([
//           VlcAdvancedOptions.networkCaching(50),
//           VlcAdvancedOptions.clockJitter(0),
//           VlcAdvancedOptions.fileCaching(30),
//           VlcAdvancedOptions.liveCaching(30),
//           VlcAdvancedOptions.clockSynchronization(1),
//         ]),
//         sout: VlcStreamOutputOptions([
//           VlcStreamOutputOptions.soutMuxCaching(0),
//         ]),
//         extras: ['--h264-fps=60']
//     ),)..addListener(() {
//     setState(() {}
//     );
//   });
// }




import 'package:flutter/material.dart';
import 'package:flutter_vlc_player/flutter_vlc_player.dart';

// SettingsPage remains the same...
class SettingsPage extends StatefulWidget {
  final String currentUrl;

  const SettingsPage({super.key, required this.currentUrl});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  late TextEditingController _urlController;

  @override
  void initState() {
    super.initState();
    _urlController = TextEditingController(text: widget.currentUrl);
  }

  @override
  void dispose() {
    _urlController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Settings'),
        backgroundColor: Colors.grey[200],
        foregroundColor: Colors.black87,
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            const Text(
              'RTSP Stream URL',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 10),
            TextFormField(
              controller: _urlController,
              decoration: const InputDecoration(
                border: OutlineInputBorder(),
                hintText: 'e.g., rtsp://192.168.1.100:8554/stream',
              ),
            ),
            const SizedBox(height: 20),
            ElevatedButton(
              onPressed: () {
                Navigator.pop(context, _urlController.text);
              },
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.symmetric(vertical: 16.0),
                backgroundColor: Theme.of(context).primaryColor,
                foregroundColor: Colors.white,
              ),
              child: const Text('Save and Restart Stream'),
            ),
          ],
        ),
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
  final TextEditingController _urlController = TextEditingController(
    text: 'rtsp://192.168.0.158:8554/cam0',
  );

  bool _isStarted = false;
  bool _isAutoAttack = false;
  String? _errorMessage;

  @override
  void initState() {
    super.initState();
    _initializePlayer(_urlController.text);
  }

  void _initializePlayer(String url) {
    _vlcPlayerController?.dispose();
    _vlcPlayerController = VlcPlayerController.network(
      url,
      hwAcc: HwAcc.disabled,
      autoPlay: true,
      options: VlcPlayerOptions(
        // Prioritize low latency over quality and smoothness
        advanced: VlcAdvancedOptions([
          VlcAdvancedOptions.networkCaching(100), // CRITICAL: Lower this for low latency
          VlcAdvancedOptions.liveCaching(0),      // Shave off a few ms
          VlcAdvancedOptions.fileCaching(0),      // Shave off a few ms
          VlcAdvancedOptions.clockJitter(0),      // Already good for low latency
        ]),
        // To try UDP for lower latency (if your network is stable),
        // remove the :rtsp-tcp option.
        // rtp: VlcRtpOptions([
        //   VlcRtpOptions.rtpOverRtsp(true),
        // ]),
        video: VlcVideoOptions([
          VlcVideoOptions.dropLateFrames(true), // Good for real-time
          VlcVideoOptions.skipFrames(true),     // Good for real-time
        ]),
        extras: [
          '--h264-fps=60',        // Your existing setting
          '--aspect-ratio=16:9',  // Your existing setting
          '--no-audio',           // If you don't need audio, disabling it saves bandwidth & processing
        ],
      ),
    )
      ..addListener(() {
        // ... your listener code ...
      });
    setState(() {});
  }

  @override
  void dispose() {
    _vlcPlayerController?.dispose();
    _urlController.dispose();
    super.dispose();
  }

  void _sendCommand(String command) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Command sent: $command')),
    );
  }

  void _navigateToSettings() async {
    final newUrl = await Navigator.push<String>(
      context,
      MaterialPageRoute(
        builder: (context) => SettingsPage(currentUrl: _urlController.text),
      ),
    );

    if (newUrl != null && newUrl != _urlController.text) {
      setState(() {
        _urlController.text = newUrl;
        _errorMessage = null;
      });
      _initializePlayer(newUrl);
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
                      _buildCommandButton('Return', () => _sendCommand('CMD_MODE_RETURN')),
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
                          style: const TextStyle(
                              color: Colors.red, fontSize: 16),
                        ),
                      ),
                    )
                        : VlcPlayer(
                      controller: _vlcPlayerController!,
                      placeholder: const Center(
                        child: CircularProgressIndicator(),
                      ), aspectRatio: 1/1,
                    ),
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
                          setState(() {
                            _isAutoAttack = !_isAutoAttack;
                          });
                          _sendCommand(_isAutoAttack ? 'CMD_AUTO_ATTACK' : 'CMD_ATTACK');
                        },
                      ),
                      _buildBottomButton(
                        _isStarted ? 'Stop' : 'Start',
                        _isStarted ? Colors.green : Colors.grey[300],
                            () {
                          setState(() {
                            _isStarted = !_isStarted;
                          });
                          _sendCommand(_isStarted ? 'CMD_STOP' : 'CMD_START');
                        },
                      ),
                      // UPDATED: Using the new icon button builder
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
      child: ElevatedButton(
        onPressed: onPressed,
        style: ElevatedButton.styleFrom(
          minimumSize: const Size(0, 50),
          backgroundColor: Colors.grey[300],
          shape: const BeveledRectangleBorder(borderRadius: BorderRadius.zero),
          padding: const EdgeInsets.symmetric(horizontal: 4.0),
        ),
        child: SizedBox(
          width: double.infinity,
          child: Text(
            label,
            textAlign: TextAlign.center,
            style: const TextStyle(fontSize: 14, fontWeight: FontWeight.normal),
          ),
        ),
      ),
    );
  }

  // NEW: A dedicated builder for buttons with icons
  Widget _buildIconBottomButton(IconData icon, VoidCallback onPressed) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 4.0),
      child: ElevatedButton(
        onPressed: onPressed,
        style: ElevatedButton.styleFrom(
          minimumSize: const Size(60, 50),
          backgroundColor: Colors.grey[300],
          shape: const BeveledRectangleBorder(borderRadius: BorderRadius.zero),
        ),
        child: Icon(
          icon,
          size: 30.0, // Increased size for better visibility
        ),
      ),
    );
  }

  Widget _buildBottomButton(String label, Color? color, VoidCallback onPressed) {
    final textColor = (label == 'EXIT' || (label == 'Auto Attack' && _isAutoAttack))
        ? Colors.white
        : Colors.black87;

    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 4.0),
      child: ElevatedButton(
        onPressed: onPressed,
        style: ElevatedButton.styleFrom(
          minimumSize: const Size(60, 50),
          backgroundColor: color,
          foregroundColor: textColor,
          shape: const BeveledRectangleBorder(borderRadius: BorderRadius.zero),
        ),
        child: Text(
          label,
          textAlign: TextAlign.center,
          style: const TextStyle(fontSize: 14, fontWeight: FontWeight.normal),
        ),
      ),
    );
  }
}