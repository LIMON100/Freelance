import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:get/get.dart';
import 'package:groom/data_models/provider_service_model.dart';
import 'package:groom/data_models/user_model.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/firebase/provider_service_firebase.dart';
import 'package:groom/firebase/user_firebase.dart';
import 'package:cached_network_image/cached_network_image.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

import '../../../generated/assets.dart';
import '../../../states/customer_service_state.dart';
import '../../../widgets/loading_widget.dart';
import '../services/provider_details_screen.dart';
import '../services/service_details_screen.dart';

class FavoriteServicesScreen extends StatefulWidget {
  @override
  _FavoriteServicesScreenState createState() => _FavoriteServicesScreenState();
}

class _FavoriteServicesScreenState extends State<FavoriteServicesScreen> {
  final UserFirebase userFirebase = UserFirebase();
  final ProviderServiceFirebase providerServiceFirebase =
      ProviderServiceFirebase();
  RxList<ProviderServiceModel> serviceList = RxList([]);
  RxList<UserModel> providerList = RxList([]);
  RxString type = RxString("Service");
  RxBool loadingService = false.obs;
  RxBool loadingProvider = false.obs;
  @override
  void initState() {
    super.initState();
    _getFavoriteServices().then(
      (value) {
        loadingService.value = false;
        serviceList.value = value;
      },
    );
    _getFavoriteProviders().then(
      (value) {
        loadingProvider.value = false;
        providerList.value = value;
      },
    );
  }

  Future<List<ProviderServiceModel>> _getFavoriteServices() async {
    loadingService.value = true;
    List<String> favoriteServiceIds =
        await userFirebase.getFavoriteServiceIds1(auth.value.uid);
    return await providerServiceFirebase.getServicesByIds(favoriteServiceIds);
  }

  Future<List<UserModel>> _getFavoriteProviders() async {
    loadingProvider.value = true;
    List<String> favoriteServiceIds =
        await userFirebase.getFavoriteProvidersIds(auth.value.uid);
    return await providerServiceFirebase.getProviderByIds(favoriteServiceIds);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget(
          'Favorite',
          color: Colors.white,
        ),
      ),
      body: Obx(
        () => Container(
          padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 10),
          child: Column(
            children: [
              Row(
                children: [
                  ActionChip(
                      label: SubTxtWidget(
                        'Service',
                        color: type.value == "Service"
                            ? Colors.white
                            : Colors.grey,
                      ),
                      onPressed: () {
                        type.value = "Service";
                      },
                      color: WidgetStatePropertyAll(type.value == "Service"
                          ? primaryColorCode
                          : Colors.white)),
                  const SizedBox(
                    width: 10,
                  ),
                  ActionChip(
                    label: SubTxtWidget('Provider',
                        color: type.value == "Provider"
                            ? Colors.white
                            : Colors.grey),
                    onPressed: () {
                      type.value = "Provider";
                    },
                    color: WidgetStatePropertyAll(type.value == "Provider"
                        ? primaryColorCode
                        : Colors.white),
                  ),
                ],
              ),
              Visibility(
                visible: type.value == "Service",
                child: Expanded(child: _favoriteService()),
              ),
              Visibility(
                visible: type.value == "Provider",
                child: Expanded(child: _favoriteProvider()),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _favoriteService() {
    if (loadingService.value) {
      return LoadingWidget();
    }
    if (serviceList.isEmpty) {
      return SubTxtWidget('No Favorite Service');
    }
    return ListView.builder(
      scrollDirection: Axis.vertical,
      itemCount: serviceList.length,
      shrinkWrap: true,
      primary: false,
      padding: EdgeInsets.zero,
      itemBuilder: (context, index) {
        var service = serviceList[index];
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
                      imageUrl: service.serviceImages!.first,
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
                            service.address!.toMiniAddress(),
                            color: "#8683A1".toColor(),
                          ),
                        )
                      ],
                    ),
                    Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
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
                                'Book Now',
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
            CustomerServiceState similarServiceState =
                Get.put(CustomerServiceState());
            similarServiceState.selectedService.value = service;
            Get.to(() => ServiceDetailsScreen());
          },
        );
      },
    );
  }

  Widget _favoriteProvider() {
    if (loadingProvider.value) {
      return LoadingWidget();
    }
    if (providerList.isEmpty) {
      return SubTxtWidget("No  favorite provider found!");
    }
    return ListView.builder(
      scrollDirection: Axis.vertical,
      itemCount: providerList.length,
      shrinkWrap: true,
      primary: false,
      padding: EdgeInsets.zero,
      itemBuilder: (context, index) {
        var data = providerList[index];
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
                      imageUrl: data.providerUserModel!.providerImages!.first,
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
                    HeaderTxtWidget(
                        data.providerUserModel!.providerType != "Independent"
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
                              : data.providerUserModel!.addressLine
                                  .toMiniAddress(limit: 20),
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
            Get.to(() => ProviderDetailsScreen(
                  provider: data,
                ));
          },
        );
      },
    );
  }
}
