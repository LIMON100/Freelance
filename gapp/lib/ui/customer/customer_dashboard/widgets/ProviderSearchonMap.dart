import 'package:flutter/material.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get_core/src/get_main.dart';
import 'package:get/get_navigation/get_navigation.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/ext/themehelper.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/ui/customer/customer_dashboard/widgets/DetailofproviderMap.dart';
import 'package:groom/ui/customer/customer_dashboard/widgets/customeMarkerIcon.dart';
import 'package:groom/utils/colors.dart';

class ProviderSearchMap extends StatefulWidget {
  final providerlocation; // Ensure this is a list of your provider model
  final Function() onSelected;

  const ProviderSearchMap({
    super.key,
    required this.providerlocation,
    required this.onSelected,
  });

  @override
  State<ProviderSearchMap> createState() => _ProviderSearchMapState();
}

class _ProviderSearchMapState extends State<ProviderSearchMap> {
  bool socialbuyisclick = false;
  GoogleMapController? _mapController;
  double _currentZoomLevel = 13.0;
  Set<Marker> markerList = {};
  List providerLatLngLis = [];
  LatLng? _currentLatLng; // Variable to store current location

  @override
  void initState() {
    super.initState();
    _getCurrentLocation();
    _addMarkers();
  }

  @override
  Widget build(BuildContext context) {
    // Filter and store provider locations that are not null
    providerLatLngLis = widget.providerlocation
        .where((element) {
          final providerLatLng = element.providerUserModel?.location;
          return providerLatLng != null &&
              providerLatLng is LatLng; // Ensure it's a LatLng
        })
        .map((element) => element.providerUserModel!.location
            as LatLng) // Explicitly cast to LatLng
        .toList();

    // Add provider markers after filtering

    return mapWidget();
  }

  Widget mapWidget() {
    return Stack(
      fit: StackFit.expand,
      children: [
        Positioned.fill(
          child: GoogleMap(
            mapType: MapType.normal,
            zoomControlsEnabled: false,
            myLocationEnabled: true,
            markers: markerList, // Use the markerList here
            initialCameraPosition: CameraPosition(
              target: _currentLatLng ??
                  const LatLng(
                      9.0023366, 38.74689), // Use current location or default
              zoom: _currentZoomLevel,
            ),
            onMapCreated: (GoogleMapController controller) {
              _mapController = controller;
            },
            gestureRecognizers: Set(),
          ),
        ),
        Align(
          alignment: Alignment.bottomRight,
          child: FindNowButton(
            locationlist: widget.providerlocation,
          ),
        ),
      ],
    );
  }

  double _calculateBottomSheetHeight() {
    final int itemCount = widget.providerlocation.length;

    if (!socialbuyisclick) {
      return 238; // Base height when collapsed
    }

    return 210.0 *
        itemCount; // Calculate total height based on the number of items
  }

  Future<void> _addmarkerofCurrentlocation() async {
    var currentPosition = await _getCurrentLocation();

    if (currentPosition != null) {
      _currentLatLng = LatLng(currentPosition.latitude,
          currentPosition.longitude); // Store current location

      Marker currentLocationMarker = Marker(
        markerId: MarkerId("current_location"),
        position: _currentLatLng!,
        icon: BitmapDescriptor.defaultMarkerWithHue(BitmapDescriptor.hueRed),
      );

      setState(() {
        markerList.add(currentLocationMarker);
      });
    }
  }

  Future<void> _addMarkers() async {
    try {
      for (var providerLocation in providerLatLngLis) {
        Marker providerMarker = Marker(
          markerId: MarkerId(
              providerLocation.toString()), // Unique ID for each marker
          position: providerLocation,
          icon: await CustomMarkerIcon(
            ismarkeractive: true,
            markerid:
                MarkerId(providerLocation.toString()), // Customize as needed
            iscategoryseleted: false,
            text: '', // Customize text or remove if not needed
            imageUrl: '', // Customize image URL if needed
            imageSourceType: ImageSourceType.network,
          ).toBitmapDescriptor(),
          onTap: () {
            // Handle marker tap if needed
          },
        );

        setState(() {
          markerList.add(providerMarker);
        });
      }
    } catch (e) {
      print("Error adding provider markers: $e");
    }
  }

  Future<Position?> _getCurrentLocation() async {
    try {
      bool serviceEnabled = await Geolocator.isLocationServiceEnabled();

      if (!serviceEnabled) {
        print("Location services are disabled.");
        return null; // Location services are not enabled
      }

      LocationPermission permission = await Geolocator.checkPermission();
      if (permission == LocationPermission.denied) {
        permission = await Geolocator.requestPermission();
        if (permission == LocationPermission.denied) {
          print("Location permissions are denied.");
          return null; // Permissions are denied
        }
      }

      return await Geolocator.getCurrentPosition(
          desiredAccuracy: LocationAccuracy.high);
    } catch (e) {
      print("Error getting current location: $e");
      return null;
    }
  }
}

class FindNowButton extends StatelessWidget {
  final locationlist;

  const FindNowButton({super.key, this.locationlist});
  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () {
        Get.to(DetailofProviderMap(
          providerlocation: locationlist,
        ));
      },
      child: Container(
        width: 141,
        height: 42,
        decoration: BoxDecoration(
          color: btnColorCode.withOpacity(1), // Background color
          borderRadius: BorderRadius.only(topLeft: Radius.circular(10)),
          border: Border.all(color: Colors.white, width: 2), // Border
        ),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Text(
              'Find now',
              style: ThemeText.mediumText.copyWith(
                color: Colors.white,
                fontSize: 14,
                fontWeight: FontWeight.w700,
              ),
            ),
            SizedBox(width: 8), // Spacing for the search icon
            Icon(
              Icons.search, // Search icon
              color: Colors.white,
            ),
          ],
        ),
      ),
    );
  }
}
