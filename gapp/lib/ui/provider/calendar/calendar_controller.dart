import 'dart:collection';

import 'package:calendar_view/calendar_view.dart';
import 'package:country_code_picker/country_code_picker.dart';
import 'package:get/get.dart';
import 'package:groom/Utils/tools.dart';

import '../../../data_models/customer_offer_model.dart';
import '../../../data_models/request_reservation_model.dart';
import '../../../firebase/customer_offer_firebase.dart';
import '../../../firebase/reservation_firebase.dart';
import '../../../repo/setting_repo.dart';
import '../../../utils/utils.dart';
import 'm/date_model.dart';

class CalendarController extends GetxController {
  RxString selectedFilter="Day".obs;
  RxString today="Day".obs;
  RxList<DateModel>calendarList=RxList([]);
  CustomerOfferFirebase customerOfferFirebase = CustomerOfferFirebase();
  List<CustomerOfferModel> _services = [];
  Rx<Map<String,List<Map<String,dynamic>>>> eventListAll =Rx({});
  Rx<Map<String,List<Map<String,dynamic>>>> eventList =Rx({});
  EventController eventController=EventController();
  ReservationFirebase reservationFirebase = ReservationFirebase();
  int page=0;
  bool refresing=true;
  @override
  void onInit() {
    // TODO: implement onInit
    super.onInit();
    refreshCalendar();
    getRequestReservations();
  }
  void refreshCalendar(){
    refresing=false;
    page=page+10;
    calendarList.clear();
    DateTime start=DateTime.now().subtract(Duration(days: page));
    DateTime now=DateTime.now();
    today.value=Tools.changeDateFormat(now.toString(), "EEEE, MM-dd-yyyy");
    if(selectedFilter.value=="Day") {
      for (int i = 0; i <= now.difference(start).inDays+50; i++) {
        DateTime nextDate = start.add(Duration(days: i));
        calendarList.add(DateModel(selected: false,
            startDate: nextDate,
            fullDate:Tools.changeDateFormat(now.toString(), "EEEE, MM-dd-yyyy"),
            date: Tools.changeDateFormat(nextDate.toString(), "dd"),
            day: Tools.changeDateFormat(nextDate.toString(), "EEE")));
      }
    }
    if(selectedFilter.value=="Week") {
      int dayCount=start.weekday;
      print("dayCount==$dayCount");
      DateTime endDate = start.add(Duration(days: 7 - dayCount));
      if(dayCount!=7) {
        String date = '${Tools.changeDateFormat(start.toString(), "dd/MM")}-${Tools
            .changeDateFormat(endDate.toString(), "dd/MM")}';
        String day = '${Tools.changeDateFormat(start.toString(), "EEE")}-${Tools
            .changeDateFormat(endDate.toString(), "EEE")}';
        calendarList.add(DateModel(selected: false,
            startDate: start,
            endDate: endDate,
            fullDate:'${Tools.changeDateFormat(start.toString(), globalTimeFormat)} - ${Tools.changeDateFormat(endDate.toString(), globalTimeFormat)}',
            date: date,
            day: day));
      }
      for (int i = 7; i <= 100; i=i+7) {
        DateTime nextDate = endDate.add(Duration(days: i-1));
        String date='${Tools.changeDateFormat(endDate.toString(), "dd/MM")}-${Tools.changeDateFormat(nextDate.toString(), "dd/MM")}';
        String day='${Tools.changeDateFormat(endDate.toString(), "EEE")}-${Tools.changeDateFormat(nextDate.toString(), "EEE")}';
        calendarList.add(DateModel(selected: false,
            startDate: endDate,
            endDate: nextDate,
            fullDate:'${Tools.changeDateFormat(endDate.toString(), globalTimeFormat)} - ${Tools.changeDateFormat(nextDate.toString(), globalTimeFormat)}',
            date: date,
            day: day));
        endDate = nextDate.add(const Duration(days: 1));
      }
    }
    if(selectedFilter.value=="Month") {
      for (int i = 0; i <= 24; i++) {
        DateTime nextDate = DateTime(start.year,start.month+i);
        calendarList.add(DateModel(selected: false,
            startDate: nextDate,
            fullDate:Tools.changeDateFormat(nextDate.toString(), "MMM yyyy"),
            date: Tools.changeDateFormat(nextDate.toString(), "yyyy"),
            day: Tools.changeDateFormat(nextDate.toString(), "MMM")));
      }
    }
    refresing=true;
  }
  void updateSelected(int index){
    for (var element in calendarList) {
      element.selected=false;
    }
    today.value=calendarList[index].fullDate;
    calendarList[index].selected=true;
    eventList.value.clear();
    eventListAll.value.forEach((key, value) {
      if(selectedFilter.value=="Day") {
        if(key==Tools.changeDateFormat(calendarList[index].startDate.toString(), globalTimeFormat)){
          eventList.value[key]=value;
        }
      }
      if(selectedFilter.value=="Week") {
        if(Tools.changeToDate(key,frm: globalTimeFormat).compareWithoutTime(calendarList[index].startDate!)||
            Tools.changeToDate(key,frm: globalTimeFormat).compareWithoutTime(calendarList[index].endDate!)||(
            Tools.changeToDate(key,frm: globalTimeFormat).isAfter(calendarList[index].startDate!)&&
                Tools.changeToDate(key,frm: globalTimeFormat).isBefore(calendarList[index].endDate!)
        )){
          eventList.value[key]=value;
        }
      }
      if(selectedFilter.value=="Month") {
        if(Tools.changeToDate(key,frm: globalTimeFormat).month==calendarList[index].startDate!.month){
          eventList.value[key]=value;
        }
      }
    },);
  }
  double getWidth(){
    if(selectedFilter.value=="Day") {
      return 50;
    }
    if(selectedFilter.value=="Week") {
      return 120;
    }
    return 60;
  }
  void fetchServices() async {
    // eventController.removeAll(eventController.allEvents);
    var services = await customerOfferFirebase.getAllOffers();
    _services = services;
    for(CustomerOfferModel e in services){
      final event = CalendarEventData(
        date: e.dateTime!,
        event: e,
        title: '${e.serviceName}',
        description: e.description
      );
      // eventController.add(event);
    }
  }
  void getRequestReservations(){
    reservationFirebase.getProviderRequestReservationsByUserId(auth.value.uid).then((value) {
      for(RequestReservationModel data in value){
        print('data.selectedDate ${data.selectedDate}');
        bool hasData=eventListAll.value.containsKey(formatDateInt(data.selectedDate));
        if(hasData) {
          eventListAll.value[formatDateInt(data.selectedDate)]!.add({
            "model": "RequestReservationModel",
            "data": data
          });
        }else{
          eventListAll.value[formatDateInt(data.selectedDate)]=[
            {
              "model": "RequestReservationModel",
              "data": data
            }
          ];
        }
      }
      eventList.value.addAll(eventListAll.value);
    },);
  }
}
