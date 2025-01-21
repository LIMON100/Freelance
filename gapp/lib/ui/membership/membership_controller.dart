// import 'package:firebase_core/firebase_core.dart';
// import 'package:get/get.dart';
// import 'package:groom/firebase/user_firebase.dart';
//
// import '../../../utils/tools.dart';
// import '../../../firebase/provider_user_firebase.dart';
// import '../../../repo/setting_repo.dart';
// import '../../../repo/stripre_repo.dart';
// import 'm/membership_model.dart';
//
// class MembershipController extends GetxController {
//   RxList<MembershipModel>list=RxList([]);
//   Rx<MembershipModel?> selectedMembership=Rx(null);
//   ProviderUserFirebase userFirebase=ProviderUserFirebase();
//   @override
//   void onInit() {
//     // TODO: implement onInit
//     super.onInit();
//     initList();
//   }
//   void initList(){
//     list.add(MembershipModel(title: "New", duration: '1 week', amount: '8.49',totalAmount: 8.49));
//     list.add(MembershipModel(title: "Save 52%", duration: '1 month', amount: '4.04',totalAmount: 16.16));
//     list.add(MembershipModel(title: "Save 70%", duration: '3 months', amount: '2.52',totalAmount: 30.24));
//     list.add(MembershipModel(title: "Save 77%", duration: '6 months', amount: '1.94',totalAmount: 46.56));
//   }
//   void onSelectItem(int index){
//     for (var element in list) {
//       element.isSelected=false;
//     }
//     list[index].isSelected=true;
//     selectedMembership.value=list[index];
//   }
//   void payWithCard(MembershipModel? membership) {
//     DateTime now = DateTime.now();
//     selectedMembership.value!.startDate = now.toString();
//     selectedMembership.value!.endDate = now.add(Duration(days: getDays())).toString();
//     if(membership!=null) {
//       if(DateTime.now().isBefore(Tools.changeToDate(membership.endDate!))){
//         int days=Tools.changeToDate(membership.endDate!).difference(DateTime.now()).inDays;
//         selectedMembership.value!.startDate = now.add(Duration(days: days)).toString();
//         selectedMembership.value!.endDate = now.add(Duration(days: days+getDays())).toString();
//       }
//     }
//     stripeMakePayment(
//         amount: selectedMembership.value!.totalAmount.toString(),
//         description: 'Membership ${selectedMembership.value!.amount}',
//         paymentFailed: () {
//           Tools.ShowErrorMessage("Payment failed");
//         },
//         paymentSuccess: (data) {
//           userFirebase.addMembership(auth.value.uid, selectedMembership.value!).then((value) {
//             Get.offAllNamed('/customer_dashboard');
//             UserFirebase().getUserDetails(auth.value.uid);
//           },);
//
//         },
//         username: auth.value.fullName);
//   }
//   int getDays(){
//     if(selectedMembership.value!.duration.contains("month")){
//       String month=selectedMembership.value!.duration.split(" ")[0];
//       return int.parse(month)*30;
//     }
//     return 7;
//   }
// }


import 'package:firebase_core/firebase_core.dart';
import 'package:get/get.dart';
import 'package:groom/firebase/user_firebase.dart';

import '../../../utils/tools.dart';
import '../../../firebase/provider_user_firebase.dart';
import '../../../repo/setting_repo.dart';
import '../../../repo/stripre_repo.dart';
import 'm/membership_model.dart';

