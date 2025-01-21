import 'dart:convert';

import 'package:cached_network_image/cached_network_image.dart';
import 'package:carousel_slider/carousel_slider.dart';
import 'package:firebase_database/ui/firebase_animated_list.dart';
import 'package:flutter/material.dart';
import 'package:flutter_rating_bar/flutter_rating_bar.dart';
import 'package:flutter_svg/svg.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/firebase/service_booking_firebase.dart';
import 'package:groom/screens/chat_screen.dart';
import 'package:groom/ui/customer/services/provider_details_controller.dart';
import 'package:groom/ui/customer/services/service_details_screen.dart';
import 'package:groom/ui/provider/schedule/schedule_model.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/loading_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../utils/tools.dart';
import '../../../data_models/provider_service_model.dart';
import '../../../data_models/user_model.dart';
import '../../../firebase/notifications_firebase.dart';
import '../../../firebase/provider_service_firebase.dart';
import '../../../firebase/user_firebase.dart';
import '../../../generated/assets.dart';
import '../../../repo/setting_repo.dart';
import '../../../states/customer_service_state.dart';
import '../../../utils/colors.dart';
import '../../../widgets/network_image_widget.dart';
import '../../../widgets/read_more_text.dart';
import '../../provider/reviews/model/review_model.dart';

class ProviderDetailsScreen extends StatefulWidget {
  UserModel provider;

  ProviderDetailsScreen({super.key, required this.provider});

  @override
  State<ProviderDetailsScreen> createState() => _ServiceDisplayScreenState();
}

class _ServiceDisplayScreenState extends State<ProviderDetailsScreen> {
  String selectedFilter = "All";
  final ProviderDetailsController _con = Get.put(ProviderDetailsController());
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
  UserFirebase userFirebase = UserFirebase();
  String groomerName = '';
  String selectedPage = 'service';
  DateTime? selectedDate = DateTime.now();
  TimeOfDay? selectedTime;
  CustomerServiceState similarServiceState = Get.put(CustomerServiceState());
  NotificationsFirebase notificationsFirebase = NotificationsFirebase();
  UserModel? userModel;
  RxInt selectedIndex = 0.obs;
  List<String> favoriteList = [];
  final List<String> filters = [
    "All",
    "Hair Style",
    "Nails",
    "Coloring",
    "Wax",
    "Spa",
    "Massage",
    "Facial",
    "Makeup",
  ];
  List<ProviderServiceModel> serviceList = [];

  Future<UserModel> fetchUser(String userId) async {
    return await userFirebase.getUser(userId);
  }

  @override
  void initState() {
    super.initState();
    _con.selectedProvider.value = widget.provider;
    _con.getScheduleList(_con.selectedProvider.value!.uid);
    providerServiceFirebase
        .getAllServicesByUserId(_con.selectedProvider.value!.uid)
        .then(
      (value) {
        setState(() {
          serviceList = value;
        });
      },
    );
  }

