import 'package:get/get.dart';

class ProviderDashboardController extends GetxController {
  var currentIndex = 0.obs; // Observable index

  void changeTabIndex(int index) {
    currentIndex.value = index;
  }
}
class BookingController extends GetxController {
  var selectedTabIndex = 0.obs;

  void changeTab(int index) {
    selectedTabIndex.value = index;
  }
}