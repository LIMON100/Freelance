import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import '../../utils/tools.dart';
import '../../generated/assets.dart';
import '../../widgets/sub_txt_widget.dart';
import 'membership_controller.dart';
import 'membership_page.dart';
class MembershipActivePlanPage extends StatefulWidget {
  MembershipActivePlanPage({super.key});

  @override
  State<MembershipActivePlanPage> createState() => _MembershipPageState();
}

class _MembershipPageState extends State<MembershipActivePlanPage> {
  final _con = Get.put(MembershipController());
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget('Membership'),
        backgroundColor: Colors.white,
        centerTitle: true,
        iconTheme: IconThemeData(
          color: primaryColorCode
        ),
      ),
      body: Column(
        children: [
          SizedBox(
            height: 180,
            child: Stack(
              children: [
                Image.asset(Assets.imgMembershipBanner),
                Image.asset(Assets.imgMembershipTrans),
                Positioned(
                    bottom: 20,
                    left: 20,
                    right: 20,
                    child: SubTxtWidget(getMessage(),
                      color: Colors.white,textAlign: TextAlign.center,))
              ],
            ),
          ),
          if(auth.value.membership!=null)
            currentMembership(),
          membership(),
        ],
      ),
    );
  }
  String getMessage(){
    if(auth.value.membership==null){
      return "Start your membership now";
    }
    if(Tools.changeToDate(auth.value.membership!.endDate!).difference(DateTime.now()).inDays<=7) {
      return 'Your plan is about to expire in ${Tools.changeToDate(auth.value.membership!.endDate!).difference(DateTime.now()).inDays} Days';
    }
    if(DateTime.now().isAfter(Tools.changeToDate(auth.value.membership!.endDate!))){
      return 'Your premium plan is expired';
    }
    return "Start your membership now";
  }
  Widget membership(){
    if(auth.value.membership==null) {
      return startMembership();
    }
    if(Tools.changeToDate(auth.value.membership!.endDate!).difference(DateTime.now()).inDays<=7) {
      return renewMembership();
    }
    if(DateTime.now().isAfter(Tools.changeToDate(auth.value.membership!.endDate!))){
      return expireMembership();
    }
    return ButtonPrimaryWidget('Upgrade Membership',
    marginHorizontal: 50,marginVertical: 50,onTap: () {
        Get.to(MembershipPage(data: auth.value,));
    },);
  }
  Widget startMembership(){
    return Container(
      padding: const EdgeInsets.all(15),
      child: Column(
        children: [
          const SizedBox(height: 10,),
          HeaderTxtWidget('Upgrade your plan today'),
          const SizedBox(height: 10,),
          SubTxtWidget('Your have free membership plan'),
          const SizedBox(height: 10,),
          ButtonPrimaryWidget('Upgrade membership',onTap: () {
            Get.to(MembershipPage(data: auth.value,));
          },color: '#F9A114'.toColor(),
          marginVertical: 20,marginHorizontal: 20,),
        ],
      ),
    );
  }
  Widget renewMembership(){
    return Container(
      padding: const EdgeInsets.all(15),
      child: Column(
        children: [
          const SizedBox(height: 10,),
          HeaderTxtWidget('Reminder'),
          const SizedBox(height: 10,),
          SubTxtWidget('Your plan is about to expire in ${Tools.changeToDate(auth.value.membership!.endDate!).difference(DateTime.now()).inDays} Days'),
          const SizedBox(height: 10,),
          ButtonPrimaryWidget('Renew membership',onTap: () {
            Get.to(MembershipPage(data: auth.value,));
          },color: '#F9A114'.toColor(),
            marginVertical: 20,marginHorizontal: 20,),
        ],
      ),
    );
  }
  Widget expireMembership(){
    return Container(
      padding: const EdgeInsets.all(15),
      child: Column(
        children: [
          const SizedBox(height: 10,),
          HeaderTxtWidget('Oops! Plan Expired'),
          const SizedBox(height: 10,),
          SubTxtWidget('your premium plan is expired'),
          const SizedBox(height: 10,),
          ButtonPrimaryWidget('buy now',onTap: () {
            Get.to(MembershipPage(data: auth.value,));
          },color: '#F9A114'.toColor(),
            marginVertical: 20,marginHorizontal: 20,),
        ],
      ),
    );
  }
  Widget currentMembership(){
    return Container(
      margin: const EdgeInsets.all(10),
      alignment: AlignmentDirectional.centerStart,
      decoration: BoxDecoration(
        borderRadius: const BorderRadius.all(Radius.circular(10)),
        color: "#FFF0D9".toColor(),
      ),
      padding: const EdgeInsets.all(15),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
            Expanded(flex: 1,child: SubTxtWidget('Plan Name :'),),
            Expanded(flex: 2,child: HeaderTxtWidget(auth.value.membership!.duration),),
            ],
          ),
          Row(
            children: [
              Expanded(flex: 1,child: SubTxtWidget('Start Date :'),),
              Expanded(flex: 2,child: HeaderTxtWidget(Tools.changeDateFormat(auth.value.membership!.startDate!, globalTimeFormat)),),
            ],
          ),
          Row(
            children: [
              Expanded(flex: 1,child: SubTxtWidget('End Date :'),),
              Expanded(flex: 2,child: HeaderTxtWidget(Tools.changeDateFormat(auth.value.membership!.endDate!, globalTimeFormat)),),
            ],
          ),
          Row(
            children: [
              Expanded(flex: 1,child: SubTxtWidget('Amount :'),),
              Expanded(flex: 2,child: HeaderTxtWidget('\$ ${auth.value.membership!.totalAmount.toStringAsFixed(2)}',)),
            ],
          ),
         ],
      ),
    );
  }
}
