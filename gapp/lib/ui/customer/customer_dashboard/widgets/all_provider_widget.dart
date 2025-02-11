// import 'package:cached_network_image/cached_network_image.dart';
// import 'package:carousel_slider/carousel_slider.dart';
// import 'package:flutter/material.dart';
// import 'package:flutter_svg/svg.dart';
// import 'package:geolocator/geolocator.dart';
// import 'package:get/get.dart';
// import 'package:groom/ext/hex_color.dart';
// import '../../../../data_models/user_model.dart';
// import '../../../../generated/assets.dart';
// import '../../../../repo/setting_repo.dart';
// import '../../../../widgets/loading_widget.dart';
// import '../../services/provider_details_screen.dart';
// import '../../../../states/provider_service_state.dart';
// import '../../../../utils/colors.dart';
// import '../../../../widgets/header_txt_widget.dart';
// import '../../../../widgets/sub_txt_widget.dart';
//
// // ignore: must_be_immutable
// class AllProviderWidget extends StatelessWidget {
//   AllProviderWidget({super.key});
//   CarouselSliderController controller = CarouselSliderController();
//   ProviderServiceState providerServiceState = Get.put(ProviderServiceState());
//
//   @override
//   Widget build(BuildContext context) {
//     return Column(
//       crossAxisAlignment: CrossAxisAlignment.start,
//       children: [
//         HeaderTxtWidget('Featured Providers'),
//         const SizedBox(
//           height: 10,
//         ),
//         Obx(
//           () => _body(),
//         ),
//       ],
//     );
//   }
//
//   Widget _dot(List<UserModel> providers) {
//     List<Widget> list = [];
//     for (int i = 0; i < providers.length; i++) {
//       list.add(AnimatedContainer(
//         duration: const Duration(milliseconds: 500),
//         width: providerServiceState.index == i ? 28 : 8,
//         height: 8,
//         margin: const EdgeInsets.symmetric(vertical: 5, horizontal: 3),
//         decoration: BoxDecoration(
//             borderRadius: const BorderRadius.all(Radius.circular(20)),
//             color: providerServiceState.index == i
//                 ? primaryColorCode
//                 : Colors.grey),
//       ));
//     }
//     return Row(
//       mainAxisAlignment: MainAxisAlignment.end,
//       children: list,
//     );
//   }
//
//   Widget _body() {
//     if (providerServiceState.isProviderLoading.value &&
//         providerServiceState.allProviders.isEmpty) {
//       return LoadingWidget();
//     }
//     if (providerServiceState.allProviders.isEmpty) {
//       return Row(
//         children: [
//           SubTxtWidget(
//             "No Featured pros, ",
//             fontWeight: FontWeight.w300,
//           ),
//           InkWell(
//             onTap: () {
//               Get.toNamed('/invite');
//             },
//             child: SubTxtWidget(
//               "invite a pro",
//               fontWeight: FontWeight.w300,
//               color: secoundryColorCode,
//             ),
//           ),
//         ],
//       );
//     }
//     List<UserModel> allProviders = providerServiceState.allProviders;
//     if (filter.value.isNotEmpty) {
//       Map<String, dynamic> map = filter.value;
//       if (map.containsKey('min_price')) {
//         allProviders = allProviders
//             .where((element) =>
//                 double.parse(element.providerUserModel!.basePrice.toString()) >
//                 map['min_price'])
//             .toList();
//       }
//       if (map.containsKey('max_price')) {
//         allProviders = allProviders
//             .where((element) =>
//                 double.parse(element.providerUserModel!.basePrice.toString()) <
//                 map['max_price'])
//             .toList();
//       }
//       if (map.containsKey('rating')) {
//         allProviders = allProviders
//             .where((element) =>
//                 double.parse(
//                     element.providerUserModel!.numReviews.toString()) >=
//                 map['rating'])
//             .toList();
//       }
//       if (map.containsKey('location')) {
//         var latLng = map['location'];
//         allProviders = allProviders
//             .where((element) =>
//                 radius >=
//                 (Geolocator.distanceBetween(
//                         element.providerUserModel!.location!.latitude,
//                         element.providerUserModel!.location!.latitude,
//                         latLng.latitude,
//                         latLng.longitude) /
//                     1000))
//             .toList();
//       }
//     }
//     if (allProviders.isEmpty) {
//       return SubTxtWidget("No provider found for your filter");
//     }
//     return Column(
//       children: [
//         CarouselSlider(
//           carouselController: controller,
//           options: CarouselOptions(
//             viewportFraction: 1,
//             aspectRatio: 1.15,
//             autoPlay: false,
//             onPageChanged: (index, reason) {
//               providerServiceState.index.value = index;
//             },
//           ),
//           items: allProviders.map((i) {
//             return Builder(
//               builder: (BuildContext context) {
//                 return InkWell(
//                   child: Container(
//                       margin: const EdgeInsets.symmetric(horizontal: 5),
//                       width: MediaQuery.of(context).size.width,
//                       color: Colors.white,
//                       child:
//                       Column(
//                         crossAxisAlignment: CrossAxisAlignment.start,
//                         children: [
//                           SizedBox(
//                             height: 220,
//                             child: ClipRRect(
//                               borderRadius:
//                                   const BorderRadius.all(Radius.circular(10)),
//                               child: Stack(
//                                 children: [
//                                   CachedNetworkImage(
//                                     height: 220,
//                                     width: double.infinity,
//                                     fit: BoxFit.fill,
//                                     imageUrl: i.providerUserModel!
//                                         .providerImages!.first,
//                                   ),
//                                   Positioned(
//                                     bottom: 0,
//                                     right: 0,
//                                     child: Container(
//                                       width: isGuest.value ? 90 : 140,
//                                       padding: const EdgeInsets.symmetric(
//                                           horizontal: 10, vertical: 5),
//                                       decoration: BoxDecoration(
//                                           borderRadius: const BorderRadius.only(
//                                               topLeft: Radius.circular(15),
//                                               bottomRight: Radius.circular(15)),
//                                           color: primaryColorCode),
//                                       child: HeaderTxtWidget(
//                                         fontSize:
//                                             13.0, // Font size in logical pixels
//                                         fontWeight: FontWeight.w700,
//                                         isGuest.value
//                                             ? 'Login'
//                                             : 'Provider Details',
//                                         color: Colors.white,
//                                       ),
//                                     ),
//                                   )
//                                 ],
//                               ),
//                             ),
//                           ),
//                           Container(
//                             padding: const EdgeInsets.symmetric(
//                                 horizontal: 10, vertical: 10),
//                             child: Column(
//                               crossAxisAlignment: CrossAxisAlignment.start,
//                               children: [
//                                 HeaderTxtWidget(
//                                     i.providerUserModel!.providerType !=
//                                             "Independent"
//                                         ? i.providerUserModel!.salonTitle!
//                                         : i.fullName),
//                                 Row(
//                                   children: [
//                                     SvgPicture.asset(Assets.svgLocationGray),
//                                     const SizedBox(
//                                       width: 5,
//                                     ),
//                                     Expanded(
//                                       child: SubTxtWidget(
//                                         '${i.providerUserModel!.addressLine.toMiniAddress(limit: 20)} (${i.distance})',
//                                         color: "#8683A1".toColor(),
//                                       ),
//                                     )
//                                   ],
//                                 ),
//                                 Row(
//                                   children: [
//                                     SvgPicture.asset(Assets.svgStar),
//                                     const SizedBox(
//                                       width: 5,
//                                     ),
//                                     SubTxtWidget(
//                                       '${i.providerUserModel!.overallRating.toStringAsFixed(1)}',
//                                       color: "#8683A1".toColor(),
//                                     ),
//                                   ],
//                                 ),
//                               ],
//                             ),
//                           )
//                         ],
//                       ),
//                   ),
//                   onTap: () {
//                     if (isGuest.value) {
//                       Get.toNamed('/login');
//                     } else {
//                       Get.to(() => ProviderDetailsScreen(
//                             provider: i,
//                           ));
//                     }
//                   },
//                 );
//               },
//             );
//           }).toList(),
//         ),
//         _dot(providerServiceState.allProviders)
//       ],
//     );
//   }
// }


