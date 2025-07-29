// import 'dart:async';
// import 'dart:typed_data';
//
// import 'package:flutter/material.dart';
// import 'package:flutter_blue_plus/flutter_blue_plus.dart';
// import 'package:vector_math/vector_math.dart' hide Colors; // Use hide to avoid name conflict with Material Colors
// import 'package:witmotion_app/ahrs.dart'; // Import our new AHRS file
//
// const String DEVICE_MAC_ADDRESS = "D2:3E:5A:7F:6C:3D";
// final Guid DATA_CHARACTERISTIC_UUID = Guid("0000ffe4-0000-1000-8000-00805f9a34fb");
//
// void main() {
//   FlutterBluePlus.setLogLevel(LogLevel.verbose, color: true);
//   runApp(const WitmotionApp());
// }
//
// class WitmotionApp extends StatelessWidget {
//   const WitmotionApp({super.key});
//
//   @override
//   Widget build(BuildContext context) {
//     return MaterialApp(
//       debugShowCheckedModeBanner: false,
//       title: 'Witmotion Fusion Demo',
//       theme: ThemeData(
//         primarySwatch: Colors.indigo,
//         useMaterial3: true,
//         cardTheme: const CardTheme(elevation: 4, margin: EdgeInsets.all(8)),
//       ),
//       home: const IMUDataPage(),
//     );
//   }
// }
//
// class IMUDataPage extends StatefulWidget {
//   const IMUDataPage({super.key});
//
//   @override
//   State<IMUDataPage> createState() => _IMUDataPageState();
// }
//
// class _IMUDataPageState extends State<IMUDataPage> {
//   String _connectionStatus = "Disconnected";
//   BluetoothDevice? _witmotionDevice;
//   StreamSubscription<List<int>>? _dataSubscription;
//
//   // --- State Variables ---
//   // Raw Inputs
//   final Vector3 _rawAccel = Vector3.zero();
//   final Vector3 _rawGyro = Vector3.zero();
//
//   // Fused Outputs
//   final Vector3 _witmotionEuler = Vector3.zero();
//   final Vector3 _dartFusionEuler = Vector3.zero();
//
//   // AHRS and System State
//   final MadgwickAHRS _ahrs = MadgwickAHRS(samplePeriod: 1.0 / 100.0, beta: 0.1); // Assuming 100Hz
//   DateTime? _lastUpdateTime;
//   bool _isSynchronized = false;
//
//   @override
//   void dispose() {
//     _dataSubscription?.cancel();
//     _witmotionDevice?.disconnect();
//     super.dispose();
//   }
//
//   void _startScanAndConnect() async {
//     // Same scan and connect logic as before
//     // ... (This part is unchanged, so I'm omitting it for brevity)
//     // For completeness, here is the scan logic again:
//     setState(() { _connectionStatus = "Scanning for ${DEVICE_MAC_ADDRESS}..."; });
//     try {
//       await FlutterBluePlus.startScan(timeout: const Duration(seconds: 15));
//       FlutterBluePlus.scanResults.listen((results) {
//         for (ScanResult r in results) {
//           if (r.device.remoteId.toString().toUpperCase() == DEVICE_MAC_ADDRESS.toUpperCase()) {
//             _witmotionDevice = r.device;
//             FlutterBluePlus.stopScan();
//             _connectToDevice();
//             break;
//           }
//         }
//       });
//     } catch(e) { setState(() { _connectionStatus = "Scan Error: $e"; }); }
//   }
//
//   void _connectToDevice() async {
//     // Unchanged connect logic
//     // ...
//     if (_witmotionDevice == null) return;
//     setState(() { _connectionStatus = "Connecting..."; });
//     try {
//       await _witmotionDevice!.connect();
//       List<BluetoothService> services = await _witmotionDevice!.discoverServices();
//       for (var s in services) {
//         for (var c in s.characteristics) {
//           if (c.uuid == DATA_CHARACTERISTIC_UUID) {
//             await c.setNotifyValue(true);
//             _dataSubscription = c.lastValueStream.listen(_onDataReceived);
//             setState(() { _connectionStatus = "Connected & Streaming..."; });
//             return;
//           }
//         }
//       }
//     } catch(e) { setState(() { _connectionStatus = "Connection Error: $e"; }); }
//   }
//
//
//   void _onDataReceived(List<int> data) {
//     if (data.length >= 20 && data[0] == 0x55 && data[1] == 0x61) {
//       ByteData byteData = ByteData.view(Uint8List.fromList(data).buffer);
//
//       // --- Unpack all data from the packet ---
//       final ax = byteData.getInt16(2, Endian.little);
//       final ay = byteData.getInt16(4, Endian.little);
//       final az = byteData.getInt16(6, Endian.little);
//       final gx = byteData.getInt16(8, Endian.little);
//       final gy = byteData.getInt16(10, Endian.little);
//       final gz = byteData.getInt16(12, Endian.little);
//       final roll = byteData.getInt16(14, Endian.little);
//       final pitch = byteData.getInt16(16, Endian.little);
//       final yaw = byteData.getInt16(18, Endian.little);
//
//       setState(() {
//         // --- 1. Capture Raw Data (The "Ingredients") ---
//         _rawAccel.setValues(ax / 32768.0 * 16, ay / 32768.0 * 16, az / 32768.0 * 16);
//         _rawGyro.setValues(gx / 32768.0 * 2000, gy / 32768.0 * 2000, gz / 32768.0 * 2000);
//
//         // --- 2. Capture Witmotion's Fused Data (Chef A's Cake) ---
//         _witmotionEuler.setValues(roll / 32768.0 * 180, pitch / 32768.0 * 180, yaw / 32768.0 * 180);
//
//         final now = DateTime.now();
//
//         // --- 3. Synchronize Yaw on first valid packet ---
//         if (!_isSynchronized && _witmotionEuler.z != 0) {
//           _ahrs.setHeading(_witmotionEuler.z);
//           _isSynchronized = true;
//           _connectionStatus = "Connected & Synchronized!";
//         }
//
//         // --- 4. Calculate Our Fused Data (Chef B's Cake) ---
//         if (_lastUpdateTime != null) {
//           final dt = now.difference(_lastUpdateTime!).inMicroseconds / 1000000.0;
//           _ahrs.samplePeriod = dt; // Update sample period dynamically
//           _ahrs.updateIMU(_rawGyro, _rawAccel);
//           _dartFusionEuler.setFrom(_ahrs.toEulerAngles());
//         }
//
//         _lastUpdateTime = now;
//       });
//     }
//   }
//
//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       appBar: AppBar(title: const Text('Witmotion Fusion Demo')),
//       body: SingleChildScrollView(
//         child: Center(
//           child: Column(
//             children: <Widget>[
//               const SizedBox(height: 10),
//               ElevatedButton(onPressed: _startScanAndConnect, child: const Text('Scan & Connect')),
//               const SizedBox(height: 10),
//               Text(_connectionStatus),
//               const SizedBox(height: 10),
//
//               // --- Raw Data Display ---
//               Card(
//                 child: Padding(
//                   padding: const EdgeInsets.all(16.0),
//                   child: Column(
//                     crossAxisAlignment: CrossAxisAlignment.start,
//                     children: [
//                       const Text("Raw Sensor Data (Inputs)", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
//                       const Divider(),
//                       Text("Accel (g):  X: ${_rawAccel.x.toStringAsFixed(2)}, Y: ${_rawAccel.y.toStringAsFixed(2)}, Z: ${_rawAccel.z.toStringAsFixed(2)}"),
//                       Text("Gyro (°/s): X: ${_rawGyro.x.toStringAsFixed(2)}, Y: ${_rawGyro.y.toStringAsFixed(2)}, Z: ${_rawGyro.z.toStringAsFixed(2)}"),
//                     ],
//                   ),
//                 ),
//               ),
//
//               // --- Fused Data Comparison ---
//               Card(
//                 child: Padding(
//                   padding: const EdgeInsets.all(16.0),
//                   child: Column(
//                     children: [
//                       const Text("Fused Orientation (Outputs)", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
//                       const Divider(),
//                       _buildIMUDataTable(),
//                     ],
//                   ),
//                 ),
//               ),
//             ],
//           ),
//         ),
//       ),
//     );
//   }
//
//   Widget _buildIMUDataTable() {
//     return DataTable(
//       columnSpacing: 24,
//       columns: const [
//         DataColumn(label: Text('Axis')),
//         DataColumn(label: Text('Witmotion')),
//         DataColumn(label: Text('Flutter')),
//       ],
//       rows: [
//         DataRow(cells: [
//           const DataCell(Text('Roll')),
//           DataCell(Text(_witmotionEuler.x.toStringAsFixed(2))),
//           DataCell(Text(_dartFusionEuler.x.toStringAsFixed(2))),
//         ]),
//         DataRow(cells: [
//           const DataCell(Text('Pitch')),
//           DataCell(Text(_witmotionEuler.y.toStringAsFixed(2))),
//           DataCell(Text(_dartFusionEuler.y.toStringAsFixed(2))),
//         ]),
//         DataRow(cells: [
//           const DataCell(Text('Yaw')),
//           DataCell(Text(_witmotionEuler.z.toStringAsFixed(2))),
//           DataCell(Text(_dartFusionEuler.z.toStringAsFixed(2))),
//         ]),
//       ],
//     );
//   }
// }

