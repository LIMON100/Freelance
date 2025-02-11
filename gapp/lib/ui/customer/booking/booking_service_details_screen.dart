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
import 'package:groom/ext/hex_color.dart';
import 'package:groom/firebase/service_booking_firebase.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/screens/chat_screen.dart';
import 'package:groom/ui/customer/services/provider_details_controller.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import 'package:groom/widgets/read_more_text.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../utils/tools.dart';
import '../../../data_models/user_model.dart';
import '../../../firebase/provider_service_firebase.dart';
import '../../../firebase/user_firebase.dart';
import '../../../generated/assets.dart';
import '../../../states/booking_state.dart';
import '../../../states/customer_service_state.dart';
import '../../../utils/colors.dart';
import '../../../widgets/loading_widget.dart';
import '../../provider/reviews/model/review_model.dart';
import '../customer_dashboard/customer_dashboard_page.dart';
import '../services/service_details_screen.dart';

class BookingServiceDetailsScreen extends StatefulWidget {
  ServiceBookingModel booking;
  BookingServiceDetailsScreen({super.key,required this.booking});

  @override
  State<BookingServiceDetailsScreen> createState() =>
      _SimilarServiceDisplayScreenState();
}

class _SimilarServiceDisplayScreenState extends State<BookingServiceDetailsScreen> {
  final ProviderDetailsController _con = Get.put(ProviderDetailsController());
  CustomerServiceState similarServiceState = Get.put(CustomerServiceState());
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
  UserFirebase userFirebase = UserFirebase();
  String groomerName = '';

  DatePickerController datePickerController=DatePickerController();
  bool isLoading=false;
  Rx<UserModel?>user=Rx(null);


  Future<UserModel> fetchUser(String userId) async {
    return await userFirebase.getUser(userId);
  }




  @override
  void initState() {
    // TODO: implement initState
    super.initState();
    _con.selectedService=similarServiceState.selectedService;
    fetchUser(similarServiceState.selectedService.value.userId).then((value) {
      user.value=value;
      _con.selectedProvider.value=value;
    },);
  }

