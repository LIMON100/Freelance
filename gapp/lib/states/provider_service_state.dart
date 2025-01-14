// import 'package:flutter/material.dart';
// import 'package:get/get.dart';
// import 'package:google_maps_flutter/google_maps_flutter.dart';
// import 'package:groom/data_models/provider_service_model.dart';
// import 'package:groom/data_models/provider_user_model.dart';
// import 'package:groom/data_models/user_model.dart';
// import 'package:groom/ext/tobitdescription.dart';
// import 'package:groom/repo/setting_repo.dart';
//
// import '../firebase/provider_service_firebase.dart';
// import '../firebase/provider_user_firebase.dart';
// import '../ui/customer/services/provider_details_screen.dart';
// import '../widgets/network_image_widget.dart';
//
// class ProviderServiceState extends GetxController {
//   ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
//   ProviderUserFirebase providerUserFirebase = ProviderUserFirebase();
//   var index = 0.obs;
//   RxBool isServiceLoading = false.obs;
//   RxBool isProviderLoading = false.obs;
//   RxBool isMarkerAdded = false.obs;
//   RxList<ProviderServiceModel> allService = RxList();
//   RxList<UserModel> allProviders = RxList();
//   Rx<Set<Marker>> markers = Rx(<Marker>{});
//   GoogleMapController? mapController;
//   @override
//   void onInit() {
//     super.onInit();
//     getAllServices();
//     getAllProviders();
//   }
//
//   var serviceCreate = ProviderServiceModel(
//     userId: "userId",
//     serviceId: "serviceId",
//     description: "description",
//     serviceType: "serviceType",
//     serviceName: "serviceName",
//     location: LatLng(0, 0),
//   ).obs;
//
//   var selectedProvider = UserModel(
//     uid: "userId",
//     contactNumber: "serviceId",
//     city: "description",
//     country: "serviceType",
//     state: "0",
//     email: '',
//     isblocked: false,
//     photoURL: '',
//     fullName: '',
//     joinedOn: 2,
//     providerUserModel: ProviderUserModel(
//       about: '',
//       workDayFrom: '',
//       workDayTo: '',
//       location: null,
//       addressLine: '',
//       createdOn: 0,
//       providerType: '',
//       providerImages: [],
//       providerServices: [],
//
//     ),
//     requestsThisMonth: 0,
//   ).obs;
//   void getAllServices() {
//     isServiceLoading.value = true;
//     providerServiceFirebase.getAllServices().then(
//       (value) {
//         isServiceLoading.value = false;
//         allService.value = value;
//       },
//     );
//   }
//
//   void getAllProviders() {
//     isProviderLoading.value = true;
//     providerUserFirebase.getAllProviders().then(
//       (value) {
//         isProviderLoading.value = false;
//         allProviders.value = value;
//         createMarkers();
//         if(mapController!=null){
//           mapController!.animateCamera(CameraUpdate.newCameraPosition(CameraPosition(target: auth.value.location!,zoom: 10)));
//         }
//       },
//     );
//   }
//   Future<void> createMarkers() async {
//     for (var element in allProviders) {
//       var wid= await SizedBox(
//         width: 50,
//         height: 50,
//         child:Stack(
//           children: [
//             Image.asset("assets/img/location_marker.png"),
//             Positioned(
//                 top: 1,
//                 right: 10,
//                 left: 10,
//                 child: ClipRRect(
//                   borderRadius: const BorderRadius.all(Radius.circular(40)),
//                   child: Image.network(element.providerUserModel!.providerImages!.first,
//                     height: 30,width: 30,),
//                 )
//             )
//           ],
//         ),
//       ).toBitmapDescriptor(
//           logicalSize: const Size(100, 100), imageSize: const Size(300, 300));
//       markers.value.add(
//         Marker(
//           markerId: MarkerId(element.uid.toString()),
//           icon: wid,
//           position: element.providerUserModel!.location!,
//           infoWindow: InfoWindow(title: element.providerUserModel!.providerType !=
//               "Independent"
//               ? element.providerUserModel!.salonTitle!
//               : element.fullName, snippet: '\$ ${element.providerUserModel!.basePrice}'),
//           onTap: () {
//             Get.to(() => ProviderDetailsScreen(
//               provider: element,
//             ));
//           },
//         ),
//       );
//       isMarkerAdded.value=true;
//     }
//   }
// }


