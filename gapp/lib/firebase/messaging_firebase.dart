import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_messaging/firebase_messaging.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/firebase/user_firebase.dart';

Future<void> handleBackgroundMessaging(RemoteMessage message) async {
  print("Title: ${message.notification?.title}");
  print("Body: ${message.notification?.body}");
  print("Payload: ${message.data}");
}

class FirebaseApi {
  final _firebaseMessaging = FirebaseMessaging.instance;
  UserFirebase userFirebase = UserFirebase();
  late FlutterLocalNotificationsPlugin flutterLocalNotificationsPlugin =
  FlutterLocalNotificationsPlugin();

  FirebaseApi() {
    _initializeFlutterLocalNotifications();
  }

  Future<void> initNotifications() async {
    NotificationSettings settings = await _firebaseMessaging.requestPermission(
      alert: true,
      announcement: false,
      badge: true,
      carPlay: false,
      criticalAlert: false,
      provisional: false,
      sound: true,
    );
    if (settings.authorizationStatus == AuthorizationStatus.authorized) {
      print("granted access");
    } else if (settings.authorizationStatus ==
        AuthorizationStatus.provisional) {
      print("provisional");
    } else {
      print("denied");
    }

    final fCMToken = await _firebaseMessaging.getToken();
    print("Token : $fCMToken");
    if(FirebaseAuth.instance.currentUser!=null) {
      await userFirebase.updateUserNotificationToken(
        FirebaseAuth.instance.currentUser!.uid, fCMToken!);
    }

    FirebaseMessaging.onBackgroundMessage(handleBackgroundMessaging);
    FirebaseMessaging.onMessage.listen(_handleForegroundMessaging);
  }

  void _initializeFlutterLocalNotifications() {
    const AndroidInitializationSettings initializationSettingsAndroid =
    AndroidInitializationSettings('@mipmap/launcher_icon');

    final DarwinInitializationSettings initializationSettingsIOS =
    DarwinInitializationSettings(
      requestAlertPermission: false,
      requestBadgePermission: false,
      requestSoundPermission: false,
      onDidReceiveLocalNotification:
          (int id, String? title, String? body, String? payload) async {
        // handle the received notification
      },
    );

    final InitializationSettings initializationSettings = InitializationSettings(
      android: initializationSettingsAndroid,
      iOS: initializationSettingsIOS,
    );

    flutterLocalNotificationsPlugin.initialize(initializationSettings,
        onDidReceiveNotificationResponse: (NotificationResponse response) async {
          if (response.payload != null && response.payload!.isNotEmpty) {
            // Handle the notification tapped logic here
            print('Notification payload: ${response.payload}');
          }
        });
  }

  void _handleForegroundMessaging(RemoteMessage message) async {
    print("Title: ${message.notification?.title}");
    print("Body: ${message.notification?.body}");
    print("Payload: ${message.data}");

    if (message.notification != null) {
      _showNotification(
        message.notification!.title ?? 'No Title',
        message.notification!.body ?? 'No Body',
        message.data,
      );
    }
  }

  Future<void> _showNotification(String title, String body, Map<String, dynamic> payload) async {
    BigTextStyleInformation bigTextStyleInformation = BigTextStyleInformation(
        body,
        htmlFormatBigText: true,
        contentTitle: title,
        htmlFormatContentTitle: true
    );

    AndroidNotificationDetails androidPlatformChannelSpecifics = AndroidNotificationDetails(
        'dbfood',
        'dbfood',
        importance: Importance.high,
        styleInformation: bigTextStyleInformation,
        priority: Priority.high,
        playSound: true
    );

    NotificationDetails platformChannelSpecifics = NotificationDetails(
        android: androidPlatformChannelSpecifics
    );

    await flutterLocalNotificationsPlugin.show(
        0,
        title,
        body,
        platformChannelSpecifics,
        payload: payload.toString()
    );
  }

  void initInfo() {
    var androidInitialize = const AndroidInitializationSettings('@mipmap/launcher_icon');
    var initializationSettings = InitializationSettings(android: androidInitialize);
    flutterLocalNotificationsPlugin.initialize(initializationSettings,
        onDidReceiveNotificationResponse: (NotificationResponse response) {
          try {
            if (response.payload != null && response.payload!.isNotEmpty) {
              // Handle notification response
            } else {
              // Handle case where there is no payload
            }
          } catch (e) {
            print("Error handling notification response: $e");
          }
        });

    FirebaseMessaging.onMessage.listen((RemoteMessage message) async {
      print(".................onMessage.................");
      print("onMessage: ${message.notification?.body}");

      BigTextStyleInformation bigTextStyleInformation = BigTextStyleInformation(
          message.notification!.body.toString(),
          htmlFormatBigText: true,
          contentTitle: message.notification!.title.toString(),
          htmlFormatContentTitle: true
      );

      AndroidNotificationDetails androidPlatformChannelSpecifics = AndroidNotificationDetails(
          'dbfood',
          'dbfood',
          importance: Importance.high,
          styleInformation: bigTextStyleInformation,
          priority: Priority.high,
          playSound: true
      );

      NotificationDetails platformChannelSpecifics = NotificationDetails(
          android: androidPlatformChannelSpecifics
      );

      await flutterLocalNotificationsPlugin.show(
          0,
          message.notification!.title,
          message.notification!.body,
          platformChannelSpecifics,
          payload: message.data['body']
      );
    });
  }
}