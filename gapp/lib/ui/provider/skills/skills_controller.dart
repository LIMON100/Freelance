import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/Utils/tools.dart';
import 'package:groom/firebase/user_firebase.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/ui/provider/skills/skill_model.dart';

class SkillsController extends GetxController {
RxList<SkillModel>skillsList=RxList();
UserFirebase userFirebase=UserFirebase();
RxBool isLoading=false.obs;
@override
  void onInit() {
    // TODO: implement onInit
    super.onInit();
    skillsList.add(SkillModel('Celebrity Makeup'));
    skillsList.add(SkillModel('Thai Massage'));
    skillsList.add(SkillModel('Sensitive Skin'));
    skillsList.add(SkillModel('Virgin Color'));
    skillsList.add(SkillModel('Special Face'));
    skillsList.add(SkillModel('Super long Nails'));
    skillsList.add(SkillModel('Natural Hair'));
    updateSelectedSkills();
  }
  Future<void> saveSkilled() async {
  await userFirebase.updateProviderSkill(getSelectedSkills());
  Tools.ShowSuccessMessage("Skills updated successfully");
  }
  void updateSelectedSkills(){
    isLoading.value=true;
  userFirebase.getProviderSkills(auth.value.uid).then((value) {
    value.forEach((element) {
      skillsList.firstWhere((elt) => elt.title==element).isSelected=true;
    },);
    isLoading.value=false;
  },);
  }

  List<String>getSelectedSkills(){
  List<String>list=[];
  skillsList.forEach((element) {
    if(element.isSelected){
      list.add(element.title);
    }
  },);
  return list;
  }
}
