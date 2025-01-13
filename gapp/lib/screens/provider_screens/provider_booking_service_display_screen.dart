import 'package:cached_network_image/cached_network_image.dart';
import 'package:carousel_slider/carousel_slider.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:font_awesome_flutter/font_awesome_flutter.dart';
import 'package:get/get.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:groom/data_models/service_booking_model.dart';
import 'package:groom/firebase/firebase_notifications.dart';
import 'package:groom/firebase/notifications_firebase.dart';
import 'package:groom/firebase/service_booking_firebase.dart';
import 'package:groom/screens/chat_screen.dart';
import 'package:groom/ui/customer/customer_dashboard/customer_dashboard_page.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import 'package:groom/widgets/read_more_text.dart';
import 'package:intl/intl.dart';
import '../../utils/tools.dart';
import '../../data_models/notification_model.dart';
import '../../data_models/user_model.dart';
import '../../firebase/provider_service_firebase.dart';
import '../../firebase/user_firebase.dart';
import '../../repo/setting_repo.dart';
import '../../states/customer_service_state.dart';
import '../../utils/colors.dart';
import '../../utils/utils.dart';
import '../services_screens/provider_edit_service_screen.dart';

class ProviderBookingServiceDisplayScreen extends StatefulWidget {
  const ProviderBookingServiceDisplayScreen({super.key});

  @override
  State<ProviderBookingServiceDisplayScreen> createState() =>
      _ProviderBookingServiceDisplayScreenState();
}

