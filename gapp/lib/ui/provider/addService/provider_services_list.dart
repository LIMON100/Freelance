import 'dart:convert';

import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_database/ui/firebase_animated_list.dart';
import 'package:flutter/material.dart';
import 'package:flutter_rating_bar/flutter_rating_bar.dart';
import 'package:get/get.dart';
import 'package:groom/ui/provider/addService/provider_create_service_screen.dart';
import 'package:groom/widgets/loading_widget.dart';
import '../../../constant/global_configuration.dart';
import '../../../data_models/provider_service_model.dart';
import '../../../repo/setting_repo.dart';
import '../../../states/customer_service_state.dart';
import '../../../utils/colors.dart';
import '../../../widgets/header_txt_widget.dart';
import '../../../widgets/network_image_widget.dart';
import '../../../widgets/sub_txt_widget.dart';
import '../../customer/services/service_details_screen.dart';

class ServiceProviderList extends StatefulWidget {
  const ServiceProviderList({super.key});

  @override
  State<ServiceProviderList> createState() => _ServiceProviderListState();
}

class _ServiceProviderListState extends State<ServiceProviderList> {
  TextEditingController myController = TextEditingController();
  final _ref = FirebaseDatabase.instanceFor(
      app: Firebase.app(),
      databaseURL: GlobalConfiguration().getValue('databaseURL'));

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        title: HeaderTxtWidget(
          "Service List",
          color: primaryColorCode,
        ),
        iconTheme: IconThemeData(color: primaryColorCode),
      ),
      body: _body(),
    );
  }

  Widget _body() {
    return FirebaseAnimatedList(
      query: _ref
          .ref("providerServices")
          .orderByChild("userId")
          .equalTo(auth.value.uid),
      itemBuilder: (context, snapshot, animation, index) {
        ProviderServiceModel? data = ProviderServiceModel.fromJson(
            jsonDecode(jsonEncode(snapshot.value)));
        return SizeTransition(
          sizeFactor: animation,
          child: InkWell(
            onTap: () {
              CustomerServiceState customerServiceState =
                  Get.put(CustomerServiceState());
              customerServiceState.selectedService.value = data;
              Get.to(() =>  ServiceDetailsScreen());
            },
            child: Card(
              child: ClipRRect(
                borderRadius: const BorderRadius.all(Radius.circular(10)),
                child: SizedBox(
                  height: 220,
                  child: Stack(
                    children: [
                      NetworkImageWidget(
                        url: data.serviceImages!.first,
                        height: 150,
                        fit: BoxFit.cover,
                      ),
                      Container(
                        margin: const EdgeInsets.all(10),
                        padding: const EdgeInsets.symmetric(
                            horizontal: 10, vertical: 5),
                        decoration: const BoxDecoration(
                            borderRadius: BorderRadius.all(Radius.circular(15)),
                            color: Colors.white),
                        child: SubTxtWidget(
                          data.serviceType,
                          fontSize: 9,
                        ),
                      ),
                      Positioned(
                        right: 10,
                        top: 120,
                        child: Container(
                          margin: const EdgeInsets.all(10),
                          padding: const EdgeInsets.symmetric(
                              horizontal: 20, vertical: 8),
                          decoration: BoxDecoration(
                              borderRadius:
                                  const BorderRadius.all(Radius.circular(20)),
                              color: primaryColorCode,
                              border:
                                  Border.all(color: Colors.white, width: 2)),
                          child: HeaderTxtWidget(
                            '\$ ${data.servicePrice}',
                            fontSize: 12,
                            color: Colors.white,
                          ),
                        ),
                      ),
                      Positioned(
                        top: 145,
                        right: 0,
                        left: 0,
                        child: Container(
                          padding: const EdgeInsets.all(10),
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              HeaderTxtWidget(data.serviceName ?? ""),
                              RatingBarIndicator(
                                itemBuilder: (context, index) {
                                  return const Icon(
                                    Icons.star,
                                    color: Colors.amber,
                                  );
                                },
                                itemCount: 5,
                                itemSize: 18,
                                rating: 5,
                              ),
                              Row(
                                children: [
                                  /*   const Icon(Icons.remove_red_eye_outlined),
                              HeaderTxtWidget('1250',fontSize: 10,),
                              const SizedBox(width: 50,),
                              HeaderTxtWidget('50 watchers',fontSize: 10,),
                           */
                                  const Spacer(),
                                  InkWell(
                                    onTap: () {
                                      Get.to(() => ProviderCreateServiceScreen(
                                            data: data,
                                          ));
                                    },
                                    child: Container(
                                      decoration: const BoxDecoration(
                                          color: Colors.black,
                                          borderRadius: BorderRadius.all(
                                              Radius.circular(10))),
                                      padding: const EdgeInsets.symmetric(
                                          horizontal: 10, vertical: 5),
                                      child: Row(
                                        children: [
                                          const Icon(
                                            Icons.edit_rounded,
                                            color: Colors.white,
                                            size: 10,
                                          ),
                                          const SizedBox(
                                            width: 3,
                                          ),
                                          SubTxtWidget(
                                            'EDIT',
                                            color: Colors.white,
                                            fontSize: 8,
                                          )
                                        ],
                                      ),
                                    ),
                                  )
                                ],
                              )
                            ],
                          ),
                        ),
                      )
                    ],
                  ),
                ),
              ),
            ),
          ),
        );
      },
      defaultChild: LoadingWidget(),
    );
  }
}
