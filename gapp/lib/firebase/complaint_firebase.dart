import 'dart:convert';

import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:groom/data_models/complaint_model.dart';

import '../constant/global_configuration.dart';

class ComplaintFirebase {
  var ref = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();

  Future<bool> writeComplaintToFirebase(
      String complaintId, ComplaintModel complaintModel) async {
    try {
      await ref
          .child('complaints')
          .child(complaintId)
          .set(complaintModel.toJson());

      return true;
    } catch (e) {
      print(e);
    }
    return false;
  }

  Future<bool> writeProviderComplaintToFirebase(
      String complaintId, ComplaintModel complaintModel) async {
    try {
      await ref
          .child('providerComplaints')
          .child(complaintId)
          .set(complaintModel.toJson());

      return true;
    } catch (e) {
      print(e);
    }
    return false;
  }

  Future<bool> writeCustomerComplaintToFirebase(
      String complaintId, ComplaintModel complaintModel) async {
    try {
      await ref
          .child('customerComplaints')
          .child(complaintId)
          .set(complaintModel.toJson());

      return true;
    } catch (e) {
      print(e);
    }
    return false;
  }
  Future<List<ComplaintModel>> getAllCustomerComplaintsByUserId(String userId) async {
    List<ComplaintModel> lst = [];
    var source = await ref
        .child('customerComplaints')
        .orderByChild('userId')
        .equalTo(userId)
        .once();
    var data = source.snapshot;
    data.children.forEach((complaint) {
      ComplaintModel complaintModel = ComplaintModel.fromJson(
        jsonDecode(
          jsonEncode(complaint.value),
        ),
      );

      lst.add(complaintModel);
    });

    return lst;
  }
  Future<List<ComplaintModel>> getAllProviderComplaintsByUserId(String userId) async {
    List<ComplaintModel> lst = [];
    var source = await ref
        .child('providerComplaints')
        .orderByChild('userId')
        .equalTo(userId)
        .once();
    var data = source.snapshot;
    data.children.forEach((complaint) {
      ComplaintModel complaintModel = ComplaintModel.fromJson(
        jsonDecode(
          jsonEncode(complaint.value),
        ),
      );

      lst.add(complaintModel);
    });

    return lst;
  }
}
