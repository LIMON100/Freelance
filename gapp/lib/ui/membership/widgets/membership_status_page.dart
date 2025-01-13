import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../../data_models/user_model.dart';
import '../../../repo/setting_repo.dart';
import '../membership_controller.dart';
import '../membership_page.dart';

class MembershipStatusPage extends StatelessWidget {
  UserModel data;
  MembershipStatusPage({super.key,required this.data});
  final controller = Get.put(MembershipController());
  @override
  Widget build(BuildContext context) {
    if(isDismissMembership.value){
      return const SizedBox();
    }
    if(data.membership==null) {
      return Dismissible(key: const ValueKey("start"),
          confirmDismiss: (direction) {
            isDismissMembership.value=true;
            return Future.value(true);
          },
          background: Container(
            color: primaryColorCode,
            alignment: AlignmentDirectional.centerEnd,
            padding: const EdgeInsets.all(20),
            child: SubTxtWidget('Close',color: Colors.white,),
          ),
          child: startMembership());
    }
    if(Tools.changeToDate(data.membership!.endDate!).difference(DateTime.now()).inDays<=7) {
      return Dismissible(key: const ValueKey('renew'),
      confirmDismiss: (direction) {
        isDismissMembership.value=true;
        return Future.value(true);
      },
      background: Container(
        color: primaryColorCode,
        alignment: AlignmentDirectional.centerEnd,
        padding: const EdgeInsets.all(20),
        child: SubTxtWidget('Close',color: Colors.white,),
      ),
        child: renewMembership(),);
    }
    if(DateTime.now().isAfter(Tools.changeToDate(data.membership!.endDate!))){
      return Dismissible(key: const ValueKey("upgrade"),
          confirmDismiss: (direction) {
            isDismissMembership.value=true;
            return Future.value(true);
          },
          background: Container(
            color: primaryColorCode,
            alignment: AlignmentDirectional.centerEnd,
            padding: const EdgeInsets.all(20),
            child: SubTxtWidget('Close',color: Colors.white,),
          ),
          child: expireMembership());
    }
    return const SizedBox();
  }
  Widget startMembership(){
    return Container(
      color: "#FFF0D9".toColor(),
      padding: const EdgeInsets.all(15),
      child: Row(
        children: [
          Expanded(child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              HeaderTxtWidget('Upgrade your plan today'),
              SubTxtWidget('Your have free membership plan'),
            ],
          )),
          const SizedBox(width: 10,),
          ActionChip(label: SubTxtWidget('Upgrade',color: Colors.white,),
            backgroundColor: '#F9A114'.toColor(),onPressed: () {
            Get.to(MembershipPage(data: data,));
            },)
        ],
      ),
    );
  }
  Widget renewMembership(){
    return Container(
      color: "#FFF0D9".toColor(),
      padding: const EdgeInsets.all(15),
      child: Row(
        children: [
          Expanded(child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              HeaderTxtWidget('Reminder'),
              SubTxtWidget('Your plan is about to expire in ${Tools.changeToDate(data.membership!.endDate!).difference(DateTime.now()).inDays} Days'),
            ],
          )),
          const SizedBox(width: 10,),
          ActionChip(label: SubTxtWidget('Renew',color: Colors.white,),
            backgroundColor: '#F9A114'.toColor(),onPressed: () {
              Get.to(MembershipPage(data: data,));
            },)
        ],
      ),
    );
  }
  Widget expireMembership(){
    return Container(
      color: "#FFF0D9".toColor(),
      padding: const EdgeInsets.all(15),
      child: Row(
        children: [
          Expanded(child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              HeaderTxtWidget('Oops! Plan Expired'),
              SubTxtWidget('your premium plan is expired'),
            ],
          )),
          const SizedBox(width: 10,),
          ActionChip(label: SubTxtWidget('buy now',color: Colors.white,),
            backgroundColor: Colors.red,onPressed: () {
              Get.to(MembershipPage(data: data,));
            },)
        ],
      ),
    );
  }
}
