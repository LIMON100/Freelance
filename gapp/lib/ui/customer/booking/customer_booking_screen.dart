import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/screens/services_screens/customer_booking_service_display_screen.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../generated/assets.dart';
import 'customer_offers_list_screen.dart';
import '../../../utils/colors.dart';
import '../../../widgets/app_bar_widget.dart';
import '../../../widgets/guest_widget.dart';
import 'customer_reservation_widgets/request_reservation_widget.dart';
import 'customer_reservation_widgets/service_reservation_widget.dart';

class CustomerBookingScreen extends StatefulWidget {
  const CustomerBookingScreen({super.key});

  @override
  State<CustomerBookingScreen> createState() => _CustomerBookingScreenState();
}

class _CustomerBookingScreenState extends State<CustomerBookingScreen> {
  String selected="1";
  String selectedSub="1";
  @override
  void initState() {
    // TODO: implement initState
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    if (isGuest.value) {
      return GuestWidget(
        message: 'Login to use Booking',
        isBackButton: false,
      );
    }
    return Scaffold(
      backgroundColor: Colors.white,
      body: CustomScrollView(
        slivers: [
          SliverAppBar(
              backgroundColor: Colors.white,
              pinned: false,
              automaticallyImplyLeading: false,
              floating: true,
              snap: true,
              toolbarHeight: 90,
              title: AppBarWidget(
                hideIcon: true,
              ),
              surfaceTintColor: Colors.white,
              bottom: PreferredSize(
                preferredSize: const Size.fromHeight(60),
                child: Column(
                  children: [
                    Row(
                      children: [
                        Expanded(
                          flex: 3,
                          child: InkWell(
                            onTap: (){
                              setState(() {
                                selected="1";
                              });
                            },
                            child: Container(
                              alignment: AlignmentDirectional.center,
                              margin: const EdgeInsets.symmetric(horizontal: 5),
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 10, vertical: 5),
                              decoration: BoxDecoration(
                                  color:selected=="1"?"#F0EEFF".toColor():Colors.white,
                                  borderRadius:
                                  const BorderRadius.all(Radius.circular(8))),
                              child: SubTxtWidget(
                                'Service Requests',
                                color:  selected=="1"?Colors.blue:Colors.grey,
                                fontWeight: FontWeight.w600,
                                fontSize: 14,
                              ),
                            ),
                          ),
                        ),
                        Expanded(
                          flex: 2,
                          child: InkWell(
                            onTap: (){
                              setState(() {
                                selected="2";
                              });
                            },
                            child: Container(
                              alignment: AlignmentDirectional.center,
                              margin: const EdgeInsets.symmetric(horizontal: 5),
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 10, vertical: 5),
                              decoration: BoxDecoration(
                                  color:selected=="2"?"#F0EEFF".toColor():Colors.white,
                                  borderRadius:
                                  const BorderRadius.all(Radius.circular(8))),
                              child: SubTxtWidget(
                                'Pending',
                                fontWeight: FontWeight.w600,
                                color:  selected=="2"?Colors.blue:Colors.grey,
                                fontSize: 14,
                              ),
                            ),
                          ),
                        ),
                        Expanded(
                          flex: 2,
                          child: InkWell(
                            onTap: (){
                              setState(() {
                                selected="3";
                              });
                            },
                            child: Container(
                              alignment: AlignmentDirectional.center,
                              margin: const EdgeInsets.symmetric(horizontal: 5),
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 10, vertical: 5),
                              decoration: BoxDecoration(
                                  color:selected=="3"?"#F0EEFF".toColor():Colors.white,
                                  borderRadius:
                                  const BorderRadius.all(Radius.circular(8))),
                              child: SubTxtWidget(
                                'History',
                                fontWeight: FontWeight.w600,
                                color:  selected=="3"?Colors.blue:Colors.grey,
                                fontSize: 14,
                              ),
                            ),
                          ),
                        )
                      ],
                    ),
                    Container(
                      padding: const EdgeInsets.only(
                          bottom: 10, left: 10, right: 10,top: 10),
                      alignment: AlignmentDirectional.topCenter,
                      child: TextFormField(
                        decoration: InputDecoration(
                          fillColor: Colors.white,
                          contentPadding: const EdgeInsets.symmetric(
                              horizontal: 5, vertical: 5),
                          focusedBorder: OutlineInputBorder(
                            borderRadius: BorderRadius.circular(10),
                            borderSide: BorderSide(
                                color: Colors.black.withOpacity(0.1), width: 1),
                          ),
                          errorBorder: OutlineInputBorder(
                            borderRadius: BorderRadius.circular(10),
                            borderSide: BorderSide(
                                color: Colors.black.withOpacity(0.1), width: 1),
                          ),
                          errorStyle: TextStyle(color: errorColor),
                          enabledBorder: OutlineInputBorder(
                            borderRadius: BorderRadius.circular(10),
                            borderSide: BorderSide(
                                color: Colors.black.withOpacity(0.1), width: 1),
                          ),
                          border: OutlineInputBorder(
                            borderRadius: BorderRadius.circular(10),
                            borderSide: BorderSide(
                                color: Colors.black.withOpacity(0.1), width: 1),
                          ),
                          prefixIcon: Padding(
                            padding: const EdgeInsets.all(10),
                            child: SvgPicture.asset(
                              Assets.svgSearch,
                            ),
                          ),
                          focusedErrorBorder: OutlineInputBorder(
                            borderRadius: BorderRadius.circular(10),
                            borderSide: BorderSide(
                                color: Colors.black.withOpacity(0.1), width: 1),
                          ),
                          filled: true,
                          hoverColor: Colors.transparent,
                          hintText: "Shop name or service",
                          hintStyle: TextStyle(
                            overflow: TextOverflow.ellipsis,
                            fontWeight: FontWeight.w400,
                            fontSize: 16,
                            color: "#5E5E5E7A".toColor().withOpacity(0.5),
                          ),
                        ),
                      ),
                    ),
                  ],
                ),
              )),
          SliverToBoxAdapter(
            child: Column(
              children: [
                Visibility(visible: selected=="1",child: const CustomerOffersListScreen(),),
                Visibility(visible: selected=="2",child:  CustomerServiceBookingDisplayScreen(),),
                Visibility(visible: selected=="3",child:  Column(
                  children: [
                    Row(
                      children: [
                        Expanded(
                          flex: 1,
                          child: InkWell(
                            onTap: (){
                              setState(() {
                                selectedSub="1";
                              });
                            },
                            child: AnimatedContainer(
                              duration: const Duration(milliseconds: 500),
                              alignment: AlignmentDirectional.center,
                              margin: const EdgeInsets.symmetric(horizontal: 5),
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 10, vertical: 5),
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
                              margin: const EdgeInsets.symmetric(horizontal: 5),
                              padding: const EdgeInsets.symmetric(
                                  horizontal: 10, vertical: 5),
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
                    const SizedBox(height: 20,),
                    Visibility(visible: selectedSub=="1",child: const ServiceReservationWidget(),),
                    Visibility(visible: selectedSub=="2",child: RequestReservationWidget(),),
                  ],
                ),),
              ],
            ),
          ),
        ],
      ),
    );
  }
}


