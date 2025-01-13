import 'dart:convert';
import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_database/ui/firebase_animated_list.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_rating_bar/flutter_rating_bar.dart';
import 'package:flutter_svg/svg.dart';
import 'package:get/get.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/firebase/provider_user_firebase.dart';
import 'package:groom/firebase/user_firebase.dart';
import 'package:groom/generated/assets.dart';
import 'package:groom/repo/session_repo.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/ui/provider/home/widgets/bar_chart_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../constant/global_configuration.dart';
import '../../../data_models/provider_service_model.dart';
import '../../../data_models/request_reservation_model.dart';
import '../../../data_models/service_reservation_model.dart';
import '../../../firebase/reservation_firebase.dart';
import '../../../screens/chat_list_screen.dart';
import '../../membership/widgets/membership_status_page.dart';
import '../../notification/notification_screen.dart';
import '../../../screens/provider_screens/provider_financial_dashboard_screen.dart';
import '../../../screens/provider_screens/provider_form_screen.dart';
import '../../../states/customer_service_state.dart';
import '../../../utils/colors.dart';
import '../../../widgets/card_widget.dart';
import '../../../widgets/loading_widget.dart';
import '../../customer/services/service_details_screen.dart';
import '../addService/provider_create_service_screen.dart';
import '../booking/widgets/provider_booking_request_reservation_widget.dart';
import '../booking/widgets/provider_booking_service_reservation_widget.dart';
import '../reviews/review_list.dart';
import 'm/transaction.dart';

class ProviderHomeScreen extends StatefulWidget {
  const ProviderHomeScreen({super.key});

  @override
  State<ProviderHomeScreen> createState() => _ProviderHomeScreenState();
}

class _ProviderHomeScreenState extends State<ProviderHomeScreen> {
  final _ref = FirebaseDatabase.instanceFor(
      app: Firebase.app(),
      databaseURL: GlobalConfiguration().getValue('databaseURL'));

