import 'package:cached_network_image/cached_network_image.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:googleapis/admob/v1.dart';
import 'package:groom/firebase/provider_service_firebase.dart';
import 'package:groom/firebase/service_booking_firebase.dart';
import '../../data_models/provider_service_model.dart';
import '../../data_models/service_booking_model.dart';
import '../../data_models/user_model.dart';
import '../../firebase/user_firebase.dart';
import '../../states/customer_service_state.dart';
import '../../utils/colors.dart';
import '../provider_screens/provider_booking_service_display_screen.dart';

class ProviderBookingServiceListRawScreen extends StatefulWidget {
  const ProviderBookingServiceListRawScreen({super.key});

  @override
  State<ProviderBookingServiceListRawScreen> createState() =>
      _ProviderBookingServiceListRawScreenState();
}

class _ProviderBookingServiceListRawScreenState
    extends State<ProviderBookingServiceListRawScreen> {
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  UserFirebase userFirebase = UserFirebase();
  CustomerServiceState customerServiceState = Get.put(CustomerServiceState());
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();

  Future<UserModel> fetchUser(String userId) async {
    return await userFirebase.getUser(userId);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(),
      body: Column(
        children: [
          Expanded(
              child: FutureBuilder(
                future: providerServiceFirebase
                    .getAllServicesByUserId(FirebaseAuth.instance.currentUser!.uid),
                builder: (context, snapshot) {
                  if (snapshot.connectionState == ConnectionState.waiting) {
                    return const Center(
                      child: CircularProgressIndicator(),
                    );
                  } else if (snapshot.hasError) {
                    return Text("Error: ${snapshot.error}");
                  } else if (snapshot.hasData) {
                    var services = snapshot.data as List<ProviderServiceModel>;

                    return ListView.builder(
                        scrollDirection: Axis.vertical,
                        itemCount: services.length,
                        itemBuilder: (context, index) {
                          var service = services[index];
                          return FutureBuilder<UserModel>(
                            future: fetchUser(service.userId),
                            builder: (context, snapshot) {
                              if (snapshot.connectionState ==
                                  ConnectionState.waiting) {
                                return const Text("");
                              } else if (snapshot.hasError) {
                                return Text("Error: ${snapshot.error}");
                              } else if (snapshot.hasData) {
                                var userModelService = snapshot.data;

                                return FutureBuilder(
                                    future: serviceBookingFirebase
                                        .getServiceBookingByServiceId(
                                        service.serviceId),
                                    builder: (context, snapshot) {
                                      if (snapshot.connectionState ==
                                          ConnectionState.waiting) {
                                        return const Text("");
                                      } else if (snapshot.hasError) {
                                        return Text("Error: ${snapshot.error}");
                                      } else if (snapshot.hasData) {
                                        var services = snapshot.data
                                        as List<ServiceBookingModel>;
                                        return Padding(
                                          padding: const EdgeInsets.all(8.0),
                                          child: Column(
                                            crossAxisAlignment:
                                            CrossAxisAlignment.start,
                                            mainAxisAlignment:
                                            MainAxisAlignment.start,
                                            children: [
                                              GestureDetector(
                                                onTap: () {
                                                  customerServiceState.selectedService
                                                      .value = service;
                                                  Get.to(() =>
                                                      ProviderBookingServiceDisplayScreen());
                                                },
                                                child: Card(
                                                  child: Padding(
                                                    padding:
                                                    const EdgeInsets.all(8.0),
                                                    child: Row(
                                                      mainAxisAlignment:
                                                      MainAxisAlignment.start,
                                                      crossAxisAlignment:
                                                      CrossAxisAlignment.start,
                                                      children: [
                                                        Stack(
                                                          alignment:
                                                          Alignment.topRight,
                                                          children: [
                                                            ClipRRect(
                                                              borderRadius:
                                                              BorderRadius
                                                                  .circular(9),
                                                              child: SizedBox(
                                                                width: MediaQuery.of(
                                                                    context)
                                                                    .size
                                                                    .width *
                                                                    0.25,
                                                                height: MediaQuery.of(
                                                                    context)
                                                                    .size
                                                                    .height *
                                                                    0.12,
                                                                child:
                                                                CachedNetworkImage(
                                                                  imageUrl: service
                                                                      .serviceImages!
                                                                      .first,
                                                                  fit: BoxFit.fill,
                                                                ),
                                                              ),
                                                            ),
                                                          ],
                                                        ),
                                                        SizedBox(
                                                          width: 7,
                                                        ),
                                                        Container(
                                                          decoration: BoxDecoration(),
                                                          width:
                                                          MediaQuery.of(context)
                                                              .size
                                                              .width *
                                                              0.45,
                                                          height:
                                                          MediaQuery.of(context)
                                                              .size
                                                              .height *
                                                              0.12,
                                                          child: Column(
                                                            mainAxisAlignment:
                                                            MainAxisAlignment
                                                                .start,
                                                            crossAxisAlignment:
                                                            CrossAxisAlignment
                                                                .start,
                                                            children: [
                                                              Text(
                                                                service.serviceType,
                                                                textAlign:
                                                                TextAlign.start,
                                                                style: TextStyle(
                                                                    fontSize: 12,
                                                                    color: Colors.blue
                                                                        .shade400),
                                                              ),
                                                              userModelService!
                                                                  .providerUserModel!
                                                                  .providerType !=
                                                                  "Independent"
                                                                  ? Text(
                                                                userModelService
                                                                    .providerUserModel!
                                                                    .salonTitle!,
                                                                style: TextStyle(
                                                                    fontSize:
                                                                    18,
                                                                    fontWeight:
                                                                    FontWeight
                                                                        .bold),
                                                              )
                                                                  : Text(
                                                                userModelService
                                                                    .fullName ??
                                                                    'Unknown',
                                                                style: TextStyle(
                                                                    fontSize:
                                                                    18,
                                                                    fontWeight:
                                                                    FontWeight
                                                                        .bold),
                                                              ),
                                                              Container(
                                                                width: 190,
                                                                child: Text(
                                                                  userModelService
                                                                      .providerUserModel!
                                                                      .addressLine,
                                                                  overflow:
                                                                  TextOverflow
                                                                      .ellipsis,
                                                                ),
                                                              ),
                                                              Row(
                                                                mainAxisAlignment:
                                                                MainAxisAlignment
                                                                    .spaceBetween,
                                                                children: [
                                                                  Row(
                                                                    children: [
                                                                      Icon(
                                                                        Icons.star,
                                                                        color: Colors
                                                                            .orangeAccent
                                                                            .shade700,
                                                                      ),
                                                                      SizedBox(
                                                                        width: 3,
                                                                      ),
                                                                      Text(
                                                                          "4.8(3.1 ml)"),
                                                                    ],
                                                                  ),
                                                                  Row(
                                                                    children: [
                                                                      Icon(
                                                                        Icons
                                                                            .local_offer,
                                                                        color: Colors
                                                                            .blue,
                                                                      ),
                                                                      SizedBox(
                                                                        width: 3,
                                                                      ),
                                                                      Text(
                                                                        services
                                                                            .length
                                                                            .toString(),
                                                                        style: TextStyle(
                                                                            fontSize:
                                                                            15,
                                                                            fontWeight:
                                                                            FontWeight
                                                                                .bold),
                                                                      ),
                                                                    ],
                                                                  ),
                                                                ],
                                                              ),
                                                            ],
                                                          ),
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
                                      return Text("No Data!");
                                    });
                              } else {
                                return Text("no data found");
                              }
                            },
                          );
                        });
                  } else {
                    return Text("no data found");
                  }
                },
              ))
        ],
      ),
    );
  }
}
