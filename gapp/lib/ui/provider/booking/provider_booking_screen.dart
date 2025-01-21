import 'package:cached_network_image/cached_network_image.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:get/utils.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/ui/provider/booking/widgets/provider_booking_request_list_screen.dart';
import 'package:groom/screens/services_screens/provider_booking_service_list_screen.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import '../../../utils/tools.dart';
import '../../../data_models/request_reservation_model.dart';
import '../../../data_models/service_reservation_model.dart';
import '../../../data_models/user_model.dart';
import '../../../firebase/provider_service_firebase.dart';
import '../../../firebase/reservation_firebase.dart';
import '../../../firebase/service_booking_firebase.dart';
import '../../../firebase/user_firebase.dart';
import '../../../repo/setting_repo.dart';
import '../../../screens/customer_screens/customer_rating_screen.dart';
import '../../../utils/utils.dart';
import '../../../widgets/sub_txt_widget.dart';
import '../../customer/booking/cancel_booking_screen.dart';
import '../../customer/customer_dashboard/customer_dashboard_page.dart';

class BookingScreen extends StatefulWidget {
  const BookingScreen({super.key});

  @override
  State<BookingScreen> createState() => _BookingScreenState();
}

class _BookingScreenState extends State<BookingScreen> {
  String selected = "1";
  String selectedSub = "1";
  ReservationFirebase reservationFirebase = ReservationFirebase();
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  UserFirebase userFirebase = UserFirebase();
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: CustomScrollView(
        slivers: [
          SliverAppBar(
              backgroundColor: Colors.white,
              pinned: false,
              automaticallyImplyLeading: false,
              floating: true,
              snap: true,
              toolbarHeight: 60,
              title: HeaderTxtWidget('Booking'),
              surfaceTintColor: Colors.white,
              bottom: PreferredSize(
                preferredSize: Size.fromHeight(selected == "3" ? 50 : 30),
                child: Column(
                  children: [
                    Row(
                      children: [
                        Expanded(
                          flex: 3,
                          child: InkWell(
                            onTap: () {
                              setState(() {
                                selected = "1";
                              });
                            },
                            child: Container(
                              alignment: AlignmentDirectional.center,
                              margin: const EdgeInsets.symmetric(horizontal: 5),
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 10, vertical: 5),
                              decoration: BoxDecoration(
                                  color: selected == "1"
                                      ? "#F0EEFF".toColor()
                                      : Colors.white,
                                  borderRadius: const BorderRadius.all(
                                      Radius.circular(8))),
                              child: SubTxtWidget(
                                'Service Requests',
                                color:
                                    selected == "1" ? Colors.blue : Colors.grey,
                                fontWeight: FontWeight.w600,
                                fontSize: 14,
                              ),
                            ),
                          ),
                        ),
                        Expanded(
                          flex: 2,
                          child: InkWell(
                            onTap: () {
                              setState(() {
                                selected = "2";
                              });
                            },
                            child: Container(
                              alignment: AlignmentDirectional.center,
                              margin: const EdgeInsets.symmetric(horizontal: 5),
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 10, vertical: 5),
                              decoration: BoxDecoration(
                                  color: selected == "2"
                                      ? "#F0EEFF".toColor()
                                      : Colors.white,
                                  borderRadius: const BorderRadius.all(
                                      Radius.circular(8))),
                              child: SubTxtWidget(
                                'Pending',
                                fontWeight: FontWeight.w600,
                                color:
                                    selected == "2" ? Colors.blue : Colors.grey,
                                fontSize: 14,
                              ),
                            ),
                          ),
                        ),
                        Expanded(
                          flex: 2,
                          child: InkWell(
                            onTap: () {
                              setState(() {
                                selected = "3";
                              });
                            },
                            child: Container(
                              alignment: AlignmentDirectional.center,
                              margin: const EdgeInsets.symmetric(horizontal: 5),
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 10, vertical: 5),
                              decoration: BoxDecoration(
                                  color: selected == "3"
                                      ? "#F0EEFF".toColor()
                                      : Colors.white,
                                  borderRadius: const BorderRadius.all(
                                      Radius.circular(8))),
                              child: SubTxtWidget(
                                'History',
                                fontWeight: FontWeight.w600,
                                color:
                                    selected == "3" ? Colors.blue : Colors.grey,
                                fontSize: 14,
                              ),
                            ),
                          ),
                        )
                      ],
                    ),
                    const SizedBox(
                      height: 10,
                    ),
                    Visibility(
                      visible: selected == "3",
                      child: Column(
                        children: [
                          Row(
                            children: [
                              Expanded(
                                flex: 1,
                                child: InkWell(
                                  onTap: () {
                                    setState(() {
                                      selectedSub = "1";
                                    });
                                  },
                                  child: AnimatedContainer(
                                    duration: const Duration(milliseconds: 500),
                                    alignment: AlignmentDirectional.center,
                                    margin: const EdgeInsets.symmetric(
                                        horizontal: 5),
                                    padding: const EdgeInsets.symmetric(
                                        horizontal: 10, vertical: 5),
                                    decoration: BoxDecoration(
                                        color: selectedSub == "1"
                                            ? "#F0EEFF".toColor()
                                            : Colors.white,
                                        borderRadius: const BorderRadius.all(
                                            Radius.circular(8))),
                                    child: SubTxtWidget(
                                      'Service',
                                      color: selectedSub == "1"
                                          ? Colors.blue
                                          : Colors.grey,
                                      fontWeight: FontWeight.w600,
                                      fontSize: 14,
                                    ),
                                  ),
                                ),
                              ),
                              Expanded(
                                flex: 1,
                                child: InkWell(
                                  onTap: () {
                                    setState(() {
                                      selectedSub = "2";
                                    });
                                  },
                                  child: Container(
                                    alignment: AlignmentDirectional.center,
                                    margin: const EdgeInsets.symmetric(
                                        horizontal: 5),
                                    padding: const EdgeInsets.symmetric(
                                        horizontal: 10, vertical: 5),
                                    decoration: BoxDecoration(
                                        color: selectedSub == "2"
                                            ? "#F0EEFF".toColor()
                                            : Colors.white,
                                        borderRadius: const BorderRadius.all(
                                            Radius.circular(8))),
                                    child: SubTxtWidget(
                                      'Requests',
                                      fontWeight: FontWeight.w600,
                                      color: selectedSub == "2"
                                          ? Colors.blue
                                          : Colors.grey,
                                      fontSize: 14,
                                    ),
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              )),
          SliverToBoxAdapter(
            child: Column(
              children: [
                Visibility(
                  visible: selected == "1",
                  child: const ProviderBookingRequestsListScreen(),
                ),
                Visibility(
                  visible: selected == "2",
                  child: const ProviderBookingServiceListScreen(),
                ),
                Visibility(
                  visible: selected == "3" && selectedSub == "1",
                  child: FutureBuilder(
                      future: reservationFirebase
                          .getProviderServiceReservationsByUserId(
                              auth.value.uid),
                      builder: (context, snapshot) {
                        if (snapshot.connectionState ==
                            ConnectionState.waiting) {
                          return const Center(
                            child: CircularProgressIndicator(),
                          );
                        } else if (snapshot.hasError) {
                          return Text("Error: ${snapshot.error}");
                        } else if (snapshot.hasData) {
                          var reservations =
                              snapshot.data as List<ServiceReservationModel>;
                          if (reservations.isEmpty) {
                            return const Center(
                                child: Text("No Reservations Found"));
                          }
                          return ListView.builder(
                              itemCount: reservations.length,
                              shrinkWrap: true,
                              primary: false,
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 10, vertical: 10),
                              itemBuilder: (context, index) {
                                var reservation = reservations[index];
                                return FutureBuilder(
                                    future: providerServiceFirebase
                                        .getServiceByServiceId(
                                            reservation.serviceId),
                                    builder: (context, snapshot) {
                                      if (snapshot.connectionState ==
                                          ConnectionState.waiting) {
                                        return const Center(
                                          child: CircularProgressIndicator(),
                                        );
                                      } else if (snapshot.hasError) {
                                        return Text("Error: ${snapshot.error}");
                                      } else if (snapshot.hasData) {
                                        var service = snapshot.data;
                                        return FutureBuilder(
                                            future: userFirebase
                                                .getUser(service!.userId),
                                            builder: (context, snapshot) {
                                              if (snapshot.connectionState ==
                                                  ConnectionState.waiting) {
                                                return const Center(
                                                  child:
                                                      CircularProgressIndicator(),
                                                );
                                              } else if (snapshot.hasError) {
                                                return Text(
                                                    "Error: ${snapshot.error}");
                                              } else if (snapshot.hasData) {
                                                var user =
                                                    snapshot.data as UserModel;
                                                bool isPast = DateTime.now()
                                                    .isAfter(Tools.changeToDate(
                                                        reservation
                                                            .selectedDate));
                                                return Container(
                                                  decoration: BoxDecoration(
                                                      borderRadius:
                                                          BorderRadius.circular(
                                                              8.0),
                                                      color: Colors.white,
                                                      boxShadow: [
                                                        BoxShadow(
                                                            color: Colors
                                                                .grey.shade200,
                                                            blurRadius: 2,
                                                            spreadRadius: 3)
                                                      ]),
                                                  padding:
                                                      const EdgeInsets.all(5),
                                                  child: Row(
                                                    children: [
                                                      ClipRRect(
                                                        borderRadius:
                                                            BorderRadius
                                                                .circular(8),
                                                        child:
                                                            CachedNetworkImage(
                                                          fit: BoxFit.fill,
                                                          imageUrl: service
                                                              .serviceImages!
                                                              .first,
                                                          height: 120,
                                                          width: 100,
                                                        ),
                                                      ),
                                                      const SizedBox(
                                                        width: 6,
                                                      ),
                                                      Expanded(
                                                        child: Column(
                                                          mainAxisAlignment:
                                                              MainAxisAlignment
                                                                  .start,
                                                          crossAxisAlignment:
                                                              CrossAxisAlignment
                                                                  .start,
                                                          children: [
                                                            user.providerUserModel!
                                                                        .providerType !=
                                                                    "Independent"
                                                                ? Text(
                                                                    user
                                                                        .providerUserModel!
                                                                        .salonTitle!
                                                                        .capitalizeFirst!,
                                                                    style: const TextStyle(
                                                                        fontSize:
                                                                            16,
                                                                        fontWeight:
                                                                            FontWeight.bold),
                                                                  )
                                                                : Text(
                                                                    user.fullName
                                                                        .capitalizeFirst!,
                                                                    style: const TextStyle(
                                                                        fontSize:
                                                                            16,
                                                                        fontWeight:
                                                                            FontWeight.bold),
                                                                  ),
                                                            Text(
                                                              service
                                                                  .serviceType,
                                                              style: const TextStyle(
                                                                  fontSize: 12,
                                                                  fontWeight:
                                                                      FontWeight
                                                                          .w600,
                                                                  color: Colors
                                                                      .blue),
                                                            ),
                                                            Row(
                                                              mainAxisSize:
                                                                  MainAxisSize
                                                                      .max,
                                                              mainAxisAlignment:
                                                                  MainAxisAlignment
                                                                      .spaceBetween,
                                                              children: [
                                                                Column(
                                                                  children: [
                                                                    Row(
                                                                      children: [
                                                                        Icon(
                                                                          Icons
                                                                              .calendar_month,
                                                                          color:
                                                                              Colors.blueAccent,
                                                                        ),
                                                                        Text(Tools.changeDateFormat(
                                                                            reservation.selectedDate,
                                                                            globalTimeFormat)),
                                                                      ],
                                                                    ),
                                                                    Row(
                                                                      children: [
                                                                        const Icon(
                                                                          Icons
                                                                              .timelapse,
                                                                          color:
                                                                              Colors.blueAccent,
                                                                        ),
                                                                        Text(Tools.changeDateFormat(
                                                                            reservation.selectedDate,
                                                                            'hh:mm a')),
                                                                      ],
                                                                    ),
                                                                  ],
                                                                ),
                                                                TextButton(
                                                                  onPressed:
                                                                      () {
                                                                    if (isPast) {
                                                                      Get.to(
                                                                        () =>
                                                                            FeedbackScreen(
                                                                          providerId:
                                                                              user.uid,
                                                                          serviceId:
                                                                              reservation.serviceId,
                                                                        ),
                                                                      );
                                                                    } else {
                                                                      Get.to(
                                                                          CancelBookingScreen(
                                                                        data:
                                                                            reservation,
                                                                        success:
                                                                            () async {
                                                                          Get.to(() =>
                                                                              CustomerDashboardPage());
                                                                        },
                                                                        canceled_by:
                                                                            "PROVIDER",
                                                                      ));
                                                                    }
                                                                  },
                                                                  style: ButtonStyle(
                                                                      backgroundColor: MaterialStateProperty.all(isPast
                                                                          ? Colors
                                                                              .green
                                                                          : Colors
                                                                              .red)),
                                                                  child: Text(
                                                                    isPast
                                                                        ? "Review Service"
                                                                        : "Cancel Reservation",
                                                                    style: const TextStyle(
                                                                        fontWeight:
                                                                            FontWeight
                                                                                .bold,
                                                                        color: Colors
                                                                            .white),
                                                                  ),
                                                                )
                                                              ],
                                                            ),
                                                          ],
                                                        ),
                                                      ),
                                                      const SizedBox(
                                                        width: 6,
                                                      ),
                                                    ],
                                                  ),
                                                );
                                              }
                                              return const Text('No Data');
                                            });
                                      }
                                      return const Text("No Data");
                                    });
                              });
                        }
                        return const Center(child: Text("No Data"));
                      }),
                ),
                Visibility(
                  visible: selected == "3" && selectedSub == "2",
                  child: FutureBuilder(
                      future: reservationFirebase
                          .getProviderRequestReservationsByUserId(
                              auth.value.uid),
                      builder: (context, snapshot) {
                        if (snapshot.connectionState ==
                            ConnectionState.waiting) {
                          return const Center(
                            child: CircularProgressIndicator(),
                          );
                        } else if (snapshot.hasError) {
                          return Text("Error: ${snapshot.error}");
                        } else if (snapshot.hasData) {
                          var reservations =
                              snapshot.data as List<RequestReservationModel>;
                          if (reservations.isEmpty) {
                            return Container(
                              padding: const EdgeInsets.all(20),
                              alignment: AlignmentDirectional.center,
                              child: SubTxtWidget("No Reservations Found"),
                            );
                          }
                          return ListView.builder(
                              itemCount: reservations.length,
                              primary: false,
                              shrinkWrap: true,
                              itemBuilder: (context, index) {
                                var reservation = reservations[index];
                                return FutureBuilder(
                                    future: userFirebase
                                        .getUser(reservation.providerId),
                                    builder: (context, snapshot) {
                                      if (snapshot.connectionState ==
                                          ConnectionState.waiting) {
                                        return const Center(
                                          child: CircularProgressIndicator(),
                                        );
                                      } else if (snapshot.hasError) {
                                        return Text("Error: ${snapshot.error}");
                                      } else if (snapshot.hasData) {
                                        var user = snapshot.data as UserModel;
                                        return Container(
                                          decoration: BoxDecoration(
                                            borderRadius:
                                                BorderRadius.circular(8),
                                          ),
                                          child: Card(
                                            child: Row(
                                              children: [
                                                // ClipRRect(
                                                //   borderRadius:
                                                //       const BorderRadius.only(
                                                //           topLeft:
                                                //               Radius.circular(
                                                //                   10),
                                                //           bottomLeft:
                                                //               Radius.circular(
                                                //                   10)),
                                                //   child:
                                                //   CachedNetworkImage(
                                                //       height: 90,
                                                //       width: 100,
                                                //       fit: BoxFit.fill,
                                                //       imageUrl: user
                                                //           .providerUserModel!
                                                //           .providerImages!
                                                //           .first),
                                                // ),
                                                ClipRRect(
                                                  borderRadius:
                                                  const BorderRadius.only(
                                                      topLeft:
                                                      Radius.circular(
                                                          10),
                                                      bottomLeft:
                                                      Radius.circular(
                                                          10)),
                                                  child: user.providerUserModel != null &&
                                                      user.providerUserModel!.providerImages != null &&
                                                      user.providerUserModel!.providerImages!.isNotEmpty
                                                      ? CachedNetworkImage(
                                                      height: 90,
                                                      width: 100,
                                                      fit: BoxFit.fill,
                                                      imageUrl:
                                                      user.providerUserModel!.providerImages!.first)
                                                      : const SizedBox(
                                                      height: 90,
                                                      width: 100,
                                                      child: Center(
                                                        child: Icon(
                                                          Icons.image_not_supported,
                                                          size: 30,
                                                        ),
                                                      )),
                                                ),
                                                const SizedBox(
                                                  width: 6,
                                                ),
                                                Column(
                                                  mainAxisAlignment:
                                                      MainAxisAlignment.start,
                                                  crossAxisAlignment:
                                                      CrossAxisAlignment.start,
                                                  children: [
                                                    user.providerUserModel!
                                                                .providerType !=
                                                            "Independent"
                                                        ? Text(
                                                            user.providerUserModel!
                                                                .salonTitle!,
                                                            style: TextStyle(
                                                                fontSize: 16,
                                                                fontWeight:
                                                                    FontWeight
                                                                        .bold),
                                                          )
                                                        : Text(
                                                            user.fullName,
                                                            style: const TextStyle(
                                                                fontSize: 16,
                                                                fontWeight:
                                                                    FontWeight
                                                                        .bold),
                                                          ),
                                                    Text(formatDateInt(
                                                        reservation
                                                            .selectedDate)),
                                                    Text(formatTimeInt(
                                                        reservation
                                                            .selectedDate)),
                                                  ],
                                                ),
                                              ],
                                            ),
                                          ),
                                        );
                                      }
                                      return Text("No data found");
                                    });
                              });
                        }
                        return const Center(child: Text("No Data"));
                      }),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
