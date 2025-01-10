import 'package:cached_network_image/cached_network_image.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/widgets/loading_widget.dart';
import '../../../../data_models/provider_service_model.dart';
import '../../../../data_models/user_model.dart';
import '../../../../firebase/provider_service_firebase.dart';
import '../../../../firebase/user_firebase.dart';
import '../../../../generated/assets.dart';
import '../../../../states/provider_service_state.dart';
import '../../search/customer_service_search_screen.dart';
import '../../../../states/customer_service_state.dart';
import '../../services/service_details_screen.dart';
import '../../../../utils/colors.dart';
import '../../../../widgets/header_txt_widget.dart';
import '../../../../widgets/sub_txt_widget.dart';

class AllServiceWidget extends StatefulWidget {
  AllServiceWidget({super.key});

  @override
  State<AllServiceWidget> createState() => _AllServiceWidgetState();
}

class _AllServiceWidgetState extends State<AllServiceWidget> {
  ProviderServiceState providerServiceState = Get.put(ProviderServiceState());
  UserFirebase userFirebase = UserFirebase();
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            HeaderTxtWidget(
              'Services for you',
              fontSize: 18,
            ),
            if (providerServiceState.allService.isNotEmpty)
              TextButton(
                  onPressed: () {
                    Get.to(() => SearchScreen(
                          showBackButton: true,
                        ));
                  },
                  child: SubTxtWidget(
                    'See more',
                    color: primaryColorCode,
                  ))
          ],
        ),
        Obx(
          () => _body(),
        ),
      ],
    );
  }

  Widget _body() {
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
            "Finding your services, ",
            fontWeight: FontWeight.w300,
          ),
          InkWell(
            onTap: (){
              Get.to(() => SearchScreen(
                showBackButton: true,
              ));
            },
            child: SubTxtWidget(
              "search all",
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
      if (map.containsKey('location')) {
        var latLng = map['location'];
        allService = allService
            .where((element) =>
                radius >=
                (Geolocator.distanceBetween(
                        element.location!.latitude,
                        element.location!.latitude,
                        latLng.latitude,
                        latLng.longitude) /
                    1000))
            .toList();
      }
    }
    if (allService.isEmpty) {
      return SubTxtWidget("No Services found for your filter");
    }
    return SizedBox(
      height: 150,
      child: ListView.builder(
        primary: true,
        scrollDirection: Axis.horizontal,
        itemCount: providerServiceState.allService.length,
        itemBuilder: (context, index) {
          var service = providerServiceState.allService[index];
          return ServiceCard(
            service: service,
            userModelService: service.provider!,
            userFirebase: userFirebase,
          );
        },
        padding: EdgeInsets.zero,
      ),
    );
  }
}

class ServiceCard extends StatelessWidget {
  final ProviderServiceModel service;
  final UserModel userModelService;
  final UserFirebase userFirebase;

  const ServiceCard({
    Key? key,
    required this.service,
    required this.userModelService,
    required this.userFirebase,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final isFavorite = false.obs;
    void _loadFavoriteStatus() async {
      if (isGuest.value) return;
      bool favoriteStatus = await userFirebase.checkServiceFavorite(
          FirebaseAuth.instance.currentUser!.uid, service.serviceId);
      isFavorite.value = favoriteStatus;
    }

    void _toggleFavorite() async {
      final userId = FirebaseAuth.instance.currentUser!.uid;
      if (isFavorite.value) {
        // Remove from favorites
        bool success = await userFirebase.removeServiceToFavorites(
            userId, service.serviceId);
        if (success) {
          isFavorite.value = false;
        }
      } else {
        // Add to favorites
        await userFirebase.addServiceToFavorites(userId, service.serviceId);
        isFavorite.value = true;
      }
    }

    _loadFavoriteStatus();
    return SizedBox(
      width: 270,
      child: Card(
        child: Padding(
          padding: const EdgeInsets.all(8),
          child: InkWell(
            child: Row(
              children: [
                SizedBox(
                    width: 120,
                    height: 140,
                    child: ClipRRect(
                      borderRadius: const BorderRadius.all(Radius.circular(10)),
                      child: Stack(
                        children: [
                          CachedNetworkImage(
                            imageUrl: service.serviceImages!.first,
                            fit: BoxFit.fill,
                            height: 140,
                            width: 134,
                          ),
                          Padding(
                            padding: const EdgeInsets.all(4.0),
                            child: Obx(() => GestureDetector(
                                  onTap: _toggleFavorite,
                                  child: CircleAvatar(
                                    radius: 16,
                                    child: Icon(
                                      isFavorite.value
                                          ? Icons.favorite
                                          : Icons.favorite_border_outlined,
                                      color: Colors.red,
                                    ),
                                  ),
                                )),
                          ),
                          Positioned(
                            bottom: 5,
                            left: 5,
                            child: Container(
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 10, vertical: 5),
                              decoration: BoxDecoration(
                                  color: "#234F68".toColor(),
                                  borderRadius: const BorderRadius.all(
                                      Radius.circular(10))),
                              child: SubTxtWidget(
                                service.serviceType,
                                color: Colors.white,
                                fontSize: 8,
                              ),
                            ),
                          )
                        ],
                      ),
                    )),
                const SizedBox(
                  width: 10,
                ),
                Expanded(
                    child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    HeaderTxtWidget(
                      service.serviceName!,
                      fontSize: 14,
                      maxLines: 2,
                    ),
                    Row(
                      children: [
                        SvgPicture.asset(Assets.svgStar),
                        const SizedBox(
                          width: 5,
                        ),
                        SubTxtWidget(
                          '${service.provider!.providerUserModel!.overallRating}',
                          color: "#8683A1".toColor(),
                          fontSize: 12,
                        ),
                      ],
                    ),
                    Row(
                      children: [
                        SvgPicture.asset(Assets.svgLocationGray),
                        const SizedBox(
                          width: 5,
                        ),
                        Expanded(
                          child: SubTxtWidget(
                            '${service.address!.toMiniAddress(limit: 20)} (${service.distance})',
                            color: "#8683A1".toColor(),
                            maxLines: 2,
                            overflow: TextOverflow.ellipsis,
                            fontSize: 12,
                          ),
                        )
                      ],
                    ),
                    const Spacer(),
                    Container(
                      width: isGuest.value ? 100 : 110,
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
                ))
              ],
            ),
            onTap: () {
              if (isGuest.value) {
                Get.toNamed('/login');
              } else {
                CustomerServiceState similarServiceState =
                    Get.put(CustomerServiceState());
                similarServiceState.selectedService.value = service;
                Get.to(() => ServiceDetailsScreen());
              }
            },
          ),
        ),
      ),
    );
  }
}
