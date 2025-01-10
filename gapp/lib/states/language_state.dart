import 'dart:ui';

import 'package:easy_localization/easy_localization.dart';
import 'package:get/get.dart';

class LanguageController extends GetxController {
  Rx<Locale> selectedLocale = Rx(const Locale('en', 'US'));

  void changeLanguage(Locale newLocale) {
    selectedLocale.value = newLocale;
    EasyLocalization.of(Get.context!)!.setLocale(newLocale);
  }
}