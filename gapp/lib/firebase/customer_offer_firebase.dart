import 'dart:convert';
import 'dart:io';

import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_storage/firebase_storage.dart';
import 'package:groom/data_models/customer_offer_model.dart';
import 'package:groom/repo/setting_repo.dart';

import '../constant/global_configuration.dart';

class CustomerOfferFirebase {
  final _ref = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'));

  final FirebaseStorage _storage = FirebaseStorage.instance;

  Future<bool> writeOfferToFirebase(
      String offerId, CustomerOfferModel customerOfferModel) async {
    try {
      await _ref
          .ref("customerOffers")
          .child(offerId)
          .set(customerOfferModel.toJson());
    } catch (e) {
      print(e);
    }

    return false;
  }
  Future<bool> updateOffer(
      String offerId,Map<String,dynamic> map) async {
    try {
      await _ref
          .ref("customerOffers")
          .child(offerId)
          .update(map);
    } catch (e) {
      print(e);
    }

    return false;
  }
  Future<bool> removeOfferFromFirebase(
      String offerId) async {
    try {
      await _ref
          .ref("customerOffers")
          .child(offerId)
          .remove();
    } catch (e) {
      print(e);
    }

    return false;
  }

  Future<List<CustomerOfferModel>> getAllOffers() async {
    List<CustomerOfferModel> lst = [];
    var source = await _ref.ref("customerOffers").once();
    var data = source.snapshot;

    if (data.value != null) {
      try{
        for (var e in data.children) {
          CustomerOfferModel? customerOfferModel = CustomerOfferModel.fromJson(
            jsonDecode(
              jsonEncode(e.value),
            ),
          );
          if(customerOfferModel.status=="pending"&&customerOfferModel.userId!=auth.value.uid) {
            lst.add(customerOfferModel);
          }
        }
      }catch(e){
        print("customerOffers==${e}");
      }
    }

    return lst.reversed.toList();
  }

  Future<List<CustomerOfferModel>> getAllOffersByUserId(String userId) async {
    List<CustomerOfferModel> lst = [];
    var source = await _ref
        .ref("customerOffers")
        .orderByChild("userId")
        .equalTo(userId)
        .once();
    var data = source.snapshot;

    if (data.value != null) {
      data.children.forEach((e) {
        CustomerOfferModel? customerOfferModel = CustomerOfferModel.fromJson(
          jsonDecode(
            jsonEncode(e.value),
          ),
        );
        lst.add(customerOfferModel);
      });
    }

    return lst;
  }

  List<CustomerOfferModel> filterRecentOffers(
      List<CustomerOfferModel> allOffers, DateTime resetDate) {
    // Calculate the date 30 days before the resetDate
    DateTime startDate = resetDate.subtract(Duration(days: 30));

    // Filter offers created within the last 30 days
    List<CustomerOfferModel> recentOffers = allOffers.where((offer) {
      if (offer.dateTime != null) {
        return offer.dateTime!.isAfter(startDate) && offer.dateTime!.isBefore(resetDate);
      }
      return false;
    }).toList();

    return recentOffers;
  }

  Future<String> uploadImage(File image, String userID) async {
    try {
      String fileName =
          'users/$userID/customerOfferImages/${DateTime.now().millisecondsSinceEpoch}.jpg';
      Reference ref = _storage.ref().child(fileName);
      UploadTask uploadTask = ref.putFile(image);
      TaskSnapshot snapshot = await uploadTask;
      return await snapshot.ref.getDownloadURL();
    } catch (e) {
      print(e);
      return '';
    }
  }

  Future<List<CustomerOfferModel>> getAllOffersByServiceId(
      String serviceId) async {
    List<CustomerOfferModel> lst = [];
    var source = await _ref
        .ref("customerOffers")
        .orderByChild("userId")
        .equalTo(serviceId)
        .once();
    var data = source.snapshot;

    if (data.value != null) {
      data.children.forEach((e) {
        CustomerOfferModel? customerOfferModel = CustomerOfferModel.fromJson(
          jsonDecode(
            jsonEncode(e.value),
          ),
        );
        lst.add(customerOfferModel);
      });
    }

    return lst;
  }

  Future<CustomerOfferModel> getOfferByOfferId(String offerId) async {
    var source = await _ref
        .ref("customerOffers/$offerId")
       .once();
    var data = source.snapshot;
    CustomerOfferModel? customerOfferModel;
      customerOfferModel = CustomerOfferModel.fromJson(
        jsonDecode(
          jsonEncode(data.value),
        ));
    return customerOfferModel;

  }
  Future<bool> checkIfOfferAccepted(String offerId)async{

    var source = await _ref.ref('requestReservation').orderByChild("offerId").equalTo(offerId).once();
    var data = source.snapshot;
    if(data.exists){
      return true;
    }

    return false;
  }
}