  @override
  Widget build(BuildContext context) {
    double distance = 0.0;
    if (auth.value.location != null &&
        _con.selectedProvider.value!.location != null) {
      distance = Geolocator.distanceBetween(
            auth.value.location!.latitude,
            auth.value.location!.longitude,
            _con.selectedProvider.value!.location!.latitude,
            _con.selectedProvider.value!.location!.longitude,
          ) /
          1000; // Convert meters to kilometers
    }
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        iconTheme: IconThemeData(color: primaryColorCode),
        title: HeaderTxtWidget('Detail Barber'),
      ),
      body: SingleChildScrollView(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.start,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            _slider(),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 10),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  HeaderTxtWidget(_con.selectedProvider.value!
                              .providerUserModel!.providerType !=
                          "Independent"
                      ? _con.selectedProvider.value!.providerUserModel!
                          .salonTitle!
                      : _con.selectedProvider.value!.fullName),
                  Row(
                    children: [
                      SvgPicture.asset(Assets.svgLocationGray),
                      const SizedBox(
                        width: 5,
                      ),
                      SubTxtWidget(
                        '${_con.selectedProvider.value!.providerUserModel!.addressLine} (${distance.toStringAsFixed(0)} km)',
                        color: "#8683A1".toColor(),
                      ),
                    ],
                  ),
                  Row(
                    children: [
                      SvgPicture.asset(Assets.svgStar),
                      const SizedBox(
                        width: 5,
                      ),
                      SubTxtWidget(
                        // '${_con.selectedProvider.value!.providerUserModel!.overallRating}',
                        '${_con.selectedProvider.value!.providerUserModel!.overallRating.toStringAsFixed(1)}',
                        color: "#8683A1".toColor(),
                      ),
                    ],
                  ),
                  socialIcon(),
                ],
              ),
            ),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 10),
              decoration: BoxDecoration(
                color: Color(0xEDEFFB).withOpacity(1),
              ),
              child: Row(
                children: [
                  Expanded(
                      child: InkWell(
                    onTap: () {
                      setState(() {
                        selectedPage = "service";
                      });
                    },
                    child: Container(
                      padding: const EdgeInsets.all(8),
                      margin: const EdgeInsets.symmetric(
                          horizontal: 5, vertical: 8),
                      decoration: BoxDecoration(
                          borderRadius:
                              const BorderRadius.all(Radius.circular(8)),
                          border: Border.all(
                              color: selectedPage == "service"
                                  ? primaryColorCode
                                  : Colors.grey.shade200,
                              width: 1)),
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          SvgPicture.asset(
                            Assets.svgScissors,
                          ),
                          const SizedBox(
                            width: 5,
                          ),
                          SubTxtWidget('Service'),
                        ],
                      ),
                    ),
                  )),
                  Expanded(
                      child: InkWell(
                    onTap: () {
                      setState(() {
                        selectedPage = "about";
                      });
                    },
                    child: Container(
                      padding: const EdgeInsets.all(8),
                      margin: const EdgeInsets.symmetric(
                          horizontal: 5, vertical: 8),
                      decoration: BoxDecoration(
                          borderRadius:
                              const BorderRadius.all(Radius.circular(8)),
                          border: Border.all(
                              color: selectedPage == "about"
                                  ? primaryColorCode
                                  : Colors.grey.shade200,
                              width: 1)),
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          SvgPicture.asset(Assets.svgAbout),
                          const SizedBox(
                            width: 5,
                          ),
                          SubTxtWidget('About'),
                        ],
                      ),
                    ),
                  )),
                  Expanded(
                      child: InkWell(
                    onTap: () {
                      setState(() {
                        selectedPage = "review";
                      });
                    },
                    child: Container(
                      padding: const EdgeInsets.all(8),
                      margin: const EdgeInsets.symmetric(
                          horizontal: 5, vertical: 8),
                      decoration: BoxDecoration(
                          borderRadius:
                              const BorderRadius.all(Radius.circular(8)),
                          border: Border.all(
                              color: selectedPage == "review"
                                  ? primaryColorCode
                                  : Colors.grey.shade200,
                              width: 1)),
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          SvgPicture.asset(
                            Assets.svgStar,
                          ),
                          const SizedBox(
                            width: 5,
                          ),
                          SubTxtWidget('Review'),
                        ],
                      ),
                    ),
                  )),
                ],
              ),
            ),
            Visibility(
              visible: selectedPage == "about",
              child: about(),
            ),
            Visibility(
              visible: selectedPage == "service",
              child: service(),
            ),
            Visibility(
              visible: selectedPage == "review",
              child: _review(),
            ),
          ],
        ),
      ),
    );
  }

  Widget _dots(int length) {
    List<Widget> list = [];
    for (int i = 0; i < length; i++) {
      list.add(AnimatedContainer(
        duration: const Duration(milliseconds: 500),
        height: 8,
        width: selectedIndex.value == i ? 28 : 8,
        margin: const EdgeInsets.symmetric(horizontal: 5),
        decoration: BoxDecoration(
            borderRadius: const BorderRadius.all(Radius.circular(10)),
            color: selectedIndex.value == i
                ? Color(0x9368E9).withOpacity(1)
                : Colors.grey.shade400),
      ));
    }
    return Row(
      children: list,
    );
  }

  Widget socialIcon() {
    return Container(
      padding: const EdgeInsets.symmetric(vertical: 20),
      child: Row(
        children: [
          Expanded(
            flex: 1,
            child: InkWell(
              onTap: () {
                userFirebase.launchGoogleMaps(
                    destinationLatitude:
                        _con.selectedProvider.value!.location!.latitude,
                    destinationLongitude:
                        _con.selectedProvider.value!.location!.longitude);
              },
              child: Column(
                children: [
                  SvgPicture.asset(Assets.svgLogosGoogleMaps),
                  const SizedBox(
                    height: 5,
                  ),
                  SubTxtWidget(
                    'Maps',
                    fontSize: 14,
                  ),
                ],
              ),
            ),
          ),
          Expanded(
            flex: 1,
            child: InkWell(
              onTap: () {
                Get.to(ChatDetailScreen(
                    userId: auth.value.uid,
                    friendId: _con.selectedProvider.value!.uid));
              },
              child: Column(
                children: [
                  SvgPicture.asset(Assets.svgChat),
                  const SizedBox(
                    height: 5,
                  ),
                  SubTxtWidget(
                    'Chat',
                    fontSize: 14,
                  ),
                ],
              ),
            ),
          ),
          Expanded(
            flex: 1,
            child: InkWell(
              onTap: () {
                _con.shareProvider();
              },
              child: Column(
                children: [
                  Obx(
                    () => _con.isSharing.value
                        ? const SizedBox(
                            height: 25,
                            width: 25,
                            child: CircularProgressIndicator(),
                          )
                        : SvgPicture.asset(Assets.svgShare),
                  ),
                  const SizedBox(
                    height: 5,
                  ),
                  SubTxtWidget(
                    'Share',
                    fontSize: 14,
                  ),
                ],
              ),
            ),
          ),
          Expanded(
            flex: 1,
            child: InkWell(
              onTap: () {
                userFirebase.addProviderToFavorites(
                    auth.value.uid, _con.selectedProvider.value!.uid);
                setState(() {
                  _con.isFavorite.value = !_con.isFavorite.value;
                });

                print(_con.isFavorite.value);
              },
              child: Obx(
                () => Column(
                  children: [
                    _con.isFavorite.value
                        ? const Icon(
                            Icons.favorite,
                            color: Colors.red,
                          )
                        : const Icon(Icons.favorite_border),
                    const SizedBox(
                      height: 5,
                    ),
                    SubTxtWidget(
                      'Save',
                      fontSize: 14,
                    ),
                  ],
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }

  void showLoadingDialog() {
    showDialog(
      barrierDismissible: false,
      context: context,
      builder: (BuildContext context) {
        return Dialog(
          child: Padding(
            padding: const EdgeInsets.all(16.0),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                CircularProgressIndicator(),
                SizedBox(width: 16),
                Text("Uploading data..."),
              ],
            ),
          ),
        );
      },
    );
  }

  void hideLoadingDialog() {
    Navigator.of(context).pop();
  }

  Widget about() {
    return Obx(
      () => Container(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 10),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            ReadMoreText(
              _con.selectedProvider.value!.providerUserModel!.about,
            ),
            const SizedBox(
              height: 10,
            ),
            HeaderTxtWidget('Business Hours'),
            const SizedBox(
              height: 5,
            ),
            ListView.builder(
              itemBuilder: (context, index) {
                ScheduleModel data = _con.scheduleList[index];
                return Container(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
                  child: Row(
                    children: [
                      Expanded(
                        flex: 3,
                        child: SubTxtWidget(data.dayName!),
                      ),
                      Expanded(
                          flex: 5,
                          child: Container(
                            alignment: AlignmentDirectional.centerStart,
                            padding: const EdgeInsets.all(5),
                            child: data.isEnable!
                                ? SubTxtWidget(
                                    '${data.fromTime!}-${data.toTime!}')
                                : SubTxtWidget(
                                    "Closed",
                                    color: Colors.grey,
                                  ),
                          )),
                    ],
                  ),
                );
              },
              itemCount: _con.scheduleList.length,
              primary: false,
              shrinkWrap: true,
            ),
            const SizedBox(
              height: 10,
            ),
            HeaderTxtWidget('Skill'),
            const SizedBox(
              height: 5,
            ),
            Row(
              children: [
                SizedBox(
                  height: 50,
                  width: 50,
                  child: ClipRRect(
                    borderRadius: const BorderRadius.all(Radius.circular(30)),
                    child: CachedNetworkImage(
                      imageUrl: _con.selectedProvider.value!.photoURL,
                      fit: BoxFit.cover,
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
                    HeaderTxtWidget(_con.selectedProvider.value!.fullName),
                    SubTxtWidget(
                      getSkills(),
                      fontSize: 14,
                      fontWeight: FontWeight.w300,
                    )
                  ],
                ))
              ],
            ),
            Divider(
              color: Colors.grey.shade200,
              height: 20,
            ),
            const SizedBox(
              height: 50,
            ),
          ],
        ),
      ),
    );
  }

  Widget service() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 10),
      child: Column(
        children: [
          SizedBox(
            height: 40,
            child: ListView.builder(
              itemBuilder: (context, index) {
                return InkWell(
                  onTap: () {
                    setState(() {
                      selectedFilter = filters[index];
                    });
                  },
                  child: Container(
                    padding:
                        const EdgeInsets.symmetric(horizontal: 8, vertical: 5),
                    alignment: AlignmentDirectional.center,
                    margin: const EdgeInsets.symmetric(horizontal: 5),
                    decoration: BoxDecoration(
                      border: selectedFilter == filters[index]
                          ? Border.all(color: Colors.grey)
                          : null,
                      color: selectedFilter == filters[index]
                          ? Colors.grey.shade300
                          : Colors.transparent,
                      borderRadius: const BorderRadius.all(Radius.circular(10)),
                    ),
                    child: SubTxtWidget(filters[index]),
                  ),
                );
              },
              itemCount: filters.length,
              scrollDirection: Axis.horizontal,
              shrinkWrap: true,
            ),
          ),
          ListView.builder(
              scrollDirection: Axis.vertical,
              itemCount: getFilterServiceList().length,
              primary: false,
              shrinkWrap: true,
              itemBuilder: (context, index) {
                var service = getFilterServiceList()[index];
                service.provider = _con.selectedProvider.value!;
                double distance = 0.0;
                if (auth.value.location != null && service.location != null) {
                  distance = Geolocator.distanceBetween(
                        auth.value.location!.latitude,
                        auth.value.location!.longitude,
                        service.location!.latitude,
                        service.location!.longitude,
                      ) /
                      1000; // Convert meters to kilometers
                }
                return InkWell(
                  child: Container(
                    margin: const EdgeInsets.only(top: 15),
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
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
                            HeaderTxtWidget('${service.serviceName}'),
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
                                        : '${service.address!.toMiniAddress()} (${distance.toStringAsFixed(0)} mi)',
                                    color: "#8683A1".toColor(),
                                  ),
                                )
                              ],
                            ),
                            Row(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              mainAxisAlignment: MainAxisAlignment.spaceBetween,
                              children: [
                                Column(
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
                                    HeaderTxtWidget(
                                      '\$ ${service.servicePrice}',
                                      color: Colors.blue,
                                    )
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
                                        isGuest.value ? "Login" : 'Book Now',
                                        color: Colors.white,
                                        fontSize: 13,
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
                      similarServiceState.selectedService.value = service;
                      Get.to(() => ServiceDetailsScreen());
                    }
                  },
                );
              }),
          if (getFilterServiceList().isEmpty)
            Container(
              padding: const EdgeInsets.all(20),
              child: HeaderTxtWidget("No Service available in $selectedFilter"),
            )
        ],
      ),
    );
  }

  Widget _review() {
    return FirebaseAnimatedList(
      query: providerServiceFirebase
          .getReviewListByServiceId(_con.selectedProvider.value!.uid),
      itemBuilder: (context, snapshot, animation, index) {
        if (!snapshot.exists) {
          return Center(
            child: HeaderTxtWidget("No review found"),
          );
        }
        ReviewModel? data =
            ReviewModel.fromJson(jsonDecode(jsonEncode(snapshot.value)));
        return FutureBuilder(
          future: UserFirebase().getUser(data.userId!),
          builder: (context, snapshot) {
            if (snapshot.data == null) {
              return const Center(
                child: CircularProgressIndicator(),
              );
            }
            return Container(
              padding: const EdgeInsets.all(10),
              margin: const EdgeInsets.all(10),
              decoration: const BoxDecoration(
                  borderRadius: BorderRadius.all(Radius.circular(10)),
                  color: Colors.white),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      ClipRRect(
                        borderRadius:
                            const BorderRadius.all(Radius.circular(30)),
                        child: NetworkImageWidget(
                          url: snapshot.data!.photoURL,
                          height: 60,
                          width: 60,
                        ),
                      ),
                      const SizedBox(
                        width: 10,
                      ),
                      Expanded(
                          child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        mainAxisAlignment: MainAxisAlignment.start,
                        children: [
                          HeaderTxtWidget(snapshot.data!.fullName),
                          RatingBarIndicator(
                            itemBuilder: (context, index) {
                              return const Icon(
                                Icons.star,
                                color: Colors.amber,
                              );
                            },
                            itemCount: 5,
                            rating: data.ratingValue ?? 0,
                            itemSize: 20,
                          )
                        ],
                      ))
                    ],
                  ),
                  SubTxtWidget(data.comment!),
                  SubTxtWidget("${data.createdOn!}"),
                ],
              ),
            );
          },
        );
      },
      defaultChild: LoadingWidget(),
      shrinkWrap: true,
      primary: false,
    );
  }

  List<ProviderServiceModel> getFilterServiceList() {
    if (selectedFilter == "All") return serviceList;
    return serviceList
        .where((element) => element.serviceType == selectedFilter)
        .toList();
  }

  // Widget _slider() {
  //   return Container(
  //     height: 250,
  //     margin: const EdgeInsets.all(10),
  //     child: Stack(
  //       children: [
  //         CarouselSlider(
  //           items: _con
  //               .selectedProvider.value!.providerUserModel!.providerImages!
  //               .map((e) {
  //             return ClipRRect(
  //               borderRadius: const BorderRadius.all(Radius.circular(10)),
  //               child: CachedNetworkImage(
  //                 imageUrl: e,
  //                 fit: BoxFit.cover,
  //                 width: 1000.0,
  //               ),
  //             );
  //           }).toList(),
  //           options: CarouselOptions(
  //             aspectRatio: 1,
  //             height: 250,
  //             viewportFraction: 1,
  //             initialPage: 0,
  //             enableInfiniteScroll: true,
  //             reverse: false,
  //             autoPlay: true,
  //             autoPlayInterval: const Duration(seconds: 3),
  //             autoPlayAnimationDuration: const Duration(milliseconds: 800),
  //             autoPlayCurve: Curves.fastOutSlowIn,
  //             enlargeCenterPage: true,
  //             scrollDirection: Axis.horizontal,
  //             onPageChanged: (index, reason) {
  //               selectedIndex.value = index;
  //             },
  //           ),
  //         ),
  //         Positioned(
  //           top: 0,
  //           right: 0,
  //           child: Container(
  //             padding: const EdgeInsets.all(5),
  //             decoration: const BoxDecoration(
  //                 borderRadius: BorderRadius.only(
  //                     bottomLeft: Radius.circular(10),
  //                     topRight: Radius.circular(10)),
  //                 color: Colors.green),
  //             child: SubTxtWidget(
  //               _con.openStatus.value,
  //               color: Colors.white,
  //               fontSize: 14,
  //             ),
  //           ),
  //         ),
  //         Positioned(
  //           bottom: 10,
  //           right: 10,
  //           child: Obx(
  //             () => _dots(_con.selectedProvider.value!.providerUserModel!
  //                 .providerImages!.length),
  //           ),
  //         ),
  //         InkWell(
  //           onTap: () {
  //             Get.toNamed('/add_provider_report', arguments: widget.provider);
  //           },
  //           child: Container(
  //             padding: const EdgeInsets.all(10),
  //             child: Row(
  //               children: [
  //                 const Icon(
  //                   Icons.report_outlined,
  //                   color: Colors.red,
  //                   size: 14,
  //                 ),
  //                 const SizedBox(
  //                   width: 5,
  //                 ),
  //                 SubTxtWidget(
  //                   'Report User',
  //                   color: Colors.white,
  //                   fontSize: 13,
  //                 ),
  //               ],
  //             ),
  //           ),
  //         )
  //       ],
  //     ),
  //   );
  // }

  Widget _slider() {
    return Obx(() {
      if (_con.selectedProvider.value?.providerUserModel?.providerImages ==
          null) {
        return Container(
          height: 250,
          margin: const EdgeInsets.all(10),
          child: Center(
            child: HeaderTxtWidget('Loading...'),
          ),
        );
      }
      return Container(
        height: 250,
        margin: const EdgeInsets.all(10),
        child: Stack(
          children: [
            CarouselSlider(
              items: _con
                  .selectedProvider.value!.providerUserModel!.providerImages!
                  .map((e) {
                return ClipRRect(
                  borderRadius: const BorderRadius.all(Radius.circular(10)),
                  child: CachedNetworkImage(
                    imageUrl: e,
                    fit: BoxFit.cover,
                    width: 1000.0,
                  ),
                );
              }).toList(),
              options: CarouselOptions(
                aspectRatio: 1,
                height: 250,
                viewportFraction: 1,
                initialPage: 0,
                enableInfiniteScroll: true,
                reverse: false,
                autoPlay: true,
                autoPlayInterval: const Duration(seconds: 3),
                autoPlayAnimationDuration: const Duration(milliseconds: 800),
                autoPlayCurve: Curves.fastOutSlowIn,
                enlargeCenterPage: true,
                scrollDirection: Axis.horizontal,
                onPageChanged: (index, reason) {
                  selectedIndex.value = index;
                },
              ),
            ),
            Positioned(
              top: 0,
              right: 0,
              child: Container(
                padding: const EdgeInsets.all(5),
                decoration: const BoxDecoration(
                    borderRadius: BorderRadius.only(
                        bottomLeft: Radius.circular(10),
                        topRight: Radius.circular(10)),
                    color: Colors.green),
                child: SubTxtWidget(
                  _con.openStatus.value,
                  color: Colors.white,
                  fontSize: 14,
                ),
              ),
            ),
            Positioned(
              bottom: 10,
              right: 10,
              child: Obx(
                    () => _dots(_con.selectedProvider.value!.providerUserModel!
                    .providerImages!.length),
              ),
            ),
            InkWell(
              onTap: () {
                Get.toNamed('/add_provider_report', arguments: widget.provider);
              },
              child: Container(
                padding: const EdgeInsets.all(10),
                child: Row(
                  children: [
                    const Icon(
                      Icons.report_outlined,
                      color: Colors.red,
                      size: 14,
                    ),
                    const SizedBox(
                      width: 5,
                    ),
                    SubTxtWidget(
                      'Report User',
                      color: Colors.white,
                      fontSize: 13,
                    ),
                  ],
                ),
              ),
            )
          ],
        ),
      );
    });
  }

  String getSkills() {
    StringBuffer buffer = StringBuffer();
    _con.selectedProvider.value!.providerUserModel!.skills!.forEach(
      (element) {
        if (buffer.isNotEmpty) {
          buffer.write(" - ");
        }
        buffer.write(element);
      },
    );
    return buffer.toString();
  }
}
