import 'package:firebase_dynamic_links/firebase_dynamic_links.dart';
import 'package:get/get.dart';
import 'package:groom/repo/session_repo.dart';
import 'package:groom/repo/setting_repo.dart';
import '../../../firebase/provider_service_firebase.dart';
import '../../../firebase/user_firebase.dart';
import '../../../states/customer_service_state.dart';
import '../services/provider_details_screen.dart';
import '../services/service_details_screen.dart';

class CustomerDashboardController extends GetxController {
  var currentIndex = 0.obs;
  UserFirebase userService = UserFirebase();
  int randomNumber=1;
  @override
  void onInit() {
    super.onInit();
    if(!isGuest.value) {
      getSession();
      userService.getUserDetails(auth.value.uid).then((value) {
      auth.value=value;
    },);
    }
    initLink();
  }

  void changePage(int index) {
    if (currentIndex.value != index) {
      currentIndex.value = index;
      switch(index){
        case 0:
          Get.toNamed(isProvider.value?"provider_home":"home", id: randomNumber);
          break;
        case 1:
          Get.toNamed(isProvider.value?"provider_booking":"booking", id: randomNumber);
          break;
        case 2:
          Get.toNamed(isProvider.value?"calendar":"search", id: randomNumber);
          break;
        case 3:
          Get.toNamed("profile", id: randomNumber);
          break;
      }
    }
  }
  Future<void> initLink() async {
    // Check if you received the link via `getInitialLink` first
    final PendingDynamicLinkData? initialLink = await FirebaseDynamicLinks.instance.getInitialLink();
    if (initialLink != null) {
      final Uri deepLink = initialLink.link;
      // Example of using the dynamic link to push the user to a different screen
      print("deepLink1==>$deepLink");
    }

    FirebaseDynamicLinks.instance.onLink.listen(
          (pendingDynamicLinkData) async {
        // Set up the `onLink` event listener next as it may be received here
        if (pendingDynamicLinkData != null) {
          final Uri deepLink = pendingDynamicLinkData.link;
          print("deepLink2==>$deepLink");
          print("deepLink2==>${deepLink.queryParameters}");
          Map<String,String> map=deepLink.queryParameters;
          if(map.isNotEmpty) {
            String? type = map['type'];
            String? id = map['id'];
            if(type!=null){
              if(type=="PROVIDER"){
                var data=await UserFirebase().getUser(id!);
                Get.to(() => ProviderDetailsScreen(
                  provider: data,
                ));
              }
              if(type=="SERVICE"){
                var data=await ProviderServiceFirebase().getService(id!);
                CustomerServiceState customerServiceState = Get.put(CustomerServiceState());
                customerServiceState
                    .selectedService.value = data;
                Get.to(() => ServiceDetailsScreen());

              }
            }
          }

        }
      },
    );
  }

}
