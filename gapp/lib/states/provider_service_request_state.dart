import 'package:get/get.dart';

import '../data_models/provider_service_request_offer_model.dart';

class ProviderServiceRequestState extends GetxController {
  var selectedProvider = ProviderServiceRequestOfferModel(
    selectedDate: 0,
          description: "description",
          requestId: "requestId",
          providerId: "providerId",
          serviceId: "serviceId",
          createdOn: 0,
          deposit: false,
          depositAmount: 0,
          clientId: "clientId")
      .obs;
}