class _ProviderBookingServiceDisplayScreenState
    extends State<ProviderBookingServiceDisplayScreen> {
  String selectedFilter = "All";
  final TextEditingController _searchController = TextEditingController();

  CustomerServiceState customerServiceState = Get.find();
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  ServiceBookingFirebase serviceBookingFirebase = ServiceBookingFirebase();
  UserFirebase userFirebase = UserFirebase();
  NotificationsFirebase notificationsFirebase =NotificationsFirebase();
  String groomerName = '';
  DateTime? selectedDate;
  TimeOfDay? selectedTime;
  String _selectedFilter = "All";
  String _selectedSort = "Rating";
  String _searchQuery = "";
  UserModel? _clientUser;
  UserModel? _providerUser;

  final List<String> filters = [
    "All",
    "Makeup",
    "Hair Style",
    "Nails",
    "Coloring",
    "Wax",
    "Spa",
    "Massage",
    "Facial"
  ];
  final List<String> sortOptions = ["Rating", "Price"];

  String formatDate(int timestamp) {
    // Convert the timestamp to a DateTime object
    DateTime date = DateTime.fromMillisecondsSinceEpoch(timestamp);

    // Format the date as day/month
    DateFormat dateFormat = DateFormat('dd/MM');
    return dateFormat.format(date);
  }

  String formatTime(int timestamp) {
    // Convert the timestamp to a DateTime object
    DateTime date = DateTime.fromMillisecondsSinceEpoch(timestamp);

    // Format the time as HH:mm
    DateFormat timeFormat = DateFormat('HH:mm');
    return timeFormat.format(date);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget('Service Details',color: Colors.white,),
      ),
      body: CustomScrollView(
        slivers: [
          SliverToBoxAdapter(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.start,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                CarouselSlider(
                  items: customerServiceState.selectedService.value.serviceImages!
                      .map((e) {
                    return Container(
                      margin: const EdgeInsets.all(5.0),
                      child: ClipRRect(
                        borderRadius: BorderRadius.all(Radius.circular(5.0)),
                        child: CachedNetworkImage(
                          imageUrl: e,
                          fit: BoxFit.cover,
                          width: 1000.0,
                        ),
                      ),
                    );
                  }).toList(),
                  options: CarouselOptions(
                    height: 300,
                    aspectRatio: 16 / 9,
                    viewportFraction: 0.8,
                    initialPage: 0,
                    enableInfiniteScroll: true,
                    reverse: false,
                    autoPlay: true,
                    autoPlayInterval: const Duration(seconds: 3),
                    autoPlayAnimationDuration: const Duration(milliseconds: 800),
                    autoPlayCurve: Curves.fastOutSlowIn,
                    enlargeCenterPage: true,
                    scrollDirection: Axis.horizontal,
                  ),
                ),
                SizedBox(
                  height: 12,
                ),
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 15, vertical: 10),
                  child: Column(
                    children: [
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Text(
                            customerServiceState.selectedService.value.serviceType,
                            style: GoogleFonts.manrope(
                                fontSize: 25,
                                fontWeight: FontWeight.bold,
                                color: titleColor),
                          ),
                          Row(
                            children: [
                              IconButton(
                                icon: Icon(Icons.edit, color: Colors.blue, size: 30),
                                onPressed: () {
                                  Get.to(() => EditProviderServiceScreen(
                                    service:
                                    customerServiceState.selectedService.value,
                                  ));
                                },
                              ),
                              SizedBox(width: 12),
                              IconButton(
                                icon: Icon(Icons.delete,
                                    color: Colors.redAccent, size: 30),
                                onPressed: () {
                                  Get.dialog(AlertDialog(
                                    title: Text("Confirm Delete"),
                                    content: Text(
                                        "Are you sure you want to delete this service?"),
                                    actions: [
                                      TextButton(
                                        onPressed: () {
                                          Get.back();
                                        },
                                        child: Text("Cancel"),
                                      ),
                                      TextButton(
                                        onPressed: () async {
                                          bool result = await providerServiceFirebase
                                              .deleteServiceById(customerServiceState
                                              .selectedService.value.serviceId);
                                          if (result) {
                                            ScaffoldMessenger.of(context).showSnackBar(
                                              SnackBar(
                                                content: Text(
                                                    'Service deleted successfully'),
                                                backgroundColor: Colors.green,
                                              ),
                                            );
                                            Get.off(()=>CustomerDashboardPage());
                                            // Navigate back or refresh the list
                                          } else {
                                            ScaffoldMessenger.of(context).showSnackBar(
                                              SnackBar(
                                                content:
                                                Text('Failed to delete service'),
                                                backgroundColor: Colors.red,
                                              ),
                                            );
                                          }
                                        },
                                        child: Text("Delete"),
                                      ),
                                    ],
                                  ));
                                },
                              )
                            ],
                          )
                        ],
                      ),
                      HeaderTxtWidget('${groomerName.capitalizeFirst}'),
                      Row(
                        children: [
                          FaIcon(Icons.price_change),
                          Padding(
                            padding:
                            const EdgeInsets.only(left: 8.0, right: 8,),
                            child: Text(
                              customerServiceState
                                  .selectedService.value.servicePrice
                                  .toString(),
                              style: GoogleFonts.nunitoSans(
                                  fontSize: 14,
                                  fontWeight: FontWeight.w500,
                                  color: chatColor),
                            ),
                          ),
                        ],
                      )
                    ],
                  ),
                ),
                const Padding(
                  padding: EdgeInsets.symmetric(horizontal: 25.0),
                  child: Divider(
                    height: 8,
                    color: Colors.grey,
                  ),
                ),
                Padding(
                  padding: const EdgeInsets.only(left: 8.0, right: 8, top: 8),
                  child: Text(
                    "About Service :",
                    style: GoogleFonts.manrope(
                        fontSize: 16,
                        fontWeight: FontWeight.bold,
                        color: titleColor),
                  ),
                ),
                SizedBox(
                  height: 12,
                ),
                Padding(
                  padding: const EdgeInsets.only(left: 8.0, right: 8, top: 8),
                  child: ReadMoreText(
                    customerServiceState.selectedService.value.description,
                  ),
                ),
                SizedBox(
                  height: 12,
                ),
                Padding(
                  padding: const EdgeInsets.only(left: 8.0, right: 8, top: 8),
                  child: Text(
                    "Booking Requests :",
                    style: GoogleFonts.manrope(
                        fontSize: 16,
                        fontWeight: FontWeight.bold,
                        color: titleColor),
                  ),
                ),
                SizedBox(
                  height: 12,
                ),
               /* Row(
                  children: [
                    Expanded(
                      child: TextField(
                        controller: _searchController,
                        onChanged: (value) {
                          setState(() {
                            _searchQuery = value;
                          });
                        },
                        decoration: InputDecoration(
                          hintText: 'Search...',
                          border: OutlineInputBorder(
                            borderRadius: BorderRadius.circular(8),
                          ),
                          suffixIcon: Icon(Icons.search),
                        ),
                      ),
                    ),
                    const SizedBox(width: 8),
                    DropdownButton<String>(
                      value: _selectedFilter,
                      items: filters.map((String filter) {
                        return DropdownMenuItem<String>(
                          value: filter,
                          child: Text(filter),
                        );
                      }).toList(),
                      onChanged: (String? newValue) {
                        setState(() {
                          _selectedFilter = newValue!;
                        });
                      },
                    ),
                    SizedBox(width: 8),
                    DropdownButton<String>(
                      value: _selectedSort,
                      items: sortOptions.map((String sortOption) {
                        return DropdownMenuItem<String>(
                          value: sortOption,
                          child: Text(sortOption),
                        );
                      }).toList(),
                      onChanged: (String? newValue) {
                        setState(() {
                          _selectedSort = newValue!;
                        });
                      },
                    ),
                  ],
                ),*/
                SizedBox(
                  height: MediaQuery.sizeOf(context).height*0.6,
                  width: double.infinity,
                  child: FutureBuilder(
                    future: serviceBookingFirebase.getServiceBookingByServiceId(customerServiceState.selectedService.value.serviceId),
                    builder: (context, snapshot) {
                      if (snapshot.connectionState == ConnectionState.waiting) {
                        return const Text("");
                      } else if (snapshot.hasError) {
                        return Text("Error: ${snapshot.error}");
                      } else if (snapshot.hasData) {
                        var services = snapshot.data as List<ServiceBookingModel>;
                        return ListView.builder(
                            itemCount: services.length,
                            padding: const EdgeInsets.symmetric(vertical: 10),
                            primary: false,
                            shrinkWrap: true,
                            itemBuilder: (context, index) {
                              var service = services[index];
                              return FutureBuilder(
                                  future: userFirebase.getUser(service.clientId),
                                  builder: (context, snapshot) {
                                    if (snapshot.connectionState ==
                                        ConnectionState.waiting) {
                                      return const Text("");
                                    } else if (snapshot.hasError) {
                                      return Text("Error: ${snapshot.error}");
                                    } else if (snapshot.hasData) {
                                      var client = snapshot.data;
                                      _clientUser = client;
                                      return Card(
                                        margin: const EdgeInsets.symmetric(horizontal: 5,vertical: 5),
                                        child: Row(
                                          crossAxisAlignment: CrossAxisAlignment.start,
                                          children: [
                                           NetworkImageWidget(url: _clientUser!.photoURL,height: 80,width: 80,),
                                            const SizedBox(
                                              width: 12,
                                            ),
                                            Expanded(
                                              child: Column(
                                                crossAxisAlignment:
                                                CrossAxisAlignment
                                                    .start,
                                                children: [
                                                  Text(
                                                    client!.fullName,
                                                    style: TextStyle(
                                                        fontSize: 18,
                                                        fontWeight:
                                                        FontWeight
                                                            .bold),
                                                  ),
                                                  Row(
                                                    children: [
                                                      Expanded(child:  Row(
                                                        children: [
                                                          Icon(Icons
                                                              .calendar_month),
                                                          Text(
                                                              "${Tools.changeDateFormat(service.selectedDate, 'MM-dd-yyyy')}"),

                                                        ],
                                                      ),),
                                                      Expanded(child: Row(
                                                        children: [
                                                          Icon(Icons.timelapse),
                                                          Text(
                                                            service
                                                                .selectedTime,
                                                          ),
                                                        ],
                                                      ),)
                                                    ],
                                                  ),
                                                  Text(
                                                    service.status,
                                                  ),
                                                  Row(
                                                    mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                                                    crossAxisAlignment:
                                                    CrossAxisAlignment
                                                        .center,
                                                    children: [
                                                      if(service.status == 'Pending')
                                                      TextButton(
                                                        onPressed: () {
                                                          if (service.status != 'Pending') {
                                                            Get.dialog(
                                                                AlertDialog(
                                                                  title: Text(
                                                                      "Booking Already Confirmed"),
                                                                  content: Text(
                                                                      "You have already confirmed this booking."),
                                                                  actions: [
                                                                    TextButton(
                                                                      onPressed:
                                                                          () {
                                                                        Get.back();
                                                                      },
                                                                      child: Text(
                                                                          "OK"),
                                                                    ),
                                                                  ],
                                                                ));
                                                          } else {
                                                            Get.dialog(
                                                                AlertDialog(
                                                                  title: Text("Request Confirmation"),
                                                                  content:
                                                                  SizedBox(
                                                                    height:
                                                                    MediaQuery.sizeOf(context).height *
                                                                        0.2,
                                                                    child:
                                                                    Column(
                                                                      children: [
                                                                        Text(
                                                                            "Deposit payment will be sent to the client, You will be notified when the reservation is confirmed")
                                                                      ],
                                                                    ),
                                                                  ),
                                                                  actions: [
                                                                    TextButton(
                                                                      onPressed:
                                                                          () {
                                                                        Get.back();
                                                                      },
                                                                      child: Text(
                                                                          "Cancel"),
                                                                    ),
                                                                    TextButton(
                                                                      onPressed:
                                                                          () async {
                                                                        bool result = await serviceBookingFirebase.updateBookingStatus(
                                                                            service.bookingId,
                                                                            "Confirmed");
                                                                        if (result) {
                                                                          ScaffoldMessenger.of(context)
                                                                              .showSnackBar(
                                                                            SnackBar(
                                                                              content: Text('Booking confirmed'),
                                                                              backgroundColor: Colors.green,
                                                                            ),
                                                                          );
                                                                          PushNotificationService.sendProviderConfirmReservationNotificationToClient(
                                                                              _clientUser!.notificationToken!,
                                                                              context,
                                                                              "",
                                                                              auth.value.providerUserModel!.providerType != "Independent"
                                                                                  ? auth.value.providerUserModel!.salonTitle!
                                                                                  : auth.value.fullName);

                                                                          NotificationModel notificationModel =
                                                                          NotificationModel(
                                                                              userId:   _clientUser!.uid,
                                                                              createdOn: DateTime.now()
                                                                                  .millisecondsSinceEpoch,
                                                                              notificationText: "Your Groomer has accepted your service request",
                                                                              type: "SERVICE",
                                                                              sendBy: "PROVIDER",
                                                                              serviceId: service.serviceId,
                                                                              notificationId: generateProjectId());
                                                                          notificationsFirebase
                                                                              .writeNotificationToUser(
                                                                              _clientUser!.uid,
                                                                              notificationModel,
                                                                              generateProjectId());

                                                                          Get.back();
                                                                          setState(
                                                                                  () {
                                                                                service.status =
                                                                                "Confirmed";
                                                                              });
                                                                        } else {
                                                                          ScaffoldMessenger.of(context)
                                                                              .showSnackBar(
                                                                            const SnackBar(
                                                                              content: Text('Failed to confirm booking'),
                                                                              backgroundColor: Colors.red,
                                                                            ),
                                                                          );
                                                                        }
                                                                      },
                                                                      child: Text(
                                                                          "Confirm"),
                                                                    ),
                                                                  ],
                                                                ));
                                                          }
                                                        },
                                                        style: ButtonStyle(
                                                            backgroundColor:
                                                            WidgetStateProperty
                                                                .all(Colors
                                                                .green)),
                                                        child: const Icon(Icons.check),
                                                      ),
                                                      TextButton(
                                                        onPressed: () {
                                                          Get.to(() => ChatDetailScreen(
                                                                userId: FirebaseAuth
                                                                    .instance
                                                                    .currentUser!
                                                                    .uid,
                                                                friendId:
                                                                client
                                                                    .uid),
                                                          );
                                                        },
                                                        style: ButtonStyle(
                                                          backgroundColor:
                                                          WidgetStateProperty.all(Colors.grey.shade200),
                                                        ),
                                                        child: Icon(Icons.chat_bubble),
                                                      ),
                                                    ],
                                                  )
                                                ],
                                              ),
                                            ),
                                          ],
                                        ),
                                      );
                                    }
                                    return Text('No Data');
                                  });
                            });
                      }
                      return Text("No Data");
                    },
                  ),
                )
              ],
            ),
          ),
        ],
      ),
    );
  }
}
