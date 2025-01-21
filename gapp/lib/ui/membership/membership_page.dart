// import 'package:flutter/material.dart';
// import 'package:get/get.dart';
// import 'package:groom/repo/setting_repo.dart';
// import 'package:groom/utils/colors.dart';
// import 'package:groom/widgets/button_primary_widget.dart';
// import 'package:groom/widgets/header_txt_widget.dart';
// import 'package:groom/widgets/sub_txt_widget.dart';
//
// import '../../../utils/tools.dart';
// import '../../../data_models/user_model.dart';
// import '../../../generated/assets.dart';
// import 'm/membership_model.dart';
// import 'membership_controller.dart';
//
// class MembershipPage extends StatefulWidget {
//   UserModel data;
//    MembershipPage({super.key,required this.data});
//
//   @override
//   State<MembershipPage> createState() => _MembershipPageState();
// }
//
// class _MembershipPageState extends State<MembershipPage> {
//   final _con = Get.put(MembershipController());
//
//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       appBar: AppBar(
//         title: HeaderTxtWidget('Get choose your best plan'),
//         backgroundColor: Colors.white,
//         centerTitle: true,
//         iconTheme: IconThemeData(
//           color: primaryColorCode
//         ),
//       ),
//       body: Obx(() => Column(
//         children: [
//           SizedBox(
//             height: 180,
//             child: Stack(
//               children: [
//                 Image.asset(Assets.imgMembershipBanner),
//                 Image.asset(Assets.imgMembershipTrans),
//                 Positioned(
//                     bottom: 20,
//                     left: 20,
//                     right: 20,
//                     child: SubTxtWidget(getMessage(),
//                       color: Colors.white,textAlign: TextAlign.center,))
//               ],
//             ),
//           ),
//
//           Container(
//             height: 105,
//             margin: const EdgeInsets.symmetric(vertical: 10),
//             child: ListView.builder(itemBuilder: (context, index) {
//               MembershipModel membership=_con.list[index];
//               return Container(
//                 padding: const EdgeInsets.all(5),
//                 margin: const EdgeInsets.symmetric(horizontal: 5),
//                 decoration: BoxDecoration(
//                   color:membership.isSelected?Colors.blue: Colors.grey.shade300,
//                   borderRadius: const BorderRadius.all(Radius.circular(10)),
//                 ),
//                 child: InkWell(
//                   onTap: () {
//                     _con.onSelectItem(index);
//                     setState(() {
//                     });
//                   },
//                   child: Column(
//                     children: [
//                       HeaderTxtWidget(membership.title,color:membership.isSelected? Colors.white:Colors.black,),
//                       Container(
//                         margin: const EdgeInsets.only(top: 5),
//                         color: Colors.white,
//                         padding: const EdgeInsets.all(10),
//                         child: Column(
//                           children: [
//                             SubTxtWidget(membership.duration),
//                             SubTxtWidget('\$${membership.amount}/week'),
//                           ],
//                         ),
//                       )
//                     ],
//                   ),
//                 ),
//               );
//             },itemCount: _con.list.length,shrinkWrap: true,scrollDirection: Axis.horizontal,),
//           ),
//           Expanded(child: ListView(
//             children: [
//               ListTile(
//                 leading: const Icon(Icons.check_circle,color: Colors.green,),
//                 title: HeaderTxtWidget('Send unlimited chat messages',color: Colors.grey,
//                   fontSize: 14,),
//               ),ListTile(
//                 leading: const Icon(Icons.check_circle,color: Colors.green,),
//                 title: HeaderTxtWidget('See all client requests',color: Colors.grey,
//                   fontSize: 14,),
//               ),ListTile(
//                 leading: const Icon(Icons.check_circle,color: Colors.green,),
//                 title: HeaderTxtWidget('Create unlimited service listings ',color: Colors.grey,
//                   fontSize: 14,),
//               ),ListTile(
//                 leading: const Icon(Icons.check_circle,color: Colors.green,),
//                 title: HeaderTxtWidget('Sort and Filter client requests',color: Colors.grey,
//                   fontSize: 14,),
//               ),
//
//             ],
//           )),
//           Container(
//             padding:const EdgeInsets.symmetric(horizontal: 30),
//             child: SubTxtWidget('You will be charged, your subscription will auto-renew for the same price and package length until you cancel via App store settings, and you agree to our Terms.',fontSize: 8,
//               textAlign: TextAlign.center,),
//           ),
//           if(_con.selectedMembership.value!=null)
//           ButtonPrimaryWidget('Continue \$${_con.selectedMembership.value!.totalAmount}',
//             onTap: () {
//              _con.payWithCard(widget.data.membership);
//             },
//             marginHorizontal: 20,marginVertical: 20,),
//         ],
//       ),),
//     );
//   }
//   String getMessage(){
//     if(widget.data.membership==null){
//       return "Start your membership now";
//     }
//     if(Tools.changeToDate(widget.data.membership!.endDate!).difference(DateTime.now()).inDays<=7) {
//       return 'Your plan is about to expire in ${Tools.changeToDate(widget.data.membership!.endDate!).difference(DateTime.now()).inDays} Days';
//     }
//     if(DateTime.now().isAfter(Tools.changeToDate(widget.data.membership!.endDate!))){
//       return 'Your premium plan is expired';
//     }
//     return "Start your membership now";
//   }
//
// }

