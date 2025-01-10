import 'dart:convert';
import 'dart:io';

import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_storage/firebase_storage.dart';
import 'package:groom/data_models/customer_offer_model.dart';
import 'package:groom/repo/setting_repo.dart';

import '../constant/global_configuration.dart';

class TransactionFirebase {
  final DatabaseReference _databaseReference = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();

  void addTransactionToFirebase(Map<String,dynamic>map) async {
    final userRef = _databaseReference.ref
        .child('transaction');
    String? key= userRef.push().key;
    map['id']=key!;
    userRef.child(key).set(map);
  }

}