import 'package:cached_network_image/cached_network_image.dart';
import 'package:carousel_slider/carousel_slider.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import '../../../../data_models/user_model.dart';
import '../../../../generated/assets.dart';
import '../../../../repo/setting_repo.dart';
import '../../../../widgets/loading_widget.dart';
import '../../services/provider_details_screen.dart';
import '../../../../states/provider_service_state.dart';
import '../../../../utils/colors.dart';
import '../../../../widgets/header_txt_widget.dart';
import '../../../../widgets/sub_txt_widget.dart';

// ignore: must_be_immutable
class AllProviderWidget extends StatelessWidget {
  AllProviderWidget({super.key});
  CarouselSliderController controller = CarouselSliderController();
  ProviderServiceState providerServiceState = Get.put(ProviderServiceState());

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        HeaderTxtWidget('Featured Providers'),
        const SizedBox(
          height: 10,
        ),
        Obx(
              () => _body(),
        ),
      ],
    );
  }

  Widget _dot(List<UserModel> providers) {
    List<Widget> list = [];
    for (int i = 0; i < providers.length; i++) {
      list.add(AnimatedContainer(
        duration: const Duration(milliseconds: 500),
        width: providerServiceState.index == i ? 28 : 8,
        height: 8,
        margin: const EdgeInsets.symmetric(vertical: 5, horizontal: 3),
        decoration: BoxDecoration(
            borderRadius: const BorderRadius.all(Radius.circular(20)),
            color: providerServiceState.index == i
                ? primaryColorCode
                : Colors.grey),
      ));
    }
    return Row(
      mainAxisAlignment: MainAxisAlignment.end,
      children: list,
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
            "No Featured pros, ",
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
        var latLng = map['location'];
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
        CarouselSlider(
          carouselController: controller,
          options: CarouselOptions(
            viewportFraction: 1,
            aspectRatio: 1.15,
            autoPlay: false,
            onPageChanged: (index, reason) {
              providerServiceState.index.value = index;
            },
          ),
          items: allProviders.map((i) {
            return Builder(
              builder: (BuildContext context) {
                return InkWell(
                  child: Container(
                    margin: const EdgeInsets.symmetric(horizontal: 5),
                    width: MediaQuery.of(context).size.width,
                    color: Colors.white,
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        SizedBox(
                          height: 220,
                          child: ClipRRect(
                            borderRadius: const BorderRadius.all(Radius.circular(10)),
                            child: Stack(
                              children: [
                                CachedNetworkImage(
                                  height: 220,
                                  width: double.infinity,
                                  fit: BoxFit.fill,
                                  imageUrl: i.providerUserModel!.providerImages!.first,
                                ),
                                Positioned(
                                  bottom: 0,
                                  right: 0,
                                  child: Container(
                                    width: isGuest.value ? 90 : 140,
                                    padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
                                    decoration: BoxDecoration(
                                      borderRadius: const BorderRadius.only(
                                        topLeft: Radius.circular(15),
                                        bottomRight: Radius.circular(15),
                                      ),
                                      color: primaryColorCode,
                                    ),
                                    child: HeaderTxtWidget(
                                      fontSize: 13.0, // Font size in logical pixels
                                      fontWeight: FontWeight.w700,
                                      isGuest.value ? 'Login' : 'Provider Details',
                                      color: Colors.white,
                                    ),
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ),
                        Expanded( // Wrap the Column with Expanded
                          child: Container(
                            padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 10),
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                HeaderTxtWidget(
                                  i.providerUserModel!.providerType != "Independent"
                                      ? i.providerUserModel!.salonTitle!
                                      : i.fullName,
                                ),
                                Row(
                                  children: [
                                    SvgPicture.asset(Assets.svgLocationGray),
                                    const SizedBox(width: 5),
                                    Expanded(
                                      child: SubTxtWidget(
                                        '${i.providerUserModel!.addressLine.toMiniAddress(limit: 20)} (${i.distance})',
                                        color: "#8683A1".toColor(),
                                      ),
                                    ),
                                  ],
                                ),
                                Row(
                                  children: [
                                    SvgPicture.asset(Assets.svgStar),
                                    const SizedBox(width: 5),
                                    SubTxtWidget(
                                      '${i.providerUserModel!.overallRating.toStringAsFixed(1)}',
                                      color: "#8683A1".toColor(),
                                    ),
                                  ],
                                ),
                              ],
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                  onTap: () {
                    if (isGuest.value) {
                      Get.toNamed('/login');
                    } else {
                      Get.to(() => ProviderDetailsScreen(provider: i));
                    }
                  },
                );
              },
            );
          }).toList(),
        ),
        _dot(providerServiceState.allProviders),
      ],
    );
  }
}
