import 'package:flutter/material.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import '../../ui/provider/booking/widgets/provider_booking_request_reservation_widget.dart';
import '../../ui/provider/booking/widgets/provider_booking_service_reservation_widget.dart';

class ProviderBookingServiceReservationScreen extends StatefulWidget {
  bool showAppBar;
   ProviderBookingServiceReservationScreen({super.key,this.showAppBar=false});

  @override
  State<ProviderBookingServiceReservationScreen> createState() =>
      _ProviderBookingServiceReservationScreenState();
}

class _ProviderBookingServiceReservationScreenState
    extends State<ProviderBookingServiceReservationScreen>
    with SingleTickerProviderStateMixin {
  late TabController _innerTabController;

  @override
  void initState() {
    super.initState();
    _innerTabController = TabController(length: 2, vsync: this);
  }

  @override
  void dispose() {
    _innerTabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        automaticallyImplyLeading: widget.showAppBar,
        title:widget.showAppBar?HeaderTxtWidget("Service History",color: Colors.white,): TabBar(
          controller: _innerTabController,
          unselectedLabelColor: Colors.grey,
          labelColor: Colors.white,
          indicatorColor: Colors.white,
          tabs: [
            Tab(text: "Services"),
            Tab(text: "Requests"),
          ],
        ),
        bottom:widget.showAppBar? PreferredSize(preferredSize: const Size.fromHeight(50), child: TabBar(
          controller: _innerTabController,
          unselectedLabelColor: Colors.grey,
          labelColor: Colors.white,
          indicatorColor: Colors.white,
          tabs: [
            Tab(text: "Services"),
            Tab(text: "Requests"),
          ],
        )):null,
      ),
      body: TabBarView(
        controller: _innerTabController,
        children: [
          ProviderServiceReservationWidget(),
           ProviderRequestReservationWidget()
        ],
      ),
    );
  }
}
