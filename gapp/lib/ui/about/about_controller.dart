import 'dart:io';
import 'package:flutter/cupertino.dart';
import 'package:get/get.dart';
import 'package:rate_my_app/rate_my_app.dart';


class AboutController extends GetxController {

  RateMyApp rateMyApp = RateMyApp(
    preferencesPrefix: 'rateMyApp_',
    minDays: 7,
    minLaunches: 10,
    remindDays: 7,
    remindLaunches: 10,
    googlePlayIdentifier: 'com.groomapp.android',
    appStoreIdentifier: '1491556149',
  );
  @override
  void onInit() {
    // TODO: implement onInit
    super.onInit();
  }

  Future<void> review(context) async {
    rateMyApp.showRateDialog(
      context,
      title: 'Rate this app',
      message: 'If you like this app, please take a little bit of your time to review it !\nIt really helps us and it shouldn\'t take you more than one minute.', // The dialog message.
      rateButton: 'RATE',
      noButton: 'NO THANKS',
      laterButton: 'MAYBE LATER',
      ignoreNativeDialog: Platform.isAndroid,
      listener: (button) {
        switch(button){
          case RateMyAppDialogButton.rate:
        rateMyApp.callEvent(RateMyAppEventType.rateButtonPressed);
        Navigator.pop<RateMyAppDialogButton>(context, RateMyAppDialogButton.rate);
        break;
          case RateMyAppDialogButton.later:
            Navigator.pop(context);
            break;
          case RateMyAppDialogButton.no:
            Navigator.pop(context);
            break;
        }
        return false;
      },
      dialogStyle: const DialogStyle(),
    );
  }
}