  @override
  Widget build(BuildContext context) {
    double distance = 0.0;
    if (auth.value.location != null && similarServiceState.selectedService.value.location != null) {
      distance = Geolocator.distanceBetween(
        auth.value.location!.latitude,
        auth.value.location!.longitude,
        similarServiceState.selectedService.value.location!.latitude,
        similarServiceState.selectedService.value.location!.longitude,
      ) /
          1000; // Convert meters to kilometers
    }
    Size size=MediaQuery.of(context).size;
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        iconTheme: IconThemeData(color: primaryColorCode),
        title: HeaderTxtWidget("Service details"),
        actions: [
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 5,vertical: 2),
            decoration: BoxDecoration(
                color: "#EB833C".toColor().withOpacity(0.1),
                borderRadius: BorderRadius.circular(10)
            ),
            child: SubTxtWidget(widget.booking.status,color: "#EB833C".toColor(),fontSize: 12,),
          )
        ],
      ),

      body: SingleChildScrollView(
        child: Column(
          children: [
            SizedBox(
              height: 250,
              width: size.width,
              child: Stack(
                children: [
                  CachedNetworkImage(
                    imageUrl: similarServiceState.selectedService.value.serviceImages!.first,
                    fit: BoxFit.cover,
                    height: 300,
                    width: size.width,
                  ),
                  Positioned(bottom: 10,right: 10,child:Column(
                    children:[
                      if(similarServiceState.selectedService.value.serviceImages!.length>1)...{
                        Container(
                          height: 70,
                          width: 70,
                          margin: const EdgeInsets.symmetric(vertical: 2),
                          padding: const EdgeInsets.all(2),
                          decoration: BoxDecoration(
                            border: Border.all(color: Colors.white,width: 1),
                            borderRadius: const BorderRadius.all(Radius.circular(10)),
                          ),

                          child: ClipRRect(
                            borderRadius: const BorderRadius.all(Radius.circular(10)),
                            child: CachedNetworkImage(
                              imageUrl: similarServiceState.selectedService.value.serviceImages![1],
                              fit: BoxFit.cover,
                            ),
                          ),
                        ),
                      },
                      if(similarServiceState.selectedService.value.serviceImages!.length>2)...{
                        Container(
                          height: 70,
                          width: 70,
                          margin: const EdgeInsets.symmetric(vertical: 2),
                          padding: const EdgeInsets.all(2),
                          decoration: BoxDecoration(
                            border: Border.all(color: Colors.white,width: 1),
                            borderRadius: const BorderRadius.all(Radius.circular(10)),
                          ),
                          child: ClipRRect(
                            borderRadius: const BorderRadius.all(Radius.circular(10)),
                            child: CachedNetworkImage(
                              imageUrl: similarServiceState.selectedService.value.serviceImages![2],
                              fit: BoxFit.cover,
                            ),
                          ),
                        ),
                      },
                      if(similarServiceState.selectedService.value.serviceImages!.length>3)...{
                        Container(
                          height: 70,
                          width: 70,
                          margin: const EdgeInsets.symmetric(vertical: 2),
                          padding: const EdgeInsets.all(2),
                          decoration: BoxDecoration(
                            border: Border.all(color: Colors.white,width: 1),
                            borderRadius: const BorderRadius.all(Radius.circular(10)),
                          ),
                          child: ClipRRect(
                            borderRadius: const BorderRadius.all(Radius.circular(10)),
                            child: CachedNetworkImage(
                              imageUrl: similarServiceState.selectedService.value.serviceImages![3],
                              fit: BoxFit.cover,
                            ),
                          ),
                        ),
                      }
                    ],
                  ),),
                  Positioned(bottom: 20,
                    child: Obx(() => user.value!=null?Row(
                      children: [
                        Container(
                          decoration: BoxDecoration(
                            color: primaryColorCode,
                            borderRadius: const BorderRadius.all(Radius.circular(20)),
                          ),
                          padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 5),
                          margin: const EdgeInsets.symmetric(horizontal: 10),
                          child: Row(
                            children: [
                              const Icon(Icons.star,color: Colors.amber,size: 18,),
                              const SizedBox(width: 5,),
                              SubTxtWidget('${user.value!.providerUserModel!.overallRating.toStringAsFixed(1)}',color: Colors.white,),
                              const SizedBox(width: 5,),
                              SubTxtWidget('(${user.value!.providerUserModel!.numReviews.toInt()})',color: Colors.white)
                            ],
                          ),
                        ),
                        Container(
                          decoration: BoxDecoration(
                            color: primaryColorCode,
                            borderRadius: const BorderRadius.all(Radius.circular(20)),
                          ),
                          margin: const EdgeInsets.symmetric(horizontal: 10),
                          padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 5),
                          child: SubTxtWidget(_con.selectedService.value!.serviceType,color: Colors.white,),
                        )
                      ],
                    ):SizedBox(),),),
                ],
              ),
            ),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 15,vertical: 10),
              child: Column(
                mainAxisAlignment: MainAxisAlignment.start,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Expanded(child: Column(
                        children: [
                          Row(
                            children: [
                              Expanded(child: HeaderTxtWidget(similarServiceState.selectedService.value.serviceType,fontSize: 25,),),
                              HeaderTxtWidget('\$${similarServiceState.selectedService.value.servicePrice.toString()}',fontSize: 25,),
                            ],
                          ),
                          Row(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              SvgPicture.asset(Assets.svgLocationGray),
                              const SizedBox(
                                width: 5,
                              ),
                              Expanded(child: SubTxtWidget('${similarServiceState.selectedService.value.address}')),
                            ],
                          ),
                        ],
                      )),
                      const SizedBox(width: 20,),
                    ],
                  ),
                  const SizedBox(height: 5,),
                  Obx(() =>user.value==null?const SizedBox():Container(
                    padding: const EdgeInsets.all(10),
                    decoration: BoxDecoration(
                      borderRadius: const BorderRadius.all(Radius.circular(20)),
                      color: Colors.grey.shade200,
                    ),
                    child: Row(
                      children: [
                        SizedBox(
                          height: 50,
                          width: 50,
                          child: ClipRRect(
                            borderRadius: const BorderRadius.all(Radius.circular(50)),
                            child: NetworkImageWidget(url: user.value!.photoURL),
                          ),
                        ),
                        const SizedBox(width: 10,),
                        Expanded(child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            HeaderTxtWidget(user.value!.fullName),
                            SubTxtWidget('Ask a question...',fontSize: 9,),
                          ],
                        )),
                        IconButton(onPressed: (){
                          Get.to(ChatDetailScreen(userId: auth.value.uid, friendId: user.value!.uid));
                        }, icon: SvgPicture.asset(Assets.svgMessage))
                      ],
                    ),
                  ),),
                  const SizedBox(
                    height: 20,
                  ),
                  SubTxtWidget('Service Details',fontSize: 20,),
                  const SizedBox(
                    height: 12,
                  ),
                  ReadMoreText(similarServiceState.selectedService.value.description,),
                  const Divider(
                    height: 30,
                  ),
                  SubTxtWidget('Location & Public Facilities',fontSize: 20,),
                  Container(
                    padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 5),
                    margin: const EdgeInsets.symmetric(vertical: 10),
                    child: Row(
                      children: [
                        Image.asset(Assets.imgGroupLocation),
                        const SizedBox(
                          width: 5,
                        ),
                        Expanded(
                            child: SubTxtWidget(similarServiceState.selectedService.value.address!)
                        ),
                      ],
                    ),
                  ),
                  Container(
                    padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 10),
                    margin: const EdgeInsets.symmetric(vertical: 10),
                    decoration: BoxDecoration(
                      color: Colors.white,
                      border: Border.all(color: Colors.grey.shade300,width: 1),
                      borderRadius: const BorderRadius.all(Radius.circular(20)),
                    ),
                    child: Row(
                      children: [
                        const Icon(Icons.location_on,color: Colors.blue,),
                        const SizedBox(
                          width: 5,
                        ),
                        Expanded(child: SubTxtWidget('${distance.toStringAsFixed(2)} mi from your location')),
                        const Icon(Icons.keyboard_arrow_down_outlined),
                      ],
                    ),
                  ),

                  const SizedBox(
                    height: 12,
                  ),
                  HeaderTxtWidget('Booking Details'),
                  const SizedBox(height: 10),
                  Container(
                    margin: const EdgeInsets.symmetric(horizontal: 10),
                    decoration: BoxDecoration(
                        borderRadius: BorderRadius.circular(8.0),
                        color: Colors.white,
                        boxShadow: [
                          BoxShadow(
                              color: Colors.grey.shade200,
                              blurRadius: 2,
                              spreadRadius: 3
                          )
                        ]
                    ),
                    padding: const EdgeInsets.all(5),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      mainAxisAlignment: MainAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            SubTxtWidget('Booking Date'),
                            const SizedBox(width: 10,),
                            const Icon(Icons.calendar_month,
                              color: Colors.blue,size: 14,),
                            Text(Tools.changeDateFormat(widget.booking.selectedDate, globalTimeFormat)),
                          ],
                        ),
                        Row(
                          children: [
                            SubTxtWidget('Booking Time'),
                            const SizedBox(width: 10,),
                            const Icon(Icons.timelapse,
                                size: 14,
                                color: Colors.blue),
                            Text(widget.booking.selectedTime),
                          ],
                        ),
                        const SizedBox(height: 5,),
                        button(widget.booking),
                      ],
                    ),
                  ),
                  const SizedBox(height: 20),
                  HeaderTxtWidget('Photos'),
                  const SizedBox(height: 20),
                  GridView.builder(gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(crossAxisCount: 2,
                      crossAxisSpacing: 5,mainAxisSpacing: 5),
                    itemBuilder: (context, index) {
                      return ClipRRect(
                        borderRadius: BorderRadius.circular(10),
                        child: NetworkImageWidget(url: similarServiceState.selectedService.value.serviceImages![index]),
                      );
                    },itemCount:similarServiceState.selectedService.value.serviceImages!.length ,
                    primary: false,shrinkWrap: true,),
                  const SizedBox(height: 20),
                ],
              ),
            ),

          ],
        ),
      ),
    );
  }
  Widget button(ServiceBookingModel booking){
    if(booking.status == "Confirmed"){
      return Wrap(
        children: [
          TextButton(
            onPressed: () async {
              BookingState bookingState = Get.put(BookingState());
              bookingState.selectedBooking.value=booking;
              Get.toNamed('/payment_screen');
            },
            style: ButtonStyle(
              backgroundColor:
              WidgetStateProperty.all(Colors.yellowAccent.shade700),
            ),
            child: SubTxtWidget('Confirm Reservation'),
          )
        ],
      );
    }
    if(booking.status == "Pending") {
      return Wrap(
        children: [
          TextButton(
            onPressed: () async {
              Tools.ShowBottomSheet(context, Container(
                height: MediaQuery.sizeOf(context).height*0.5,
                padding: const EdgeInsets.all(15),
                decoration: const BoxDecoration(
                    color: Colors.white,
                    borderRadius: BorderRadius.only(topLeft: Radius.circular(20),topRight: Radius.circular(20))
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Center(
                      child: SizedBox(
                        width: 100,
                        child: Divider(
                          thickness: 3,
                        ),
                      ),
                    ),
                    const SizedBox(height: 10,),
                    HeaderTxtWidget('Are you sure you want to Cancel this Approved Booking?',fontSize: 20,),
                    const SizedBox(height: 10,),
                    SubTxtWidget('You will be subject to the cancellation policies that apply. This may cause the loss of your deposit.'),
                    const SizedBox(height: 30,),
                    TextButton(
                      onPressed: () async {
                        Navigator.pop(context);
                        CustomerServiceState similarServiceState = Get.put(CustomerServiceState());
                        ProviderServiceFirebase().getServiceByServiceId(booking.serviceId).then((value) {
                          similarServiceState.selectedService.value = value;
                          Get.to(() =>  ServiceDetailsScreen(bookingId: booking.bookingId,));
                        },);
                      },
                      style: ButtonStyle(
                        backgroundColor:
                        WidgetStateProperty.all('#367604'.toColor()),
                      ),
                      child: SubTxtWidget('Reschedule', color: Colors.white,),
                    ),
                    const SizedBox(height: 20,),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                      children: [
                        TextButton(
                          onPressed: () async {
                            Navigator.pop(context);
                          },
                          style: ButtonStyle(
                            backgroundColor:
                            WidgetStateProperty.all(Colors.grey.shade100),
                          ),
                          child: SubTxtWidget('Cancel', color: Colors.black,),
                        ),
                        TextButton(
                          onPressed: () async {
                            Navigator.pop(context);
                            serviceBookingFirebase.updateBookingStatus(booking.bookingId, "Canceled").then((value) {
                              Navigator.pop(context);
                            },);
                          },
                          style: ButtonStyle(
                            backgroundColor:
                            WidgetStateProperty.all(Colors.red),
                          ),
                          child: SubTxtWidget('Confirm', color: Colors.white,),
                        ),
                        const SizedBox(width: 10,),
                      ],
                    )
                  ],
                ),
              ));
            },
            style: ButtonStyle(
              backgroundColor:
              WidgetStateProperty.all(Colors.red.shade700),
            ),
            child: SubTxtWidget('Cancel', color: Colors.white,),
          ),
        ],
      );
    }
    return const SizedBox();
  }
}
