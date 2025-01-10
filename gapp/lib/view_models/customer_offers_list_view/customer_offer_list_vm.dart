import 'package:groom/data_models/customer_offer_model.dart';

abstract class CustomerOffersListVm{

  Future<List<CustomerOfferModel>> displayCustomerOffersList(String userId);
}