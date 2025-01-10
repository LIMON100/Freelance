import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/ext/tobitdescription.dart';

import '../data_models/map_model.dart';

class MapController extends GetxController {
  List<MapModel> mapModel = <MapModel>[].obs;
  var markers = <Marker>{}.obs;
  var isLoading = false.obs;
  Rx<Marker?> marker = Rx<Marker?>(null);
  GoogleMapController? gMapController;

  Future<void> createMarkers() async {
    for (var element in mapModel) {
      var wid= await SizedBox(
        width: 50,
        height: 50,
        child:Stack(
          children: [
            Image.asset("assets/img/location_marker.png"),
            /*Positioned(
                top: 1,
                right: 10,
                left: 10,
                child: ClipRRect(
                  borderRadius: const BorderRadius.all(Radius.circular(40)),
                  child: Image.network(element.!.first,
                    height: 30,width: 30,),
                )
            )*/
          ],
        ),
      ).toBitmapDescriptor(
          logicalSize: const Size(100, 100), imageSize: const Size(300, 300));
      markers.add(
        Marker(
          markerId: MarkerId(element.id.toString()),
          icon: BitmapDescriptor.defaultMarkerWithHue(BitmapDescriptor.hueRed),
          position: LatLng(element.latitude, element.longitude),
          infoWindow: InfoWindow(title: element.name, snippet: element.city),
          onTap: () {
            print("marker tapped");
          },
        ),
      );
    }
  }

  void addMarkerAtPosition(LatLng position) {
    final newMarker = Marker(
      markerId: MarkerId(position.toString()),
      position: position,
      infoWindow: const InfoWindow(title: 'New Marker', snippet: 'Custom Location'),
      onTap: () {
        print("marker at $position tapped");
      },
    );

    marker.value = newMarker; // Update the marker
  }
}