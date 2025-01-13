import 'dart:async';
import 'dart:convert';
import 'package:dio/dio.dart';
import 'package:flutter/material.dart';
import 'package:flutter_stripe/flutter_stripe.dart';
import '../constant/global_configuration.dart';
import '../utils/tools.dart';

import '../utils/colors.dart';


Future<void> stripeMakePayment({required String amount,required String description,required String username,
  required Function(Map data)paymentSuccess,required Function()paymentFailed,}) async {

  try {
    var paymentIntentData = await createStripeIntent(
       amount: amount, description: description,username: username
    );
    print(paymentIntentData);
    if (paymentIntentData!.containsKey("error")) {
      print(paymentIntentData);
      Tools.ShowErrorMessage('Something went wrong');
      paymentFailed.call();
    } else {
      await Stripe.instance
          .initPaymentSheet(
          paymentSheetParameters: SetupPaymentSheetParameters(
            paymentIntentClientSecret: paymentIntentData!['client_secret'],
            applePay: const PaymentSheetApplePay(
              merchantCountryCode: 'US',
            ),
            allowsDelayedPaymentMethods: false,
            googlePay: const PaymentSheetGooglePay(
              merchantCountryCode: 'US',
              testEnv: false,
              currencyCode: "USD",
            ),
            style: ThemeMode.system,
            appearance: PaymentSheetAppearance(
              colors: PaymentSheetAppearanceColors(
                primary: primaryColorCode,
              ),
            ),
            merchantDisplayName: 'Groom',
          ))
          .then((value) {
        displayStripePaymentSheet().then((value) {
          if(value){
            paymentSuccess.call(paymentIntentData);
          }else{
            paymentFailed.call();
          }
        },);
      });
    }
  } catch (e, s) {
    print('exception:$e$s');
    paymentFailed.call();
  }
}
  createStripeIntent({required String amount,required String description,required String username,}) async {
    String stripe_api_key_sk= "sk_test_UAzA5ww2KfqlhvN6ocYA3Dez";
    try {
      Map<String, dynamic> body = {
        'amount': calculateAmount(amount),
        'currency': "USD",
        'payment_method_types[0]': 'card',
        // 'payment_method_types[1]': 'ideal',
        "description": description,
        "shipping[name]": username,
        "shipping[address][line1]": "510 Townsend St",
        "shipping[address][postal_code]": "98140",
        "shipping[address][city]": "San Francisco",
        "shipping[address][state]": "CA",
        "shipping[address][country]": "US",
      };
      Dio dio = Dio();
      var res = await dio.post('https://api.stripe.com/v1/payment_intents',
          data: body,
          options: Options(headers: {
            'Authorization':
            'Bearer $stripe_api_key_sk',
            //$_paymentIntentClientSecret',
            'Content-Type': 'application/x-www-form-urlencoded'
          },validateStatus: (status) {
            return true;
          },));
      return res.data;
    } catch (err) {
      print('error charging user: ${err.toString()}');
    }
  }
  calculateAmount(String amount) {
    final a = (double.parse(amount)) * 100;
    return a.toInt().toString();
  }
  Future<bool>displayStripePaymentSheet() async {
  bool status=false;
    try {
      await Stripe.instance.presentPaymentSheet().then((value) {
        Tools.ShowSuccessMessage("Payment success");
        status= true;
      });
    } on StripeException catch (e) {
      status=false;
      Tools.ShowErrorMessage("Payment failed");
      var lo1 = jsonEncode(e);
      var lo2 = jsonDecode(lo1);
      print("paymentFailed==>${lo2}");
    } catch (e) {
      print('$e');
      status=false;
    }
    return status;
  }

