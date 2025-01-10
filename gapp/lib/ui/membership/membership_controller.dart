import 'package:firebase_core/firebase_core.dart';
import 'package:get/get.dart';
import 'package:groom/firebase/user_firebase.dart';

import '../../../Utils/tools.dart';
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
  void onSelectItem(int index){
    for (var element in list) {
      element.isSelected=false;
    }
    list[index].isSelected=true;
    selectedMembership.value=list[index];
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
}
