import 'dart:convert';

import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';

import '../constant/global_configuration.dart';
import '../data_models/service_booking_model.dart';

class ServiceBookingFirebase {
  var ref = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();


  Future<bool> addBookingToFirebase(String bookingId, String userId, ServiceBookingModel bookingModel) async {
    try {
      await ref
          .child("serviceBooking")
          .child(bookingId)
          .set(bookingModel.toJson());
      return true;
    } catch (e) {
      print(e);
    }
    return false;
  }

  Future<bool> removeBookingFromFirebase(
      String bookingId,) async {
    try {
      await ref
          .child("serviceBooking")
          .child(bookingId)
    .remove();
      return true;
    } catch (e) {
      print(e);
    }
    return false;
  }
  Future<bool> updateBookingStatus(String bookingId, String newStatus) async {
    try {
      await ref.child("serviceBooking").child(bookingId).update({'status': newStatus});
      return true;
    } catch (e) {
      print(e);
      return false;
    }
  }

  Future<List<ServiceBookingModel>> getServiceBookingByUserId(String clientId) async {
    List<ServiceBookingModel> list = [];
    var source = await ref
        .child("serviceBooking")
        .orderByChild("clientId")
        .equalTo(clientId)
        .once();
    var data = source.snapshot;
    if (data.value != null) {
      for (var element in data.children) {
        var serviceBookingModel = ServiceBookingModel.fromJson(
          jsonDecode(
            jsonEncode(element.value),
          ),
        );
        if(serviceBookingModel.status!="Reserved") {
          list.add(serviceBookingModel);
        }
      }
    }
    return list.reversed.toList();
  }
  Future<List<ServiceBookingModel>> getServiceBookingByProviderId(String clientId) async {
    List<ServiceBookingModel> list = [];
    var source = await ref
        .child("serviceBooking")
        .orderByChild("providerId")
        .equalTo(clientId)
        .once();
    var data = source.snapshot;
    if (data.value != null) {
      try{
        for (var element in data.children) {
          var serviceBookingModel = ServiceBookingModel.fromJson(
            jsonDecode(
              jsonEncode(element.value),
            ),
          );
          if(serviceBookingModel.status!="Reserved") {
            list.add(serviceBookingModel);
          }
        }
      }catch(e){
        print('getServiceBookingByProviderId ${e}');
      }
    }
    return list;
  }
  Future<List<ServiceBookingModel>> getServiceBookingByProviderAll(String clientId) async {
    List<ServiceBookingModel> list = [];
    var source = await ref
        .child("serviceBooking")
        .orderByChild("providerId")
        .equalTo(clientId)
        .once();
    var data = source.snapshot;
    if (data.value != null) {
      for (var element in data.children) {
        var serviceBookingModel = ServiceBookingModel.fromJson(
          jsonDecode(
            jsonEncode(element.value),
          ),
        );
        list.add(serviceBookingModel);
      }
    }
    return list;
  }

  Future<List<ServiceBookingModel>> getServiceBookingByServiceId(
      String serviceId) async {
    List<ServiceBookingModel> list = [];
    var source = await ref
        .child("serviceBooking")
        .orderByChild("serviceId")
        .equalTo(serviceId)
        .once();
    var data = source.snapshot;
    if (data.value != null) {
      try{
        data.children.forEach((element) {
          var serviceBookingModel = ServiceBookingModel.fromJson(
            jsonDecode(
              jsonEncode(element.value),
            ),
          );
          list.add(serviceBookingModel);
        });
      }catch(e){}
    }
    return list;
  }
}
