import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ui/signup/widgets/complete_profile_widget.dart';
import 'package:groom/ui/signup/widgets/mobile_widget.dart';
import 'package:groom/ui/signup/widgets/otp_widget.dart';

import 'signup_controller.dart';

class SignupPage extends StatelessWidget {
   SignupPage({Key? key}) : super(key: key);
  final _con = Get.put(SignupController());

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: PageView(
        controller: _con.pageController,
        physics: const NeverScrollableScrollPhysics(),
        children:[
          MobileWidget(),
          OtpWidget(),
          CompleteProfileWidget(),
        ],
      ),
    );
  }
}
