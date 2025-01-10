import 'package:flutter/material.dart';
import 'package:get/get.dart';
import '../../generated/assets.dart';
import '../../widgets/onboarding_card_widget.dart';
import 'onboard_controller.dart';

class OnboardPage extends StatelessWidget {
  OnboardPage({Key? key}) : super(key: key);
  final _con = Get.put(OnboardController());
  static final PageController pageController = PageController(initialPage: 0);
  final List<Widget> _onBoardingPages = [
    OnboardingCardWidget(
      image: Assets.imgOnboardImg1,
      index: 1,
      text1: 'Welcome Groom',
      text2: 'Skip the frustration! Upload a photo & chat directly with barbers to get the perfect cut on Groom.',
      onPressed: () {
        Get.toNamed('/signup');
        },
    ),
    OnboardingCardWidget(
      image: Assets.imgOnboardImg2,
      index: 2,
      text1: "Looking for barber?",
      text2: "Find the best barbershop around you in seconds, make an appointment, and enjoy the best grooming experience.",
      onPressed: () {
        Get.toNamed('/signup');
        },
    ),
    OnboardingCardWidget(
      image: Assets.imgOnboardImg3,
      index: 3,
      text1: "Everything in your hands",
      text2: "Describe your desired style with precision. Book barbers who match your needs on Groom! Let's start now!",
      onPressed: () {
        Get.toNamed('/signup');
        },
    ),

  ];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: PageView(
        controller: pageController,
        children: _onBoardingPages,
      ),
    );
  }
}
