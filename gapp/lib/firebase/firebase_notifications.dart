import 'dart:convert';
import 'package:firebase_messaging/firebase_messaging.dart';
import 'package:flutter/cupertino.dart';
import 'package:http/http.dart' as http;
import 'package:googleapis_auth/auth_io.dart' as auth;
import 'package:googleapis/servicecontrol/v1.dart' as serviceControl;

class PushNotificationService {


  static Future<String> getAccessToken() async {
    final serviceAccountJson = {
    
    };
    List<String> scopes = [
      "https://www.googleapis.com/auth/userinfo.email",
      "https://www.googleapis.com/auth/firebase.database",
      "https://www.googleapis.com/auth/firebase.messaging"
    ];
    http.Client client = await auth.clientViaServiceAccount(
      auth.ServiceAccountCredentials.fromJson(serviceAccountJson),
      scopes,
    );

    auth.AccessCredentials credentials =
        await auth.obtainAccessCredentialsViaServiceAccount(
            auth.ServiceAccountCredentials.fromJson(serviceAccountJson),
            scopes,
            client);
    client.close();
    return credentials.accessToken.data;
  }



  static sendCustomerBookingNotificationToProvider(
      String deviceToken, BuildContext context, String tripId,String userName) async {
    final String serverAccessTokenKey = await getAccessToken();
    String endpointFirebaseCloudMessaging =
        'https://fcm.googleapis.com/v1/projects/groom202406-web/messages:send';
    final  currentFCMToken = await FirebaseMessaging.instance.getToken();
    print ("fcmkey: $currentFCMToken");

    final Map<String, dynamic> message = {
      'message': {
        'token': deviceToken,
        'notification': {
          'title': "You Have received a new offer",
          'body': "$userName has made an offer  "
        },
        'data': {
          'tripId': tripId,
        }
      }
    };
    final http.Response response = await http.post(
      Uri.parse(endpointFirebaseCloudMessaging),
      headers: <String, String>{
        'Content-Type': 'application/json',
        'Authorization': 'Bearer $serverAccessTokenKey'
      },
      body: jsonEncode(message),
    );
    if (response.statusCode == 200) {
      print("Fcm message sent");
    } else {
      print("Fcm message failed ${response.statusCode}");
    }
  }

  static sendCustomerAcceptPaymentNotificationToProvider(
      String deviceToken, BuildContext context, String tripId,String userName) async {
    final String serverAccessTokenKey = await getAccessToken();
    String endpointFirebaseCloudMessaging =
        'https://fcm.googleapis.com/v1/projects/groom202406-web/messages:send';
    final  currentFCMToken = await FirebaseMessaging.instance.getToken();
    print ("fcmkey: $currentFCMToken");

    final Map<String, dynamic> message = {
      'message': {
        'token': deviceToken,
        'notification': {
          'title': "Reservation Confirmed",
          'body': "$userName has Confirmed the reservation Payment successfully   "
        },
        'data': {
          'tripId': tripId,
        }
      }
    };
    final http.Response response = await http.post(
      Uri.parse(endpointFirebaseCloudMessaging),
      headers: <String, String>{
        'Content-Type': 'application/json',
        'Authorization': 'Bearer $serverAccessTokenKey'
      },
      body: jsonEncode(message),
    );
    if (response.statusCode == 200) {
      print("Fcm message sent");
    } else {
      print("Fcm message failed ${response.statusCode}");
    }
  }


  static sendCustomerConfirmReservationNotificationToProvider(
      String deviceToken, BuildContext context, String tripId,String userName) async {
    final String serverAccessTokenKey = await getAccessToken();
    String endpointFirebaseCloudMessaging =
        'https://fcm.googleapis.com/v1/projects/groom202406-web/messages:send';
    final  currentFCMToken = await FirebaseMessaging.instance.getToken();
    print ("fcmkey: $currentFCMToken");

    final Map<String, dynamic> message = {
      'message': {
        'token': deviceToken,
        'notification': {
          'title': "Reservation Confirmed",
          'body': "$userName has Confirmed the reservation Payment successfully   "
        },
        'data': {
          'tripId': tripId,
        }
      }
    };
    final http.Response response = await http.post(
      Uri.parse(endpointFirebaseCloudMessaging),
      headers: <String, String>{
        'Content-Type': 'application/json',
        'Authorization': 'Bearer $serverAccessTokenKey'
      },
      body: jsonEncode(message),
    );
    if (response.statusCode == 200) {
      print("Fcm message sent");
    } else {
      print("Fcm message failed ${response.statusCode}");
    }
  }

  static sendProviderOfferNotificationToClient(
      String deviceToken, BuildContext context, String offerId) async {
    final String serverAccessTokenKey = await getAccessToken();
    String endpointFirebaseCloudMessaging =
        'https://fcm.googleapis.com/v1/projects/groom202406-web/messages:send';
    final  currentFCMToken = await FirebaseMessaging.instance.getToken();
    print ("fcmkey: $currentFCMToken");

    final Map<String, dynamic> message = {
      'message': {
        'token': deviceToken,
        'notification': {
          'title': "Request update",
          'body': "You have received a new offer for your request"
        },
        'data': {
          'offerId': offerId,
        }
      }
    };
    final http.Response response = await http.post(
      Uri.parse(endpointFirebaseCloudMessaging),
      headers: <String, String>{
        'Content-Type': 'application/json',
        'Authorization': 'Bearer $serverAccessTokenKey'
      },
      body: jsonEncode(message),
    );
    if (response.statusCode == 200) {
      print("Fcm message sent");
    } else {
      print("Fcm message failed ${response.statusCode}");
    }
  }

  static sendProviderConfirmReservationNotificationToClient(
      String deviceToken, BuildContext context, String tripId,String userName) async {
    final String serverAccessTokenKey = await getAccessToken();
    String endpointFirebaseCloudMessaging =
        'https://fcm.googleapis.com/v1/projects/groom202406-web/messages:send';
    final  currentFCMToken = await FirebaseMessaging.instance.getToken();
    print ("fcmkey: $currentFCMToken");

    final Map<String, dynamic> message = {
      'message': {
        'token': deviceToken,
        'notification': {
          'title': "Reservation Confirmed",
          'body': "$userName has Confirmed the reservation Payment successfully   "
        },
        'data': {
          'tripId': tripId,
        }
      }
    };
    final http.Response response = await http.post(
      Uri.parse(endpointFirebaseCloudMessaging),
      headers: <String, String>{
        'Content-Type': 'application/json',
        'Authorization': 'Bearer $serverAccessTokenKey'
      },
      body: jsonEncode(message),
    );
    if (response.statusCode == 200) {
      print("Fcm message sent");
    } else {
      print("Fcm message failed ${response.statusCode}");
    }
  }

}
