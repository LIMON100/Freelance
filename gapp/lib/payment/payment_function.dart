import 'package:flutter/material.dart';
import 'package:flutter_stripe/flutter_stripe.dart';
import 'package:groom/payment/payment.dart';

Future<void> initPaymentSheet(
    BuildContext context,
    String name,
    String address,
    String pin,
    String state,
    String country,
    String city,
    String currency,
    String amount) async {
  try {
    // 1. create payment intent on the server
    final data = await createPaymentIntent(
        name: name,
        address: address,
        pin: pin,
        city: city,
        state: state,
        country: country,
        currency: currency,
        amount: amount);

    // 2. initialize the payment sheet
    await Stripe.instance.initPaymentSheet(
      paymentSheetParameters: SetupPaymentSheetParameters(
        // Set to true for custom flow
        customFlow: false,
        // Main params
        merchantDisplayName: 'Groom',
        paymentIntentClientSecret: data['client_secret'],
        // Customer keys
        customerEphemeralKeySecret: data['ephemeralKey'],
        customerId: data['id'],
        // Extra options
        style: ThemeMode.light,
      ),
    );
  } catch (e) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Error: $e')),
    );
    rethrow;
  }
}
