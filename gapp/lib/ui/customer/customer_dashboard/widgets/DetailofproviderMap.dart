import 'package:flutter/material.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/ui/customer/customer_dashboard/widgets/customeMarkerIcon.dart';
import 'package:groom/utils/colors.dart';

class DetailofProviderMap extends StatefulWidget {
  final providerlocation; // Ensure this is a list of your provider model

  const DetailofProviderMap({
    super.key,
    required this.providerlocation,
  });

  @override
  State<DetailofProviderMap> createState() => _DetailofProviderMapState();
}

class _DetailofProviderMapState extends State<DetailofProviderMap> {
  GoogleMapController? _mapController;
  double _currentZoomLevel = 13.0;
  Set<Marker> markerList = {};
  List providerLatLngLis = [];
  LatLng? _currentLatLng;

  @override
  void initState() {
    super.initState();
    _getCurrentLocation();
    _addMarkers();
  }

  @override
  Widget build(BuildContext context) {
    providerLatLngLis = widget.providerlocation
        .where((element) {
          final providerLatLng = element.providerUserModel?.location;
          return providerLatLng != null && providerLatLng is LatLng;
        })
        .map((element) => element.providerUserModel!.location as LatLng)
        .toList();

    return Scaffold(
      body: Column(
        children: [
          // Search Bar
          Container(
            height: 50,
            margin: EdgeInsets.only(top: 30, left: 10),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                InkWell(
                    onTap: () => Get.back(),
                    child:
                        Icon(Icons.arrow_back, color: Colors.black, size: 18)),
                Text(
                  'Find a  barber nearby',
                  style: TextStyle(
                    fontSize: 16,
                    fontWeight: FontWeight.w700,
                    height: 20.16 / 16, // Line height divided by font size
                    textBaseline: TextBaseline.alphabetic,
                    decoration: TextDecoration.none, // No decoration applied
                    decorationStyle: TextDecorationStyle.solid,
                  ),
                ),
              ],
            ),
          ),

          // Map Widget
          Expanded(
            child: Stack(
              children: [
                mapWidget(),
                // Current Location Marker
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: SizedBox(
                    width: 271,
                    child: TextField(
                      decoration: InputDecoration(
                        filled: true,
                        fillColor: Colors.white,
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(16),
                          borderSide: BorderSide.none,
                        ),
                        hintText: 'Search location',
                        prefixIcon: Icon(Icons.search, color: Colors.grey),
                      ),
                    ),
                  ),
                ),
                if (_currentLatLng != null)
                  Positioned(
                    top: 0,
                    child:
                        Icon(Icons.location_on, color: Colors.blue, size: 40),
                  ),
              ],
            ),
          ),
          // List of Providers
          Container(
            height: 261,
            padding: const EdgeInsets.all(16.0),
            decoration: BoxDecoration(
              color: Colors.white,
              borderRadius: BorderRadius.vertical(top: Radius.circular(20)),
              boxShadow: [
                BoxShadow(
                  color: Colors.black26,
                  blurRadius: 6,
                  offset: Offset(0, -3),
                ),
              ],
            ),
            child: SizedBox(
              child: Column(
                children: [
                  // Filter Button
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      Text('Nearby Providers',
                          style: Theme.of(context).textTheme.bodyMedium),
                      Container(
                        width: 80, // Width of 80 pixels
                        height: 40, // Height of 40 pixels
                        decoration: BoxDecoration(
                          borderRadius: BorderRadius.circular(10),
                          color: btnColorCode.withOpacity(
                              1), // Set background color with opacity
                        ),
                        child: InkWell(
                          onTap: () {
                            // Add your onPressed functionality here
                          },
                          borderRadius: BorderRadius.circular(10),
                          child: Center(
                            child: Text(
                              'Filter',
                              style: TextStyle(
                                color: Colors.white, // Set text color
                              ),
                            ),
                          ),
                        ),
                      ),
                    ],
                  ),
                  // List of Provider Cards
                  SizedBox(height: 10),
                  Expanded(
                    child: ListView.builder(
                      itemCount: widget.providerlocation.length,
                      itemBuilder: (context, index) {
                        final provider = widget.providerlocation[index];
                        return ProviderCard(provider: provider);
                      },
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget mapWidget() {
    return GoogleMap(
      mapType: MapType.normal,
      zoomControlsEnabled: false,
      myLocationEnabled: true,
      markers: markerList,
      initialCameraPosition: CameraPosition(
        target: _currentLatLng ?? const LatLng(9.0023366, 38.74689),
        zoom: _currentZoomLevel,
      ),
      onMapCreated: (GoogleMapController controller) {
        _mapController = controller;
      },
    );
  }

  Future<void> _addMarkers() async {
    try {
      for (var providerLocation in providerLatLngLis) {
        Marker providerMarker = Marker(
          markerId: MarkerId(providerLocation.toString()),
          position: providerLocation,
          icon: await CustomMarkerIcon(
            ismarkeractive: true,
            markerid: MarkerId(providerLocation.toString()),
            iscategoryseleted: false,
            text: '',
            imageUrl: '',
            imageSourceType: ImageSourceType.network,
          ).toBitmapDescriptor(),
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
      if (!serviceEnabled) return null;

      LocationPermission permission = await Geolocator.checkPermission();
      if (permission == LocationPermission.denied) {
        permission = await Geolocator.requestPermission();
        if (permission == LocationPermission.denied) return null;
      }

      return await Geolocator.getCurrentPosition(
          desiredAccuracy: LocationAccuracy.high);
    } catch (e) {
      return null;
    }
  }
}

class ProviderCard extends StatelessWidget {
  final provider;

  const ProviderCard({super.key, required this.provider});

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      child: Card(
        margin: const EdgeInsets.symmetric(vertical: 8),
        elevation: 4,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(15),
        ),
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Row(
            children: [
              // Placeholder for provider image
              ClipRRect(
                borderRadius: BorderRadius.circular(10),
                child: Image.network(
                  'https://via.placeholder.com/100',
                  width: 100,
                  height: 100,
                  fit: BoxFit.cover,
                ),
              ),
              SizedBox(width: 10),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Provider Name',
                      style: Theme.of(context).textTheme.bodyMedium,
                    ),
                    SizedBox(height: 4),
                    Text(
                      'Provider Address',
                      style: Theme.of(context).textTheme.bodyMedium,
                    ),
                    SizedBox(height: 4),
                    Row(
                      children: [
                        Icon(Icons.star, color: Colors.amber, size: 16),
                        SizedBox(width: 4),
                        Text(
                          '',
                          style: Theme.of(context).textTheme.bodyMedium,
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
