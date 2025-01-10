import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/main.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../repo/setting_repo.dart';
import '../utils/colors.dart';

class OnboardingCardWidget extends StatelessWidget {
  final String image, text1, text2;
  final Function onPressed;
  int index;
   OnboardingCardWidget(
      {super.key,
      required this.image,
      required this.index,
      required this.text1,
      required this.text2,
      required this.onPressed});

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        Positioned(top: 50,left: 0,right: 0,bottom: 200,child: Image.asset(
          image,
          fit: BoxFit.contain,
        ),),
        Positioned(left: 0,right: 0,bottom: 0,child: Container(
          padding: const EdgeInsets.only(top: 20,left: 20,right: 20,bottom: 20),
          decoration: BoxDecoration(
            borderRadius: const BorderRadius.only(topRight: Radius.circular(20),topLeft: Radius.circular(20)),
            color: secoundryColorCode
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              HeaderTxtWidget(text1,color: Colors.white,fontSize: 24,),
              const SizedBox(height: 10,),
              SubTxtWidget(text2,color: Colors.white,fontSize: 14,),
              const SizedBox(height: 10,),
              indicator(),
              const SizedBox(height: 20,),
              ButtonPrimaryWidget('Get Started',txtColor: Colors.white,onTap: ()=>onPressed.call(),),
              Container(
              alignment: AlignmentDirectional.center,
              child: TextButton(onPressed: () {
                Get.toNamed('/login');
              }, child: HeaderTxtWidget("Login",textAlign: TextAlign.center,),),
            )
            ],
          ),
        ),),
        Positioned(top: 40,right: 10,child:
        TextButton(onPressed: () {
          isGuest.value=true;
          Get.offAllNamed('/customer_dashboard');
        }, child: SubTxtWidget("Skip",textAlign: TextAlign.center,),),)
       ],
    );
  }
  Widget indicator(){
    List<Widget>list=[];
    for (int i=1;i<=3;i++){
      list.add(Container(
        width: index==i?25:10,
        height: 10,
        margin: const EdgeInsets.symmetric(horizontal: 3),
        decoration: BoxDecoration(
          borderRadius: const BorderRadius.all(Radius.circular(30)),
          color:index==i?primaryColorCode:Colors.grey
        ),
      ));
    }
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: list,
    );
  }

}