  @override
  Widget build(BuildContext context) {
    return WillPopScope(
      onWillPop: () async {
        final shouldPop = await _showExitDialog(context);
        return shouldPop ?? false;
      },
      child: Scaffold(
        body: FutureBuilder(
          future: UserFirebase().getUserDetails(auth.value.uid),
          builder: (context, snapshot) {
            if (snapshot.data == null) {
              return LoadingWidget(
                type: LoadingType.DASHBOARD,
              );
            }
            if (snapshot.data!.providerUserModel == null) {
              Get.to(() => ProviderFormPage());
              return Container();
            }
            return CustomScrollView(
              slivers: [
                SliverAppBar(
                  backgroundColor: primaryColorCode,
                  pinned: false,
                  floating: true,
                  snap: true,
                  leadingWidth: 0,
                  toolbarHeight: 65,
                  title: Container(
                    color: primaryColorCode,
                    child: Row(
                      children: [
                        HeaderTxtWidget(
                          'HOME',
                          color: Colors.white,
                        ),
                        Container(
                          decoration: BoxDecoration(
                              color: "#399DDC".toColor().withOpacity(0.5),
                              borderRadius:
                                  const BorderRadius.all(Radius.circular(15))),
                          padding: const EdgeInsets.symmetric(
                              horizontal: 10, vertical: 5),
                          margin: const EdgeInsets.symmetric(horizontal: 10),
                          child: Column(
                            children: [
                              FutureBuilder(
                                future: getSession(),
                                builder: (context, snapshot) {
                                  return HeaderTxtWidget(
                                    auth.value.providerUserModel!
                                                .providerType !=
                                            "Independent"
                                        ? auth.value.providerUserModel!
                                            .salonTitle!
                                        : auth.value.fullName,
                                    color: Colors.white,
                                    fontSize: 10,
                                  );
                                },
                              ),
                              FutureBuilder(
                                future: totalEarning(),
                                builder: (context, snapshot) {
                                  return HeaderTxtWidget(
                                    '\$${snapshot.data}USD',
                                    color: Colors.white,
                                    fontSize: 12,
                                  );
                                },
                              )
                            ],
                          ),
                        ),
                        Spacer(),
                        IconButton(
                            onPressed: () {
                              Get.to(() => ChatListScreen(
                                  id: auth.value.providerUserModel!.createdOn
                                      .toString()));
                            },
                            icon: SvgPicture.asset(Assets.svgChat)),
                        IconButton(
                            onPressed: () {
                              Get.to(() => ProviderCreateServiceScreen());
                            },
                            icon: Container(
                              decoration: const BoxDecoration(
                                  shape: BoxShape.circle, color: Colors.red),
                              child: const Icon(
                                Icons.add,
                                color: Colors.white,
                                size: 32,
                              ),
                            )),
                        IconButton(
                            onPressed: () {
                              Get.to(() => const NotificationScreen());
                            },
                            icon: Container(
                              padding: const EdgeInsets.all(5),
                              decoration: const BoxDecoration(
                                  borderRadius:
                                      BorderRadius.all(Radius.circular(10)),
                                  color: Colors.white),
                              child: Icon(
                                Icons.notifications_none,
                                color: "#2F6EB6".toColor(),
                                size: 25,
                              ),
                            )),
                      ],
                    ),
                  ),
                  surfaceTintColor: primaryColorCode,
                ),
                SliverToBoxAdapter(
                  child: MembershipStatusPage(data: snapshot.data!,),
                ),
                SliverToBoxAdapter(
                  child: Container(
                    padding: const EdgeInsets.symmetric(
                        horizontal: 15, vertical: 10),
                    child: Column(
                      children: [
                        Padding(
                          padding: const EdgeInsets.all(8.0),
                          child: Row(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              Expanded(
                                child: FutureBuilder(
                                  future: ReservationFirebase()
                                      .getProviderServiceReservationsByUserId(
                                          auth.value.uid),
                                  builder: (context, snapshot) {
                                    if (snapshot.connectionState ==
                                        ConnectionState.waiting) {
                                      return CardWidget(
                                        title: '0', // Set default value to '0'
                                        subTitle: "Booking",
                                        icon: "assets/book.png",
                                      );
                                    } else {
                                      List reservations = snapshot.data
                                          as List<ServiceReservationModel>;
                                      return GestureDetector(
                                        onTap: () {
                                          Get.to(() =>
                                              ProviderServiceReservationWidget(
                                                showAppbar: true,
                                              ));
                                        },
                                        child: CardWidget(
                                          title: '${reservations.length}',
                                          subTitle: "Booking",
                                          icon: "assets/book.png",
                                        ),
                                      );
                                    }
                                  },
                                ),
                              ),
                              const SizedBox(width: 10),
                              // Adding space between ListTiles
                              FutureBuilder(
                                future: servicesCount(),
                                builder: (context, snapshot) {
                                  // Check if the snapshot has data or is still loading
                                  if (snapshot.connectionState ==
                                      ConnectionState.waiting) {
                                    // While waiting, return a CardWidget with the title set to '0'
                                    return CardWidget(
                                      title: '0', // Set default value to '0'
                                      subTitle: "Requests",
                                      icon: "assets/services.png",
                                    );
                                  } else {
                                    // Once the future completes, display the actual data
                                    return GestureDetector(
                                      onTap: () {
                                        Get.to(() =>
                                            ProviderRequestReservationWidget(
                                              showAppBar: true,
                                            ));
                                      },
                                      child: CardWidget(
                                        title: snapshot.data == null
                                            ? "0"
                                            : snapshot.data.toString(),
                                        subTitle: "Services",
                                        icon: "assets/services.png",
                                      ),
                                    );
                                  }
                                },
                              ),
                            ],
                          ),
                        ),
                        Padding(
                          padding: const EdgeInsets.only(
                            left: 8.0,
                            right: 8,
                          ),
                          child: Row(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              Expanded(
                                child: FutureBuilder(
                                  future: reviewCount(),
                                  builder: (context, snapshot) {
                                    // Check if the snapshot has data or is still loading
                                    if (snapshot.connectionState ==
                                        ConnectionState.waiting) {
                                      // While waiting, return a CardWidget with the title set to '0'
                                      return CardWidget(
                                        title: '0', // Set default value to '0'
                                        subTitle: "Total Reviews",
                                        icon: "assets/book.png",
                                      );
                                    } else {
                                      // Once the future completes, display the actual data
                                      return GestureDetector(
                                        onTap: () {
                                          Get.to(() => const ReviewList());
                                        },
                                        child: CardWidget(
                                          title: snapshot.data == null
                                              ? "0"
                                              : snapshot.data.toString(),
                                          subTitle: "Total Reviews",
                                          icon: "assets/book.png",
                                        ),
                                      );
                                    }
                                  },
                                ),
                              ),
                              const SizedBox(width: 10),
                              Expanded(
                                child: FutureBuilder(
                                  future: totalEarning(),
                                  builder: (context, snapshot) {
                                    // Check if the snapshot has data or is still loading
                                    if (snapshot.connectionState ==
                                        ConnectionState.waiting) {
                                      // While waiting, return a CardWidget with the title set to '0'
                                      return CardWidget(
                                        title: '0', // Set default value to '0'
                                        subTitle: "Total Earnings",
                                        icon: "assets/services.png",
                                      );
                                    } else {
                                      return GestureDetector(
                                        onTap: () {
                                          Get.to(() =>
                                              const ProviderFinancialDashboard());
                                        },
                                        child: CardWidget(
                                          title: snapshot.data == null
                                              ? "0"
                                              : snapshot.data.toString(),
                                          subTitle: "Total Earnings",
                                          icon: "assets/services.png",
                                        ),
                                      );
                                    }
                                  },
                                ),
                              ), // Adding space between ListTiles
                            ],
                          ),
                        ),
                        BarChartWidget(),
                        Container(
                          margin: const EdgeInsets.symmetric(vertical: 20),
                          padding: const EdgeInsets.all(8.0),
                          child: Row(
                            mainAxisAlignment: MainAxisAlignment.spaceBetween,
                            children: [
                              Expanded(child: HeaderTxtWidget('Listings')),
                              GestureDetector(
                                onTap: () {
                                  Get.toNamed('/provider_service_list');
                                },
                                child: Text(
                                  "See more",
                                  style: GoogleFonts.workSans(
                                      color: textformColor, fontSize: 14),
                                ),
                              ),
                            ],
                          ),
                        ),
                        _serviceList()
                      ],
                    ),
                  ),
                ),
              ],
            );
          },
        ),
      ),
    );
  }

  //Functions
  servicesCount() async {
    AggregateQuerySnapshot query = await FirebaseFirestore.instanceFor(
            app: Firebase.app(),
            databaseURL: GlobalConfiguration().getValue('databaseURL'))
        .collection('requestReservation')
        .where("providerId", isEqualTo: auth.value.uid)
        .count()
        .get();

    int? numberOfDocuments = query.count;
    return numberOfDocuments;
  }

  //Booking Count
  Future<int> bookingCount() async {
    List<RequestReservationModel> list = await ReservationFirebase()
        .getProviderRequestReservationsByUserId(auth.value.uid);
    return list.length;
  }

  reviewCount() async {
    DataSnapshot data = await _ref
        .ref("ratings")
        .orderByChild("providerId")
        .equalTo(auth.value.uid)
        .get();
    return data.children.length;
  }

  totalEarning() async {
    double amount = 0;
    List<TransactionModel> list =
        await ProviderUserFirebase().getAllTransaction(auth.value.uid);
    list.forEach(
      (element) {
        amount = amount + element.amount! ?? 0;
      },
    );
    return amount;
  }

  _showExitDialog(BuildContext context) {
    Future<bool?> _showExitDialog(BuildContext context) {
      return showDialog<bool>(
        context: context,
        builder: (context) => AlertDialog(
          title: Text('Exit App'),
          content: Text('Do you want to exit the app?'),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(false),
              child: Text('No'),
            ),
            TextButton(
              onPressed: () => Navigator.of(context).pop(true),
              child: Text('Yes'),
            ),
          ],
        ),
      );
    }
  }

  Widget _serviceList() {
    return FirebaseAnimatedList(
      query: _ref
          .ref("providerServices")
          .orderByChild("userId")
          .equalTo(auth.value.uid),
      itemBuilder: (context, snapshot, animation, index) {
        ProviderServiceModel? data = ProviderServiceModel.fromJson(
            jsonDecode(jsonEncode(snapshot.value)));
        return InkWell(
          onTap: () {
            CustomerServiceState customerServiceState =
                Get.put(CustomerServiceState());
            customerServiceState.selectedService.value = data;
            Get.to(() => ServiceDetailsScreen());
          },
          child: Card(
            child: ClipRRect(
              borderRadius: const BorderRadius.all(Radius.circular(10)),
              child: SizedBox(
                height: 220,
                child: Stack(
                  children: [
                    NetworkImageWidget(
                      url: data.serviceImages!.first,
                      height: 150,
                      fit: BoxFit.cover,
                    ),
                    Container(
                      margin: const EdgeInsets.all(10),
                      padding: const EdgeInsets.symmetric(
                          horizontal: 10, vertical: 5),
                      decoration: const BoxDecoration(
                          borderRadius: BorderRadius.all(Radius.circular(15)),
                          color: Colors.white),
                      child: SubTxtWidget(
                        data.serviceType,
                        fontSize: 9,
                      ),
                    ),
                    Positioned(
                      right: 10,
                      top: 120,
                      child: Container(
                        margin: const EdgeInsets.all(10),
                        padding: const EdgeInsets.symmetric(
                            horizontal: 20, vertical: 8),
                        decoration: BoxDecoration(
                            borderRadius:
                                const BorderRadius.all(Radius.circular(20)),
                            color: primaryColorCode,
                            border: Border.all(color: Colors.white, width: 2)),
                        child: HeaderTxtWidget(
                          '\$ ${data.servicePrice}',
                          fontSize: 12,
                          color: Colors.white,
                        ),
                      ),
                    ),
                    Positioned(
                      top: 145,
                      right: 0,
                      left: 0,
                      child: Container(
                        padding: const EdgeInsets.all(10),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            HeaderTxtWidget(data.serviceName!),
                            RatingBarIndicator(
                              itemBuilder: (context, index) {
                                return const Icon(
                                  Icons.star,
                                  color: Colors.amber,
                                );
                              },
                              itemCount: 5,
                              itemSize: 18,
                              rating: 5,
                            ),
                            Row(
                              children: [
                                /* const Icon(Icons.remove_red_eye_outlined),
                              HeaderTxtWidget(
                                '1250',
                                fontSize: 10,
                              ),
                              const SizedBox(
                                width: 50,
                              ),
                              HeaderTxtWidget(
                                '50 watchers',
                                fontSize: 10,
                              ),*/
                                const Spacer(),
                                InkWell(
                                  child: Container(
                                    decoration: const BoxDecoration(
                                        color: Colors.black,
                                        borderRadius: BorderRadius.all(
                                            Radius.circular(10))),
                                    padding: const EdgeInsets.symmetric(
                                        horizontal: 10, vertical: 5),
                                    child: Row(
                                      children: [
                                        const Icon(
                                          Icons.edit_rounded,
                                          color: Colors.white,
                                          size: 10,
                                        ),
                                        const SizedBox(
                                          width: 3,
                                        ),
                                        SubTxtWidget(
                                          'EDIT',
                                          color: Colors.white,
                                          fontSize: 8,
                                        )
                                      ],
                                    ),
                                  ),
                                  onTap: () {
                                    Get.to(() => ProviderCreateServiceScreen(
                                          data: data,
                                        ));
                                  },
                                )
                              ],
                            )
                          ],
                        ),
                      ),
                    )
                  ],
                ),
              ),
            ),
          ),
        );
      },
      defaultChild: LoadingWidget(),
      shrinkWrap: true,
      primary: false,
      physics: const NeverScrollableScrollPhysics(),
      padding: EdgeInsets.zero,
    );
  }

}
