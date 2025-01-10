import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/utils/colors.dart';

import '../../generated/assets.dart';
import 'splash_controller.dart';

class SplashPage extends StatelessWidget {
  SplashPage({Key? key}) : super(key: key);
  final _con = Get.put(SplashController());

  @override
  Widget build(BuildContext context) {
    return Container(
      color: primaryColorCode,
      alignment: AlignmentDirectional.center,
      child: Image.asset(Assets.imgLogo),
    );
  }
}