import 'dart:async';
import 'dart:typed_data';

import 'package:camera/camera.dart';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:vector_math/vector_math.dart' hide Colors;
import 'package:witmotion_app/ahrs.dart';

const String DEVICE_MAC_ADDRESS = "D2:3E:5A:7F:6C:3D";
final Guid DATA_CHARACTERISTIC_UUID = Guid("0000ffe4-0000-1000-8000-00805f9a34fb");

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  final cameras = await availableCameras();
  final firstCamera = cameras.first;
  runApp(WitmotionApp(camera: firstCamera));
}

class WitmotionApp extends StatelessWidget {
  final CameraDescription camera;
  const WitmotionApp({super.key, required this.camera});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Witmotion Fusion',
      theme: ThemeData(
        // We are back to a standard light theme
        primarySwatch: Colors.indigo,
        useMaterial3: true,
        brightness: Brightness.light,
        scaffoldBackgroundColor: const Color(0xFFF0F0F0), // A light grey background
        cardTheme: const CardTheme(elevation: 2, margin: EdgeInsets.all(8)),
      ),
      home: IMUDataPage(camera: camera),
    );
  }
}

class IMUDataPage extends StatefulWidget {
  final CameraDescription camera;
  const IMUDataPage({super.key, required this.camera});

  @override
  State<IMUDataPage> createState() => _IMUDataPageState();
}

