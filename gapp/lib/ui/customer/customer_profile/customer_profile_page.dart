import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:get/get.dart';
import 'package:googleapis/firestore/v1.dart';
import 'package:groom/Utils/tools.dart';
import 'package:groom/data_models/provider_user_model.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/generated/assets.dart';
import 'package:groom/repo/session_repo.dart';
import 'package:groom/screens/provider_screens/provider_financial_dashboard_screen.dart';
import 'package:groom/ui/customer/customer_dashboard/customer_dashboard_controller.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/custom_switch.dart';
import 'package:groom/widgets/guest_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import 'package:share_plus/share_plus.dart';
import 'package:url_launcher/url_launcher_string.dart';
import '../../../firebase/user_firebase.dart';
import '../../../repo/setting_repo.dart';
import '../../../screens/customer_screens/customer_payment_screen.dart';
import '../../../screens/customer_screens/customer_support_screen.dart';
import '../../../screens/provider_screens/provider_form_screen.dart';
import '../favorite/customer_favorite_service_list_screen.dart';
import '../../../screens/services_screens/provider_booking_service_reservation_screen.dart';
import '../booking/customer_booking_service_reservation_list_screen.dart';
import 'customer_profile_controller.dart';

class CustomerProfilePage extends StatelessWidget {
  CustomerProfilePage({Key? key}) : super(key: key);
  final _con = Get.put(CustomerProfileController());
  UserFirebase userFirebase = UserFirebase();

