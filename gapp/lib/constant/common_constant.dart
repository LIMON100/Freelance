import 'package:firebase_app_check/firebase_app_check.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_messaging/firebase_messaging.dart';
import 'package:flutter_stripe/flutter_stripe.dart';
import 'enums.dart';
import 'global_configuration.dart';

class CommonConstant {
  static Mode APP_MODE = Mode.DEV;

  static void initializeApp(Mode mode) async {
    APP_MODE = mode;
    await GlobalConfiguration()
        .loadFromAsset("configurations_${mode.name.toLowerCase()}");
    String stripe_api_key_pk = "pk_test_ajyjX25IOpFaCxbfsbLYdNGT";
    Stripe.publishableKey = stripe_api_key_pk;
    Stripe.merchantIdentifier = 'merchant.flutter.stripe.test';
    Stripe.urlScheme = 'flutterstripe';
    await Stripe.instance.applySettings();
    await Firebase.initializeApp();
    // await Firebase.initializeApp(options: DefaultFirebaseOptions.currentPlatform,);
    await FirebaseMessaging.instance.getInitialMessage();
    await FirebaseAppCheck.instance.activate(
      androidProvider: AndroidProvider.playIntegrity,
      appleProvider: AppleProvider.appAttest,
    );
  }
}
