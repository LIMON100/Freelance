import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:get/get.dart';
import 'package:groom/repo/setting_repo.dart';
import '../../../generated/assets.dart';
import '../../../screens/provider_screens/provider_form_screen.dart';
import '../../../services/route_generator.dart';
import '../../../utils/colors.dart';
import 'customer_dashboard_controller.dart';

class CustomerDashboardPage extends StatelessWidget {
   CustomerDashboardPage({super.key});
  final _con = Get.put(CustomerDashboardController());
   final GlobalKey<NavigatorState>? nestedKey=Get.nestedKey(1);
  @override
  Widget build(BuildContext context) {
    return ValueListenableBuilder(valueListenable: isProvider, builder: (context, value, child) {
      return Scaffold(
        resizeToAvoidBottomInset: false,
        body: Navigator(
          key:nestedKey,
          initialRoute: value?'provider_home':'home',
          onGenerateRoute: onGenerateRoute,
        ),
        bottomNavigationBar: Obx(() => BottomNavigationBar(
          items:  <BottomNavigationBarItem>[
            BottomNavigationBarItem(
              activeIcon: SvgPicture.asset(Assets.svgHomeSelected),
              icon: SvgPicture.asset(Assets.svgHome),
              label: "home",
            ),
            BottomNavigationBarItem(
              activeIcon: SvgPicture.asset(Assets.svgBookingSelected),
              icon: SvgPicture.asset(Assets.svgBooking),
              label: "Booking",
            ),
              BottomNavigationBarItem(
                activeIcon: value?SvgPicture.asset(Assets.svgCalSelected,width: 22,):SvgPicture.asset(Assets.svgSearchSelected),
                icon:value?SvgPicture.asset(Assets.svgCalHome,width: 22,):SvgPicture.asset(Assets.svgSearch),
                label: value?"Calendar":"Search",
              ),
            BottomNavigationBarItem(
              activeIcon: SvgPicture.asset(Assets.svgProfileSelected),
              icon: SvgPicture.asset(Assets.svgProfile),
              label: "Account",
            ),
          ],
          currentIndex: _con.currentIndex.value,
          onTap: _con.changePage,
          backgroundColor: Colors.white,
          selectedItemColor: primaryColorCode,
          iconSize: 24,
          type: BottomNavigationBarType.fixed,
          showUnselectedLabels: true,
          showSelectedLabels: true,
          selectedLabelStyle: const TextStyle(
            fontSize: 12,
            fontWeight: FontWeight.w400,
          ),
        )),
      );
    },);
  }
}
