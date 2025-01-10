import 'package:cached_network_image/cached_network_image.dart';
import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/firebase/notifications_firebase.dart';
import 'package:groom/firebase/provider_service_firebase.dart';
import 'package:groom/firebase/service_booking_firebase.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:intl/intl.dart';

import '../../Utils/tools.dart';
import '../../data_models/notification_model.dart';
import '../../data_models/provider_service_model.dart';
import '../../data_models/service_booking_model.dart';
import '../../data_models/service_reservation_model.dart';
import '../../data_models/user_model.dart';
import '../../firebase/firebase_notifications.dart';
import '../../firebase/reservation_firebase.dart';
import '../../firebase/user_firebase.dart';
import '../../states/customer_service_state.dart';
import '../../utils/colors.dart';
import '../../utils/utils.dart';
import '../../widgets/header_txt_widget.dart';
import '../../widgets/sub_txt_widget.dart';
import '../provider_screens/provider_booking_service_display_screen.dart';
import '../../ui/customer/services/provider_details_screen.dart';

class ProviderBookingServiceListScreen extends StatefulWidget {
  const ProviderBookingServiceListScreen({super.key});

  @override
  State<ProviderBookingServiceListScreen> createState() =>
      _ProviderBookingServiceListScreenState();
}

class _ProviderBookingServiceListScreenState
    extends State<ProviderBookingServiceListScreen> {
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  UserFirebase userFirebase = UserFirebase();
  CustomerServiceState customerServiceState = Get.put(CustomerServiceState());
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();

  Future<UserModel> fetchUser(String userId) async {
    return await userFirebase.getUser(userId);
  }

  @override
  Widget build(BuildContext context) {
    return FutureBuilder(
      future: serviceBookingFirebase.getServiceBookingByProviderId(auth.value.uid),
      builder: (context, snapshot) {
        if (snapshot.connectionState == ConnectionState.waiting) {
          return const Center(
            child: CircularProgressIndicator(),
          );
        } else if (snapshot.hasError) {
          return Text("Error: ${snapshot.error}");
        } else if (snapshot.hasData) {
          var reservations = snapshot.data as List<ServiceBookingModel>;
          return ListView.builder(
              scrollDirection: Axis.vertical,
              itemCount: reservations.length,
              shrinkWrap: true,
              primary: false,
              padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 10),
              itemBuilder: (context, index) {
                var reservation = reservations[index];
                var service = reservation.serviceDetails;
                return Container(
                  margin: const EdgeInsets.symmetric(vertical: 5),
                  decoration: BoxDecoration(
                      borderRadius: BorderRadius.circular(8.0),
                      color: Colors.white,
                      boxShadow: [
                        BoxShadow(
                            color: Colors.grey.shade200,
                            blurRadius: 2,
                            spreadRadius: 3)
                      ]),
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
                                Expanded(
                                  child: HeaderTxtWidget(
                                      '${service!.serviceName}'),
                                ),
                                Container(
                                  padding: const EdgeInsets.symmetric(
                                      horizontal: 5, vertical: 2),
                                  decoration: BoxDecoration(
                                      color: "#EB833C"
                                          .toColor()
                                          .withOpacity(0.1),
                                      borderRadius:
                                      BorderRadius.circular(10)),
                                  child: SubTxtWidget(
                                    reservation.status,
                                    color: "#EB833C".toColor(),
                                    fontSize: 12,
                                  ),
                                )
                              ],
                            ),
                            Row(
                              children: [
                                Row(
                                  children: [
                                    const Icon(
                                      Icons.calendar_month,
                                      color: Colors.blue,
                                      size: 14,
                                    ),
                                    Text(Tools.changeDateFormat(
                                        reservation.selectedDate,
                                        globalTimeFormat)),
                                  ],
                                ),
                                Spacer(),
                                const Icon(Icons.monetization_on_rounded,
                                    size: 14, color: Colors.green),
                                Text(
                                  "${service.servicePrice} \$",
                                ),
                              ],
                            ),
                            Row(
                              children: [
                                Row(
                                  children: [
                                    Icon(Icons.timelapse,
                                        size: 14, color: Colors.blue),
                                    Text(reservation.selectedTime),
                                  ],
                                ),
                                Spacer(),
                              ],
                            ),
                            const SizedBox(
                              height: 5,
                            ),
                            Row(
                              mainAxisAlignment: reservation.status=="Pending"?MainAxisAlignment.spaceEvenly:MainAxisAlignment.start,
                              children: [
                                if(reservation.status=="Pending")
                                  ActionChip(
                                    label: SubTxtWidget(
                                      "Accept",
                                      fontSize: 14,
                                      color: Colors.white,
                                    ),
                                    padding: const EdgeInsets.symmetric(
                                        horizontal: 10, vertical: 5),
                                    onPressed: () {
                                      acceptBooking(reservation);
                                    },
                                    shape: const StadiumBorder(
                                        side: BorderSide(
                                            color: Colors.transparent)),
                                    color: WidgetStatePropertyAll(
                                        primaryColorCode),
                                  ),
                                ActionChip(
                                    label: SubTxtWidget(
                                      "View",
                                      fontSize: 14,
                                    ),
                                    padding: const EdgeInsets.symmetric(
                                        horizontal: 10, vertical: 5),
                                    onPressed: () {
                                      customerServiceState
                                          .selectedService.value = service;
                                      Get.to(() =>
                                      const ProviderBookingServiceDisplayScreen());
                                    },
                                    shape: const StadiumBorder())
                              ],
                            ),
                            const SizedBox(
                              height: 5,
                            ),
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
              });
        } else {
          return const Text("no data found");
        }
      },
    );
  }

  Future<void> acceptBooking(ServiceBookingModel data) async {
    UserModel user = await userFirebase.getUser(data.clientId);
    Get.dialog(AlertDialog(
      title: HeaderTxtWidget("Request Accept Confirmation"),
      content: SizedBox(
        height: MediaQuery.sizeOf(context).height * 0.2,
        child: Column(
          children: [
            SubTxtWidget(
                "Deposit payment will be sent to the client, You will be notified when the reservation is confirmed")
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: () {
            Get.back();
          },
          child: Text("Cancel"),
        ),
        TextButton(
          onPressed: () async {
            bool result = await serviceBookingFirebase.updateBookingStatus(
                data.bookingId, "Confirmed");
            if (result) {
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(
                  content: Text('Booking confirmed'),
                  backgroundColor: Colors.green,
                ),
              );

              PushNotificationService
                  .sendProviderConfirmReservationNotificationToClient(
                      user.notificationToken!,
                      context,
                      "",
                      auth.value.providerUserModel!.providerType != "Independent"
                          ? auth.value.providerUserModel!.salonTitle!
                          : auth.value.fullName);

              NotificationModel notificationModel = NotificationModel(
                  userId: user.uid,
                  createdOn: DateTime.now().millisecondsSinceEpoch,
                  notificationText:
                      "Your Groomer has accepted your service request",
                  type: "SERVICE",
                  sendBy: "PROVIDER",
                  serviceId: data.serviceId,
                  notificationId: generateProjectId());
              NotificationsFirebase().writeNotificationToUser(
                  user.uid, notificationModel, generateProjectId());

              Get.back();
              setState(() {
                data.status = "Confirmed";
              });
            } else {
              Tools.ShowErrorMessage('Failed to confirm booking');
            }
          },
          child: Text("Confirm"),
        ),
      ],
    ));
  }
}
