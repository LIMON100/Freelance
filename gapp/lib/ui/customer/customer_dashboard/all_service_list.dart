import 'dart:convert';
import 'package:cached_network_image/cached_network_image.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_database/ui/firebase_animated_list.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/widgets/loading_widget.dart';
import '../../../constant/global_configuration.dart';
import '../../../data_models/provider_service_model.dart';
import '../../../data_models/user_model.dart';
import '../../../firebase/provider_service_firebase.dart';
import '../../../firebase/provider_user_firebase.dart';
import '../../../generated/assets.dart';
import '../../../repo/setting_repo.dart';
import '../../../states/customer_service_state.dart';
import '../../../utils/colors.dart';
import '../../../widgets/header_txt_widget.dart';
import '../../../widgets/network_image_widget.dart';
import '../../../widgets/sub_txt_widget.dart';
import '../services/provider_details_screen.dart';
import '../services/service_details_screen.dart';

class AllServiceList extends StatefulWidget {
  const AllServiceList({super.key});

  @override
  State<AllServiceList> createState() => _ListState();
}

class _ListState extends State<AllServiceList> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        title: HeaderTxtWidget(
          "All Service List",
          color: primaryColorCode,
        ),
        iconTheme: IconThemeData(color: primaryColorCode),
      ),
      body: FutureBuilder(
        future: ProviderServiceFirebase().getAllServices(),
        builder: (context, snapshot) {
          if (snapshot.connectionState == ConnectionState.waiting) {
            return LoadingWidget();
          } else if (snapshot.hasError) {
            return const Text("No Providers");
          } else if (snapshot.hasData) {
            var allService = snapshot.data as List<ProviderServiceModel>;
            return ListView.builder(
              scrollDirection: Axis.vertical,
              itemCount: allService.length>5?5:allService.length,
              shrinkWrap: true,
              primary: false,
              padding: const EdgeInsets.symmetric(horizontal: 10),
              itemBuilder: (context, index) {
                var service = allService[index];
                return InkWell(
                  child: Container(
                    margin: const EdgeInsets.only(top: 15),
                    child: Row(
                      children: [
                        ClipRRect(
                          borderRadius: BorderRadius.circular(9),
                          child: SizedBox(
                            width: 100,
                            height: 100,
                            child:
                            // CachedNetworkImage(
                            //   imageUrl: service.serviceImages!.first,
                            //   fit: BoxFit.fill,
                            // ),
                            CachedNetworkImage(
                              imageUrl: service.serviceImages?.isNotEmpty == true ? service.serviceImages!.first : '',
                              fit: BoxFit.fill,
                            ),
                          ),
                        ),
                        const SizedBox(
                          width: 10,
                        ),
                        Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                HeaderTxtWidget(service.serviceName!),
                                Row(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    SvgPicture.asset(Assets.svgLocationGray),
                                    const SizedBox(
                                      width: 5,
                                    ),
                                    Expanded(
                                      child: SubTxtWidget(
                                        isGuest.value
                                            ? 'Login to view'
                                            : '${service.address!.toMiniAddress()} (${service.distance})',
                                        color: "#8683A1".toColor(),
                                      ),
                                    )
                                  ],
                                ),
                                Row(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  mainAxisAlignment:
                                  MainAxisAlignment.spaceBetween,
                                  children: [
                                    Row(
                                      children: [
                                        SvgPicture.asset(Assets.svgStar),
                                        const SizedBox(
                                          width: 5,
                                        ),
                                        SubTxtWidget(
                                          '${service.provider!.providerUserModel!.overallRating}',
                                          color: "#8683A1".toColor(),
                                        ),
                                      ],
                                    ),
                                    Container(
                                      margin: const EdgeInsets.only(top: 20),
                                      padding: const EdgeInsets.symmetric(
                                          horizontal: 10, vertical: 8),
                                      decoration: BoxDecoration(
                                          borderRadius: const BorderRadius.only(
                                              topLeft: Radius.circular(15),
                                              bottomRight: Radius.circular(15)),
                                          color: primaryColorCode),
                                      child: Row(
                                        children: [
                                          HeaderTxtWidget(
                                            isGuest.value ? 'Login' : 'Book Now',
                                            color: Colors.white,
                                            fontSize: 11,
                                          ),
                                          const SizedBox(
                                            width: 10,
                                          ),
                                          SvgPicture.asset(Assets.svgCalendarMark)
                                        ],
                                      ),
                                    )
                                  ],
                                ),
                              ],
                            ))
                      ],
                    ),
                  ),
                  onTap: () {
                    if (isGuest.value) {
                      Get.toNamed('/login');
                    } else {
                      CustomerServiceState similarServiceState =
                      Get.put(CustomerServiceState());
                      similarServiceState.selectedService.value = service;
                      Get.to(() =>  ServiceDetailsScreen());
                    }
                  },
                );
              },
            );
          } else {
            return const Text("no data found");
          }
        },
      ),
    );
  }

}