// updated for null issue

import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/data_models/provider_service_model.dart';
import 'package:groom/data_models/provider_user_model.dart';
import 'package:groom/data_models/user_model.dart';
import 'package:groom/ext/tobitdescription.dart';
import 'package:groom/repo/setting_repo.dart';

import '../firebase/provider_service_firebase.dart';
import '../firebase/provider_user_firebase.dart';
import '../ui/customer/services/provider_details_screen.dart';
import '../widgets/network_image_widget.dart';

class ProviderServiceState extends GetxController {
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  ProviderUserFirebase providerUserFirebase = ProviderUserFirebase();
  var index = 0.obs;
  RxBool isServiceLoading = false.obs;
  RxBool isProviderLoading = false.obs;
  RxBool isMarkerAdded = false.obs; // Initialize to false
  RxList<ProviderServiceModel> allService = RxList();
  RxList<UserModel> allProviders = RxList();
  Rx<Set<Marker>> markers = Rx(<Marker>{});
  GoogleMapController? mapController;

  @override
  void onInit() {
    super.onInit();
    getAllServices();
    getAllProviders();
  }

  var serviceCreate = ProviderServiceModel(
    userId: "userId",
    serviceId: "serviceId",
    description: "description",
    serviceType: "serviceType",
    serviceName: "serviceName",
    location: LatLng(0, 0),
  ).obs;

  var selectedProvider = UserModel(
    uid: "userId",
    contactNumber: "serviceId",
    city: "description",
    country: "serviceType",
    state: "0",
    email: '',
    isblocked: false,
    photoURL: '',
    fullName: '',
    joinedOn: 2,
    providerUserModel: ProviderUserModel(
      about: '',
      workDayFrom: '',
      workDayTo: '',
      location: null,
      addressLine: '',
      createdOn: 0,
      providerType: '',
      providerImages: [],
      providerServices: [],
    ),
    requestsThisMonth: 0,
  ).obs;

  void getAllServices() {
    isServiceLoading.value = true;
    providerServiceFirebase.getAllServices().then((value) {
      isServiceLoading.value = false;
      allService.value = value;
    });
  }

  void getAllProviders() {
    isProviderLoading.value = true;
    providerUserFirebase.getAllProviders().then((value) {
      allProviders.value = value;
      createMarkers().then((_) { // Call createMarkers and then set isMarkerAdded
        isProviderLoading.value = false;
        isMarkerAdded.value = true; // Set to true after markers are created
        if (mapController != null) {
          mapController!.animateCamera(
              CameraUpdate.newCameraPosition(
                  CameraPosition(target: auth.value.location!, zoom: 10)));
        }
      });
    });
  }

  Future<void> createMarkers() async {
    markers.value.clear(); // Clear existing markers before adding new ones
    List<Future<BitmapDescriptor>> futures = []; // List to hold Futures

    for (var element in allProviders) {
      futures.add(SizedBox(
        width: 50,
        height: 50,
        child: Stack(
          children: [
            Image.asset("assets/img/location_marker.png"),
            Positioned(
              top: 1,
              right: 10,
              left: 10,
              child: ClipRRect(
                borderRadius: const BorderRadius.all(Radius.circular(40)),
                child: Image.network(
                  element.providerUserModel!.providerImages!.first,
                  height: 30,
                  width: 30,
                ),
              ),
            )
          ],
        ),
      ).toBitmapDescriptor(
          logicalSize: const Size(100, 100), imageSize: const Size(300, 300)));
    }

    final List<BitmapDescriptor> icons = await Future.wait(futures); // Wait for all icons to be created

    for (int i = 0; i < allProviders.length; i++) {
      final element = allProviders[i];
      markers.value.add(
        Marker(
          markerId: MarkerId(element.uid.toString()),
          icon: icons[i], // Use the pre-computed icon
          position: element.providerUserModel!.location!,
          infoWindow: InfoWindow(
              title: element.providerUserModel!.providerType != "Independent"
                  ? element.providerUserModel!.salonTitle!
                  : element.fullName,
              snippet: '\$ ${element.providerUserModel!.basePrice}'),
          onTap: () {
            Get.to(() => ProviderDetailsScreen(
              provider: element,
            ));
          },
        ),
      );
    }
    // isMarkerAdded.value = true; // Moved to getAllProviders after markers are created
  }
}