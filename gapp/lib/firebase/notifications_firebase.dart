import 'dart:convert';

import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:groom/data_models/notification_model.dart';
import 'package:groom/repo/setting_repo.dart';

import '../constant/global_configuration.dart';

class NotificationsFirebase {
  final _ref = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'))
      .ref();
  final _auth =  FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'));

  Future<bool> writeNotificationToUser(String userId,
      NotificationModel notificationModel, String notificationId) async {
    try {
      await _ref
          .child("users")
          .child(userId)
          .child("notifications")
          .child(notificationId)
          .set(notificationModel.toJson());
      return true;
    } catch (e) {
      print(e);
    }

    return false;
  }

  Future<List<NotificationModel>> getNotificationsByUserId(String userId) async {
    List<NotificationModel> notifications = [];
    var source = await _auth.ref("users").child(userId).child("notifications").once();
    var data = source.snapshot;

    if (data.value != null) {
      data.children.forEach((element) {
        NotificationModel notificationModel = NotificationModel.fromJson(jsonDecode(jsonEncode(element.value)));
        if(notificationModel.sendBy=="USER"&&isProvider.value) {
          notifications.add(notificationModel);
        }
        if(notificationModel.sendBy=="PROVIDER"&&!isProvider.value) {
          notifications.add(notificationModel);
        }
      });
    }
    return notifications;
  }
}
