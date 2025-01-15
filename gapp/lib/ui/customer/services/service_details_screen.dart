import 'dart:convert';
import 'dart:math';
import 'package:cached_network_image/cached_network_image.dart';
import 'package:date_picker_timeline/date_picker_widget.dart';
import 'package:firebase_database/ui/firebase_animated_list.dart';
import 'package:flutter/material.dart';
import 'package:flutter_rating_bar/flutter_rating_bar.dart';
import 'package:flutter_svg/svg.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get.dart';
import 'package:groom/data_models/service_booking_model.dart';
import 'package:groom/firebase/service_booking_firebase.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/screens/chat_screen.dart';
import 'package:groom/ui/customer/services/provider_details_controller.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import 'package:groom/widgets/read_more_text.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import 'package:groom/widgets/time_widget.dart';
import '../../../utils/tools.dart';
import '../../../data_models/user_model.dart';
import '../../../firebase/provider_service_firebase.dart';
import '../../../firebase/user_firebase.dart';
import '../../../generated/assets.dart';
import '../../../states/customer_service_state.dart';
import '../../../utils/colors.dart';
import '../../../widgets/loading_widget.dart';
import '../../provider/reviews/model/review_model.dart';
import '../customer_dashboard/customer_dashboard_page.dart';

class ServiceDetailsScreen extends StatefulWidget {
  String? bookingId;
  ServiceDetailsScreen({super.key, this.bookingId});

  @override
  State<ServiceDetailsScreen> createState() =>
      _SimilarServiceDisplayScreenState();
}

class _SimilarServiceDisplayScreenState extends State<ServiceDetailsScreen> {
  final ProviderDetailsController _con = Get.put(ProviderDetailsController());
  CustomerServiceState similarServiceState = Get.put(CustomerServiceState());
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
  UserFirebase userFirebase = UserFirebase();
  String groomerName = '';
  String? selectedTime2;



  DatePickerController datePickerController = DatePickerController();
  bool isLoading = false;
  Rx<UserModel?> user = Rx(null);

  Future<UserModel> fetchUser(String userId) async {
    return await userFirebase.getUser(userId);
  }

  static String generateServiceBookingId() {
    final random = Random();

    final currentDateTime = DateTime.now();
    final formattedDate =
        "${currentDateTime.year}${currentDateTime.month.toString().padLeft(2, '0')}${currentDateTime.day.toString().padLeft(2, '0')}";
    final formattedTime =
        "${currentDateTime.hour.toString().padLeft(2, '0')}${currentDateTime.minute.toString().padLeft(2, '0')}${currentDateTime.second.toString().padLeft(2, '0')}";

    final randomNumbers =
        List.generate(10, (index) => random.nextInt(10)).join();

    final projectId = '$formattedDate$formattedTime$randomNumbers';
    return projectId;
  }

  @override
  void initState() {
    // TODO: implement initState
    super.initState();
    _con.selectedService = similarServiceState.selectedService;
    fetchUser(similarServiceState.selectedService.value.userId).then(
      (value) {
        user.value = value;
        _con.selectedProvider.value = value;
        _con.getScheduleList(_con.selectedService.value!.userId);
      },
    );
    if (similarServiceState.selectedService.value == null) {
      // Perhaps fetch the service details again if needed
      // Or simply wait for it to be populated
      Future.delayed(const Duration(milliseconds: 500), () { // Adjust the duration
        setState(() {
          isLoading = false;
        });
      });
    } else {
      isLoading = false;
    }
  }