import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

import '../../../data_models/user_model.dart';
import '../../../generated/assets.dart';
import '../../../repo/setting_repo.dart';
import '../../widgets/button_primary_widget.dart';
import 'm/membership_model.dart';
import 'membership_controller.dart';

class MembershipPage extends StatefulWidget {
  UserModel data;
  bool isUpgrade;
  bool isDowngrade;
  MembershipPage({super.key,required this.data,this.isUpgrade=false,this.isDowngrade=false});

  @override
  State<MembershipPage> createState() => _MembershipPageState();
}

class _MembershipPageState extends State<MembershipPage> {
  final _con = Get.put(MembershipController());
  List<MembershipModel> filteredList=[];
  @override
  void initState() {
    super.initState();
    filteredList =  _con.getFilteredList(
        currentMembershipDuration: widget.data.membership?.duration,
        isUpgrade: widget.isUpgrade,
        isDowngrade: widget.isDowngrade
    );
  }
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget('Get choose your best plan'),
        backgroundColor: Colors.white,
        centerTitle: true,
        iconTheme: IconThemeData(
            color: primaryColorCode
        ),
      ),
      body: Obx(() => Column(
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

          Container(
            height: 105,
            margin: const EdgeInsets.symmetric(vertical: 10),
            child: ListView.builder(itemBuilder: (context, index) {
              MembershipModel membership=filteredList[index];
              return Container(
                padding: const EdgeInsets.all(5),
                margin: const EdgeInsets.symmetric(horizontal: 5),
                decoration: BoxDecoration(
                  color:membership.isSelected?Colors.blue: Colors.grey.shade300,
                  borderRadius: const BorderRadius.all(Radius.circular(10)),
                ),
                child: InkWell(
                  onTap: () {
                    _con.onSelectItem(filteredList, index);
                    setState(() {
                    });
                  },
                  child: Column(
                    children: [
                      HeaderTxtWidget(membership.title,color:membership.isSelected? Colors.white:Colors.black,),
                      Container(
                        margin: const EdgeInsets.only(top: 5),
                        color: Colors.white,
                        padding: const EdgeInsets.all(10),
                        child: Column(
                          children: [
                            SubTxtWidget(membership.duration),
                            SubTxtWidget('\$${membership.amount}/week'),
                          ],
                        ),
                      )
                    ],
                  ),
                ),
              );
            },itemCount: filteredList.length,shrinkWrap: true,scrollDirection: Axis.horizontal,),
          ),
          Expanded(child: ListView(
            children: [
              ListTile(
                leading: const Icon(Icons.check_circle,color: Colors.green,),
                title: HeaderTxtWidget('Send unlimited chat messages',color: Colors.grey,
                  fontSize: 14,),
              ),ListTile(
                leading: const Icon(Icons.check_circle,color: Colors.green,),
                title: HeaderTxtWidget('See all client requests',color: Colors.grey,
                  fontSize: 14,),
              ),ListTile(
                leading: const Icon(Icons.check_circle,color: Colors.green,),
                title: HeaderTxtWidget('Create unlimited service listings ',color: Colors.grey,
                  fontSize: 14,),
              ),ListTile(
                leading: const Icon(Icons.check_circle,color: Colors.green,),
                title: HeaderTxtWidget('Sort and Filter client requests',color: Colors.grey,
                  fontSize: 14,),
              ),

            ],
          )),
          Container(
            padding:const EdgeInsets.symmetric(horizontal: 30),
            child: SubTxtWidget('You will be charged, your subscription will auto-renew for the same price and package length until you cancel via App store settings, and you agree to our Terms.',fontSize: 8,
              textAlign: TextAlign.center,),
          ),
          if(_con.selectedMembership.value!=null)
            ButtonPrimaryWidget('Continue \$${_con.selectedMembership.value!.totalAmount}',
              onTap: () {
                _con.payWithCard(widget.data.membership);
              },
              marginHorizontal: 20,marginVertical: 20,),
        ],
      ),),
    );
  }
  String getMessage(){
    if(widget.data.membership==null){
      return "Start your membership now";
    }
    if(Tools.changeToDate(widget.data.membership!.endDate!).difference(DateTime.now()).inDays<=7) {
      return 'Your plan is about to expire in ${Tools.changeToDate(widget.data.membership!.endDate!).difference(DateTime.now()).inDays} Days';
    }
    if(DateTime.now().isAfter(Tools.changeToDate(widget.data.membership!.endDate!))){
      return 'Your premium plan is expired';
    }
    return "Start your membership now";
  }
}