import 'package:cached_network_image/cached_network_image.dart';
import 'package:carousel_slider/carousel_slider.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/data_models/provider_service_request_offer_model.dart';
import 'package:groom/data_models/user_model.dart';
import 'package:groom/ext/themehelper.dart';
import 'package:groom/firebase/notifications_firebase.dart';
import 'package:groom/firebase/provider_service_firebase.dart';
import 'package:groom/firebase/user_firebase.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/utils/utils.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/input_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../data_models/notification_model.dart';
import '../../../firebase/firebase_notifications.dart';
import '../../../repo/setting_repo.dart';
import '../../../states/provider_request_state.dart';
import '../../customer/customer_dashboard/customer_dashboard_page.dart';

class ProviderServiceRequestViewScreen extends StatefulWidget {
  const ProviderServiceRequestViewScreen({super.key});

  @override
  State<ProviderServiceRequestViewScreen> createState() =>
      _ProviderServiceRequestViewScreenState();
}

class _ProviderServiceRequestViewScreenState
    extends State<ProviderServiceRequestViewScreen> {
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  NotificationsFirebase notificationsFirebase = NotificationsFirebase();
  ProviderRequestState providerRequestState = Get.find();
  bool depositAllowed = false;
  int depositPrice = 0;
  final TextEditingController depositController = TextEditingController();
  final TextEditingController descriptionController = TextEditingController();
  PushNotificationService pushNotificationService = PushNotificationService();
  RxInt selectedIndex = 0.obs;

  UserFirebase userFirebase = UserFirebase();
  UserModel? _userModel;
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget(
          providerRequestState.selectedProviderRequest.value.serviceName!,
          color: Colors.white,
        ),
      ),
      body: FutureBuilder(
          future: userFirebase.getUser(
              providerRequestState.selectedProviderRequest.value.userId),
          builder: (context, snapshot) {
            if (snapshot.connectionState == ConnectionState.waiting) {
              return const Center(
                child: CircularProgressIndicator(),
              );
            } else if (snapshot.hasError) {
              return const Text("This user is deleted");
            } else if (snapshot.hasData) {
              UserModel? userModel = snapshot.data;
              _userModel = userModel;
              return SingleChildScrollView(
                child: Column(
                  children: [
                    _imageDisplay(),
                    Container(
                      padding: const EdgeInsets.symmetric(horizontal: 10),
                      alignment: AlignmentDirectional.centerStart,
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          HeaderTxtWidget(
                            providerRequestState
                                .selectedProviderRequest.value.serviceName!,
                          ),
                          SubTxtWidget(
                            providerRequestState
                                .selectedProviderRequest.value.serviceType!,
                          ),
                          SubTxtWidget(
                            providerRequestState
                                .selectedProviderRequest.value.description!,
                          ),
                          Wrap(
                            children: [
                              const Icon(Icons.calendar_month),
                              Text(
                                Tools.changeDateFormat(
                                    providerRequestState
                                        .selectedProviderRequest.value.dateTime
                                        .toString(),
                                    globalTimeFormat),
                              ),
                              const SizedBox(
                                width: 10,
                              ),
                              const Icon(Icons.timelapse),
                              Text(
                                providerRequestState.selectedProviderRequest
                                    .value.selectedTime!,
                              ),
                            ],
                          ),
                          const Divider(),
                          HeaderTxtWidget("Your Offer"),
                          myOffer(),
                        ],
                      ),
                    )
                  ],
                ),
              );
            }
            return const Text("No data ");
          }),
    );
  }

  Widget _imageDisplay() {
    if (providerRequestState
        .selectedProviderRequest.value.offerImages!.isEmpty) {
      return const SizedBox();
    }

    return Container(
      margin: const EdgeInsets.all(10),
      child: Column(
        children: [
          ClipRRect(
            borderRadius: const BorderRadius.all(Radius.circular(10)),
            child: CachedNetworkImage(
              imageUrl: providerRequestState
                  .selectedProviderRequest.value.offerImages![0],
              fit: BoxFit.cover,
              width: double.infinity, // Full width
              height: 250, // Adjust height as needed
            ),
          ),
        ],
      ),
    );
  }
  // Widget _slider() {
  //   if(providerRequestState
  //       .selectedProviderRequest.value.offerImages!.isEmpty){
  //     return const SizedBox();
  //   }
  //   return Container(
  //     height: 250,
  //     margin: const EdgeInsets.all(10),
  //     child: Stack(
  //       children: [
  //         CarouselSlider(
  //           items: providerRequestState
  //               .selectedProviderRequest.value.offerImages!
  //               .map((e) {
  //             return ClipRRect(
  //               borderRadius: const BorderRadius.all(Radius.circular(10)),
  //               child: CachedNetworkImage(
  //                 imageUrl: e,
  //                 fit: BoxFit.cover,
  //                 width: 1000.0,
  //               ),
  //             );
  //           }).toList(),
  //           options: CarouselOptions(
  //             aspectRatio: 1,
  //             height: 250,
  //             viewportFraction: 1,
  //             initialPage: 0,
  //             enableInfiniteScroll: true,
  //             reverse: false,
  //             autoPlay: true,
  //             autoPlayInterval: const Duration(seconds: 3),
  //             autoPlayAnimationDuration: const Duration(milliseconds: 800),
  //             autoPlayCurve: Curves.fastOutSlowIn,
  //             enlargeCenterPage: true,
  //             scrollDirection: Axis.horizontal,
  //             onPageChanged: (index, reason) {
  //               selectedIndex.value=index;
  //             },
  //           ),
  //         ),
  //         Positioned(
  //           bottom: 10,
  //           right: 10,
  //           child: Obx(
  //                 () => _dots(providerRequestState
  //                     .selectedProviderRequest.value.offerImages!.length),
  //           ),
  //         ),
  //       ],
  //     ),
  //   );
  // }

  Widget _dots(int length) {
    List<Widget> list = [];
    for (int i = 0; i < length; i++) {
      list.add(AnimatedContainer(
        duration: const Duration(milliseconds: 500),
        height: 8,
        width: selectedIndex.value == i ? 28 : 8,
        margin: const EdgeInsets.symmetric(horizontal: 5),
        decoration: BoxDecoration(
            borderRadius: const BorderRadius.all(Radius.circular(10)),
            color:
                selectedIndex.value == i ? Colors.blue : Colors.grey.shade400),
      ));
    }
    return Row(
      children: list,
    );
  }

  Widget myOffer() {
    return FutureBuilder(
      future: providerServiceFirebase.getProviderOffer(
          providerRequestState.selectedProviderRequest.value.offerId,
          auth.value.uid),
      builder: (context, snapshot) {
        if (snapshot.connectionState == ConnectionState.waiting) {
          return const Text("");
        } else if (snapshot.hasError) {
          return Text("Error: ${snapshot.error}");
        }
        if (snapshot.data == null) {
          return Container(
            alignment: AlignmentDirectional.center,
            padding: const EdgeInsets.all(10),
            child: Column(
              children: [
                SubTxtWidget('You can make an offer'),
                GestureDetector(
                  onTap: createOffer,
                  child: Container(
                    width: double.infinity, // Full width
                    margin: const EdgeInsets.all(10), // Add margin if needed
                    padding: const EdgeInsets.symmetric(
                        vertical: 15), // Adjust padding
                    decoration: BoxDecoration(
                      color: btntextcolor2.withOpacity(1), // Background color
                      borderRadius:
                          BorderRadius.circular(10), // Rounded corners
                    ),
                    child: Center(
                      child: Text("Make an offer",
                          style: ThemeText.mediumText.copyWith(
                            fontFamily: 'Plus Jakarta Sans',
                            fontSize: 16,
                            fontWeight: FontWeight.w700,
                            height: 24 / 16, // Line height divided by font size
                            decoration: TextDecoration.none,
                            color: Colors.white, // Text color
                          )),
                    ),
                  ),
                )
              ],
            ),
          );
        }
        var request = snapshot.data!;
        return FutureBuilder(
          future: userFirebase.getUser(request.providerId),
          builder: (context, snapshot) {
            if (snapshot.connectionState == ConnectionState.waiting) {
              return const Text("");
            } else if (snapshot.hasError) {
              return Text("Error: ${snapshot.error}");
            } else if (snapshot.hasData) {
              var user = snapshot.data as UserModel;
              return Column(
                crossAxisAlignment: CrossAxisAlignment.end,
                children: [
                  ListTile(
                    leading: ClipRRect(
                      borderRadius: BorderRadius.circular(12),
                      child: NetworkImageWidget(
                        url: user.photoURL ?? "",
                        width: 60,
                        height: 60,
                      ),
                    ),
                    title: user.providerUserModel!.providerType != "Salon"
                        ? Text(
                            user.fullName.capitalizeFirst!,
                            style: const TextStyle(
                                fontSize: 20, fontWeight: FontWeight.bold),
                          )
                        : Text(
                            user.providerUserModel!.salonTitle!
                                .capitalizeFirst!,
                            style: const TextStyle(
                                fontSize: 20, fontWeight: FontWeight.bold),
                          ),
                    subtitle: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        SubTxtWidget(request.description),
                        SubTxtWidget(
                          providerRequestState
                              .selectedProviderRequest.value.status!,
                          color: Colors.blue,
                          fontSize: 12,
                        ),
                      ],
                    ),
                    trailing: Column(
                      mainAxisAlignment: MainAxisAlignment.start,
                      children: [
                        const Text(
                          "Deposit :",
                          style: TextStyle(
                              fontWeight: FontWeight.bold,
                              fontSize: 16,
                              color: Colors.black),
                        ),
                        request.deposit != false
                            ? Text(
                                "${request.depositAmount.toString()} \$",
                                style: const TextStyle(
                                    fontWeight: FontWeight.bold,
                                    fontSize: 14,
                                    color: Colors.black),
                              )
                            : const Text(
                                "0 \$",
                                style: TextStyle(
                                    fontWeight: FontWeight.bold,
                                    fontSize: 14,
                                    color: Colors.black),
                              ),
                      ],
                    ),
                  ),
                  ActionChip(
                    label: SubTxtWidget('Edit Offer'),
                    onPressed: () {
                      createOffer(data: request);
                    },
                  )
                ],
              );
            }
            return Text("");
          },
        );
      },
    );
  }

  void createOffer({ProviderServiceRequestOfferModel? data}) {
    if (data != null) {
      descriptionController.text = data.description;
      depositController.text = data.depositAmount.toString();
      depositAllowed = data.deposit;
    }
    Tools.ShowDailog(
        context,
        Scaffold(
          backgroundColor: Colors.transparent,
          body: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [
              Container(
                padding: const EdgeInsets.all(10),
                margin: const EdgeInsets.all(20),
                decoration: BoxDecoration(
                    borderRadius: BorderRadius.circular(20),
                    color: Colors.white),
                child: StatefulBuilder(
                  builder: (BuildContext context, StateSetter setState) {
                    return Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        HeaderTxtWidget(
                          'Create an Offer',
                          fontSize: 16,
                          fontWeight: FontWeight.w700,
                          decoration: TextDecoration.none,
                        ),
                        InputWidget(
                          title: "Offer Description",
                          controller: descriptionController,
                          hint: "Enter description",
                          maxLines: 4,
                        ),
                        Row(
                          children: [
                            Checkbox(
                              value: depositAllowed,
                              onChanged: (newDeposit) {
                                setState(() {
                                  depositAllowed = newDeposit!;
                                });
                              },
                            ),
                            const Text("Deposit Required"),
                          ],
                        ),
                        if (depositAllowed)
                          InputWidget(
                            title: "Deposit Amount",
                            hint: 'Enter deposit amount',
                            controller: depositController,
                            sufix: "USD",
                            inputType: TextInputType.number,
                          ),
                        const SizedBox(
                          height: 10,
                        ),
                        Row(
                          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                          children: [
                            Container(
                              width: 137, // Fixed width
                              height: 50, // Fixed height
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 20), // Padding
                              decoration: BoxDecoration(
                                borderRadius: BorderRadius.circular(
                                    24), // Rounded corners
                                color: Colors.grey
                                    .shade300, // Background color, adjust as needed
                                // To achieve opacity, you can set color with opacity
                              ),
                              child: GestureDetector(
                                onTap: () {
                                  Get.back(); // Functionality to go back
                                },
                                child: const Center(
                                  child: Text(
                                    "Cancel",
                                    style: TextStyle(
                                      color: Colors.black, // Text color
                                      fontSize: 16, // Text size
                                    ),
                                  ),
                                ),
                              ),
                            ),
                            Container(
                              width: 137, // Fixed width
                              height: 50, // Fixed height
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 20), // Padding
                              decoration: BoxDecoration(
                                  borderRadius: BorderRadius.circular(
                                      24), // Rounded corners
                                  color: btnColorCode.withOpacity(
                                      1) // Background color, adjust as needed
                                  ),
                              child: GestureDetector(
                                onTap: () {
                                  String? description =
                                      descriptionController.text.trim();
                                  if (description.isEmpty) {
                                    ScaffoldMessenger.of(context).showSnackBar(
                                      const SnackBar(
                                        content:
                                            Text('Description cannot be empty'),
                                        backgroundColor: Colors.red,
                                      ),
                                    );
                                    return;
                                  }
                                  if (depositAllowed) {
                                    int? depositAmount =
                                        int.tryParse(depositController.text);
                                    if (depositAmount == null ||
                                        depositAmount <= 0) {
                                      ScaffoldMessenger.of(context)
                                          .showSnackBar(
                                        const SnackBar(
                                          content: Text(
                                              'Deposit amount must be greater than zero'),
                                          backgroundColor: Colors.red,
                                        ),
                                      );
                                      return;
                                    }
                                  }
                                  int? depositAmount = depositAllowed
                                      ? int.tryParse(depositController.text)
                                      : null;
                                  String requestId = data != null
                                      ? data.requestId
                                      : generateProjectId();
                                  ProviderServiceRequestOfferModel
                                      providerServiceRequestOfferModel =
                                      ProviderServiceRequestOfferModel(
                                          selectedDate: providerRequestState
                                              .selectedProviderRequest
                                              .value
                                              .dateTime!
                                              .millisecondsSinceEpoch,
                                          description: description,
                                          requestId: requestId,
                                          providerId: FirebaseAuth
                                              .instance.currentUser!.uid,
                                          serviceId: providerRequestState
                                              .selectedProviderRequest
                                              .value
                                              .offerId,
                                          createdOn: DateTime.now()
                                              .millisecondsSinceEpoch,
                                          deposit: depositAllowed,
                                          depositAmount: depositAllowed != false
                                              ? depositAmount!
                                              : 0,
                                          clientId: providerRequestState
                                              .selectedProviderRequest
                                              .value
                                              .userId);
                                  providerServiceFirebase
                                      .writeProviderServiceRequestOfferToFirebase(
                                          providerServiceRequestOfferModel,
                                          requestId);

                                  if (_userModel!.notification_status!) {
                                    PushNotificationService
                                        .sendProviderOfferNotificationToClient(
                                      _userModel!.notificationToken!,
                                      context,
                                      providerRequestState
                                          .selectedProviderRequest
                                          .value
                                          .offerId,
                                    );
                                  }

                                  NotificationModel notificationModel =
                                      NotificationModel(
                                          userId: _userModel!.uid,
                                          createdOn: DateTime.now()
                                              .millisecondsSinceEpoch,
                                          notificationText:
                                              "A Groomer just applied to service offer ",
                                          type: "OFFER",
                                          sendBy: "PROVIDER",
                                          serviceId: providerRequestState
                                              .selectedProviderRequest
                                              .value
                                              .offerId,
                                          notificationId: generateProjectId());
                                  notificationsFirebase.writeNotificationToUser(
                                      _userModel!.uid,
                                      notificationModel,
                                      generateProjectId());

                                  // Handle the confirm action
                                  Get.to(() => CustomerDashboardPage());
                                },
                                child: const Center(
                                  child: Text(
                                    "Send",
                                    style: TextStyle(
                                      color: Colors.white, // Text color
                                      fontSize: 16, // Text size
                                    ),
                                  ),
                                ),
                              ),
                            )
                          ],
                        )
                      ],
                    );
                  },
                ),
              )
            ],
          ),
        ));
  }
}
