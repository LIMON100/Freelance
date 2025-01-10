import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:flutter/rendering.dart';
import 'package:google_places_autocomplete_text_field/google_places_autocomplete_text_field.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'dart:ui' as ui;
import '../repo/setting_repo.dart';
import '../states/map_controller.dart';

class GoogleMapScreen extends StatefulWidget {
  const GoogleMapScreen({super.key});

  @override
  State<GoogleMapScreen> createState() => _GoogleMapScreenState();
}

class _GoogleMapScreenState extends State<GoogleMapScreen> {
  MapController mapController = Get.put(MapController());
  final GlobalKey _globalKey = GlobalKey();
  final TextEditingController _locationController = TextEditingController();
  GoogleMapController? mapcon;
  LatLng _initialLocation = auth.value.location ?? defalultLatLng;

  @override
  void initState() {
    super.initState();
  }

  Future<Uint8List?> _capturePng() async {
    try {
      RenderRepaintBoundary boundary = _globalKey.currentContext!
          .findRenderObject() as RenderRepaintBoundary;
      ui.Image image = await boundary.toImage();
      ByteData? byteData =
          await image.toByteData(format: ui.ImageByteFormat.png);
      return byteData?.buffer.asUint8List();
    } catch (e) {
      print(e);
      return null;
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        title: HeaderTxtWidget(
          'Choose Location',
          color: primaryColorCode,
        ),
        iconTheme: IconThemeData(color: primaryColorCode),
      ),
      body: Stack(
        children: [
          RepaintBoundary(
            key: _globalKey,
            child: SizedBox(
              width: MediaQuery.of(context).size.width,
              height: MediaQuery.of(context).size.height,
              child: Obx(() => GoogleMap(
                    onMapCreated: (controller) {
                      mapcon = controller;
                    },
                    initialCameraPosition: CameraPosition(target: _initialLocation, zoom: 13),
                mapToolbarEnabled: false,
                zoomControlsEnabled: false,
                    markers: mapController.marker.value != null
                        ? {mapController.marker.value!}
                        : {},
                  )),
            ),
          ),
          Positioned(
            top: 20,
            right: 20,
            left: 20,
            child: GooglePlacesAutoCompleteTextFormField(
              textEditingController: _locationController,
              googleAPIKey: "AIzaSyAiF2WXvYTrnybDDn4EwvOL9RJAwJ_4Bi4",
              countries: ["IN","US"],
              debounceTime: 400,
              isLatLngRequired: true,
              decoration: const InputDecoration(
                  border: OutlineInputBorder(
                      borderRadius: BorderRadius.all(Radius.circular(20)),
                      borderSide: BorderSide.none),
                  prefixIcon: Icon(Icons.search),
                  fillColor: Colors.white,
                  hintText: "Search location",
                  hintStyle: TextStyle(
                      color: Colors.grey,
                      fontSize: 14,
                      fontWeight: FontWeight.w300),
                  filled: true),
              getPlaceDetailWithLatLng: (prediction) {
                print("Coordinates: (${prediction.lat},${prediction.lng})");
                _initialLocation = LatLng(double.parse(prediction.lat!),
                    double.parse(prediction.lng!));
                mapcon!.animateCamera(CameraUpdate.newCameraPosition(
                    CameraPosition(target: _initialLocation, zoom: 16)));
                mapController.addMarkerAtPosition(_initialLocation);
              },
              itmClick: (prediction) {
                _locationController.text = prediction.description!;
                _locationController.selection = TextSelection.fromPosition(
                    TextPosition(offset: prediction.description!.length));
              },
              validator: (p0) {
                if (p0!.isEmpty) {
                  return "Please search location";
                }
                return null;
              },
            ),
          ),

          Positioned(
            bottom: 20,
              right: 20,
              left: 20,
              child: ElevatedButton(
            onPressed: () async {
              final location = mapController.marker.value?.position;
              if (location != null) {
                final screenshot = await _capturePng();
                Get.back(
                    result: {'location': location,
                      'address': _locationController.text,
                      'screenshot': screenshot});
              } else {
                Get.snackbar('Error', 'Please select a location first');
              }
            },
            child: const Text("Save Location"),
          ))
        ],
      ),
    );
  }
}