  @override
  Widget build(BuildContext context) {
    double distance = 0.0;
    if (isLoading || similarServiceState.selectedService.value == null) {
      return const Scaffold(
        body: Center(child: CircularProgressIndicator()),
      );
    }
    if (auth.value.location != null &&
        similarServiceState.selectedService.value.location != null) {
      distance = Geolocator.distanceBetween(
            auth.value.location!.latitude,
            auth.value.location!.longitude,
            similarServiceState.selectedService.value.location!.latitude,
            similarServiceState.selectedService.value.location!.longitude,
          ) /
          1000; // Convert meters to kilometers
    }
    Size size = MediaQuery.of(context).size;
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        iconTheme: IconThemeData(color: primaryColorCode),
        title: HeaderTxtWidget(
          "Service details", fontSize: 16.0, // Font size in logical pixels
          fontWeight: FontWeight.w700,
        ),
      ),
      body: SingleChildScrollView(
        child: Column(
          children: [
            SizedBox(
              height: 250,
              width: size.width,
              child: Stack(
                children: [
                  // CachedNetworkImage(
                  //   imageUrl: similarServiceState.selectedService.value.serviceImages!.first,
                  //   fit: BoxFit.cover,
                  //   height: 300,
                  //   width: size.width,
                  // ),
                  // CachedNetworkImage(
                  //   imageUrl: similarServiceState.selectedService.value?.serviceImages?.isNotEmpty == true
                  //       ? similarServiceState.selectedService.value!.serviceImages!.first
                  //       : '', // Or a placeholder image URL
                  //   fit: BoxFit.cover,
                  //   height: 300,
                  //   width: size.width,
                  // ),
                  // Positioned(
                  //   bottom: 10,
                  //   right: 10,
                  //   child: Column(
                  //     children: [
                  //       if (similarServiceState
                  //               .selectedService.value.serviceImages!.length >
                  //           1) ...{
                  //         Container(
                  //           height: 70,
                  //           width: 70,
                  //           margin: const EdgeInsets.symmetric(vertical: 2),
                  //           padding: const EdgeInsets.all(2),
                  //           decoration: BoxDecoration(
                  //             border: Border.all(color: Colors.white, width: 1),
                  //             borderRadius:
                  //                 const BorderRadius.all(Radius.circular(10)),
                  //           ),
                  //           child: ClipRRect(
                  //             borderRadius:
                  //                 const BorderRadius.all(Radius.circular(10)),
                  //             child: CachedNetworkImage(
                  //               imageUrl: similarServiceState
                  //                   .selectedService.value.serviceImages![1],
                  //               fit: BoxFit.cover,
                  //             ),
                  //           ),
                  //         ),
                  //       },
                  //       if (similarServiceState
                  //               .selectedService.value.serviceImages!.length >
                  //           2) ...{
                  //         Container(
                  //           height: 70,
                  //           width: 70,
                  //           margin: const EdgeInsets.symmetric(vertical: 2),
                  //           padding: const EdgeInsets.all(2),
                  //           decoration: BoxDecoration(
                  //             border: Border.all(color: Colors.white, width: 1),
                  //             borderRadius:
                  //                 const BorderRadius.all(Radius.circular(10)),
                  //           ),
                  //           child: ClipRRect(
                  //             borderRadius:
                  //                 const BorderRadius.all(Radius.circular(10)),
                  //             child: CachedNetworkImage(
                  //               imageUrl: similarServiceState
                  //                   .selectedService.value.serviceImages![2],
                  //               fit: BoxFit.cover,
                  //             ),
                  //           ),
                  //         ),
                  //       },
                  //       if (similarServiceState
                  //               .selectedService.value.serviceImages!.length >
                  //           3) ...{
                  //         Container(
                  //           height: 70,
                  //           width: 70,
                  //           margin: const EdgeInsets.symmetric(vertical: 2),
                  //           padding: const EdgeInsets.all(2),
                  //           decoration: BoxDecoration(
                  //             border: Border.all(color: Colors.white, width: 1),
                  //             borderRadius:
                  //                 const BorderRadius.all(Radius.circular(10)),
                  //           ),
                  //           child: ClipRRect(
                  //             borderRadius:
                  //                 const BorderRadius.all(Radius.circular(10)),
                  //             child: CachedNetworkImage(
                  //               imageUrl: similarServiceState
                  //                   .selectedService.value.serviceImages![3],
                  //               fit: BoxFit.cover,
                  //             ),
                  //           ),
                  //         ),
                  //       }
                  //     ],
                  //   ),
                  // ),
                  CachedNetworkImage(
                    imageUrl: similarServiceState.selectedService.value?.serviceImages?.isNotEmpty == true
                        ? similarServiceState.selectedService.value!.serviceImages!.first
                        : '', // Or a placeholder image URL
                    fit: BoxFit.cover,
                    height: 300,
                    width: size.width,
                  ),
                  Positioned(
                    bottom: 10,
                    right: 10,
                    child: Column(
                      children: [
                        if (similarServiceState.selectedService.value?.serviceImages != null &&
                            similarServiceState.selectedService.value!.serviceImages!.length >
                                1) ...{
                          Container(
                            height: 70,
                            width: 70,
                            margin: const EdgeInsets.symmetric(vertical: 2),
                            padding: const EdgeInsets.all(2),
                            decoration: BoxDecoration(
                              border: Border.all(color: Colors.white, width: 1),
                              borderRadius:
                              const BorderRadius.all(Radius.circular(10)),
                            ),
                            child: ClipRRect(
                              borderRadius:
                              const BorderRadius.all(Radius.circular(10)),
                              child: CachedNetworkImage(
                                imageUrl: similarServiceState
                                    .selectedService.value!.serviceImages![1],
                                fit: BoxFit.cover,
                                errorWidget: (context, url, error) => Icon(Icons.error),
                              ),
                            ),
                          ),
                        },
                        if (similarServiceState.selectedService.value?.serviceImages != null &&
                            similarServiceState.selectedService.value!.serviceImages!.length >
                                2) ...{
                          Container(
                            height: 70,
                            width: 70,
                            margin: const EdgeInsets.symmetric(vertical: 2),
                            padding: const EdgeInsets.all(2),
                            decoration: BoxDecoration(
                              border: Border.all(color: Colors.white, width: 1),
                              borderRadius:
                              const BorderRadius.all(Radius.circular(10)),
                            ),
                            child: ClipRRect(
                              borderRadius:
                              const BorderRadius.all(Radius.circular(10)),
                              child: CachedNetworkImage(
                                imageUrl: similarServiceState
                                    .selectedService.value!.serviceImages![2],
                                fit: BoxFit.cover,
                                errorWidget: (context, url, error) => Icon(Icons.error),

                              ),
                            ),
                          ),
                        },
                        if (similarServiceState.selectedService.value?.serviceImages != null &&
                            similarServiceState.selectedService.value!.serviceImages!.length >
                                3) ...{
                          Container(
                            height: 70,
                            width: 70,
                            margin: const EdgeInsets.symmetric(vertical: 2),
                            padding: const EdgeInsets.all(2),
                            decoration: BoxDecoration(
                              border: Border.all(color: Colors.white, width: 1),
                              borderRadius:
                              const BorderRadius.all(Radius.circular(10)),
                            ),
                            child: ClipRRect(
                              borderRadius:
                              const BorderRadius.all(Radius.circular(10)),
                              child: CachedNetworkImage(
                                imageUrl: similarServiceState
                                    .selectedService.value!.serviceImages![3],
                                fit: BoxFit.cover,
                                errorWidget: (context, url, error) => Icon(Icons.error),
                              ),
                            ),
                          ),
                        }
                      ],
                    ),
                  ),
                  Positioned(
                    bottom: 20,
                    child: Obx(
                      () => user.value != null
                          ? Row(
                              children: [
                                Container(
                                  decoration: BoxDecoration(
                                    color: Color(0x9742AC).withOpacity(1),
                                    borderRadius: const BorderRadius.all(
                                        Radius.circular(20)),
                                  ),
                                  padding: const EdgeInsets.symmetric(
                                      horizontal: 10, vertical: 5),
                                  margin: const EdgeInsets.symmetric(
                                      horizontal: 10),
                                  child: Row(
                                    children: [
                                      const Icon(
                                        Icons.star,
                                        color: Colors.amber,
                                        size: 18,
                                      ),
                                      const SizedBox(
                                        width: 5,
                                      ),
                                      SubTxtWidget(
                                        '${user.value!.providerUserModel!.overallRating}',
                                        color: Colors.white,
                                      ),
                                      const SizedBox(
                                        width: 5,
                                      ),
                                      SubTxtWidget(
                                          '(${user.value!.providerUserModel!.numReviews.toInt()})',
                                          color: Colors.white)
                                    ],
                                  ),
                                ),
                                Container(
                                  decoration: BoxDecoration(
                                    color: Color(0x9742AC).withOpacity(1),
                                    borderRadius: const BorderRadius.all(
                                        Radius.circular(20)),
                                  ),
                                  margin: const EdgeInsets.symmetric(
                                      horizontal: 10),
                                  padding: const EdgeInsets.symmetric(
                                      horizontal: 10, vertical: 5),
                                  child: SubTxtWidget(
                                    _con.selectedService.value!.serviceType,
                                    color: Colors.white,
                                  ),
                                )
                              ],
                            )
                          : SizedBox(),
                    ),
                  ),
                  if (!self())
                    InkWell(
                      onTap: () {
                        Get.toNamed('/add_provider_report',
                            arguments: user.value);
                      },
                      child: Container(
                        padding: const EdgeInsets.all(10),
                        child: Row(
                          children: [
                            const Icon(
                              Icons.report_outlined,
                              color: Colors.red,
                              size: 14,
                            ),
                            const SizedBox(
                              width: 5,
                            ),
                            SubTxtWidget(
                              'Report User',
                              color: Colors.white,
                              fontSize: 13,
                            ),
                          ],
                        ),
                      ),
                    )
                ],
              ),
            ),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 15, vertical: 10),
              child: Column(
                mainAxisAlignment: MainAxisAlignment.start,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Expanded(
                          child: Column(
                        children: [
                          Row(
                            children: [
                              Expanded(
                                child:
                                // HeaderTxtWidget(
                                //   similarServiceState.selectedService.value.serviceType,
                                //   fontSize: 25,
                                // ),
                                HeaderTxtWidget(
                                  similarServiceState.selectedService.value?.serviceType ?? 'Loading...',
                                  fontSize: 25,
                                ),
                              ),
                              HeaderTxtWidget(
                                '\$${similarServiceState.selectedService.value.servicePrice.toString()}',
                                fontSize: 25,
                              ),
                            ],
                          ),
                          Row(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              SvgPicture.asset(Assets.svgLocationGray),
                              const SizedBox(
                                width: 5,
                              ),
                              Expanded(
                                  child:
                              //     SubTxtWidget(
                              //   '${similarServiceState.selectedService.value.address}',
                              //   fontSize: 14.5,
                              //   fontWeight: FontWeight.w500,
                              // )
                                  SubTxtWidget(
                                    similarServiceState.selectedService.value?.address ?? 'Loading address...',
                                    fontSize: 14.5,
                                    fontWeight: FontWeight.w500,
                                  ),
                              ),
                            ],
                          ),
                        ],
                      )),
                      const SizedBox(
                        width: 20,
                      ),
                      if (!self())
                        ButtonPrimaryWidget(
                          'Book now',
                          width: 100,
                          height: 40,
                          padding: 0,
                          onTap: () {
                            submit();
                          },
                        )
                    ],
                  ),
                  const SizedBox(
                    height: 5,
                  ),
                  socialIcon(),
                  const SizedBox(
                    height: 5,
                  ),
                  Obx(
                    () => user.value == null
                        ? const SizedBox()
                        : Container(
                            padding: const EdgeInsets.all(10),
                            decoration: BoxDecoration(
                              borderRadius:
                                  const BorderRadius.all(Radius.circular(20)),
                              color: Colors.grey.shade200,
                            ),
                            child: Row(
                              children: [
                                SizedBox(
                                  height: 50,
                                  width: 50,
                                  child: ClipRRect(
                                    borderRadius: const BorderRadius.all(
                                        Radius.circular(50)),
                                    child: NetworkImageWidget(
                                        url: user.value!.photoURL),
                                  ),
                                ),
                                const SizedBox(
                                  width: 10,
                                ),
                                Expanded(
                                    child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    HeaderTxtWidget(user.value!.fullName),
                                    SubTxtWidget(
                                      'Ask a question...',
                                      fontSize: 9,
                                    ),
                                  ],
                                )),
                                if (!self())
                                  IconButton(
                                      onPressed: () {
                                        Get.to(ChatDetailScreen(
                                            userId: auth.value.uid,
                                            friendId: user.value!.uid));
                                      },
                                      icon: SvgPicture.asset(Assets.svgMessage))
                              ],
                            ),
                          ),
                  ),
                  const SizedBox(
                    height: 20,
                  ),
                  SubTxtWidget(
                    'Service Details',
                    fontSize: 20.0, // Font size in logical pixels
                    fontWeight: FontWeight.w500,
                  ),
                  const SizedBox(
                    height: 12,
                  ),
                  ReadMoreText(
                    similarServiceState.selectedService.value.description,
                  ),
                  const Divider(
                    height: 30,
                  ),
                  SubTxtWidget(
                    'Location & Public Facilities',
                    fontSize: 20.0, // Font size in logical pixels
                    fontWeight: FontWeight.w500,
                  ),
                  Container(
                    padding:
                        const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
                    margin: const EdgeInsets.symmetric(vertical: 10),
                    child: Row(
                      children: [
                        Image.asset(Assets.imgGroupLocation),
                        const SizedBox(
                          width: 5,
                        ),
                        Expanded(
                            child: SubTxtWidget(
                          similarServiceState.selectedService.value.address!,
                          fontSize: 14.5,
                          fontWeight: FontWeight
                              .w500, // FontWeight of 500 corresponds to FontWeight.w500
                        )),
                      ],
                    ),
                  ),
                  Container(
                    padding: const EdgeInsets.symmetric(
                        horizontal: 10, vertical: 10),
                    margin: const EdgeInsets.symmetric(vertical: 10),
                    decoration: BoxDecoration(
                      color: Colors.white,
                      border: Border.all(color: Colors.grey.shade300, width: 1),
                      borderRadius: const BorderRadius.all(Radius.circular(20)),
                    ),
                    child: Row(
                      children: [
                        const Icon(
                          Icons.location_on,
                          color: Colors.blue,
                        ),
                        const SizedBox(
                          width: 5,
                        ),
                        Expanded(
                            child: SubTxtWidget(
                                '${distance.toStringAsFixed(2)} mi from your location')),
                        const Icon(Icons.keyboard_arrow_down_outlined),
                      ],
                    ),
                  ),
                  const SizedBox(
                    height: 12,
                  ),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      HeaderTxtWidget(
                        'Select Date',
                        fontSize: 16.0, // Font size in logical pixels
                        fontWeight: FontWeight.w600,
                      ),
                      InkWell(
                        child: SubTxtWidget(
                          'Set Manually',
                          fontSize: 12.0, // Font size in logical pixels
                          fontWeight: FontWeight.w500,
                        ),
                        onTap: () {
                          showDatePicker(
                                  context: context,
                                  firstDate: DateTime.now(),
                                  lastDate: DateTime(2030))
                              .then(
                            (value) {
                              _con.selectedDate.value = value;
                              datePickerController.animateToDate(
                                  _con.selectedDate.value!,
                                  duration: const Duration(milliseconds: 500));
                            },
                          );
                        },
                      )
                    ],
                  ),
                  const SizedBox(
                    height: 12,
                  ),
                  DatePicker(
                    DateTime.now(),
                    initialSelectedDate: _con.selectedDate.value,
                    selectionColor: primaryColorCode,
                    selectedTextColor: Colors.white,
                    controller: datePickerController,
                    height: 100,
                    onDateChange: (date) {
                      _con.selectedDate.value = date;
                      _con.selectedTime.value = "";
                      _con.updateSlotList(context);
                    },
                  ),
                  const SizedBox(height: 20),
                  HeaderTxtWidget(
                    'Available time',
                    fontSize: 16.0, // Font size in logical pixels
                    fontWeight: FontWeight.w700,
                  ),
                  const SizedBox(height: 20),
                  // TimeWidget(
                  //   onChanged: (value) {
                  //     selectedTime2 = value;
                  //     _con.selectedTime.value = selectedTime2!;
                  //   },
                  //   selectedTime: selectedTime2,
                  // ),
                  Obx(
                    () => Wrap(
                      children: _con.timeList.map(
                        (e) {
                          return InkWell(
                            onTap: e.isDisable
                                ? null
                                : () {
                                    _con.selectedTime.value = e.time;
                                  },
                            child: Container(
                              margin: const EdgeInsets.all(5),
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 10, vertical: 10),
                              decoration: BoxDecoration(
                                  borderRadius: const BorderRadius.all(
                                      Radius.circular(10)),
                                  border: Border.all(
                                      color: e.isDisable
                                          ? Colors.grey.shade200
                                          : Colors.grey.shade400,
                                      width: 1),
                                  color: e.isDisable
                                      ? Colors.grey.shade200
                                      : _con.selectedTime.value == e.time
                                          ? Colors.blue.shade50
                                          : Colors.white),
                              child: SubTxtWidget(
                                e.time,
                                color: e.isDisable ? Colors.grey : Colors.black,
                              ),
                            ),
                          );
                        },
                      ).toList(),
                    ),
                  ),
                  const SizedBox(height: 20),
                  HeaderTxtWidget(
                    'Photos', fontSize: 22.0, // Font size in logical pixels
                    fontWeight: FontWeight.w700,
                  ),
                  const SizedBox(height: 20),
                  similarServiceState.selectedService.value?.serviceImages == null ?
                  Center(
                    child: HeaderTxtWidget("No photo found"),
                  ):
                  GridView.builder(
                    gridDelegate:
                    const SliverGridDelegateWithFixedCrossAxisCount(
                        crossAxisCount: 2,
                        crossAxisSpacing: 5,
                        mainAxisSpacing: 5),
                    itemBuilder: (context, index) {
                      return ClipRRect(
                        borderRadius: BorderRadius.circular(10),
                        child: NetworkImageWidget(
                            url: similarServiceState
                                .selectedService.value!.serviceImages![index]),
                      );
                    },
                    itemCount: similarServiceState
                        .selectedService.value?.serviceImages?.length ?? 0,
                    primary: false,
                    shrinkWrap: true,
                  ),
                  const SizedBox(height: 20),
                  const SizedBox(height: 20),
                  if (!self())
                    ButtonPrimaryWidget(
                      "Book Now",
                      onTap: () {
                        submit();
                      },
                      isLoading: isLoading,
                      radius: 30,
                    ),
                  const SizedBox(height: 20),
                  Padding(
                    padding: EdgeInsets.symmetric(horizontal: 8.0),
                    child: HeaderTxtWidget(
                      "Service Reviews",
                      fontSize: 16.0, // Font size in logical pixels
                      fontWeight: FontWeight.w500,
                    ),
                  ),
                  const SizedBox(height: 10),

                  Container(
                    padding: EdgeInsets.symmetric(horizontal: 8.0),
                    child: TextField(
                      decoration: InputDecoration(
                        prefixIcon: Icon(Icons.search, color: Colors.blueGrey),
                        hintText: 'Search Reviews',
                        hintStyle: TextStyle(
                          fontFamily: 'Plus Jakarta Sans',
                          fontSize: 16.0,
                          fontWeight: FontWeight.w400,
                          color: Colors.blueGrey.withOpacity(0.5),
                        ),
                        border: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(8.0),
                          borderSide: BorderSide(
                            color: Colors.blueGrey.withOpacity(0.3),
                          ),
                        ),
                        focusedBorder: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(8.0),
                          borderSide: BorderSide(
                            color: Colors.blueGrey,
                          ),
                        ),
                      ),
                    ),
                  ),
                  _review(),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget socialIcon() {
    return Container(
      padding: const EdgeInsets.symmetric(vertical: 20),
      alignment: AlignmentDirectional.center,
      child: Wrap(
        children: [
          SizedBox(
            width: 80,
            child: InkWell(
              onTap: () {
                userFirebase.launchGoogleMaps(
                    destinationLatitude: similarServiceState
                        .selectedService.value.location!.latitude,
                    destinationLongitude: similarServiceState
                        .selectedService.value.location!.longitude);
              },
              child: Column(
                children: [
                  SvgPicture.asset(Assets.svgLogosGoogleMaps),
                  const SizedBox(
                    height: 5,
                  ),
                  SubTxtWidget(
                    'Maps',
                    fontSize: 14,
                    fontWeight: FontWeight.w400,
                  ),
                ],
              ),
            ),
          ),
          SizedBox(
            width: 80,
            child: InkWell(
              onTap: () {
                _con.shareService(
                    similarServiceState.selectedService.value.serviceId);
              },
              child: Column(
                children: [
                  SvgPicture.asset(Assets.svgShare),
                  const SizedBox(
                    height: 5,
                  ),
                  SubTxtWidget(
                    'Share',
                    fontSize: 14,
                    fontWeight: FontWeight.w400,
                  ),
                ],
              ),
            ),
          ),
          SizedBox(
            width: 80,
            child: InkWell(
              child: Column(
                children: [
                  SvgPicture.asset(Assets.svgVerify),
                  const SizedBox(
                    height: 5,
                  ),
                  SubTxtWidget(
                    'Watchlist',
                    fontSize: 14,
                    fontWeight: FontWeight.w400,
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  void submit() {
    if (_con.selectedDate.value == null || _con.selectedTime.value.isEmpty) {
      Tools.ShowErrorMessage("Please select date and time");
      return;
    }
    Get.dialog(
      AlertDialog(
        title: const Text("Service Request Confirmation"),
        content: Container(
          height: MediaQuery.sizeOf(context).height * 0.3,
          decoration: BoxDecoration(
            color: Colors.white,
            border: Border.all(color: Colors.blue.shade200),
            borderRadius: BorderRadius.circular(9),
          ),
          child: Padding(
            padding: const EdgeInsets.all(8.0),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Row(
                  children: [
                    SubTxtWidget("Groomer :"),
                    const SizedBox(
                      width: 2,
                    ),
                    SubTxtWidget(
                        similarServiceState.selectedService.value.serviceType
                    ),
                  ],
                ),
                Row(
                  children: [
                    SubTxtWidget("Service Type :"),
                    const SizedBox(
                      width: 2,
                    ),
                    SubTxtWidget(
                        similarServiceState.selectedService.value.serviceType),
                  ],
                ),
                Row(
                  children: [
                    SubTxtWidget("Service Price :"),
                    const SizedBox(
                      width: 3,
                    ),
                    SubTxtWidget(
                      '\$${similarServiceState.selectedService.value.servicePrice.toString()}',
                    ),
                  ],
                ),
                Row(
                  children: [
                    SubTxtWidget("Selected Date :"),
                    const SizedBox(
                      width: 3,
                    ),
                    SubTxtWidget(Tools.changeDateFormat(
                        _con.selectedDate.toString(), 'MM-dd-yyyy'))
                  ],
                ),
                Row(
                  mainAxisAlignment: MainAxisAlignment.start,
                  children: [
                    SubTxtWidget(
                      " Selected Time :",
                      textAlign: TextAlign.start,
                    ),
                    SubTxtWidget(
                      " ${_con.selectedTime.value}",
                    ),
                  ],
                ),
              ],
            ),
          ),
        ),
        actions: [
          TextButton(
            onPressed: () {
              Get.back();
            },
            child: const Text("Cancel"),
          ),
          TextButton(
            onPressed: () async {
              Get.back();
              setState(() {
                isLoading = true;
              });
              String bookingId = widget.bookingId ?? generateServiceBookingId();
              final serviceBookingModel = ServiceBookingModel(
                  bookingId: bookingId,
                  serviceId:
                      similarServiceState.selectedService.value.serviceId,
                  providerId: similarServiceState.selectedService.value.userId,
                  clientId: auth.value.uid,
                  createdOn: DateTime.now().microsecondsSinceEpoch,
                  status: 'Pending',
                  selectedDate: _con.selectedDate.value.toString(),
                  selectedTime: _con.selectedTime.value,
                  serviceDetails: similarServiceState.selectedService.value);
              await serviceBookingFirebase.addBookingToFirebase(
                  bookingId, auth.value.uid, serviceBookingModel);
              setState(() {
                isLoading = false;
              });
              Tools.ShowSuccessMessage("Service request booking success");
              Get.to(() => CustomerDashboardPage());
            },
            child: Text("Confirm"),
          ),
        ],
      ),
    );
  }

  Widget _review() {
    return FirebaseAnimatedList(
      query: providerServiceFirebase
          .getReviewListByServiceId(_con.selectedService.value!.serviceId),
      itemBuilder: (context, snapshot, animation, index) {
        if (!snapshot.exists) {
          return Center(
            child: HeaderTxtWidget("No review found"),
          );
        }
        ReviewModel? data =
            ReviewModel.fromJson(jsonDecode(jsonEncode(snapshot.value)));
        return FutureBuilder(
          future: UserFirebase().getUser(data.userId!),
          builder: (context, snapshot) {
            if (snapshot.data == null) {
              return const Center(
                child: CircularProgressIndicator(),
              );
            }
            return Container(
              padding: const EdgeInsets.all(10),
              margin: const EdgeInsets.all(10),
              decoration: const BoxDecoration(
                  borderRadius: BorderRadius.all(Radius.circular(10)),
                  color: Colors.white),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      ClipRRect(
                        borderRadius:
                            const BorderRadius.all(Radius.circular(30)),
                        child: NetworkImageWidget(
                          url: snapshot.data!.photoURL,
                          height: 60,
                          width: 60,
                        ),
                      ),
                      const SizedBox(
                        width: 10,
                      ),
                      Expanded(
                          child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        mainAxisAlignment: MainAxisAlignment.start,
                        children: [
                          HeaderTxtWidget(
                            snapshot.data!.fullName,
                            fontSize: 24.0, // Font size in logical pixels
                            fontWeight: FontWeight.w700,
                          ),
                          RatingBarIndicator(
                            itemBuilder: (context, index) {
                              return const Icon(
                                Icons.star,
                                color: Colors.amber,
                              );
                            },
                            itemCount: 5,
                            rating: data.ratingValue ?? 0,
                            itemSize: 20,
                          )
                        ],
                      ))
                    ],
                  ),
                  SubTxtWidget(data.comment!),
                  SubTxtWidget("${data.createdOn!}"),
                ],
              ),
            );
          },
        );
      },
      defaultChild: LoadingWidget(),
      shrinkWrap: true,
      primary: false,
    );
  }

  bool self() {
    return auth.value.uid == similarServiceState.selectedService.value.userId;
  }
}
