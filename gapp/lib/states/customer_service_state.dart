import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/data_models/provider_service_model.dart';

class CustomerServiceState extends GetxController {
  RxBool isReshedule=false.obs;
  var selectedService = ProviderServiceModel(
          serviceDeposit: 0,
          servicePrice: 0,
          userId: "userId",
          serviceId: "serviceId",
          description: "description",
      serviceName: "serviceName",
          location: LatLng(9, 9),
          serviceImages: [],
          serviceType: "serviceType")
      .obs;
}
