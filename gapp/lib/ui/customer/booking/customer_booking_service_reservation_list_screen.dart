import 'package:flutter/material.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/utils/colors.dart';
import 'package:screenshot/screenshot.dart';

import '../../../widgets/header_txt_widget.dart';
import '../../../widgets/sub_txt_widget.dart';
import 'customer_reservation_widgets/request_reservation_widget.dart';
import 'customer_reservation_widgets/service_reservation_widget.dart';

class CustomerBookingServiceReservationListScreen extends StatefulWidget {
  bool showAppBar;
   CustomerBookingServiceReservationListScreen({super.key,this.showAppBar=false});

  @override
  State<CustomerBookingServiceReservationListScreen> createState() =>
      _CustomerBookingServiceReservationListScreenState();
}

class _CustomerBookingServiceReservationListScreenState extends State<CustomerBookingServiceReservationListScreen> {
String selectedSub="1";
  @override
  void initState() {
    super.initState();
  }

  @override
  void dispose() {
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title:HeaderTxtWidget("Service History",color: Colors.white,)
        ),
      body: Column(
        children: [
          Container(
            decoration: BoxDecoration(
              color: primaryColorCode
            ),
            child: Row(
              children: [
                Expanded(
                  flex: 1,
                  child: InkWell(
                    onTap: (){
                      setState(() {
                        selectedSub="1";
                      });
                    },
                    child: Container(
                      alignment: AlignmentDirectional.center,
                      margin: const EdgeInsets.symmetric(horizontal: 5,vertical: 5),
                      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 10),
                      decoration: BoxDecoration(
                          color:selectedSub=="1"?"#F0EEFF".toColor():Colors.white,
                          borderRadius:
                          const BorderRadius.all(Radius.circular(8))),
                      child: SubTxtWidget(
                        'Service',
                        color:  selectedSub=="1"?Colors.blue:Colors.grey,
                        fontWeight: FontWeight.w600,
                        fontSize: 14,
                      ),
                    ),
                  ),
                ),
                Expanded(
                  flex: 1,
                  child: InkWell(
                    onTap: (){
                      setState(() {
                        selectedSub="2";
                      });
                    },
                    child: Container(
                      alignment: AlignmentDirectional.center,
                      margin: const EdgeInsets.symmetric(horizontal: 5,vertical: 5),
                      padding: const EdgeInsets.symmetric(
                          horizontal: 10, vertical: 10),
                      decoration: BoxDecoration(
                          color:selectedSub=="2"?"#F0EEFF".toColor():Colors.white,
                          borderRadius:
                          const BorderRadius.all(Radius.circular(8))),
                      child: SubTxtWidget(
                        'Requests',
                        fontWeight: FontWeight.w600,
                        color:  selectedSub=="2"?Colors.blue:Colors.grey,
                        fontSize: 14,
                      ),
                    ),
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(height: 20,),
          Visibility(visible: selectedSub=="1",child: const Expanded(child: ServiceReservationWidget()),),
          Visibility(visible: selectedSub=="2",child: const Expanded(child: RequestReservationWidget()),),
        ],
      ),
    );
  }
}
