import 'dart:convert';
import 'dart:io';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_storage/firebase_storage.dart';
import 'package:geolocator/geolocator.dart';
import 'package:groom/data_models/provider_user_model.dart';
import 'package:groom/data_models/user_model.dart';
import 'package:groom/repo/setting_repo.dart';

import '../constant/global_configuration.dart';
import '../ui/membership/m/membership_model.dart';
import '../ui/provider/home/m/transaction.dart';

class ProviderUserFirebase {
  final DatabaseReference _databaseReference = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();


  var _auth = FirebaseDatabase.instance;
  final FirebaseStorage _storage = FirebaseStorage.instance;

  Future<bool> addProvider(String userID, ProviderUserModel providerModel) async {
    try {
      await _auth
          .ref("users")
          .child(userID)
          .child("providerUserModel")
          .update(providerModel.toJson());
      return true;
    } catch (e) {
      print(e);
    }
    return false;
  }
  Future<bool> addMembership(String userID, MembershipModel membership) async {
    try {
      await _auth
          .ref("users")
          .child(userID)
          .child("membership")
          .update(membership.toJson());
      return true;
    } catch (e) {
      print(e);
    }
    return false;
  }

  Future<List<UserModel>> getAllProviders() async {
    List<UserModel> providerList = [];
    var source = await _auth.ref("users").once();
    var snapshot = source.snapshot;
    UserModel? userModel;
    if (snapshot.value != null) {
      snapshot.children.forEach((element) {
        try{
          Map<String,dynamic> userJson = jsonDecode(jsonEncode(element.value));
          if(userJson.containsKey("providerUserModel")){
            if (userJson['providerUserModel'] != null) {
              userModel = UserModel.fromJson(userJson);
              if(userModel!.uid!=auth.value.uid) {
                double distance = 0.0;
                if (auth.value.location != null && userModel!.location != null) {
                  distance = Geolocator.distanceBetween(
                    auth.value.location!.latitude,
                    auth.value.location!.longitude,
                    userModel!.providerUserModel!.location!.latitude,
                    userModel!.providerUserModel!.location!.longitude,
                  ) / 1000;
                  if(distance<=radius) {
                    userModel!.distance="${distance.toStringAsFixed(2)} mi";
                    providerList.add(userModel!);
                  }
                }

              }
            }
          }
        }catch(e){}
      });
    }
    return providerList;
  }
  Future<String> uploadImage(File image,String userID) async {
    try {
      String fileName = 'users/$userID/providerImages/${DateTime.now().millisecondsSinceEpoch}.jpg';
      Reference ref = _storage.ref().child(fileName);
      UploadTask uploadTask = ref.putFile(image);
      TaskSnapshot snapshot = await uploadTask;
      return await snapshot.ref.getDownloadURL();
    } catch (e) {
      print(e);
      return '';
    }
  }
  Future<void> updateProviderOverallRating(String providerId, double newRating) async {
    DatabaseReference providerRef = _databaseReference.child('users').child(providerId).child('providerUserModel');

    var snapshot = await providerRef.get();
    if (snapshot.exists) {
      ProviderUserModel provider = ProviderUserModel.fromJson(Map<String, dynamic>.from(snapshot.value as Map));

      // Calculate new overall rating
      double totalRating = (provider.overallRating * provider.numReviews);
      totalRating += newRating;
      provider.numReviews += 1;
      provider.overallRating = totalRating / provider.numReviews;

      // Update provider with new overall rating and number of reviews
      await providerRef.update({
        'overallRating': provider.overallRating,
        'numReviews': provider.numReviews,
      });
    }
  }
  Future<List<TransactionModel>> getAllTransaction(providerId) async {
    List<TransactionModel> providerList = [];
    var source = await _auth.ref("transaction").orderByChild("providerId").equalTo(providerId).once();
    var snapshot = source.snapshot;
    if (snapshot.value != null) {
      snapshot.children.forEach((element) {
        Map<String,dynamic> userJson = jsonDecode(jsonEncode(element.value));
       providerList.add(TransactionModel.fromJson(userJson));
      });
    }
    return providerList;
  }
}
