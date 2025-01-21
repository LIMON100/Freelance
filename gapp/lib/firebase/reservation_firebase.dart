import 'dart:convert';

import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';

import '../constant/global_configuration.dart';
import '../data_models/request_reservation_model.dart';
import '../data_models/service_reservation_model.dart';

class ReservationFirebase {
  var ref = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();
  var auth = FirebaseAuth.instance;

  Future<bool> writeServiceReservationToFirebase(String reservationId,
      ServiceReservationModel serviceReservationModel) async {
    try {
      await ref
          .child("serviceReservation")
          .child(reservationId)
          .set(serviceReservationModel.toJson());
      return true;
    } catch (e) {
      print(e);
    }

    return false;
  }
  Future<bool> writeRequestReservationToFirebase(String reservationId,
      RequestReservationModel requestReservationModel) async {
    try {
      await ref
          .child("requestReservation")
          .child(reservationId)
          .set(requestReservationModel.toJson());
      return true;
    } catch (e) {
      print(e);
    }

    return false;
  }

  Future<List<ServiceReservationModel>> getServiceReservationsByUserId(
      String userId) async {
    print("INSIDE THIS..");
    List<ServiceReservationModel> serviceReservationsList = [];
    var source = await ref
        .child("serviceReservation")
        .orderByChild("clientId")
        .equalTo(userId)
        .once();
    var data = source.snapshot;
    if (data.value != null) {
      data.children.forEach((element ) {
        var serviceReservationModel = ServiceReservationModel.fromJson(
          jsonDecode(
            jsonEncode(element.value),
          ),
        );
        serviceReservationsList.add(serviceReservationModel);
      });
    }

    return serviceReservationsList;
  }

  Future<List<ServiceReservationModel>> getProviderServiceReservationsByUserId(
      String providerId) async {
    List<ServiceReservationModel> serviceReservationsList = [];
    var source = await ref
        .child("serviceReservation")
        .orderByChild("providerId")
        .equalTo(providerId)
        .once();
    var data = source.snapshot;
    if (data.value != null) {
      data.children.forEach((element ) {
        var serviceReservationModel = ServiceReservationModel.fromJson(
          jsonDecode(
            jsonEncode(element.value),
          ),
        );
        serviceReservationsList.add(serviceReservationModel);
      });
    }

    return serviceReservationsList;
  }
  Future<List<RequestReservationModel>> getRequestReservationsByUserId(
      String userId) async {
    List<RequestReservationModel> requestReservationsList = [];
    var source = await ref
        .child("requestReservation")
        .orderByChild("clientId")
        .equalTo(userId)
        .once();
    var data = source.snapshot;
    if (data.value != null) {
      data.children.forEach((element) {
        var requestReservationModel = RequestReservationModel.fromJson(
          jsonDecode(
            jsonEncode(element.value),
          ),
        );
        requestReservationsList.add(requestReservationModel);
      });
    }

    return requestReservationsList;
  }
  Future<List<RequestReservationModel>> getProviderRequestReservationsByUserId(
      String providerId) async {
    List<RequestReservationModel> requestReservationsList = [];
    var source = await ref
        .child("requestReservation")
        .orderByChild("providerId")
        .equalTo(providerId)
        .once();
    var data = source.snapshot;
    if (data.value != null) {
      data.children.forEach((element) {
        var requestReservationModel = RequestReservationModel.fromJson(
          jsonDecode(
            jsonEncode(element.value),
          ),
        );
        requestReservationsList.add(requestReservationModel);
      });
      print('requestReservationsList ${requestReservationsList.length}');
    }

    return requestReservationsList;
  }

  Future<void> cancelServiceReservation(String reservationId,String region,String canceled_by)async {
    try{
      await ref.child("serviceReservation").child(reservationId).update({
        "status":"Canceled",
        "cancel_region":region,
        "canceled_by":canceled_by
      });

    }catch(e){print(e);}

  }Future<void> updateReviewServiceReservation(String reservationId)async {
    try{
      await ref.child("serviceReservation").child(reservationId).update({
        "review_status":"1"
      });

    }catch(e){print(e);}

  }
  Future<void> cancelRequestReservation(String reservationId)async {

    try{
      await ref.child("requestReservation").child(reservationId).remove();

    }catch(e){print(e);}

  }
}
