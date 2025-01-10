import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../Utils/tools.dart';
import '../../../data_models/service_reservation_model.dart';
import '../../../firebase/reservation_firebase.dart';
import '../../../utils/colors.dart';

class CancelBookingScreen extends StatefulWidget {
  ServiceReservationModel data;
  String canceled_by;
  Function() success;
   CancelBookingScreen({super.key,required this.data,required this.success,required this.canceled_by});

  @override
  State<CancelBookingScreen> createState() => _CustomerBookingScreenState();
}

class _CustomerBookingScreenState extends State<CancelBookingScreen> {
  List<String> regionList = [
    'I found cheaper service elsewhere',
    "I'm looking for a different kind of shaving",
    "My plans changed",
    "Booking error (wrong date, time of booking, etc.)",
    "Other"
  ];
  int? selectedIndex;

  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: Colors.white,
        title: HeaderTxtWidget(
          'Cancel Booking',
          color: primaryColorCode,
        ),
        iconTheme: IconThemeData(color: primaryColorCode),
      ),
      body: Container(
        padding: const EdgeInsets.all(10),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const SizedBox(height: 10,),
            HeaderTxtWidget('Are you sure you want to cancel this booking?',fontSize: 20,),
            const SizedBox(height: 10,),
            SubTxtWidget(
                'Cancellation policy: Cancellations are free up to 24 hours before '),
            const SizedBox(height: 10,),
            SubTxtWidget('Total Cost: ${widget.data.serviceDetails!.servicePrice} USD'),
            const SizedBox(height: 10,),
            SubTxtWidget('Cancellation Fee: 0 USD'),
            const SizedBox(height: 10,),
            SubTxtWidget('Refund Amount:'),
            HeaderTxtWidget(
              '${widget.data.serviceDetails!.servicePrice} USD',
              fontSize: 30,
            ),
            const SizedBox(height: 10,),
            SubTxtWidget('May we ask why you canceled your service?'),
            const SizedBox(height: 10,),
            Expanded(
                child: ListView.builder(
              itemBuilder: (context, index) {
                return Container(
                  margin: const EdgeInsets.symmetric(horizontal: 10,vertical: 5),
                  padding: const EdgeInsets.symmetric(vertical: 5),
                  decoration: const BoxDecoration(
                    color: Colors.white,
                  ),
                  child: InkWell(
                    onTap: () {
                      setState(() {
                        selectedIndex=index;
                      });
                    },
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                         Icon(selectedIndex==index?Icons.radio_button_checked:Icons.radio_button_off),
                       const SizedBox(width: 8,),
                       Expanded(child:  SubTxtWidget(regionList[index]),)
                      ],
                    ),
                  ),
                );
              },
              itemCount: regionList.length,
              shrinkWrap: true,
            )),
            ButtonPrimaryWidget('Cancel booking',onTap: () async {
              if(selectedIndex!=null) {
                await ReservationFirebase()
                    .cancelServiceReservation(
                    widget.data.reservationId, regionList[selectedIndex!],widget.canceled_by);
                Get.back();
                widget.success.call();
              }else{
                Tools.ShowErrorMessage("Please select region");
              }
            },radius: 50,)
          ],
        ),
      ),
    );
  }
}
