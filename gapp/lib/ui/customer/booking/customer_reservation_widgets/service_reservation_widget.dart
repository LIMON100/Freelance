import 'package:cached_network_image/cached_network_image.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/loading_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

import '../../../../data_models/service_reservation_model.dart';
import '../../../../data_models/user_model.dart';
import '../../../../firebase/provider_service_firebase.dart';
import '../../../../firebase/reservation_firebase.dart';
import '../../../../firebase/service_booking_firebase.dart';
import '../../../../firebase/user_firebase.dart';
import '../../../../screens/customer_screens/customer_rating_screen.dart';
import '../../../../utils/tools.dart';
import '../../../../utils/utils.dart';
import '../../../../widgets/header_txt_widget.dart';
import '../cancel_booking_screen.dart';

class ServiceReservationWidget extends StatefulWidget {
  const ServiceReservationWidget({super.key});

  @override
  State<ServiceReservationWidget> createState() =>
      _ServiceReservationWidgetState();
}

class _ServiceReservationWidgetState extends State<ServiceReservationWidget> {
  ReservationFirebase reservationFirebase = ReservationFirebase();
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  UserFirebase userFirebase = UserFirebase();

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      height: MediaQuery.sizeOf(context).height,
      child: FutureBuilder(
          future: reservationFirebase
              .getServiceReservationsByUserId(auth.value.uid),
          builder: (context, snapshot) {
            if (snapshot.connectionState == ConnectionState.waiting) {
              return LoadingWidget(
                type: LoadingType.LIST,
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
                  padding: EdgeInsets.zero,
                    itemCount: reservations.length,
                    itemBuilder: (context, index) {
                      var booking = reservations[index];
                      var service = booking.serviceDetails;
                      return FutureBuilder(
                          future: userFirebase.getUser(service!.userId),
                          builder: (context, snapshot) {
                            if (snapshot.connectionState ==
                                ConnectionState.waiting) {
                              return const Center(
                                child: CircularProgressIndicator(),
                              );
                            } else if (snapshot.hasError) {
                              return const Text("User not Data found");
                            } else if (snapshot.hasData) {
                              var user = snapshot.data as UserModel;
                              return Container(
                                margin: const EdgeInsets.symmetric(horizontal: 10),
                                decoration: BoxDecoration(
                                    borderRadius: BorderRadius.circular(8.0),
                                    color: Colors.white,
                                    boxShadow: [
                                      BoxShadow(
                                          color: Colors.grey.shade200,
                                          blurRadius: 2,
                                          spreadRadius: 3
                                      )
                                    ]
                                ),
                                padding: const EdgeInsets.all(5),
                                child: Row(
                                  mainAxisAlignment: MainAxisAlignment.start,
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    Expanded(
                                      child: Column(
                                        crossAxisAlignment: CrossAxisAlignment.start,
                                        mainAxisAlignment: MainAxisAlignment.start,
                                        children: [
                                          Row(
                                            children: [
                                              Expanded(child:  HeaderTxtWidget('${service.serviceName}'),),
                                              Container(
                                                padding: const EdgeInsets.symmetric(horizontal: 5,vertical: 2),
                                                decoration: BoxDecoration(
                                                    color: "#EB833C".toColor().withOpacity(0.1),
                                                    borderRadius: BorderRadius.circular(10)
                                                ),
                                                child: SubTxtWidget(booking.status,color: "#EB833C".toColor(),fontSize: 12,),
                                              )
                                            ],),
                                          Row(
                                            mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                            children: [
                                              user!.providerUserModel!
                                                  .providerType !=
                                                  "Independent"
                                                  ? SubTxtWidget(
                                                user.providerUserModel!
                                                    .salonTitle!
                                                    .capitalizeFirst!,
                                                color: Colors.blue,
                                                fontSize: 14,
                                              )
                                                  : SubTxtWidget(
                                                user.fullName
                                                    .capitalizeFirst!,
                                                color: Colors.blue,
                                                fontSize: 14,
                                              ),
                                            ],
                                          ),
                                          Row(
                                            children: [
                                              Row(
                                                children: [
                                                  const Icon(Icons.calendar_month,
                                                    color: Colors.blue,size: 14,),
                                                  Text(Tools.changeDateFormat(booking.selectedDate, 'MM-dd-yyyy')),
                                                ],
                                              ),
                                              Spacer(),
                                              const Icon(
                                                  Icons
                                                      .monetization_on_rounded,
                                                  size: 14,
                                                  color: Colors.green),
                                              Text(
                                                "${service.servicePrice} \$",
                                              ),
                                            ],
                                          ),
                                          const SizedBox(height: 5,),
                                          _button(booking, user),
                                        ],
                                      ),
                                    ),
                                    const SizedBox(
                                      width: 7,
                                    ),
                                    ClipRRect(
                                      borderRadius: BorderRadius.circular(8),
                                      child: CachedNetworkImage(
                                        fit: BoxFit.fill,
                                        width: 100,
                                        height: 120,
                                        imageUrl: service.serviceImages!.first,
                                      ),
                                    ),
                                  ],
                                ),
                              );

                            }
                            return Text('No Data');
                          });
                    }),
              );
            }
            return const Center(child: Text("No Data"));
          }),
    );
  }

  Widget _button(ServiceReservationModel reservation, user) {
    if (reservation.status == "Canceled"||reservation.review_status=="1") {
      return const SizedBox();
    }
    bool ispast=DateTime.now().isBefore(Tools.changeToDate(reservation.selectedDate));
    if (reservation.status == "Reserved"&&ispast) {
      return TextButton(
        onPressed: () {
          Get.to(CancelBookingScreen(data: reservation, success: () async {
            setState(() {});
          },canceled_by: "USER",));
        },
        style:
            ButtonStyle(backgroundColor: WidgetStateProperty.all(Colors.red)),
        child: const Text(
          "Cancel Reservation",
          style: TextStyle(fontWeight: FontWeight.bold, color: Colors.white),
        ),
      );
    }
    return TextButton(
      onPressed: () {
        Get.to(
          () => FeedbackScreen(
            providerId: user.uid,
            serviceId: reservation.serviceId,
            onSuccess: () async {
              await reservationFirebase.updateReviewServiceReservation(reservation.reservationId);
              setState(() {});
            },
          ),
        );
      },
      style:
          ButtonStyle(backgroundColor: WidgetStateProperty.all(Colors.green)),
      child: const Text(
        "Review Service",
        style: TextStyle(fontWeight: FontWeight.bold, color: Colors.white),
      ),
    );
  }
}
