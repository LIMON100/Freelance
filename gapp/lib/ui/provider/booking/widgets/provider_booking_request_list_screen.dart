import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:font_awesome_flutter/font_awesome_flutter.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/firebase/customer_offer_firebase.dart';
import 'package:groom/firebase/provider_service_firebase.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/ui/provider/booking/provider_service_request_view_screen.dart';
import 'package:groom/states/provider_request_state.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import 'package:intl/intl.dart';

import '../../../../Utils/tools.dart';
import '../../../../data_models/customer_offer_model.dart';
import '../../../../generated/assets.dart';
import '../../../../utils/colors.dart';
import '../../../../utils/utils.dart';
import '../../../../widgets/network_image_widget.dart';

class ProviderBookingRequestsListScreen extends StatefulWidget {
  const ProviderBookingRequestsListScreen({super.key});

  @override
  State<ProviderBookingRequestsListScreen> createState() =>
      _ProviderBookingRequestsListScreenState();
}

class _ProviderBookingRequestsListScreenState
    extends State<ProviderBookingRequestsListScreen> {
  final TextEditingController _searchController = TextEditingController();

  CustomerOfferFirebase customerOfferFirebase = CustomerOfferFirebase();
  ProviderRequestState providerRequestState = Get.put(ProviderRequestState());
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  String _selectedFilter = "All";
  String _searchQuery = "";
  List<CustomerOfferModel> _services = [];



  @override
  void initState() {
    super.initState();
    _fetchServices();
  }
  void _fetchServices() async {
    var services = await customerOfferFirebase.getAllOffers();
    setState(() {
      _services = services;
    });
  }
  List<CustomerOfferModel> _getFilteredServices() {
    List<CustomerOfferModel> filteredServices = _services;
    if (_selectedFilter != "All") {
      filteredServices = filteredServices
          .where((service) => service.serviceType == _selectedFilter)
          .toList();
    }
    if (_searchQuery.isNotEmpty) {
      filteredServices = filteredServices
          .where((service) =>
      service.serviceType!
          .toLowerCase()
          .contains(_searchQuery.toLowerCase()) ||
          service.description
              .toLowerCase()
              .contains(_searchQuery.toLowerCase()))
          .toList();
    }


    return filteredServices;
  }
  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Container(
          padding: const EdgeInsets.only(
              bottom: 10, left: 10, right: 10,top: 10),
          alignment: AlignmentDirectional.topCenter,
          child: TextFormField(
            decoration: InputDecoration(
              fillColor: Colors.white,
              contentPadding: const EdgeInsets.symmetric(
                  horizontal: 5, vertical: 5),
              focusedBorder: OutlineInputBorder(
                borderRadius: BorderRadius.circular(10),
                borderSide: BorderSide(
                    color: Colors.black.withOpacity(0.1), width: 1),
              ),
              errorBorder: OutlineInputBorder(
                borderRadius: BorderRadius.circular(10),
                borderSide: BorderSide(
                    color: Colors.black.withOpacity(0.1), width: 1),
              ),
              errorStyle: TextStyle(color: errorColor),
              enabledBorder: OutlineInputBorder(
                borderRadius: BorderRadius.circular(10),
                borderSide: BorderSide(
                    color: Colors.black.withOpacity(0.1), width: 1),
              ),
              border: OutlineInputBorder(
                borderRadius: BorderRadius.circular(10),
                borderSide: BorderSide(
                    color: Colors.black.withOpacity(0.1), width: 1),
              ),
              prefixIcon: Padding(
                padding: const EdgeInsets.all(10),
                child: SvgPicture.asset(
                  Assets.svgSearch,
                ),
              ),
              focusedErrorBorder: OutlineInputBorder(
                borderRadius: BorderRadius.circular(10),
                borderSide: BorderSide(
                    color: Colors.black.withOpacity(0.1), width: 1),
              ),
              filled: true,
              hoverColor: Colors.transparent,
              hintText: "Shop name or service",
              hintStyle: TextStyle(
                overflow: TextOverflow.ellipsis,
                fontWeight: FontWeight.w400,
                fontSize: 16,
                color: "#5E5E5E7A".toColor().withOpacity(0.5),
              ),
            ),
            onChanged: (value){
              setState(() {
                _searchQuery = value;
              });
            },
          ),
        ),
        FutureBuilder(
          future: customerOfferFirebase.getAllOffers(),
          builder: (context, snapshot) {
            if (snapshot.connectionState == ConnectionState.waiting) {
              return const Center(
                child: CircularProgressIndicator(),
              );
            } else if (snapshot.hasError) {
              return Text("Error: ${snapshot.error}");
            } else if (snapshot.hasData) {
              var services = _getFilteredServices();
              return ListView.builder(
                  itemCount: services.length,
                  primary: false,
                  shrinkWrap: true,
                  padding: const EdgeInsets.symmetric(horizontal: 10),
                  itemBuilder: (context, index) {
                    var service = services[index];
                    print("getAllOffers ${service}");
                    return GestureDetector(
                      onTap: () {
                        providerRequestState
                            .selectedProviderRequest
                            .value = services[index];
                        Get.to(() => const ProviderServiceRequestViewScreen());
                      },
                      child: Container(
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
                        padding: const EdgeInsets.all(10),
                        child: Row(
                          mainAxisAlignment:
                          MainAxisAlignment.spaceBetween,
                          crossAxisAlignment: CrossAxisAlignment.center,
                          children: [
                            Expanded(
                              child: Column(
                                mainAxisAlignment:
                                MainAxisAlignment.center,
                                crossAxisAlignment:
                                CrossAxisAlignment.start,
                                children: [
                                  HeaderTxtWidget(
                                    service.serviceName!,
                                  ),
                                  SubTxtWidget(
                                    service.description,
                                    maxLines: 2,
                                  ),
                                  Row(
                                    mainAxisAlignment:
                                    MainAxisAlignment.spaceBetween,
                                    children: [
                                      Row(
                                        children: [
                                          Icon(
                                            Icons.calendar_month,
                                            color: mainBtnColor,
                                          ),
                                          const SizedBox(width: 5,),
                                          Text(
                                            Tools.changeDateFormat(service.dateTime.toString(), globalTimeFormat),
                                            textAlign: TextAlign.start,
                                            style: const TextStyle(
                                                fontSize: 15,
                                                color: Colors.black),
                                          ),
                                        ],
                                      ),
                                    ],
                                  ),
                                  Row(
                                    mainAxisAlignment:
                                    MainAxisAlignment.spaceBetween,
                                    children: [
                                      Row(
                                        children: [
                                          Icon(
                                            Icons.timelapse,
                                            color: mainBtnColor,
                                          ),
                                          const SizedBox(width: 5,),
                                          Text(service.selectedTime!,
                                            textAlign: TextAlign.start,
                                            style: const TextStyle(
                                                fontSize: 15,
                                                color: Colors.black),
                                          ),
                                        ],
                                      ),
                                    ],
                                  ),
                                  Row(
                                    children: [
                                      Icon(
                                        Icons.price_change,
                                        color: mainBtnColor,
                                      ),
                                      const SizedBox(width: 5,),
                                      Text(
                                        '\$${service.priceRange.toString().replaceAll("-", " To \$")}',
                                        style: const TextStyle(
                                            fontSize: 15,
                                            color: Colors.black),
                                      ),
                                    ],
                                  ),
                                ],
                              ),
                            ),
                            const SizedBox(
                              width: 10,
                            ),
                            SizedBox(
                              width: 100,
                              height: 120,
                              child: ClipRRect(
                                borderRadius: const BorderRadius.all(
                                    Radius.circular(10)),
                                child: service.offerImages!.isEmpty
                                    ? Image.asset(
                                  getServiceIcon(
                                      service.serviceType!),
                                  fit: BoxFit.contain,
                                )
                                    : NetworkImageWidget(
                                    url: service.offerImages!.first),
                              ),
                            ),
                          ],
                        ),
                      ),
                    );
                  });
            }
            return Text("No data");
          },
        ),
      ],
    );
  }
}
