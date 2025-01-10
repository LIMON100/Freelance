import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:geocoding/geocoding.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/screens/google_maps_screen.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/guest_widget.dart';
import 'package:groom/widgets/loading_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../firebase/geomaps_firebase.dart';
import '../firebase/user_firebase.dart';
import '../generated/assets.dart';
import '../screens/chat_list_screen.dart';
import '../states/provider_service_state.dart';
import '../ui/customer/customer_dashboard/model/location_model.dart';
import '../ui/customer/service_request/customer_create_offer_screen.dart';
import '../ui/notification/notification_screen.dart';
import '../states/user_state.dart';
import 'header_txt_widget.dart';

class AppBarWidget extends StatelessWidget {
  bool hideIcon = false;
  UserFirebase userFirebase = UserFirebase();
  ProviderServiceState providerServiceState = Get.put(ProviderServiceState());

  AppBarWidget({super.key, this.hideIcon = false});

  UserStateController userStateController = Get.put(UserStateController());

  @override
  Widget build(BuildContext context) {
    return ValueListenableBuilder(
      valueListenable: auth,
      builder: (context, value, child) {
        return Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Expanded(
                    child: InkWell(
                  child: Row(
                    children: [
                      SvgPicture.asset(
                        Assets.svgLocationMain,
                        width: 10,
                        height: 15,
                      ),
                      Expanded(
                        child: SubTxtWidget(
                          '${value.defaultAddress}',
                          fontSize: 12,
                          fontWeight: FontWeight.w400,
                        ),
                      ),
                    ],
                  ),
                  onTap: () {
                    showLocation(context);
                  },
                )),
                if (!hideIcon)
                  IconButton(
                      onPressed: () {
                        if (isGuest.value) {
                          Get.to(GuestWidget(message: "Login to view chat"));
                        } else {
                          Get.to(() => ChatListScreen(
                                id: auth.value.uid,
                              ));
                        }
                      },
                      icon: SvgPicture.asset(Assets.svgChat)),
                if (!hideIcon)
                  IconButton(
                      onPressed: () {
                        if (isGuest.value) {
                          Get.to(GuestWidget(
                              message: "Login to view notification"));
                        } else {
                          Get.to(() => const NotificationScreen());
                        }
                      },
                      icon: Icon(
                        Icons.notifications_none,
                        color: "#2F6EB6".toColor(),
                        size: 30,
                      )),
                if (!hideIcon)
                  InkWell(
                    child: Container(
                      height: 40,
                      width: 40,
                      padding: const EdgeInsets.all(1),
                      decoration: BoxDecoration(
                          shape: BoxShape.circle,
                          border: Border.all(
                              color: Colors.grey.shade200, width: 1)),
                      child: ClipRRect(
                        borderRadius: const BorderRadius.all(Radius.circular(40)),
                        child: NetworkImageWidget(url: value.photoURL),
                      ),
                    ),
                    onTap: () {
                      if (isGuest.value) {
                        Get.to(GuestWidget(message: "Login to view profile"));
                      } else {
                        Get.toNamed('/edit_profile');
                      }
                    },
                  )
              ],
            ),
            if (!hideIcon)
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Expanded(
                    flex: 1,
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        SubTxtWidget('Hello,'),
                        HeaderTxtWidget(
                          isGuest.value ? "Guest" : value.fullName,
                          fontSize: 20,
                        ),
                      ],
                    ),
                  ),
                  Container(
                    padding:
                        const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
                    decoration: BoxDecoration(
                      borderRadius: const BorderRadius.all(Radius.circular(10)),
                      color: primaryColorCode,
                    ),
                    child: InkWell(
                      child: Row(
                        children: [
                          const CircleAvatar(
                            radius: 12,
                            backgroundColor: Colors.blue,
                            child: Icon(
                              Icons.add,
                              color: Colors.white,
                            ),
                          ),
                          const SizedBox(
                            width: 5,
                          ),
                          SubTxtWidget(
                            'Service Request',
                            color: Colors.white,
                          )
                        ],
                      ),
                      onTap: () {
                        if (isGuest.value) {
                          Get.to(GuestWidget(
                              message: "Login to view create service"));
                        } else {
                          Get.to(
                            () => CustomerCreateOfferScreen(),
                          );
                        }
                      },
                    ),
                  )
                ],
              )
          ],
        );
      },
    );
  }

