import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:flutter/rendering.dart';
import 'dart:ui' as ui;
import '../states/map_controller.dart';
import '../../firebase/user_firebase.dart';
import '../../data_models/user_model.dart';
import 'package:firebase_auth/firebase_auth.dart';

class GoogleMapRadiusScreen extends StatefulWidget {
  const GoogleMapRadiusScreen({super.key});

  @override
  State<GoogleMapRadiusScreen> createState() => _GoogleMapRadiusScreenState();
}

class _GoogleMapRadiusScreenState extends State<GoogleMapRadiusScreen> {
  static const LatLng _defaultLocation = LatLng(30.0444, 31.2357);
  MapController mapController = Get.put(MapController());
  final GlobalKey _globalKey = GlobalKey();
  LatLng _initialLocation = _defaultLocation;
  bool _isLoading = true;
  double _radius = 10.0; // Initialize radius

  @override
  void initState() {
    super.initState();
    _fetchUserLocation();
  }

  Future<void> _fetchUserLocation() async {
    final User? currentUser = FirebaseAuth.instance.currentUser;
    if (currentUser == null) {
      setState(() {
        _isLoading = false;
      });
      return;
    }

    try {
      UserFirebase userFirebase = UserFirebase();
      UserModel user = await userFirebase.getUser(currentUser.uid);
      if (user.location != null) {
        setState(() {
          _initialLocation = user.location!;
          _isLoading = false;
          // Place marker at the initial location
          mapController.addMarkerAtPosition(_initialLocation);
        });
      } else {
        setState(() {
          _isLoading = false;
        });
      }
    } catch (e) {
      print('Error fetching user location: $e');
      setState(() {
        _isLoading = false;
      });
    }
  }

  Future<Uint8List?> _capturePng() async {
    try {
      RenderRepaintBoundary boundary = _globalKey.currentContext!.findRenderObject() as RenderRepaintBoundary;
      ui.Image image = await boundary.toImage();
      ByteData? byteData = await image.toByteData(format: ui.ImageByteFormat.png);
      return byteData?.buffer.asUint8List();
    } catch (e) {
      print(e);
      return null;
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(),
      body: _isLoading
          ? Center(child: CircularProgressIndicator())
          : Column(
        children: [
          RepaintBoundary(
            key: _globalKey,
            child: Container(
              width: MediaQuery.of(context).size.width,
              height: MediaQuery.of(context).size.height * 0.7,
              child: Obx(() => GoogleMap(
                initialCameraPosition: CameraPosition(target: _initialLocation, zoom: 13),
                markers: mapController.marker.value != null ? {mapController.marker.value!} : {},
                circles: {
                  Circle(
                    circleId: CircleId('radius'),
                    center: mapController.marker.value?.position ?? _initialLocation,
                    radius: _radius * 1000, // Radius in meters
                    fillColor: Colors.blue.withOpacity(0.5),
                    strokeColor: Colors.blue,
                    strokeWidth: 1,
                  ),
                },
                onTap: (position) {
                  mapController.addMarkerAtPosition(position);
                },
                onCameraMove: (position) {
                  // Update marker position and circle center when the camera moves
                  mapController.addMarkerAtPosition(position.target);
                },
              )),
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(8.0),
            child: Text(
              "Radius: ${_radius.toStringAsFixed(1)} km",
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
            ),
          ),
          Slider(
            value: _radius,
            min: 1,
            max: 50,
            divisions: 49,
            label: _radius.toStringAsFixed(1),
            onChanged: (double value) {
              setState(() {
                _radius = value;
              });
            },
          ),
          ElevatedButton(
            onPressed: () async {
              final location = mapController.marker.value?.position;
              if (location != null) {
                final screenshot = await _capturePng();
                Get.back(result: {'location': location, 'screenshot': screenshot, 'radius': _radius});
              } else {
                Get.snackbar('Error', 'Please select a location first');
              }
            },
            child: Text("Save Location"),
          )
        ],
      ),
    );
  }
}
