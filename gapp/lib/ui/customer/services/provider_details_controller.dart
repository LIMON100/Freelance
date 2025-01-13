import 'package:get/get.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/data_models/user_model.dart';
import 'package:share_plus/share_plus.dart';
import '../../../data_models/provider_service_model.dart';
import '../../../data_models/service_booking_model.dart';
import '../../../firebase/dynamic_link.dart';
import '../../../firebase/service_booking_firebase.dart';
import '../../../firebase/user_firebase.dart';
import '../../../repo/setting_repo.dart';
import '../../provider/schedule/schedule_model.dart';
import 'm/time_model.dart';

class ProviderDetailsController extends GetxController {
  UserFirebase userFirebase = UserFirebase();
  Rx<UserModel?>selectedProvider=Rx(null);
  Rx<ProviderServiceModel?> selectedService=Rx(null);
  RxBool isFavorite=false.obs;
  RxBool isSharing=false.obs;
  RxString openStatus="Closed".obs;
  List<ScheduleModel> scheduleList=[];
  Rx<DateTime?> selectedDate =Rx(null);
  RxString selectedTime=RxString("");
  RxList<TimeModel>timeList=RxList();
  List<ServiceBookingModel>booking=[];
  @override
  void onInit() {
    super.onInit();

  }

  void getScheduleList(id){
    String today=Tools.getTodayName();
    userFirebase.getScheduleList(id).then((value) {
      scheduleList=value;
      for (var element in scheduleList) {
        if(element.daySortName!.toLowerCase() ==today.toLowerCase()){
          openStatus.value="Open";
        }
      }
    },);
    ServiceBookingFirebase().getServiceBookingByProviderAll(selectedProvider.value!.uid).then((value) {
      booking=value;
    },);
  }

  void updateSlotList(context){
    timeList.clear();
    DateTime d=selectedDate.value??DateTime.now();
    String dayName=Tools.changeDateFormat(d.toString(), "EEE");
    ScheduleModel? scheduleModel=scheduleList.firstWhereOrNull((element) => element.daySortName!.toLowerCase()==dayName.toLowerCase(),);
    if(scheduleModel!=null){
      List<String> stime=scheduleModel.fromTime!.split(":").map((e) => e.toLowerCase(),).toList();
      List<String> etime=scheduleModel.toTime!.split(":").map((e) => e.toLowerCase(),).toList();
      int startH;
      int startM;
      if(stime[1].contains("pm")){
        startM=int.parse(stime[1].replaceAll("pm", ""));
        startH=int.parse(stime[0])+12;
      }else{
        startM=int.parse(stime[1].replaceAll("am", ""));
        startH=int.parse(stime[0]);
      }
      int endH;
      int endM;
      if(etime[1].contains("pm")){
        endM=int.parse(etime[1].replaceAll("pm", ""));
        endH=int.parse(etime[0])+12;
      }else{
        endM=int.parse(etime[1].replaceAll("am", ""));
        endH=int.parse(etime[0]);
      }
      int diff=selectedService.value!.appointmentDuration!;
      final DateTime startTime = DateTime(2024, 1, 1, startH, startM); // 9:00 AM
      final DateTime endTime = DateTime(2024, 1, 1, endH, endM); // 5:00 PM
      final Duration interval = Duration(minutes: diff);
      DateTime currentTime = startTime;

      while (currentTime.isBefore(endTime)) {
        DateTime nextTime = currentTime.add(interval);
        if (nextTime.isAfter(endTime)) break;
        timeList.add(TimeModel(Tools.changeDateFormat(currentTime.toString(), "hh:mm a")));
        currentTime = nextTime;
      }
      for (var element in booking) {
        print(Tools.changeToDate(element.selectedDate)==d);
        if(Tools.changeToDate(element.selectedDate)==d){
          for(int i=0;i<timeList.length;i++){
            if(timeList[i].time==element.selectedTime){
              timeList[i].isDisable=true;
            }
          }
        }
      }
    }else{
      print('TimeSlot==null');
    }
  }
  void shareProvider(){
    isSharing.value=true;
    createDynamicLink(type: 'PROVIDER',id: selectedProvider.value!.uid).then((value) {
      isSharing.value=false;
      Share.share("${auth.value.fullName} suggest you the saloon, follow link ${value.toString()}");
    },);
  }
  void shareService(id){
    isSharing.value=true;
    createDynamicLink(type: 'SERVICE',id: id).then((value) {
      isSharing.value=false;
      Share.share("${auth.value.fullName} suggest you a service, follow link ${value.toString()}");
    },);
  }
}