class _IMUDataPageState extends State<IMUDataPage> {
  // All state variables are the same as before
  String _connectionStatus = "Disconnected";
  BluetoothDevice? _witmotionDevice;
  StreamSubscription<List<int>>? _dataSubscription;

  CameraController? _cameraController;
  Future<void>? _initializeControllerFuture;
  bool _isCameraOn = false;

  final Vector3 _rawAccel = Vector3.zero();
  final Vector3 _rawGyro = Vector3.zero();
  final Vector3 _witmotionEuler = Vector3.zero();
  final Vector3 _dartFusionEuler = Vector3.zero();
  final MadgwickAHRS _ahrs = MadgwickAHRS(samplePeriod: 1.0 / 100.0, beta: 0.1);
  DateTime? _lastUpdateTime;
  bool _isSynchronized = false;

  @override
  void initState() {
    super.initState();
  }

  void _initializeCamera() {
    _cameraController = CameraController(widget.camera, ResolutionPreset.medium);
    _initializeControllerFuture = _cameraController!.initialize();
  }

  @override
  void dispose() {
    _dataSubscription?.cancel();
    _witmotionDevice?.disconnect();
    _cameraController?.dispose();
    super.dispose();
  }

  // --- IMU Logic (unchanged) ---
  void _startScanAndConnect() { /* ... same as before ... */
    setState(() { _connectionStatus = "Scanning for ${DEVICE_MAC_ADDRESS}..."; });
    try {
      FlutterBluePlus.startScan(timeout: const Duration(seconds: 15));
      FlutterBluePlus.scanResults.listen((results) {
        for (ScanResult r in results) {
          if (r.device.remoteId.toString().toUpperCase() == DEVICE_MAC_ADDRESS.toUpperCase()) {
            _witmotionDevice = r.device;
            FlutterBluePlus.stopScan();
            _connectToDevice();
            break;
          }
        }
      });
    } catch(e) { setState(() { _connectionStatus = "Scan Error: $e"; }); }
  }

