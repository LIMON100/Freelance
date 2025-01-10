import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/utils/colors.dart';
import '../generated/assets.dart';
import 'header_txt_widget.dart';

class GuestWidget extends StatelessWidget {
  String message;
  bool isBackButton;
  GuestWidget({super.key,required this.message,this.isBackButton=true});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar:isBackButton?AppBar(
        backgroundColor: Colors.white,
        iconTheme: IconThemeData(
            color: primaryColorCode
        ),
      ):null,
      body: Stack(
        children: [
          Column(
            crossAxisAlignment: CrossAxisAlignment.center,
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Image.asset(Assets.imgLogoThumb),
              const SizedBox(height: 50,),
              InkWell(
                onTap: () {
                  Get.toNamed('/login');
                },
                child: Container(
                  margin: const EdgeInsets.symmetric(vertical: 20,horizontal: 20),
                  padding: const EdgeInsets.all(15),
                  decoration: BoxDecoration(
                      borderRadius: const BorderRadius.all(Radius.circular(15)),
                      color: "#263238".toColor()
                  ),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Image.asset(Assets.imgExit),
                      const SizedBox(width: 20,),
                      HeaderTxtWidget('LOG IN',color: Colors.white,)
                    ],
                  ),
                ),
              ),
              InkWell(
                onTap: () {
                  Get.toNamed('/signup');
                },
                child: Container(
                  margin: const EdgeInsets.symmetric(horizontal: 20),
                  padding: const EdgeInsets.all(15),
                  decoration: BoxDecoration(
                      borderRadius: const BorderRadius.all(Radius.circular(15)),
                      border: Border.all(color: "#263238".toColor())
                  ),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      HeaderTxtWidget('Create an account')
                    ],
                  ),
                ),
              ),
            ],
          ),
          if(isBackButton)
          Positioned(bottom: 30,left: 0,right: 0,child: InkWell(
            onTap: () {
              Get.back();
            },
            child: Container(
              margin: const EdgeInsets.symmetric(horizontal: 20),
              padding: const EdgeInsets.all(15),
              decoration: BoxDecoration(
                  borderRadius: const BorderRadius.all(Radius.circular(10)),
                color: "#363062".toColor()
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  HeaderTxtWidget('Back',color: Colors.white,)
                ],
              ),
            ),
          ),)
        ],
      ),
    );
  }

}
