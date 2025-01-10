import 'package:get/get.dart';

import '../data_models/customer_offer_model.dart';

class CustomerRequestState extends GetxController {
  var selectedCustomerRequest = CustomerOfferModel(
    userId: '',
    offerId: '',
    description: '',
    location: null,
    serviceType: '',
    priceRange: '',
    offerImages: [],
    dateTime: DateTime.now(),
    deposit: false,
  ).obs;
}
