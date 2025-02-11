// import 'package:cached_network_image/cached_network_image.dart';
// import 'package:firebase_auth/firebase_auth.dart';
// import 'package:flutter/material.dart';
// import 'package:groom/data_models/request_reservation_model.dart';
//
// import '../../../../data_models/user_model.dart';
// import '../../../../firebase/provider_service_firebase.dart';
// import '../../../../firebase/reservation_firebase.dart';
// import '../../../../firebase/service_booking_firebase.dart';
// import '../../../../firebase/user_firebase.dart';
// import '../../../../utils/utils.dart';
//
//
// class RequestReservationWidget extends StatefulWidget {
//   const RequestReservationWidget({super.key});
//
//   @override
//   State<RequestReservationWidget> createState() =>
//       _RequestReservationWidgetState();
// }
//
// class _RequestReservationWidgetState extends State<RequestReservationWidget> {
//   ReservationFirebase reservationFirebase = ReservationFirebase();
//   ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
//   ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
//   UserFirebase userFirebase = UserFirebase();
//
//   @override
//   Widget build(BuildContext context) {
//     print("REQUESWT RESET Here....");
//     return SizedBox(
//       width: MediaQuery.sizeOf(context).width,
//       height: MediaQuery.sizeOf(context).height,
//       child: FutureBuilder(
//           future: reservationFirebase.getRequestReservationsByUserId(
//               FirebaseAuth.instance.currentUser!.uid),
//           builder: (context, snapshot) {
//             if (snapshot.connectionState == ConnectionState.waiting) {
//               return const Center(
//                 child: CircularProgressIndicator(),
//               );
//             } else if (snapshot.hasError) {
//               return Text("Error: ${snapshot.error}");
//             } else if (snapshot.hasData) {
//               var reservations = snapshot.data as List<RequestReservationModel>;
//               if (reservations.isEmpty) {
//                 return const Center(child: Text("No Reservations Found"));
//               }
//               return ListView.builder(
//                   padding: EdgeInsets.zero,
//                   itemCount: reservations.length,
//                   itemBuilder: (context, index) {
//                     var reservation = reservations[index];
//                     return FutureBuilder(
//                         future: userFirebase.getUser(reservation.providerId),
//                         builder: (context, snapshot) {
//                           if (snapshot.connectionState ==
//                               ConnectionState.waiting) {
//                             return const Center(
//                               child: CircularProgressIndicator(),
//                             );
//                           } else if (snapshot.hasError) {
//                             return Text("Error: ${snapshot.error}");
//                           } else if (snapshot.hasData) {
//                             var user = snapshot.data as UserModel;
//                             return Container(
//                               decoration: BoxDecoration(
//                                 borderRadius: BorderRadius.circular(8),
//                               ),
//                               child: Card(
//                                 child: Row(
//                                   children: [
//                                     // CachedNetworkImage(
//                                     //     height: 100,
//                                     //     width: 90,
//                                     //     fit: BoxFit.fill,
//                                     //     imageUrl: user.providerUserModel!
//                                     //         .providerImages!.first
//                                     // ),
//                                     CachedNetworkImage(
//                                       height: 100,
//                                       width: 90,
//                                       fit: BoxFit.fill,
//                                       imageUrl: user.providerUserModel != null &&
//                                           user.providerUserModel!.providerImages != null &&
//                                           user.providerUserModel!.providerImages!.isNotEmpty
//                                           ? user.providerUserModel!.providerImages!.first
//                                           : '',
//                                     ),
//                                     const SizedBox(
//                                       width: 6,
//                                     ),
//                                     Column(
//                                       mainAxisAlignment: MainAxisAlignment.start,
//                                       crossAxisAlignment: CrossAxisAlignment.start,
//                                       children: [
//                                         user.providerUserModel!
//                                             .providerType !=
//                                             "Independent"
//                                             ? Text(
//                                           user.providerUserModel!
//                                               .salonTitle!,
//                                           style: TextStyle(
//                                               fontSize: 16,
//                                               fontWeight:
//                                               FontWeight.bold),
//                                         )
//                                             : Text(
//                                           user.fullName,
//                                           style: TextStyle(
//                                               fontSize: 16,
//                                               fontWeight:
//                                               FontWeight.bold),
//                                         ),
//                                         Text(formatDateInt(
//                                             reservation.selectedDate)),
//                                         Text(formatTimeInt(
//                                             reservation.selectedDate)),
//                                       ],
//                                     ),
//                                   ],
//                                 ),
//                               ),
//                             );
//                           }
//                           return Text("No data found");
//                         });
//                   });
//             }
//             return const Center(child: Text("No Data"));
//           }),
//     );
//   }
// }


