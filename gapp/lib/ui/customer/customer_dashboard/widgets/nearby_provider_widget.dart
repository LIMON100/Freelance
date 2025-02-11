import 'package:cached_network_image/cached_network_image.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/ui/customer/services/provider_details_screen.dart';
import 'package:groom/widgets/loading_widget.dart';
import '../../../../data_models/user_model.dart';
import '../../../../generated/assets.dart';
import '../../../../states/provider_service_state.dart';
import '../../../../utils/colors.dart';
import '../../../../widgets/header_txt_widget.dart';
import '../../../../widgets/sub_txt_widget.dart';

class NearbyProviderWidget extends StatelessWidget {
  ProviderServiceState providerServiceState = Get.put(ProviderServiceState());

  NearbyProviderWidget({super.key});

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        HeaderTxtWidget(
          'Nearby Providers',
          fontSize: 18,
        ),
        const SizedBox(
          height: 10,
        ),
        Obx(
          () => _body(),
        ),
      ],
    );
  }

  Widget _body() {
    if (providerServiceState.isProviderLoading.value &&
        providerServiceState.allProviders.isEmpty) {
      return LoadingWidget();
    }
    if (providerServiceState.allProviders.isEmpty) {
      return Row(
        children: [
          SubTxtWidget(
            "No nearby pros, ",
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
      ;
    }
    List<UserModel> allProviders = providerServiceState.allProviders;
    if (filter.value.isNotEmpty) {
      Map<String, dynamic> map = filter.value;
      if (map.containsKey('min_price')) {
        allProviders = allProviders
            .where((element) =>
                double.parse(element.providerUserModel!.basePrice.toString()) >
                map['min_price'])
            .toList();
      }
      if (map.containsKey('max_price')) {
        allProviders = allProviders
            .where((element) =>
                double.parse(element.providerUserModel!.basePrice.toString()) <
                map['max_price'])
            .toList();
      }
      if (map.containsKey('rating')) {
        allProviders = allProviders
            .where((element) =>
                double.parse(
                    element.providerUserModel!.numReviews.toString()) >=
                map['rating'])
            .toList();
      }
      if (map.containsKey('location')) {
        LatLng latLng = map['location'];
        allProviders = allProviders
            .where((element) =>
                radius >=
                (Geolocator.distanceBetween(
                        element.providerUserModel!.location!.latitude,
                        element.providerUserModel!.location!.latitude,
                        latLng.latitude,
                        latLng.longitude) /
                    1000))
            .toList();
      }
    }
    if (allProviders.isEmpty) {
      return SubTxtWidget("No provider found for your filter");
    }
    return Column(
      children: [
        ListView.builder(
          scrollDirection: Axis.vertical,
          itemCount: allProviders.length > 5 ? 5 : allProviders.length,
          shrinkWrap: true,
          primary: false,
          padding: EdgeInsets.zero,
          itemBuilder: (context, index) {
            var data = allProviders[index];
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
                        child: CachedNetworkImage(
                          imageUrl:
                              data.providerUserModel!.providerImages!.first,
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
                        HeaderTxtWidget(data.providerUserModel!.providerType !=
                                "Independent"
                            ? data.providerUserModel!.salonTitle!
                            : data.fullName),
                        Row(
                          children: [
                            SvgPicture.asset(Assets.svgLocationGray),
                            const SizedBox(
                              width: 5,
                            ),
                            SubTxtWidget(
                              isGuest.value
                                  ? 'Login to view'
                                  : '${data.providerUserModel!.addressLine.toMiniAddress(limit: 20)} (${data.distance})',
                              color: "#8683A1".toColor(),
                            ),
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
                                SubTxtWidget(
                                  '${data.providerUserModel!.overallRating.toStringAsFixed(1)}',
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
                              child: HeaderTxtWidget(
                                isGuest.value ? "Login" : 'Provider Details',
                                color: Colors.white,
                                fontSize: 13,
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
                  Get.to(() => ProviderDetailsScreen(
                        provider: data,
                      ));
                }
              },
            );
          },
        ),
        const SizedBox(
          height: 20,
        ),
        if (allProviders.isNotEmpty)
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
                  Get.toNamed('/all_provider_list');
                },
              ),
            ),
          ),
      ],
    );
  }
}
