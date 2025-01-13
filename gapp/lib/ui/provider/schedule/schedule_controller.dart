import 'package:get/get.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/firebase/user_firebase.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/ui/provider/schedule/schedule_model.dart';

class ScheduleController extends GetxController {
  RxList<ScheduleModel>scheduleList=RxList();
  RxBool useSameTime=false.obs;
  UserFirebase userFirebase=UserFirebase();
  RxBool isLoading=false.obs;
  @override
  void onInit() {
    // TODO: implement onInit
    super.onInit();
    getScheduleList();
  }
  void initSchedule(){
    scheduleList.add(ScheduleModel(daySortName: "Sun",isEnable: false,dayName: "Sunday",fromTime: "",toTime: ""));
    scheduleList.add(ScheduleModel(daySortName: "Mon",isEnable: true,dayName: "Monday",fromTime: "",toTime: ""));
    scheduleList.add(ScheduleModel(daySortName: "Tues",isEnable: true,dayName: "Tuesday",fromTime: "",toTime: ""));
    scheduleList.add(ScheduleModel(daySortName: "Wed",isEnable: true,dayName: "Wednesday",fromTime: "",toTime: ""));
    scheduleList.add(ScheduleModel(daySortName: "Thu",isEnable: true,dayName: "Thursday",fromTime: "",toTime: ""));
    scheduleList.add(ScheduleModel(daySortName: "Fri",isEnable: true,dayName: "Friday",fromTime: "",toTime: ""));
    scheduleList.add(ScheduleModel(daySortName: "Sat",isEnable: true,dayName: "Saturday",fromTime: "",toTime: ""));
  }
  void updateAllTime(String fromTime, String toTime){
    if(useSameTime.value){
      for (var element in scheduleList) {
        element.fromTime=fromTime;
        element.toTime=toTime;
      }
    }
  }
  void saveSchedule()  {
    isLoading.value=true;
    scheduleList.forEach((element) async {
      await userFirebase.addScheduleTime(element.toJson());
    },);
    isLoading.value=false;
    Get.back();
    Tools.ShowSuccessMessage("Schedule saved successfully");
  }
  void getScheduleList(){
    userFirebase.getScheduleList(auth.value.uid).then((value) {
      if(value.isEmpty){
        initSchedule();
      }else{
        scheduleList.value=value;
      }
    },);
  }


}
