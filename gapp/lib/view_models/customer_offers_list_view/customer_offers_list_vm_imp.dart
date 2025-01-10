import 'package:groom/data_models/customer_offer_model.dart';

import '../../firebase/customer_offer_firebase.dart';
import 'customer_offer_list_vm.dart';

class CustomerOffersListVmImp implements CustomerOffersListVm{
  CustomerOfferFirebase customerOfferFirebase = CustomerOfferFirebase();

  @override
  Future<List<CustomerOfferModel>> displayCustomerOffersList(String userId) {
    // TODO: implement displayCustomerOffersList
    throw customerOfferFirebase.getAllOffersByUserId(userId);
  }




}