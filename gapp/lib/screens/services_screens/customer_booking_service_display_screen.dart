import 'package:cached_network_image/cached_network_image.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/firebase/service_booking_firebase.dart';
import 'package:groom/states/booking_state.dart';
import 'package:groom/utils/utils.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/loading_widget.dart';
import '../../data_models/service_booking_model.dart';
import '../../data_models/user_model.dart';
import '../../firebase/provider_service_firebase.dart';
import '../../firebase/user_firebase.dart';
import '../../repo/setting_repo.dart';
import '../../states/customer_service_state.dart';
import '../../ui/customer/booking/booking_service_details_screen.dart';
import '../../ui/customer/services/service_details_screen.dart';
import '../../utils/tools.dart';
import '../../widgets/sub_txt_widget.dart';
import '../chat_screen.dart';

class CustomerServiceBookingDisplayScreen extends StatefulWidget {
   CustomerServiceBookingDisplayScreen({super.key});

  @override
  State<CustomerServiceBookingDisplayScreen> createState() => _CustomerServiceBookingDisplayScreenState();
}

class _CustomerServiceBookingDisplayScreenState extends State<CustomerServiceBookingDisplayScreen> {
  BookingState bookingState = Get.put(BookingState());
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
  UserFirebase userFirebase = UserFirebase();
  @override
  void initState() {
    // TODO: implement initState
    super.initState();
    getList();
  }
  void getList(){
    bookingState.isLoading.value=true;
    serviceBookingFirebase.getServiceBookingByUserId(auth.value.uid)
        .then((bookings) {
      bookingState.isLoading.value=false;
      bookingState.bookings.value = bookings;
    });
  }
  Future<UserModel> fetchUser(String userId) async {
    return await userFirebase.getUser(userId);
  }
  @override
  Widget build(BuildContext context) {
    return Obx(() {
      if (bookingState.isLoading.value) {
        return LoadingWidget(type: LoadingType.LIST,);
      }if (bookingState.bookings.isEmpty) {
        return Center(
          child: SubTxtWidget("No Service found", fontWeight: FontWeight.w300,),
        );
      }
      return ListView.builder(
        itemCount: bookingState.bookings.length,
        padding: const EdgeInsets.symmetric(vertical: 5),
        shrinkWrap: true,
        primary: false,
        itemBuilder: (context, index) {
          var booking = bookingState.bookings[index];
          var service = booking.serviceDetails;
          booking.providerId=service!.userId;
          return FutureBuilder(
            future: fetchUser(service.userId),
            builder: (context, snapshot) {
              if (snapshot.connectionState == ConnectionState.waiting) {
                return const Text("");
              } else if (snapshot.hasError) {
                return const Text("User deleted");
              } else if (snapshot.hasData) {
                var user = snapshot.data;
                return InkWell(
                  onTap: (){
                    CustomerServiceState similarServiceState = Get.put(CustomerServiceState());
                    ProviderServiceFirebase().getServiceByServiceId(booking.serviceId).then((value) {
                      similarServiceState.selectedService.value = value;
                      Get.to(() =>  BookingServiceDetailsScreen(booking: booking,))!.then((value) {
                        getList();
                      },);
                    },);
                  },
                  child: Container(
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
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.start,
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Expanded(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            mainAxisAlignment: MainAxisAlignment.start,
                            children: [
                              Row(
                                children: [
                                  Expanded(child:  HeaderTxtWidget('${service.serviceName}'),),
                                  Container(
                                    padding: const EdgeInsets.symmetric(horizontal: 5,vertical: 2),
                                    decoration: BoxDecoration(
                                        color: "#EB833C".toColor().withOpacity(0.1),
                                        borderRadius: BorderRadius.circular(10)
                                    ),
                                    child: SubTxtWidget(booking.status,color: "#EB833C".toColor(),fontSize: 12,),
                                  )
                                ],),
                              Row(
                                children: [
                                  Expanded(child:  user!.providerUserModel!
                                      .providerType !=
                                      "Independent"
                                      ? SubTxtWidget(
                                    user.providerUserModel!
                                        .salonTitle!
                                        .capitalizeFirst!,
                                    color: Colors.blue,
                                    fontSize: 14,
                                  )
                                      : SubTxtWidget(
                                    user.fullName
                                        .capitalizeFirst!,
                                    color: Colors.blue,
                                    fontSize: 14,
                                  ),),
                                  Expanded(child:  Row(
                                    mainAxisAlignment: MainAxisAlignment.end,
                                    children: [
                                      const Icon(Icons.calendar_month,
                                        color: Colors.blue,size: 14,),
                                      Text(Tools.changeDateFormat(booking.selectedDate, 'MM-dd-yyyy')),
                                    ],
                                  ),)
                                ],
                              ),
                              Row(
                                children: [
                                  Row(
                                    children: [
                                      const Icon(Icons.timelapse,
                                          size: 14,
                                          color: Colors.blue),
                                      Text(booking.selectedTime),
                                    ],
                                  ),
                                  const Spacer(),
                                  const Icon(
                                      Icons
                                          .monetization_on_rounded,
                                      size: 14,
                                      color: Colors.green),
                                  Text(
                                    "${service.servicePrice} \$",
                                  ),
                                ],
                              ),
                              const SizedBox(height: 5,),
                              button(booking),
                            ],
                          ),
                        ),
                        const SizedBox(
                          width: 7,
                        ),
                        ClipRRect(
                          borderRadius: BorderRadius.circular(8),
                          child: CachedNetworkImage(
                            fit: BoxFit.fill,
                            width: 100,
                            height: 120,
                            imageUrl: service.serviceImages!.first,
                          ),
                        ),
                      ],
                    ),
                  ),
                );
              }
              return const Text("No data");
            },
          );
        },
      );
    });
  }
  Widget button(ServiceBookingModel booking){
    if(booking.status == "Confirmed"){
      return Wrap(
        children: [
          TextButton(
            onPressed: () async {
              bookingState.selectedBooking.value=booking;
              Get.toNamed('/payment_screen')!.then((value) {
                getList();
              },);
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
              Get.to(() =>
                  ChatDetailScreen(
                      userId: auth.value.uid,
                      friendId: booking.providerId));
            },
            style: ButtonStyle(
              backgroundColor:
              WidgetStateProperty.all(Colors.grey.shade100),
            ),
            child: SubTxtWidget('Chat', color: Colors.black,),
          ),
          const SizedBox(width: 10,),
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
                                getList();
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