import 'package:cached_network_image/cached_network_image.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:groom/data_models/request_reservation_model.dart';

import '../../../../data_models/user_model.dart';
import '../../../../firebase/provider_service_firebase.dart';
import '../../../../firebase/reservation_firebase.dart';
import '../../../../firebase/service_booking_firebase.dart';
import '../../../../firebase/user_firebase.dart';
import '../../../../utils/utils.dart';

class RequestReservationWidget extends StatefulWidget {
  const RequestReservationWidget({super.key});

  @override
  State<RequestReservationWidget> createState() =>
      _RequestReservationWidgetState();
}

class _RequestReservationWidgetState extends State<RequestReservationWidget> {
  ReservationFirebase reservationFirebase = ReservationFirebase();
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  UserFirebase userFirebase = UserFirebase();

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: MediaQuery.sizeOf(context).width,
      height: MediaQuery.sizeOf(context).height,
      child: FutureBuilder(
          future: reservationFirebase.getRequestReservationsByUserId(
            FirebaseAuth.instance.currentUser!.uid,
          ),
          builder: (context, snapshot) {
            if (snapshot.connectionState == ConnectionState.waiting) {
              return const Center(
                child: CircularProgressIndicator(),
              );
            } else if (snapshot.hasError) {
              return Text("Error: ${snapshot.error}");
            } else if (snapshot.hasData) {
              var reservations = snapshot.data as List<RequestReservationModel>;
              if (reservations.isEmpty) {
                return const Center(child: Text("No Reservations Found"));
              }
              // Sort reservations by selectedDate in descending order
              reservations.sort((a, b) => b.selectedDate!.compareTo(a.selectedDate!));
              return ListView.builder(
                  padding: EdgeInsets.zero,
                  itemCount: reservations.length,
                  itemBuilder: (context, index) {
                    var reservation = reservations[index];
                    return FutureBuilder(
                        future: userFirebase.getUser(reservation.providerId),
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
                                borderRadius: BorderRadius.circular(8),
                              ),
                              child: Card(
                                child: Row(
                                  children: [
                                    CachedNetworkImage(
                                      height: 100,
                                      width: 90,
                                      fit: BoxFit.fill,
                                      imageUrl: user.providerUserModel != null &&
                                          user.providerUserModel!.providerImages != null &&
                                          user.providerUserModel!.providerImages!.isNotEmpty
                                          ? user.providerUserModel!.providerImages!.first
                                          : '',
                                    ),
                                    const SizedBox(
                                      width: 6,
                                    ),
                                    Column(
                                      mainAxisAlignment: MainAxisAlignment.start,
                                      crossAxisAlignment: CrossAxisAlignment.start,
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
                                              FontWeight.bold),
                                        )
                                            : Text(
                                          user.fullName,
                                          style: TextStyle(
                                              fontSize: 16,
                                              fontWeight:
                                              FontWeight.bold),
                                        ),
                                        Text(formatDateInt(
                                            reservation.selectedDate)),
                                        Text(formatTimeInt(
                                            reservation.selectedDate)),
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
    );
  }
}