  void _connectToDevice() async { /* ... same as before ... */
    if (_witmotionDevice == null) return;
    setState(() { _connectionStatus = "Connecting..."; });
    try {
      await _witmotionDevice!.connect();
      List<BluetoothService> services = await _witmotionDevice!.discoverServices();
      for (var s in services) {
        for (var c in s.characteristics) {
          if (c.uuid == DATA_CHARACTERISTIC_UUID) {
            await c.setNotifyValue(true);
            _dataSubscription = c.lastValueStream.listen(_onDataReceived);
            setState(() { _connectionStatus = "Connected & Streaming..."; });
            return;
          }
        }
      }
    } catch(e) { setState(() { _connectionStatus = "Connection Error: $e"; }); }
  }

  void _onDataReceived(List<int> data) { /* ... same as before ... */
    if (data.length >= 20 && data[0] == 0x55 && data[1] == 0x61) {
      ByteData byteData = ByteData.view(Uint8List.fromList(data).buffer);
      final ax = byteData.getInt16(2, Endian.little);
      final ay = byteData.getInt16(4, Endian.little);
      final az = byteData.getInt16(6, Endian.little);
      final gx = byteData.getInt16(8, Endian.little);
      final gy = byteData.getInt16(10, Endian.little);
      final gz = byteData.getInt16(12, Endian.little);
      final roll = byteData.getInt16(14, Endian.little);
      final pitch = byteData.getInt16(16, Endian.little);
      final yaw = byteData.getInt16(18, Endian.little);

      setState(() {
        _rawAccel.setValues(ax / 32768.0 * 16, ay / 32768.0 * 16, az / 32768.0 * 16);
        _rawGyro.setValues(gx / 32768.0 * 2000, gy / 32768.0 * 2000, gz / 32768.0 * 2000);
        _witmotionEuler.setValues(roll / 32768.0 * 180, pitch / 32768.0 * 180, yaw / 32768.0 * 180);
        final now = DateTime.now();
        if (!_isSynchronized && _witmotionEuler.z != 0) {
          _ahrs.setHeading(_witmotionEuler.z);
          _isSynchronized = true;
          _connectionStatus = "Connected & Synchronized!";
        }
        if (_lastUpdateTime != null) {
          final dt = now.difference(_lastUpdateTime!).inMicroseconds / 1000000.0;
          _ahrs.samplePeriod = dt;
          _ahrs.updateIMU(_rawGyro, _rawAccel);
          _dartFusionEuler.setFrom(_ahrs.toEulerAngles());
        }
        _lastUpdateTime = now;
      });
    }
  }


