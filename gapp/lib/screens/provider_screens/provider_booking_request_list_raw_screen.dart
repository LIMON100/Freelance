import 'package:flutter/material.dart';
import 'package:font_awesome_flutter/font_awesome_flutter.dart';
import 'package:get/get.dart';
import 'package:groom/firebase/customer_offer_firebase.dart';
import 'package:groom/firebase/provider_service_firebase.dart';
import 'package:groom/ui/provider/booking/provider_service_request_view_screen.dart';
import 'package:groom/states/provider_request_state.dart';
import 'package:intl/intl.dart';
import '../../data_models/customer_offer_model.dart';
import '../../utils/colors.dart';

class ProviderBookingRequestsListRawScreen extends StatefulWidget {
  const ProviderBookingRequestsListRawScreen({super.key});

  @override
  State<ProviderBookingRequestsListRawScreen> createState() =>
      _ProviderBookingRequestsListRawScreenState();
}

class _ProviderBookingRequestsListRawScreenState
    extends State<ProviderBookingRequestsListRawScreen> {
  CustomerOfferFirebase customerOfferFirebase = CustomerOfferFirebase();
  ProviderRequestState providerRequestState = Get.put(ProviderRequestState());
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();

  Color _getBorderColor(String serviceType) {
    switch (serviceType) {
      case 'Hair Style':
        return Colors.blue;
      case 'Nails':
        return Colors.pink;
      case 'Facial':
        return Colors.green;
      case 'Coloring':
        return Colors.purple;
      case 'Spa':
        return Colors.orange;
      case 'Wax':
        return Colors.red;
      case 'Makeup':
        return Colors.yellow;
      case 'Massage':
        return Colors.brown;
      default:
        return Colors.grey;
    }
  }

  String _getServiceIcon(String serviceType) {
    switch (serviceType) {
      case 'Hair Style':
        return 'assets/haircutIcon.png';
      case 'Nails':
        return 'assets/nailsIcon.png';
      case 'Facial':
        return 'assets/facialIcon.png';
      case 'Coloring':
        return 'assets/coloringIcon.png';
      case 'Spa':
        return 'assets/spaIcon.png';
      case 'Wax':
        return 'assets/waxingIcon.png';
      case 'Makeup':
        return 'assets/makeupIcon.png';
      case 'Massage':
        return 'assets/massageIcon.png';
      default:
        return 'assets/spaIcon.png';
    }
  }

  String _formatDate(DateTime date) {
    final DateFormat formatter = DateFormat('d MMMM');
    return formatter.format(date);
  }

  String _formatTime(DateTime date) {
    final DateFormat timeFormatter = DateFormat('HH:mm');
    return timeFormatter.format(date);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(),
      body: Column(
        children: [
          Container(
            height: 40,
            margin: const EdgeInsets.symmetric(horizontal: 10,vertical: 10),
            decoration: BoxDecoration(
              border: Border.all(color: Colors.blue.shade200),
              borderRadius: BorderRadius.circular(25),
            ),
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 8.0),
              child: Row(
                children: [
                  Expanded(
                    child: TextField(
                      onChanged: (value) {},
                      decoration: const InputDecoration(
                        border: InputBorder.none,
                        hintText: 'Search...',
                      ),
                    ),
                  ),
                  Icon(
                    Icons.search,
                    size: 24,
                  ),
                ],
              ),
            ),
          ),
          Expanded(
            child: FutureBuilder(
              future: customerOfferFirebase.getAllOffers(),
              builder: (context, snapshot) {
                if (snapshot.connectionState == ConnectionState.waiting) {
                  return const Center(
                    child: CircularProgressIndicator(),
                  );
                } else if (snapshot.hasError) {
                  return Text("Error: ${snapshot.error}");
                } else if (snapshot.hasData) {
                  var services =
                  snapshot.data as List<CustomerOfferModel>;
                  return ListView.builder(
                      itemCount: services.length,
                      itemBuilder: (context, index) {
                        var service = services[index];
                        return FutureBuilder(
                            future: providerServiceFirebase
                                .getRequestOfferByServiceId(
                                service.offerId),
                            builder: (context, snapshot) {
                              if (snapshot.connectionState ==
                                  ConnectionState.waiting) {
                                return Text("");
                              } else if (snapshot.hasError) {
                                return Text("Error: ${snapshot.error}");
                              } else if (snapshot.hasData) {
                                var request = snapshot.data;
                                return GestureDetector(
                                  onTap: () {
                                    providerRequestState
                                        .selectedProviderRequest
                                        .value = services[index];
                                    Get.to(() =>
                                        ProviderServiceRequestViewScreen());
                                  },
                                  child: Padding(
                                    padding: const EdgeInsets.all(8.0),
                                    child: Column(
                                      children: [
                                        Container(
                                          decoration: BoxDecoration(
                                            border: Border.all(
                                              color: _getBorderColor(
                                                  service.serviceType!),
                                            ),
                                            borderRadius:
                                            BorderRadius.circular(
                                                8.0),
                                          ),
                                          child: Padding(
                                            padding: const EdgeInsets
                                                .symmetric(
                                                horizontal: 8.0),
                                            child: Row(
                                              mainAxisAlignment:
                                              MainAxisAlignment
                                                  .spaceBetween,
                                              crossAxisAlignment:
                                              CrossAxisAlignment
                                                  .center,
                                              children: [
                                                Column(
                                                  mainAxisAlignment:
                                                  MainAxisAlignment
                                                      .center,
                                                  crossAxisAlignment:
                                                  CrossAxisAlignment
                                                      .start,
                                                  children: [
                                                    Text(
                                                      service
                                                          .serviceType!,
                                                      textAlign:
                                                      TextAlign.start,
                                                      style: TextStyle(
                                                          fontSize: 20,
                                                          color: Colors
                                                              .black),
                                                    ),
                                                    Row(
                                                      children: [
                                                        Icon(
                                                          Icons
                                                              .calendar_month,
                                                          color:
                                                          mainBtnColor,
                                                        ),
                                                        SizedBox(
                                                          width: 10,
                                                        ),
                                                        Text(
                                                          _formatDate(service
                                                              .dateTime!),
                                                          textAlign:
                                                          TextAlign
                                                              .start,
                                                          style: TextStyle(
                                                              fontSize:
                                                              15,
                                                              color: Colors
                                                                  .black),
                                                        ),
                                                      ],
                                                    ),
                                                    Row(
                                                      children: [
                                                        Icon(
                                                          Icons.timelapse,
                                                          color:
                                                          mainBtnColor,
                                                        ),
                                                        SizedBox(
                                                          width: 10,
                                                        ),
                                                        Text(
                                                          _formatTime(service
                                                              .dateTime!),
                                                          textAlign:
                                                          TextAlign
                                                              .start,
                                                          style: TextStyle(
                                                              fontSize:
                                                              15,
                                                              color: Colors
                                                                  .black),
                                                        ),
                                                      ],
                                                    ),
                                                    Row(
                                                      children: [
                                                        Icon(
                                                          Icons
                                                              .price_change,
                                                          color:
                                                          mainBtnColor,
                                                        ),
                                                        SizedBox(
                                                          width: 10,
                                                        ),
                                                        Text(
                                                          "${service.priceRange!}  \$",
                                                          style: TextStyle(
                                                              fontSize:
                                                              15,
                                                              color: Colors
                                                                  .black),
                                                        ),
                                                      ],
                                                    ),
                                                  ],
                                                ),
                                                Column(
                                                  children: [
                                                    Row(
                                                      children: [
                                                        FaIcon(
                                                          FontAwesomeIcons
                                                              .eye,
                                                          size: 20,
                                                          color:
                                                          mainBtnColor,
                                                        ),
                                                        SizedBox(
                                                          width: 10,
                                                        ),
                                                        Text("120"),
                                                      ],
                                                    ),
                                                    Row(
                                                      children: [
                                                        Icon(
                                                          Icons
                                                              .local_offer,
                                                          color:
                                                          mainBtnColor,
                                                        ),
                                                        SizedBox(
                                                          width: 10,
                                                        ),
                                                        Text(request!
                                                            .length
                                                            .toString()),
                                                      ],
                                                    ),
                                                  ],
                                                ),
                                                Padding(
                                                  padding:
                                                  const EdgeInsets
                                                      .all(8.0),
                                                  child: Column(
                                                    children: [
                                                      CircleAvatar(
                                                        backgroundColor:
                                                        Colors.grey
                                                            .shade200,
                                                        radius: 35,
                                                        child: ClipOval(
                                                          child:
                                                          Image.asset(
                                                            _getServiceIcon(
                                                                service
                                                                    .serviceType!),
                                                            fit: BoxFit
                                                                .cover,
                                                            width: 40,
                                                            height: 40,
                                                          ),
                                                        ),
                                                      ),

                                                    ],
                                                  ),
                                                ),
                                              ],
                                            ),
                                          ),
                                        ),
                                      ],
                                    ),
                                  ),
                                );
                              }
                              return Text('No data');
                            });
                      });
                }
                return Text("No data");
              },
            ),
          ),
        ],
      ),
    );
  }
}
