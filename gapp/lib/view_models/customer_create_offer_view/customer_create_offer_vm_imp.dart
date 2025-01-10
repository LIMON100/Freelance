import 'package:groom/data_models/customer_offer_model.dart';
import 'package:groom/firebase/customer_offer_firebase.dart';

import 'customer_create_offer_vm.dart';

class CustomerCreateOfferVmImp
    implements CustomerCreateOfferVm {
  CustomerOfferFirebase customerOfferFirebase = CustomerOfferFirebase();

  @override
  Future<bool> submitCustomerOffer(
      CustomerOfferModel customerOfferModel) {
    // TODO: implement submitCustomerServiceRequest
    throw customerOfferFirebase.writeOfferToFirebase(
        customerOfferModel.offerId, customerOfferModel);
  }
}
