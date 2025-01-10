import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/Utils/tools.dart';
import 'package:groom/data_models/customer_offer_model.dart';
import 'package:groom/states/customer_request_state.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../firebase/customer_offer_firebase.dart';
import '../../../repo/setting_repo.dart';
import 'customer_booking_service_request_list_screen.dart';
import '../../../utils/colors.dart';
import '../../../utils/utils.dart';

class CustomerOffersListScreen extends StatelessWidget {
  const CustomerOffersListScreen({super.key});

  @override
  Widget build(BuildContext context) {
    CustomerOfferFirebase customerOfferFirebase = CustomerOfferFirebase();
    CustomerRequestState customerRequestState = Get.put(CustomerRequestState());
    return Container(
      child: FutureBuilder(
        future: customerOfferFirebase.getAllOffersByUserId(auth.value.uid),
        builder: (context, snapshot) {
          if (snapshot.connectionState == ConnectionState.waiting) {
            return const Center(
              child: CircularProgressIndicator(),
            );
          } else if (snapshot.hasError) {
            return Text("Error: ${snapshot.error}");
          } else if (snapshot.hasData) {
            var services = snapshot.data as List<CustomerOfferModel>;
            return ListView.builder(
                primary: false,
                shrinkWrap: true,
                padding: const EdgeInsets.symmetric(vertical: 10),
                scrollDirection: Axis.vertical,
                itemCount: services.length,
                itemBuilder: (context, index) {
                  var service = services[index];
                  return Padding(
                    padding: const EdgeInsets.symmetric(horizontal: 8.0,vertical: 5),
                    child: GestureDetector(
                      onTap: () {
                        customerRequestState.selectedCustomerRequest.value = services[index];
                        Get.to(() => const CustomerServiceRequestViewScreen());
                      },
                      child: Container(
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
                                      Text(
                                        service.priceRange!,
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
                    ),
                  );
                });
          } else {
            return Text("no data found");
          }
        },
      ),
    );
  }
}
