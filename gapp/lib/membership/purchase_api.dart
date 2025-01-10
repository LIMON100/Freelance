import 'dart:developer';
import 'dart:io';

import 'package:flutter/services.dart';
import 'package:purchases_flutter/purchases_flutter.dart';
import 'package:purchases_ui_flutter/purchases_ui_flutter.dart';

class PurchaseApi {
  static const _apiKey = "goog_WxLBnXaZQXzXXlRnJuBhnwLjvyx";

  static Future init() async {
    await Purchases.setDebugLogsEnabled(true);
    await Purchases.setup(_apiKey);
  }

  static Future<List<Offering>> fetchOffers() async {
    try {
      final offerings = await Purchases.getOfferings();
      final current = offerings.current;
      return current == null ? [] : [current];
    } on PlatformException catch (e) {
      return [];
    }
  }

  static Future<bool> purchasePackage(Package package) async {
    try {
      await PurchaseApi.purchasePackage(package);
      return true;
    } catch (e) {

      print(e);
      return false;
    }
  }
}


class RevenueCatApi {

  static const _apiKey = "goog_WxLBnXaZQXzXXlRnJuBhnwLjvyx";

   static Future configureSDK()async{

    await Purchases.setLogLevel(LogLevel.debug);

    PurchasesConfiguration? configuration;
    if (Platform.isAndroid){
      configuration = PurchasesConfiguration(_apiKey);
    }
    if(configuration != null){
      await Purchases.configure(configuration);
      final paywallResult = await RevenueCatUI.presentPaywallIfNeeded("Pro");
      log("Paywall Result:$paywallResult");

    }

  }
}