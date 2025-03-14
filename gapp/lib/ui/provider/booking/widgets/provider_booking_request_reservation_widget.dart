import 'package:cached_network_image/cached_network_image.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:googleapis/admob/v1.dart';
import 'package:groom/data_models/request_reservation_model.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import '../../../../data_models/user_model.dart';
import '../../../../firebase/provider_service_firebase.dart';
import '../../../../firebase/reservation_firebase.dart';
import '../../../../firebase/service_booking_firebase.dart';
import '../../../../firebase/user_firebase.dart';
import '../../../../utils/utils.dart';

class ProviderRequestReservationWidget extends StatefulWidget {
  bool showAppBar;
   ProviderRequestReservationWidget({super.key,this.showAppBar=false});

  @override
  State<ProviderRequestReservationWidget> createState() =>
      _ProviderRequestReservationWidgetState();
}

class _ProviderRequestReservationWidgetState extends State<ProviderRequestReservationWidget> {
  ReservationFirebase reservationFirebase = ReservationFirebase();
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  UserFirebase userFirebase = UserFirebase();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar:widget.showAppBar? AppBar(
        title: HeaderTxtWidget('Service Request',color: Colors.white,),
      ):null,
      body: FutureBuilder(
          future: reservationFirebase.getProviderRequestReservationsByUserId(
              FirebaseAuth.instance.currentUser!.uid),
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
              return SizedBox(
                height: MediaQuery.sizeOf(context).height,
                child: ListView.builder(
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
                                      ClipRRect(
                                        borderRadius: const BorderRadius.only(topLeft: Radius.circular(10),
                                            bottomLeft: Radius.circular(10)),
                                        child: CachedNetworkImage(
                                            height: 90,
                                            width: 100,
                                            fit: BoxFit.fill,
                                            imageUrl: user.providerUserModel!.providerImages!.first),
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
                                            style: const TextStyle(
                                                fontSize: 16,
                                                fontWeight:
                                                FontWeight.bold),
                                          ),
                                          Text(formatDateInt(reservation.selectedDate)),
                                          Text(formatTimeInt(reservation.selectedDate)),
                                        ],
                                      ),
                                    ],
                                  ),
                                ),
                              );
                            }
                            return Text("No data found");
                          });
                    }),
              );
            }
            return Center(child: Text("No Data"));
          }),
    );
  }
}
