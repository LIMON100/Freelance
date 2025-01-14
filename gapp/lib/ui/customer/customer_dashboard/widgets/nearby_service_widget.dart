import 'package:cached_network_image/cached_network_image.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/widgets/loading_widget.dart';
import '../../../../data_models/provider_service_model.dart';
import '../../../../generated/assets.dart';
import '../../../../states/provider_service_state.dart';
import '../../../../states/customer_service_state.dart';
import '../../../../utils/colors.dart';
import '../../../../widgets/header_txt_widget.dart';
import '../../../../widgets/sub_txt_widget.dart';
import '../../services/service_details_screen.dart';

// ignore: must_be_immutable
class NearbyServiceWidget extends StatelessWidget {
  ProviderServiceState providerServiceState = Get.put(ProviderServiceState());
  final CustomerServiceState customerServiceState = Get.find<CustomerServiceState>();
  NearbyServiceWidget({super.key});

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        HeaderTxtWidget(
          'Nearby Services',
          fontSize: 18,
        ),
        const SizedBox(
          height: 10,
        ),
        Obx(
          () => _body(),
        ),
        const SizedBox(
          height: 20,
        ),
      ],
    );
  }

  Widget _body() {
    // if (providerServiceState.isServiceLoading.value &&
    //     providerServiceState.allService.isEmpty) {
    //   return LoadingWidget(
    //     type: LoadingType.MyEvent,
    //   );
    // }
    // if (providerServiceState.allService.isEmpty) {
    //   return Row(
    //     children: [
    //       SubTxtWidget(
    //         "No nearby services, ",
    //         fontWeight: FontWeight.w300,
    //       ),
    //       InkWell(
    //         onTap: () {
    //           Get.toNamed('/invite');
    //         },
    //         child: SubTxtWidget(
    //           "invite a pro",
    //           fontWeight: FontWeight.w300,
    //           color: secoundryColorCode,
    //         ),
    //       ),
    //     ],
    //   );
    // }

    if (providerServiceState.isServiceLoading.value &&
        providerServiceState.allService.isEmpty) {
      return LoadingWidget(
        type: LoadingType.MyEvent,
      );
    }
    if (providerServiceState.allService.isEmpty) {
      return Row(
        children: [
          SubTxtWidget(
            "No nearby services, ",
            fontWeight: FontWeight.w300,
          ),
          InkWell(
            onTap: () {
              Get.toNamed('/invite');
            },
            child: SubTxtWidget(
              "invite a pro",
              fontWeight: FontWeight.w300,
              color: secoundryColorCode,
            ),
          ),
        ],
      );
    }
    List<ProviderServiceModel> allService = providerServiceState.allService;
    if (filter.value.isNotEmpty) {
      Map<String, dynamic> map = filter.value;
      if (map.containsKey('min_price')) {
        allService = allService
            .where((element) =>
                double.parse(element.servicePrice.toString()) >
                map['min_price'])
            .toList();
      }
      if (map.containsKey('max_price')) {
        allService = allService
            .where((element) =>
                double.parse(element.servicePrice.toString()) <
                map['max_price'])
            .toList();
      }
      if (map.containsKey('rating')) {
        allService = allService
            .where((element) =>
                double.parse(element.provider!.providerUserModel!.numReviews
                    .toString()) >=
                map['rating'])
            .toList();
      }
    }
    if (allService.isEmpty) {
      return SubTxtWidget("No Services found for your filter");
    }
    return Column(
      children: [
        ListView.builder(
          scrollDirection: Axis.vertical,
          itemCount: allService.length > 5 ? 5 : allService.length,
          shrinkWrap: true,
          primary: false,
          padding: EdgeInsets.zero,
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
                          imageUrl: service.serviceImages?.isNotEmpty == true ? service.serviceImages!.first : '', // Or a placeholder image
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
                        // HeaderTxtWidget(service.serviceName!),
                        HeaderTxtWidget(service.serviceName ?? 'Loading...'),
                        Row(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            SvgPicture.asset(Assets.svgLocationGray),
                            const SizedBox(
                              width: 5,
                            ),
                            Expanded(
                              child:
                              // SubTxtWidget(
                              //   isGuest.value
                              //       ? 'Login to view'
                              //       : '${service.address!.toMiniAddress()} (${service.distance})',
                              //   color: "#8683A1".toColor(),
                              // ),
                              SubTxtWidget(
                                isGuest.value
                                    ? 'Login to view'
                                    : '${service.address?.toMiniAddress() ?? 'Loading address...'} (${service.distance ?? ''})',
                                color: "#8683A1".toColor(),
                              ),
                            )
                          ],
                        ),
                        Row(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          mainAxisAlignment: MainAxisAlignment.spaceBetween,
                          children: [
                            Row(
                              children: [
                                SvgPicture.asset(Assets.svgStar),
                                const SizedBox(
                                  width: 5,
                                ),
                                // SubTxtWidget(
                                //   '${service.provider!.providerUserModel!.overallRating}',
                                //   color: "#8683A1".toColor(),
                                // ),
                              SubTxtWidget(
                                '${service.provider?.providerUserModel?.overallRating ?? 'No ratings yet'}',
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
              // onTap: () {
              //   if (isGuest.value) {
              //     Get.toNamed('/login');
              //   } else {
              //     CustomerServiceState similarServiceState =
              //         Get.put(CustomerServiceState());
              //     similarServiceState.selectedService.value = service;
              //     Get.to(() => ServiceDetailsScreen());
              //   }
              // },
              // In NearbyServiceWidget

              onTap: () {
                if (isGuest.value) {
                  Get.toNamed('/login');
                } else {
                  // Use the already found instance
                  customerServiceState.selectedService.value = service;
                  Get.to(() => ServiceDetailsScreen());
                }
              },
            );
          },
        ),
        if (allService.isNotEmpty)
          Center(
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
              decoration: BoxDecoration(
                borderRadius: const BorderRadius.all(Radius.circular(10)),
                border: Border.all(color: primaryColorCode, width: 2),
              ),
              child: InkWell(
                child: Wrap(
                  children: [
                    HeaderTxtWidget('See More'),
                    const SizedBox(
                      width: 10,
                    ),
                    SvgPicture.asset(Assets.svgArrowRightUp)
                  ],
                ),
                onTap: () {
                  Get.toNamed('/all_service_list');
                },
              ),
            ),
          ),
      ],
    );
  }
}