Future<String> getAddress(LatLng? lang) async {
  if (lang == null) {
    // Return Los Angeles as the default location
    return "Los Angeles, California, USA";
  }
  try {
    List<Placemark> placemarks =
        await placemarkFromCoordinates(lang.latitude, lang.longitude);
    StringBuffer b = StringBuffer();
    Placemark address = placemarks.first;
    if (address.street != null) {
      b.write("${address.street}, ");
    }
    if (address.locality != null) {
      b.write("${address.locality}, ");
    }
    if (address.subAdministrativeArea != null) {
      b.write("${address.subAdministrativeArea}, ");
    }
    return b.toString().isEmpty ? "Los Angeles, California, USA" : b.toString();
  } catch (e) {
    // If there's an error, fallback to Los Angeles
    return "Los Angeles, California, USA";
  }
}


  void showLocation(context) {
    if (auth.value.uid.isEmpty) return;
    showModalBottomSheet(
      context: context,
      builder: (context) {
        return Material(
          color: Colors.transparent,
          child: StatefulBuilder(
            builder: (context, setState) {
              return Container(
                padding: const EdgeInsets.all(20),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.center,
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        HeaderTxtWidget('Select Location'),
                        ActionChip(
                          label: SubTxtWidget('Add'),
                          onPressed: () async {
                            final result =
                                await Get.to(() => const GoogleMapScreen());
                            LatLng latLng = result['location'];
                            if (result != null) {
                              await userFirebase.addLocation({
                                "latitude": latLng.latitude,
                                "longitude": latLng.longitude,
                                "address": result['address'],
                              });
                              setState(() {});
                            }
                          },
                          shape: const StadiumBorder(),
                        )
                      ],
                    ),
                    Expanded(
                      child: FutureBuilder(
                        future: userFirebase.getLocations(),
                        builder: (context, snapshot) {
                          if (snapshot.data == null) {
                            return LoadingWidget(
                              type: LoadingType.LIST,
                            );
                          }
                          return ListView.builder(
                            itemBuilder: (context, index) {
                              LocationModel loc = snapshot.data![index];
                              return Obx(
                                () => InkWell(
                                  child: Container(
                                    margin:
                                        const EdgeInsets.symmetric(vertical: 8),
                                    padding: const EdgeInsets.all(10),
                                    decoration: BoxDecoration(
                                      borderRadius: const BorderRadius.all(
                                          Radius.circular(20)),
                                      color: userStateController
                                                  .defaultAddress.value ==
                                              loc.address
                                          ? primaryColorCode
                                          : Colors.grey.shade200,
                                    ),
                                    child: Row(
                                      children: [
                                        Image.asset(Assets.imgGroupLocation),
                                        const SizedBox(
                                          width: 10,
                                        ),
                                        Expanded(
                                            child: SubTxtWidget(
                                          '${loc.address}',
                                          color: userStateController
                                                      .defaultAddress.value ==
                                                  loc.address
                                              ? Colors.white
                                              : Colors.black,
                                          fontSize: 14,
                                        )),
                                        IconButton(
                                            onPressed: () async {
                                              if (userStateController
                                                      .defaultAddress.value ==
                                                  loc.address) {
                                                GeoMapsFirebase()
                                                    .getLocationAndUpdateUser();
                                              }
                                              await userFirebase
                                                  .deleteLocation(loc.id);
                                              setState(() {});
                                            },
                                            icon: const Icon(
                                              Icons.delete,
                                              color: Colors.red,
                                            ))
                                      ],
                                    ),
                                  ),
                                  onTap: () {
                                    userStateController.defaultAddress.value =
                                        loc.address ?? "";
                                    userStateController.defaultLatLng =
                                        LatLng(loc.latitude!, loc.longitude!);
                                  },
                                ),
                              );
                            },
                            shrinkWrap: true,
                            itemCount: snapshot.data!.length,
                          );
                        },
                      ),
                    ),
                    Obx(
                      () => ButtonPrimaryWidget(
                        'Set as Default Location',
                        onTap: () async {
                          if (userStateController.defaultAddress.value !=
                              auth.value.defaultAddress) {
                            userStateController.isLoading.value = true;
                            auth.value.defaultAddress =
                                userStateController.defaultAddress.value;
                            auth.value.location =
                                userStateController.defaultLatLng;
                            await userFirebase.updateUser(auth.value);
                            await userFirebase.getUserDetails(auth.value.uid);
                            userStateController.isLoading.value = false;
                            providerServiceState.getAllProviders();
                            providerServiceState.getAllServices();
                          }
                          Navigator.pop(context);
                        },
                        marginHorizontal: 20,
                        color: userStateController.defaultAddress.value ==
                                auth.value.defaultAddress
                            ? Colors.grey
                            : primaryColorCode,
                        isLoading: userStateController.isLoading.value,
                      ),
                    ),
                  ],
                ),
              );
            },
          ),
        );
      },
    );
  }
}