  // --- NEW UI BUILD METHOD WITH COLUMN LAYOUT ---
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Witmotion Fusion'),
        actions: [
          IconButton(
            icon: Icon(_isCameraOn ? Icons.videocam_off : Icons.videocam),
            onPressed: () {
              setState(() {
                _isCameraOn = !_isCameraOn;
                if (_isCameraOn) {
                  _initializeCamera();
                } else {
                  _cameraController?.dispose();
                }
              });
            },
          ),
        ],
      ),
      // We now use a Column to arrange widgets vertically
      body: Column(
        children: [
          // Widget 1: The Camera View (at the top)
          _buildCameraContainer(),

          // Widget 2: The Data View (takes up the rest of the space)
          // Expanded makes this widget fill the remaining vertical space
          Expanded(
            child: _buildDataView(),
          ),
        ],
      ),
    );
  }

  // A new widget that returns the camera view container
  // A new widget that returns the camera view container
  Widget _buildCameraContainer() {
    // If the camera is off, we show an empty box
    if (!_isCameraOn || _cameraController == null) {
      return Container(
        height: 200,
        color: Colors.black,
        child: const Center(child: Text("Camera is off", style: TextStyle(color: Colors.white))),
      );
    }

    // If the camera is on, we show the preview
    return Container(
      height: 200, // A fixed height for the camera view
      color: Colors.black,
      child: FutureBuilder<void>(
        future: _initializeControllerFuture,
        builder: (context, snapshot) {
          if (snapshot.connectionState == ConnectionState.done) {
            // ClipRRect gives the camera view rounded corners
            return ClipRRect(
              borderRadius: BorderRadius.circular(10.0),
              // FittedBox ensures the camera feed fills the container
              child: FittedBox(
                fit: BoxFit.cover,
                child: SizedBox(
                  // --- THE FIX IS HERE ---
                  // Replace '.size' with '.previewSize!'
                  width: _cameraController!.value.previewSize!.width,
                  height: _cameraController!.value.previewSize!.height,
                  // ---------------------
                  child: CameraPreview(_cameraController!),
                ),
              ),
            );
          } else {
            return const Center(child: CircularProgressIndicator());
          }
        },
      ),
    );
  }

  // This widget contains all our BLE and data display UI
  Widget _buildDataView() {
    // The SingleChildScrollView allows the content to scroll if it's too long
    return SingleChildScrollView(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: <Widget>[
            ElevatedButton(onPressed: _startScanAndConnect, child: const Text('Scan & Connect')),
            const SizedBox(height: 10),
            Text(_connectionStatus),
            const SizedBox(height: 20),

            Card(
              child: Padding(
                padding: const EdgeInsets.all(12.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text("Raw Sensor Data (Inputs)", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                    const Divider(),
                    Text("Accel (g):  X: ${_rawAccel.x.toStringAsFixed(2)}, Y: ${_rawAccel.y.toStringAsFixed(2)}, Z: ${_rawAccel.z.toStringAsFixed(2)}"),
                    Text("Gyro (°/s): X: ${_rawGyro.x.toStringAsFixed(2)}, Y: ${_rawGyro.y.toStringAsFixed(2)}, Z: ${_rawGyro.z.toStringAsFixed(2)}"),
                  ],
                ),
              ),
            ),

            Card(
              child: Padding(
                padding: const EdgeInsets.all(12.0),
                child: Column(
                  children: [
                    const Text("Fused Orientation (Outputs)", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                    _buildIMUDataTable(),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  // Unchanged, but will now use the default light theme text color
  Widget _buildIMUDataTable() {
    return DataTable(
      columnSpacing: 24,
      columns: const [
        DataColumn(label: Text('Axis')),
        DataColumn(label: Text('Witmotion')),
        DataColumn(label: Text('Flutter(fusion)')),
      ],
      rows: [
        DataRow(cells: [
          const DataCell(Text('Roll')),
          DataCell(Text(_witmotionEuler.x.toStringAsFixed(2))),
          DataCell(Text(_dartFusionEuler.x.toStringAsFixed(2))),
        ]),
        DataRow(cells: [
          const DataCell(Text('Pitch')),
          DataCell(Text(_witmotionEuler.y.toStringAsFixed(2))),
          DataCell(Text(_dartFusionEuler.y.toStringAsFixed(2))),
        ]),
        DataRow(cells: [
          const DataCell(Text('Yaw')),
          DataCell(Text(_witmotionEuler.z.toStringAsFixed(2))),
          DataCell(Text(_dartFusionEuler.z.toStringAsFixed(2))),
        ]),
      ],
    );
  }
}