import 'package:firebase_auth/firebase_auth.dart';
import 'package:get/get.dart';
import 'package:groom/repo/session_repo.dart';

class SplashController extends GetxController {
@override
  void onInit() {
    // TODO: implement onInit
    super.onInit();
    getSession();
    Future.delayed(const Duration(seconds: 3),() {
      isLogin().then((value) {
        Get.offAllNamed(value?'/customer_dashboard':'/onboard');
      },);

    },);
  }
}
