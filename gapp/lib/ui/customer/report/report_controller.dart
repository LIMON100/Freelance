import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:get/get.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/repo/setting_repo.dart';

import '../../../constant/global_configuration.dart';
import '../../../data_models/user_model.dart';
import 'm/region_model.dart';

class ReportController extends GetxController {
  Rx<UserModel?>provider=Rx(null);
  RxList<RegionModel>regionList=RxList([]);
  final _ref = FirebaseDatabase.instanceFor(
      app: Firebase.app(), databaseURL: GlobalConfiguration().getValue('databaseURL'));
  @override
  void onInit() {
    // TODO: implement onInit
    super.onInit();
    initRegion();
  }
  void initRegion(){
    regionList.add(RegionModel('Rude or Harassing Behavior'));
    regionList.add(RegionModel('Bad service'));
    regionList.add(RegionModel('Not Satisfied'));
    regionList.add(RegionModel('Uncomfortable'));
    regionList.add(RegionModel('Do Not Recommended'));
    regionList.add(RegionModel('UpSell'));
    regionList.add(RegionModel('Horrible results'));
    regionList.add(RegionModel('Inaccurate estimate'));
  }
  Query getList(){
    return _ref.ref().child('report_users').orderByChild('client_id').equalTo(auth.value.uid);
  }
  Future<void> reportUser() async {
    if(getRegion().isEmpty){
      Tools.ShowErrorMessage("Please select region");
      return;
    }
    Map<String,String>map={};
    map['provider_id']=provider.value!.uid;
    map['client_id']=auth.value.uid;
    map['region']=getRegion();
    map['created_date']=DateTime.now().toString();
    map['status']='reported';
    final userRef = _ref.ref()
        .child('report_users');
    String? key= userRef.push().key;
    map['id']=key!;
    userRef.child(key).set(map);
    Get.back();
    Tools.ShowSuccessMessage('Your report request submitted successfully, admin will verify & get back to you soon!');
  }
  String getRegion(){
    StringBuffer buffer=StringBuffer();
    for(var s in regionList){
      if(s.selected){
        if(buffer.isNotEmpty){
          buffer.write(", ");
        }
        buffer.write(s.region);
      }
    }
    return buffer.toString();
  }
}
