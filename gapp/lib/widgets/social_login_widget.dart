import 'dart:io';
import 'package:flutter/material.dart';
import 'package:groom/ext/hex_color.dart';
import '../generated/assets.dart';
import 'sub_txt_widget.dart';

class SocialLoginWidget extends StatelessWidget {
  Function()? loginWithGoogle;
  Function()? loginWithApple;


  SocialLoginWidget(
      {super.key, this.loginWithGoogle,this.loginWithApple});

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Stack(
          children: [
           Positioned(
            top: 10,
            left: 0,
            right: 0,
            bottom: 10,
            child: Divider(
              height: 50,
              color: Colors.grey.shade300,
            ),
          ),
            Center(
              child: Container(
                width: 40,
                alignment: AlignmentDirectional.center,
                color: Colors.white,
                child: SubTxtWidget("or"),
              ),
            )
          ],
        ),
        const SizedBox(height: 10,),
        Row(
          children: [
            Expanded(child: InkWell(
              onTap: loginWithGoogle,
              child: Container(
                height: 70,
                padding: const EdgeInsets.all(12),
                margin: const EdgeInsets.symmetric(vertical: 5, horizontal: 5),
                decoration: BoxDecoration(
                    borderRadius:  const BorderRadius.all(Radius.circular(30)),
                    color: '#F5F4F8'.toColor(),
                ),
                child: Image.asset(Assets.imgGoogle),
              ),
            )),
            Expanded(child: InkWell(
              onTap: loginWithApple,
              child: Container(
                height: 70,
                padding: const EdgeInsets.all(12),
                margin: const EdgeInsets.symmetric(vertical: 5, horizontal: 5),
                decoration: BoxDecoration(
                    borderRadius:  const BorderRadius.all(Radius.circular(30)),
                    color: '#F5F4F8'.toColor(),
                ),
                child: Image.asset(Assets.imgFacebook),
              ),
            ),)
          ],
        ),
        const SizedBox(height: 10,),
      ],
    );
  }
}