  LatLng? selectedLocation;
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: primaryColorCode,
      appBar: AppBar(
        leading: const SizedBox(),
        leadingWidth: 10,
        title: HeaderTxtWidget(
          'View Profile',
          color: Colors.white,
        ),
        actions: [
          IconButton(
              onPressed: () {
                _con.shareApp();
              },
              icon: const Icon(Icons.share)),
        ],
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
              height: 220,
            ),
          ),
          Column(
            children: [
              ValueListenableBuilder(
                valueListenable: auth,
                builder: (context, value, child) {
                  return Container(
                    padding: const EdgeInsets.all(20),
                    child: Column(
                      children: [
                        Row(
                          children: [
                            Container(
                              margin:
                                  const EdgeInsets.only(right: 10, bottom: 10),
                              decoration: BoxDecoration(
                                  shape: BoxShape.circle,
                                  border: Border.all(
                                      color: Colors.white, width: 4)),
                              child: ClipRRect(
                                borderRadius: BorderRadius.circular(40),
                                child: isGuest.value
                                    ? Image.asset(
                                        Assets.imgLogo,
                                        width: 63,
                                        height: 63,
                                      )
                                    : NetworkImageWidget(
                                        url: auth.value.photoURL,
                                        height: 63,
                                        width: 63,
                                      ),
                              ),
                            ),
                            Expanded(
                                child: HeaderTxtWidget(
                              isGuest.value ? 'Guest' : auth.value.fullName,
                              color: Colors.white,
                              fontSize: 20,
                            )),
                          ],
                        ),
                        if (!isGuest.value)
                          Row(
                            children: [
                              SvgPicture.asset(Assets.svgPhone),
                              const SizedBox(
                                width: 5,
                              ),
                              HeaderTxtWidget(
                                auth.value.contactNumber,
                                color: Colors.white,
                                fontWeight: FontWeight.w400,
                              ),
                            ],
                          ),
                        const SizedBox(
                          height: 5,
                        ),
                        if (!isGuest.value)
                          Row(
                            children: [
                              SvgPicture.asset(Assets.svgLocation),
                              const SizedBox(
                                width: 5,
                              ),
                              Expanded(
                                child: HeaderTxtWidget(
                                    '${auth.value.defaultAddress}',
                                    color: Colors.white,
                                    fontWeight: FontWeight.w400),
                              )
                            ],
                          ),
                      ],
                    ),
                  );
                },
              ),
              Expanded(
                  child: Container(
                decoration: const BoxDecoration(
                    borderRadius: BorderRadius.only(
                        topLeft: Radius.circular(20),
                        topRight: Radius.circular(20)),
                    color: Colors.white),
                child: isGuest.value
                    ? GuestWidget(
                        message: "Login to view profile",
                        isBackButton: false,
                      )
                    : SingleChildScrollView(
                        physics: const BouncingScrollPhysics(),
                        padding: const EdgeInsets.symmetric(
                            vertical: 20, horizontal: 10),
                        child: Column(
                          children: [
                            InkWell(
                              child: Container(
                                decoration: BoxDecoration(
                                  color: secoundryColorCode,
                                  borderRadius: const BorderRadius.all(
                                      Radius.circular(15)),
                                ),
                                padding: const EdgeInsets.symmetric(
                                    horizontal: 10, vertical: 10),
                                margin:
                                    const EdgeInsets.symmetric(horizontal: 10),
                                child: Row(
                                  mainAxisAlignment:
                                      MainAxisAlignment.spaceBetween,
                                  children: [
                                    HeaderTxtWidget(
                                      isProvider.value
                                          ? 'Switch to Client Portal'
                                          : auth.value.providerUserModel == null
                                              ? 'Become a Groomer'
                                              : 'Switch to Groomer Portal',
                                      color: Colors.white,
                                    ),
                                    SvgPicture.asset(Assets.svgSwitchProfile),
                                  ],
                                ),
                              ),
                              onTap: () async {
                                isProvider.value = !isProvider.value;

                                if (isProvider.value &&
                                    auth.value.providerUserModel == null) {
                                  Get.to(() => ProviderFormPage());
                                } else {
                                  await setIsGroomPartner(isProvider.value);
                                  Get.find<CustomerDashboardController>()
                                      .changePage(0);
                                }
                              },
                            ),
                            Divider(
                              height: 5,
                              color: "#F4F4F5".toColor(),
                            ),
                            ListTile(
                                title: HeaderTxtWidget('Notification'),
                                trailing: Obx(
                                  () => CustomSwitch(
                                    value: _con.isNotification.value,
                                    onChanged: (value) {
                                      _con.isNotification.value = value;
                                      _con.updateNotificationStatus();
                                    },
                                    showText: true,
                                    borderColor: Colors.red,
                                    innerColor: Color(0x363062).withOpacity(1),
                                  ),
                                )),
                            Divider(
                              height: 5,
                              color: "#F4F4F5".toColor(),
                            ),
                            ListTile(
                              title: HeaderTxtWidget('Membership'),
                              trailing: const Icon(
                                Icons.arrow_forward_ios_outlined,
                                size: 18,
                              ),
                              onTap: () {
                                Get.toNamed('/membership_active_plan');
                              },
                            ),
                            Divider(
                              height: 5,
                              color: "#F4F4F5".toColor(),
                            ),
                            ListTile(
                              title: HeaderTxtWidget('Family & Friends'),
                              trailing: const Icon(
                                Icons.arrow_forward_ios_outlined,
                                size: 18,
                              ),
                              onTap: () {
                                Get.toNamed('/invite');
                              },
                            ),
                            Divider(
                              height: 5,
                              color: "#F4F4F5".toColor(),
                            ),
                            /*ListTile(
                        title: HeaderTxtWidget('Gift Cards'),
                        trailing: const Icon(Icons.arrow_forward_ios_outlined,size: 18,),
                        onTap: () {
                          Tools.ShowSuccessMessage("Coming soon");
                        },
                      ),
                      Divider(
                        height: 5,
                        color: "#F4F4F5".toColor(),
                      ),*/
                            ListTile(
                              title: HeaderTxtWidget('Service History'),
                              trailing: const Icon(
                                Icons.arrow_forward_ios_outlined,
                                size: 18,
                              ),
                              onTap: () {
                                if (isProvider.value) {
                                  Get.to(() =>
                                      ProviderBookingServiceReservationScreen(
                                        showAppBar: true,
                                      ));
                                } else {
                                  Get.to(() =>
                                      CustomerBookingServiceReservationListScreen(
                                        showAppBar: true,
                                      ));
                                }
                              },
                            ),
                            Divider(
                              height: 5,
                              color: "#F4F4F5".toColor(),
                            ),
                            ListTile(
                              title: HeaderTxtWidget('Payment'),
                              trailing: const Icon(
                                Icons.arrow_forward_ios_outlined,
                                size: 18,
                              ),
                              onTap: () {
                                if (!isProvider.value) {
                                  Get.to(() => const CustomerPaymentScreen());
                                } else {
                                  Get.to(
                                      () => const ProviderFinancialDashboard());
                                }
                              },
                            ),
                            Divider(
                              height: 5,
                              color: "#F4F4F5".toColor(),
                            ),
                            if (!isProvider.value)
                              ListTile(
                                title: HeaderTxtWidget('Favorite'),
                                trailing: const Icon(
                                  Icons.arrow_forward_ios_outlined,
                                  size: 18,
                                ),
                                onTap: () {
                                  Get.to(() => FavoriteServicesScreen());
                                },
                              ),
                            if (!isProvider.value)
                              Divider(
                                height: 5,
                                color: "#F4F4F5".toColor(),
                              ),
                             ListTile(
                        title: HeaderTxtWidget('Donate to Groom'),
                        trailing: const Icon(Icons.arrow_forward_ios_outlined,size: 18,),
                        onTap: () {
                          launchUrlString('https://www.joingroom.com/donate/');
                        },
                      ),
                      Divider(
                        height: 5,
                        color: "#F4F4F5".toColor(),
                      ),
                            ListTile(
                              title: HeaderTxtWidget('Feedback & Support'),
                              trailing: const Icon(
                                Icons.arrow_forward_ios_outlined,
                                size: 18,
                              ),
                              onTap: () {
                                Get.to(() => const CustomerSupportScreen());
                              },
                            ),
                            Divider(
                              height: 5,
                              color: "#F4F4F5".toColor(),
                            ),
                            ListTile(
                              title: HeaderTxtWidget('Report'),
                              trailing: const Icon(
                                Icons.arrow_forward_ios_outlined,
                                size: 18,
                              ),
                              onTap: () {
                                Get.toNamed('/report');
                              },
                            ),
                            Divider(
                              height: 5,
                              color: "#F4F4F5".toColor(),
                            ),
                            ListTile(
                              title: HeaderTxtWidget('About Groom'),
                              trailing: const Icon(
                                Icons.arrow_forward_ios_outlined,
                                size: 18,
                              ),
                              onTap: () {
                                Get.toNamed('/about');
                              },
                            ),
                            Divider(
                              height: 5,
                              color: "#F4F4F5".toColor(),
                            ),

                            /*  ListTile(
                        title: HeaderTxtWidget('Privacy policy'),
                        trailing: const Icon(Icons.arrow_forward_ios_outlined,size: 18,),
                        onTap: () {
                          Get.to(() => CustomerPrivacyPolicy());
                        },
                      ),
                      Divider(
                        height: 5,
                        color: "#F4F4F5".toColor(),
                      ),*/

                            ButtonPrimaryWidget(
                              'Log out',
                              onTap: () {
                                _con.showLogoutConfirmationDialog(context);
                              },
                            ),
                          ],
                        ),
                      ),
              ))
            ],
          ),
          if (!isGuest.value)
            Positioned(
                top: 0,
                right: 10,
                child: IconButton(
                    onPressed: () {
                      Get.toNamed('/edit_profile');
                    },
                    icon: SvgPicture.asset(Assets.svgEdit)))
        ],
      ),
    );
  }
}
