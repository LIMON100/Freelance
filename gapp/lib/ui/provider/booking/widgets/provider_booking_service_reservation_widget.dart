import 'package:cached_network_image/cached_network_image.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import '../../../../utils/tools.dart';
import '../../../../data_models/service_reservation_model.dart';
import '../../../../data_models/user_model.dart';
import '../../../../firebase/provider_service_firebase.dart';
import '../../../../firebase/reservation_firebase.dart';
import '../../../../firebase/service_booking_firebase.dart';
import '../../../../firebase/user_firebase.dart';
import '../../../../repo/setting_repo.dart';
import '../../../../screens/customer_screens/customer_rating_screen.dart';
import '../../../../utils/utils.dart';
import '../../../customer/booking/cancel_booking_screen.dart';
import '../../../customer/customer_dashboard/customer_dashboard_page.dart';

class ProviderServiceReservationWidget extends StatefulWidget {
  bool showAppbar;
   ProviderServiceReservationWidget({super.key,this.showAppbar=false});

  @override
  State<ProviderServiceReservationWidget> createState() =>
      _ProviderServiceReservationWidgetState();
}

class _ProviderServiceReservationWidgetState
    extends State<ProviderServiceReservationWidget> {
  ReservationFirebase reservationFirebase = ReservationFirebase();
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  UserFirebase userFirebase = UserFirebase();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar:widget.showAppbar? AppBar(
        title: HeaderTxtWidget('Booking',color: Colors.white,),
      ):null,
      body: FutureBuilder(
          future: reservationFirebase.getProviderServiceReservationsByUserId(
              auth.value.uid),
          builder: (context, snapshot) {
            if (snapshot.connectionState == ConnectionState.waiting) {
              return const Center(
                child: CircularProgressIndicator(),
              );
            } else if (snapshot.hasError) {
              return Text("Error: ${snapshot.error}");
            } else if (snapshot.hasData) {
              var reservations = snapshot.data as List<ServiceReservationModel>;
              if (reservations.isEmpty) {
                return const Center(child: Text("No Reservations Found"));
              }
              return SizedBox(
                height: MediaQuery.sizeOf(context).height,
                child: ListView.builder(
                    itemCount: reservations.length,
                    itemBuilder: (context, index) {
                      var reservation = reservations[index];
                      return FutureBuilder(
                          future: providerServiceFirebase.getServiceByServiceId(reservation.serviceId),
                          builder: (context, snapshot) {
                            if (snapshot.connectionState == ConnectionState.waiting) {
                              return const Center(
                                child: CircularProgressIndicator(),
                              );
                            } else if (snapshot.hasError) {
                              return Text("Error: ${snapshot.error}");
                            } else if (snapshot.hasData) {
                              var service = snapshot.data;
                              return FutureBuilder(
                                  future: userFirebase.getUser(service!.userId),
                                  builder: (context, snapshot) {
                                    if (snapshot.connectionState == ConnectionState.waiting) {
                                      return const Center(
                                        child: CircularProgressIndicator(),
                                      );
                                    } else if (snapshot.hasError) {
                                      return Text("Error: ${snapshot.error}");
                                    } else if (snapshot.hasData) {
                                      var user = snapshot.data as UserModel;
                                      bool isPast = DateTime.now().isBefore(Tools.changeToDate(reservation.selectedDate));
                                      return Container(
                                        padding: const EdgeInsets.all(12.0),
                                        decoration: BoxDecoration(
                                          borderRadius: BorderRadius.circular(8),
                                          border: Border.all(
                                            color: getBorderColor(service.serviceType),
                                          ),
                                        ),
                                        child: Row(
                                          children: [
                                            ClipRRect(
                                              borderRadius: BorderRadius.circular(8),
                                              child: CachedNetworkImage(
                                                fit: BoxFit.fill,
                                                imageUrl: service
                                                    .serviceImages!.first,
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
                                                MainAxisAlignment.start,
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
                                                    style: TextStyle(
                                                        fontSize: 16,
                                                        fontWeight:
                                                        FontWeight
                                                            .bold),
                                                  )
                                                      : Text(
                                                    user.fullName
                                                        .capitalizeFirst!,
                                                    style: TextStyle(
                                                        fontSize: 16,
                                                        fontWeight:
                                                        FontWeight
                                                            .bold),
                                                  ),
                                                  Text(
                                                    service.serviceType,
                                                    style: TextStyle(
                                                        fontSize: 12,
                                                        fontWeight:
                                                        FontWeight.w600,
                                                        color: Colors.blue),
                                                  ),
                                                  Row(
                                                    children: [
                                                      Icon(
                                                        Icons
                                                            .calendar_month,
                                                        color: Colors
                                                            .blueAccent,
                                                      ),
                                                      Text(
                                                          "${Tools.changeDateFormat(reservation.selectedDate, 'MM-dd-yyyy')}"),
                                                    ],
                                                  ),
                                                  Row(
                                                    mainAxisSize:
                                                    MainAxisSize.max,
                                                    mainAxisAlignment:
                                                    MainAxisAlignment
                                                        .spaceBetween,
                                                    children: [
                                                      Row(
                                                        children: [
                                                          Icon(
                                                            Icons.timelapse,
                                                            color: Colors
                                                                .blueAccent,
                                                          ),
                                                          Text(
                                                              "${Tools.changeDateFormat(reservation.selectedDate, 'hh:mm a')}"),
                                                        ],
                                                      ),
                                                      TextButton(
                                                        onPressed: () {
                                                          if (isPast) {
                                                            Get.to(
                                                                  () =>
                                                                  FeedbackScreen(
                                                                    providerId:
                                                                    user.uid,
                                                                    serviceId:
                                                                    reservation
                                                                        .serviceId,
                                                                  ),
                                                            );
                                                          } else {
                                                            Get.to(CancelBookingScreen(data: reservation, success: () async {
                                                              Get.to(() =>
                                                                  CustomerDashboardPage());
                                                            },canceled_by: "PROVIDER",));
                                                          }
                                                        },
                                                        style: ButtonStyle(
                                                            backgroundColor:
                                                            MaterialStateProperty.all(isPast
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
                                          ],
                                        ),
                                      );
                                    }
                                    return const Text('No Data');
                                  });
                            }
                            return const Text("No Data");
                          });
                    }),
              );
            }
            return const Center(child: Text("No Data"));
          }),
    );
  }
}
