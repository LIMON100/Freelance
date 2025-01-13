import 'package:get/get.dart';

import '../../../utils/tools.dart';
import '../../../firebase/user_firebase.dart';
import '../../../repo/setting_repo.dart';
import '../skills/skill_model.dart';

class CategoryController extends GetxController {
RxBool isLoading=false.obs;
RxList<SkillModel>skillsList=RxList();
UserFirebase userFirebase=UserFirebase();
@override
void onInit() {
  // TODO: implement onInit
  super.onInit();
  skillsList.add(SkillModel('Haircuts'));
  skillsList.add(SkillModel('Hair Styling'));
  skillsList.add(SkillModel('Hair Coloring'));
  skillsList.add(SkillModel('Hair Treatments'));
  skillsList.add(SkillModel('Manicure'));
  skillsList.add(SkillModel('Pedicure'));
  skillsList.add(SkillModel('Nail Art'));
  skillsList.add(SkillModel('Waxing'));
  skillsList.add(SkillModel('Facials'));
  skillsList.add(SkillModel('Skin Care'));
  skillsList.add(SkillModel('Makeup Services'));
  skillsList.add(SkillModel('Eyebrow Shaping'));
  skillsList.add(SkillModel('Eyebrow Shaping'));
  skillsList.add(SkillModel('Eyelash Extensions'));
  skillsList.add(SkillModel('Body Treatments'));
  skillsList.add(SkillModel('Massage'));
  skillsList.add(SkillModel('Bridal Services'));
  updateSelected();
}
Future<void> saveCategory() async {
  await userFirebase.updateProviderCategory(getSelected());
  Tools.ShowSuccessMessage("category updated successfully");
}
void updateSelected(){
  isLoading.value=true;
  userFirebase.getProviderCategory(auth.value.uid).then((value) {
    value.forEach((element) {
      skillsList.firstWhere((elt) => elt.title==element).isSelected=true;
    },);
    isLoading.value=false;
  },);
}

List<String>getSelected(){
  List<String>list=[];
  skillsList.forEach((element) {
    if(element.isSelected){
      list.add(element.title);
    }
  },);
  return list;
}
}
