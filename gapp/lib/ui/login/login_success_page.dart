import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ui/login/login_controller.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../generated/assets.dart';

class LoginSuccessPage extends StatefulWidget {
  const LoginSuccessPage({super.key});

  @override
  State<LoginSuccessPage> createState() =>
      _ScreenState();
}

class _ScreenState extends State<LoginSuccessPage> {
  final _con = Get.put(LoginController());
  @override
  void initState() {
    // TODO: implement initState
    super.initState();
    Future.delayed(const Duration(seconds: 3),() {
      Get.offAllNamed('/customer_dashboard');
    },);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
    body: Container(
      padding: const EdgeInsets.all(50),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.center,
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Image.asset(Assets.imgLoginTic),
          const SizedBox(height: 20,),
          HeaderTxtWidget('Phone Number Verified'),
          const SizedBox(height: 10,),
          SubTxtWidget('You will be redirected to the main page in a few moments',textAlign: TextAlign.center,),
        ],
      ),
    ),
    );
  }
}

