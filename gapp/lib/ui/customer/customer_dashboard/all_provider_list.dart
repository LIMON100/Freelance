import 'dart:convert';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_database/ui/firebase_animated_list.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/widgets/loading_widget.dart';
import '../../../constant/global_configuration.dart';
import '../../../data_models/user_model.dart';
import '../../../firebase/provider_user_firebase.dart';
import '../../../generated/assets.dart';
import '../../../repo/setting_repo.dart';
import '../../../utils/colors.dart';
import '../../../widgets/header_txt_widget.dart';
import '../../../widgets/network_image_widget.dart';
import '../../../widgets/sub_txt_widget.dart';
import '../services/provider_details_screen.dart';

class AllProviderList extends StatefulWidget {
  const AllProviderList({super.key});

  @override
  State<AllProviderList> createState() => _ListState();
}

class _ListState extends State<AllProviderList> {
  final _ref = FirebaseDatabase.instanceFor(
      app: Firebase.app(),
      databaseURL: GlobalConfiguration().getValue('databaseURL'));

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        title: HeaderTxtWidget(
          "All Provider List",
          color: primaryColorCode,
        ),
        iconTheme: IconThemeData(color: primaryColorCode),
      ),
      body: FutureBuilder(
        future: ProviderUserFirebase().getAllProviders(),
        builder: (context, snapshot) {
          if (snapshot.connectionState == ConnectionState.waiting) {
            return LoadingWidget();
          } else if (snapshot.hasError) {
            return const Text("No Providers");
          } else if (snapshot.hasData) {
            var providers = snapshot.data as List<UserModel>;
            return ListView.builder(
              scrollDirection: Axis.vertical,
              itemCount: providers.length>5?5:providers.length,
              shrinkWrap: true,
              primary: false,
              padding: const EdgeInsets.symmetric(horizontal: 10),
              itemBuilder: (context, index) {
                var data = providers[index];
                double distance = 0.0;
                if (auth.value.location != null && data.location != null) {
                  distance = Geolocator.distanceBetween(
                    auth.value.location!.latitude,
                    auth.value.location!.longitude,
                    data.location!.latitude,
                    data.location!.longitude,
                  ) / 1000; // Convert meters to kilometers
                }
                return InkWell(
                  child: Container(
                    margin: const EdgeInsets.only(top: 15),
                    color: Colors.white,
                    child: Row(
                      children: [
                        ClipRRect(
                          borderRadius: BorderRadius.circular(9),
                          child: NetworkImageWidget(
                            url: data.providerUserModel!.providerImages!.first,
                            width: 100,
                            height: 100,
                          ),
                        ),
                        const SizedBox(
                          width: 10,
                        ),
                        Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                HeaderTxtWidget(
                                    data.providerUserModel!.providerType != "Independent"
                                        ? data.providerUserModel!.salonTitle!
                                        : data.fullName),
                                Row(
                                  children: [
                                    SvgPicture.asset(Assets.svgLocationGray),
                                    const SizedBox(
                                      width: 5,
                                    ),
                                    SubTxtWidget(
                                      isGuest.value
                                          ? 'Login to view'
                                          : '${data.providerUserModel!.addressLine} (${distance.toStringAsFixed(0)} mi)',
                                      color: "#8683A1".toColor(),
                                    ),
                                  ],
                                ),
                                Row(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                  children: [
                                    Expanded(child: Column(
                                      crossAxisAlignment: CrossAxisAlignment.start,
                                      children: [
                                        Row(
                                          children: [
                                            SvgPicture.asset(Assets.svgStar),
                                            const SizedBox(
                                              width: 5,
                                            ),
                                            SubTxtWidget(
                                              // '${data.providerUserModel!.overallRating}',
                                              '${data.providerUserModel!.overallRating.toStringAsFixed(1)}',
                                              color: "#8683A1".toColor(),
                                            ),
                                          ],
                                        ),
                                        HeaderTxtWidget("\$${data.providerUserModel!.basePrice}",color: Colors.blue,)
                                      ],
                                    )),
                                    Container(
                                      margin: const EdgeInsets.only(top: 20),
                                      padding: const EdgeInsets.symmetric(
                                          horizontal: 10, vertical: 8),
                                      decoration: BoxDecoration(
                                          borderRadius: const BorderRadius.only(
                                              topLeft: Radius.circular(15),
                                              bottomRight: Radius.circular(15)),
                                          color: primaryColorCode),
                                      child: HeaderTxtWidget(
                                        isGuest.value ? "Login" : 'Provider Details',
                                        color: Colors.white,
                                        fontSize: 13,
                                      ),
                                    )
                                  ],
                                ),
                              ],
                            ))
                      ],
                    ),
                  ),
                  onTap: () {
                    if (isGuest.value) {
                      Get.toNamed('/login');
                    } else {
                      Get.to(() => ProviderDetailsScreen(
                        provider: data,
                      ));
                    }
                  },
                );
              },
            );
          } else {
            return const Text("no data found");
          }
        },
      ),
    );
  }

  Widget _body() {
    return FirebaseAnimatedList(
      query: _ref
          .ref("user")
          .orderByChild("providerId")
          .equalTo(auth.value.uid),
      itemBuilder: (context, snapshot, animation, index) {
        if (!snapshot.exists) {
          return Center(
            child: HeaderTxtWidget("No review found"),
          );
        }
        UserModel? data =
            UserModel.fromJson(jsonDecode(jsonEncode(snapshot.value)));
        double distance = 0.0;
        if (auth.value.location != null && data.location != null) {
          distance = Geolocator.distanceBetween(
                auth.value.location!.latitude,
                auth.value.location!.longitude,
                data.location!.latitude,
                data.location!.longitude,
              ) /
              1000; // Convert meters to kilometers
        }
        return InkWell(
          child: Container(
            margin: const EdgeInsets.only(top: 15),
            child: Row(
              children: [
                ClipRRect(
                  borderRadius: BorderRadius.circular(9),
                  child: NetworkImageWidget(
                    url: data.providerUserModel!.providerImages!.first,
                    width: 100,
                    height: 100,
                  ),
                ),
                const SizedBox(
                  width: 10,
                ),
                Expanded(
                    child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    HeaderTxtWidget(
                        data.providerUserModel!.providerType != "Independent"
                            ? data.providerUserModel!.salonTitle!
                            : data.fullName),
                    Row(
                      children: [
                        SvgPicture.asset(Assets.svgLocationGray),
                        const SizedBox(
                          width: 5,
                        ),
                        SubTxtWidget(
                          isGuest.value
                              ? 'Login to view'
                              : '${data.providerUserModel!.addressLine} (${distance.toStringAsFixed(0)} mi)',
                          color: "#8683A1".toColor(),
                        ),
                      ],
                    ),
                    Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Row(
                          children: [
                            SvgPicture.asset(Assets.svgStar),
                            const SizedBox(
                              width: 5,
                            ),
                            SubTxtWidget(
                              '${data.providerUserModel!.overallRating}',
                              color: "#8683A1".toColor(),
                            ),
                          ],
                        ),
                        Container(
                          margin: const EdgeInsets.only(top: 20),
                          padding: const EdgeInsets.symmetric(
                              horizontal: 10, vertical: 8),
                          decoration: BoxDecoration(
                              borderRadius: const BorderRadius.only(
                                  topLeft: Radius.circular(15),
                                  bottomRight: Radius.circular(15)),
                              color: primaryColorCode),
                          child: HeaderTxtWidget(
                            isGuest.value ? "Login" : 'Provider Details',
                            color: Colors.white,
                            fontSize: 13,
                          ),
                        )
                      ],
                    ),
                  ],
                ))
              ],
            ),
          ),
          onTap: () {
            if (isGuest.value) {
              Get.toNamed('/login');
            } else {
              Get.to(() => ProviderDetailsScreen(
                    provider: data,
                  ));
            }
          },
        );
      },
      defaultChild: LoadingWidget(),
    );
  }
}
