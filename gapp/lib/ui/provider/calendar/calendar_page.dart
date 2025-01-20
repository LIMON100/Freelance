import 'dart:math';

import 'package:calendar_view/calendar_view.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/ui/provider/calendar/m/date_model.dart';
import 'package:groom/ui/provider/calendar/widgets/event_list_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../data_models/request_reservation_model.dart';
import '../booking/provider_service_request_view_screen.dart';
import '../../../states/provider_request_state.dart';
import 'calendar_controller.dart';
class CalendarPage extends StatefulWidget {
  const CalendarPage({super.key});

  @override
  State<CalendarPage> createState() => _ScreenState();
}
class _ScreenState extends State<CalendarPage>{
  final _con = Get.put(CalendarController());
  ProviderRequestState providerRequestState = Get.put(ProviderRequestState());
  final ScrollController _scrollController = ScrollController();
  @override
  void initState() {
    // TODO: implement initState
    super.initState();
    _con.fetchServices();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        title: SubTxtWidget('Today'),
        bottom: PreferredSize(
            preferredSize: const Size.fromHeight(20),
            child: Container(
              alignment: AlignmentDirectional.centerStart,
              padding: const EdgeInsets.only(bottom: 0,left: 20),
              child: HeaderTxtWidget(_con.today.value),
            )),
        automaticallyImplyLeading: false,
        actions: [
          Container(
            width: 100,
            height: 40,
            margin: const EdgeInsets.only(right: 10,top: 10),
            child: DropdownButtonFormField(
              items: ["Day","Week", "Month"].map(
                    (e) {
                  return DropdownMenuItem(
                    value: e,
                    child: SubTxtWidget(
                      e,
                      color: Colors.white,
                    ),
                  );
                },
              ).toList(),
              dropdownColor: Colors.blue,
              value: _con.selectedFilter.value,
              onChanged: (value) {
                _con.selectedFilter.value=value!;
                _con.refreshCalendar();
              },
              decoration: InputDecoration(
                border: const OutlineInputBorder(
                    borderRadius: BorderRadius.all(Radius.circular(20)),
                    borderSide: BorderSide.none
                ),
                fillColor: "#399DDC".toColor(),
                filled: true,
                contentPadding:
                const EdgeInsets.symmetric(horizontal: 10),
              ),
              iconEnabledColor: Colors.white,
            ),
          ),
        ],
      ),
      body: Obx(() => Column(
        children: [
          _calendar(),
          Expanded(child: SingleChildScrollView(
            child: body(),
          ),)
        ],
      ),),
    );
  }
  Widget _calendar(){
    return NotificationListener<ScrollNotification>(
      onNotification: (ScrollNotification notification) {
        if (_scrollController.position.pixels <= -80&&_con.refresing) {
          _con.refreshCalendar();
        }
        return true;
      }, child: Container(
      height: 80,
      margin: const EdgeInsets.all(10),
      child: ListView.builder(itemBuilder: (context, index) {
        DateModel data=_con.calendarList[index];

        return InkWell(
          splashColor: Colors.transparent,
          onTap: (){
            setState(() {
              _con.updateSelected(index);
            });
          },
          child: Container(
            width: _con.getWidth(),
            padding: const EdgeInsets.all(5),
            margin: const EdgeInsets.symmetric(horizontal: 5,vertical: 5),
            decoration: BoxDecoration(
                borderRadius: const BorderRadius.all(Radius.circular(30)),
                color: data.selected?"#0073CC".toColor():"#7CB8E7".toColor()
            ),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                SubTxtWidget(data.day,color: Colors.white,),
                SubTxtWidget(data.date,color: Colors.white),
              ],
            ),
          ),
        );
      },itemCount: _con.calendarList.length,shrinkWrap: true,scrollDirection: Axis.horizontal,
        controller:   _scrollController,physics: const BouncingScrollPhysics(),),
    ),
    );
  }
  Widget body(){
    if(_con.eventList.value.isEmpty){
      return Container(
        padding: const EdgeInsets.all(50),
        alignment: AlignmentDirectional.center,
        child: SubTxtWidget("No Event Found"),
      );
    }
    List<Widget>tile=[];
    _con.eventList.value.forEach((key, value) {
      tile.add(Container(
        padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 10),
        child: Row(
          children: [
            SizedBox(
              height: (55*value.length).toDouble(),
              child: Column(
                children: [
                  SubTxtWidget(key,overflow: TextOverflow.ellipsis,maxLines: 1,fontSize: 10,),
                  const SizedBox(height: 5,),
                  const Expanded(child: VerticalDivider(
                    width: 2,
                    color: Colors.black,
                  ))
                ],
              ),
            ),
            Expanded(child: ListView.builder(itemBuilder: (context, index) {
              if(value[index]['model']=="RequestReservationModel"){
                RequestReservationModel data=value[index]['data'];
                return Container(
                  padding: const EdgeInsets.all(10),
                  margin: const EdgeInsets.only(left: 10),
                  decoration: BoxDecoration(
                    borderRadius: const BorderRadius.all(Radius.circular(10)),
                    color: Colors.grey.shade100,
                    border: Border(left: BorderSide(color:Color((Random().nextDouble() * 0xFFFFFF).toInt()).withOpacity(1.0),width: 2))
                  ),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      FutureBuilder(future: _con.customerOfferFirebase.getOfferByOfferId(data.offerId), builder: (context, snapshot) {
                        if(snapshot.hasData){
                          return HeaderTxtWidget('${snapshot.data!.serviceName}');
                        }
                        return const SizedBox();
                      },),
                      SubTxtWidget('${"Deposite Amount"} - \$${data.depositAmount}', fontSize: 10,),
                      SubTxtWidget('${"Status"} - ${data.status}',fontSize: 10,),
                      SubTxtWidget('${"Description"} - ${data.description}',fontSize: 10,)
                    ],
                  ),
                );
              }
              return Container();
            },itemCount: value.length,shrinkWrap: true,primary: false,
              padding:  EdgeInsets.zero,
            ))
          ],
        ),
      ));
    },);
    return Column(
      children: tile,
    );
  }
}
