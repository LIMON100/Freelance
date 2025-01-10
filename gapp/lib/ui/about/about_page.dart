import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

import '../../generated/assets.dart';
import '../../screens/term_services_screen.dart';
import 'about_controller.dart';

class AboutPage extends StatelessWidget {
  AboutPage({super.key});

  final _con = Get.put(AboutController());

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: primaryColorCode,
      appBar: AppBar(
        title: HeaderTxtWidget(
          'About',
          color: Colors.white,
        ),
      ),
      body: Stack(
        children: [
          Positioned(
            top: 0,
            left: 0,
            right: 0,
            child: Image.asset(
              Assets.imgProfileBg,
              fit: BoxFit.cover,
            ),
          ),
          Positioned(
            top: -40,
            left: 0,
            right: 0,
            child: Image.asset(
              Assets.imgLogo3,
              fit: BoxFit.contain,
              height: 350,
            ),
          ),
          Positioned(bottom: 0,right: 0,left: 0,child: Container(
            padding: const EdgeInsets.all(20),
            decoration: const BoxDecoration(
              borderRadius: BorderRadius.only(topRight: Radius.circular(20),topLeft: Radius.circular(20)),
              color: Colors.white
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                HeaderTxtWidget('About Groom'),
                SubTxtWidget('Haircut YOUR way. Describe your style, chat with barbers & see reviews. Find your perfect match on Groom!'),
                ListTile(
                  contentPadding: const EdgeInsets.symmetric(vertical: 10),
                  title: HeaderTxtWidget('Terms & Conditions'),
                  trailing: const Icon(Icons.arrow_forward_ios_outlined,size: 18,),
                  onTap: () {
                    Get.to(() => const CustomerTerms());
                  },
                ),
                SubTxtWidget('Send app feedback'),
                ListTile(
                  contentPadding: const EdgeInsets.symmetric(vertical: 10),
                  title: HeaderTxtWidget('Let’s rate this App'),
                  trailing: const Icon(Icons.arrow_forward_ios_outlined,size: 18,),
                  onTap: () {
                    _con.review(context);
                  },
                ),
                ButtonPrimaryWidget('Back',onTap: () {
                  Get.back();
                },marginVertical: 20,)
              ],
            ),
          ),),
        ],
      ),
    );
  }
}
