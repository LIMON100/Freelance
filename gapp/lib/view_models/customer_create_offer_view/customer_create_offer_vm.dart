import '../../data_models/customer_offer_model.dart';

abstract class CustomerCreateOfferVm{
  Future<bool> submitCustomerOffer(CustomerOfferModel customerOfferModel);
}