class MembershipController extends GetxController {
  RxList<MembershipModel>list=RxList([]);
  Rx<MembershipModel?> selectedMembership=Rx(null);
  ProviderUserFirebase userFirebase=ProviderUserFirebase();
  @override
  void onInit() {
    // TODO: implement onInit
    super.onInit();
    initList();
  }
  void initList(){
    list.add(MembershipModel(title: "New", duration: '1 week', amount: '8.49',totalAmount: 8.49));
    list.add(MembershipModel(title: "Save 52%", duration: '1 month', amount: '4.04',totalAmount: 16.16));
    list.add(MembershipModel(title: "Save 70%", duration: '3 months', amount: '2.52',totalAmount: 30.24));
    list.add(MembershipModel(title: "Save 77%", duration: '6 months', amount: '1.94',totalAmount: 46.56));
  }
  void onSelectItem(List<MembershipModel> filteredList,int index){
    for (var element in filteredList) {
      element.isSelected=false;
    }
    filteredList[index].isSelected=true;
    selectedMembership.value=filteredList[index];
  }
  void payWithCard(MembershipModel? membership) {
    DateTime now = DateTime.now();
    selectedMembership.value!.startDate = now.toString();
    selectedMembership.value!.endDate = now.add(Duration(days: getDays())).toString();
    if(membership!=null) {
      if(DateTime.now().isBefore(Tools.changeToDate(membership.endDate!))){
        int days=Tools.changeToDate(membership.endDate!).difference(DateTime.now()).inDays;
        selectedMembership.value!.startDate = now.add(Duration(days: days)).toString();
        selectedMembership.value!.endDate = now.add(Duration(days: days+getDays())).toString();
      }
    }
    stripeMakePayment(
        amount: selectedMembership.value!.totalAmount.toString(),
        description: 'Membership ${selectedMembership.value!.amount}',
        paymentFailed: () {
          Tools.ShowErrorMessage("Payment failed");
        },
        paymentSuccess: (data) {
          userFirebase.addMembership(auth.value.uid, selectedMembership.value!).then((value) {
            Get.offAllNamed('/customer_dashboard');
            UserFirebase().getUserDetails(auth.value.uid);
          },);

        },
        username: auth.value.fullName);
  }
  int getDays(){
    if(selectedMembership.value!.duration.contains("month")){
      String month=selectedMembership.value!.duration.split(" ")[0];
      return int.parse(month)*30;
    }
    return 7;
  }


  // Indexing left-right checking
  // List<MembershipModel> getFilteredList({String? currentMembershipDuration, bool isUpgrade = false, bool isDowngrade = false}) {
  //   List<MembershipModel> result = list.toList();
  //   if (currentMembershipDuration == null) {
  //     return result;
  //   }
  //   // Calculate index for current duration
  //   int currentDurationIndex = list.indexWhere((element) => element.duration == currentMembershipDuration);
  //
  //   if (isUpgrade) {
  //     // Show plans with indices greater than the current, excluding the current plan
  //     result = list.where((element) => list.indexOf(element) > currentDurationIndex).toList();
  //   }
  //
  //   if (isDowngrade) {
  //     // Show plans with indices less than the current, excluding the current plan
  //     result = list.where((element) => list.indexOf(element) < currentDurationIndex).toList();
  //   }
  //   return result;
  // }

  List<MembershipModel> getFilteredList({
    String? currentMembershipDuration,
    bool isUpgrade = false,
    bool isDowngrade = false,
  }) {
    List<MembershipModel> result = list.toList();

    if (currentMembershipDuration == null) {
      return result;
    }

    // Handle 1-week plan
    if (currentMembershipDuration.contains("week")) {
      if (isUpgrade) {
        result.removeWhere((element) => element.duration.contains("week"));
      }
      if (isDowngrade) {
        result.clear(); // No downgrade for the 1-week plan
      }
      return result;
    }

    // Handle 6-month plan
    if (currentMembershipDuration.contains("6 months")) {
      if (isUpgrade) {
        result.clear(); // No upgrade for the 6-month plan
      }
      if (isDowngrade) {
        result.removeWhere((element) => element.duration.contains("6 months"));
      }
      return result;
    }

    // Handle 1-month plan
    if (currentMembershipDuration.contains("1 month")) {
      if (isUpgrade) {
        // Exclude 1-month and any lower plans (like 1-week)
        result.removeWhere((element) => !element.duration.contains("3 months") && !element.duration.contains("6 months"));
      }
      if (isDowngrade) {
        // Only include 1-week plan
        result.removeWhere((element) => !element.duration.contains("week"));
      }
      return result;
    }

    // For all other plans (e.g., 3 months), calculate based on index
    int currentDurationIndex = list.indexWhere(
            (element) => element.duration.contains(currentMembershipDuration.split(" ")[0]));

    if (isUpgrade) {
      // Include only plans with indices higher than the current plan
      result = list.where((element) => list.indexOf(element) > currentDurationIndex).toList();
    }

    if (isDowngrade) {
      // Include only plans with indices lower than the current plan
      result = list.where((element) => list.indexOf(element) < currentDurationIndex).toList();
    }

    return result;
  }

}
