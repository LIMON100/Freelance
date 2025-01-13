import 'dart:convert';
import 'dart:io';
import 'package:easy_localization/easy_localization.dart';
import 'package:file_picker_pro/file_data.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_messaging/firebase_messaging.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:get/get.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import 'package:image_picker/image_picker.dart';
import 'package:share_plus/share_plus.dart';
import '../../../data_models/user_model.dart';
import '../../../firebase/dynamic_link.dart';
import '../../../firebase/user_firebase.dart';
import '../../../repo/session_repo.dart';
import '../../signup/m/select_status_model.dart';

class CustomerProfileController extends GetxController {
  UserFirebase userService = UserFirebase();
  var fullName = TextEditingController();
  var phoneNumber = TextEditingController();
  var pinCode = TextEditingController();
  var otpCon = TextEditingController();
  var bio = TextEditingController();
  RxList<String> countryList = RxList([]);
  RxString selectedCountry = RxString("");
  RxString selectedState = RxString("");
  RxString selectedCity = RxString("");
  RxList<String> stateList = RxList([]);
  RxList<String> cityList = RxList([]);
  RxBool isLoading = false.obs;
  Rx<DateTime?> selectedDob = Rx(null);
  final ImagePicker _picker = ImagePicker();
  Rx<FileData?> imageFile = Rx(null);
  RxBool isNotification = false.obs;

  @override
  void onInit() {
    // TODO: implement onInit
    initProfile();
    super.onInit();
  }

  void initProfile() {
    fullName.text = auth.value.fullName;
    phoneNumber.text = auth.value.contactNumber;
    pinCode.text = auth.value.pincode ?? "";
    bio.text = auth.value.bio ?? "";
    selectedCountry.value = auth.value.country;
    selectedState.value = auth.value.state;
    selectedCity.value = auth.value.city;
    isNotification.value=auth.value.notification_status??false;

    print(auth.value.toJson());
    try {
      selectedDob.value = DateFormat("yyyy-MM-dd")
          .parse(Tools.changeDateFormat(auth.value.dateOfBirth!, globalTimeFormat));
    } catch (e, s) {
      print(s);
    }
    getCountries();
    getStates();
    getCities();
  }


  Future<void> updateProfile() async {
    if (fullName.value.text.isEmpty) {
      Tools.ShowErrorMessage("Please enter full name");
      return;
    }
    if (selectedDob.value == null) {
      Tools.ShowErrorMessage("Please select date of birth");
      return;
    }
    if (selectedCountry.value.isEmpty) {
      Tools.ShowErrorMessage("Please select country");
      return;
    }
    if (selectedState.value.isEmpty) {
      Tools.ShowErrorMessage("Please select state");
      return;
    }
    if (selectedCity.value.isEmpty) {
      Tools.ShowErrorMessage("Please select city");
      return;
    }
    if (pinCode.value.text.isEmpty) {
      Tools.ShowErrorMessage("Please enter pin code");
      return;
    }if (bio.value.text.isEmpty) {
      Tools.ShowErrorMessage("Please enter bio");
      return;
    }
    isLoading.value = true;
    String url = auth.value.photoURL;
    if (imageFile.value != null) {
      url = await userService.uploadImage(
          File(imageFile.value!.path), auth.value.uid);
    }

    UserModel userModel = UserModel(
      uid: auth.value.uid,
      fullName: fullName.value.text,
      isblocked: auth.value.isblocked,
      country: selectedCountry.value,
      state: selectedState.value,
      city: selectedCity.value,
      photoURL: url,
      pincode: pinCode.value.text,
        bio: bio.value.text,
      contactNumber: auth.value.contactNumber,
      joinedOn: auth.value.joinedOn,
      updatedOn: DateTime.now().microsecondsSinceEpoch,
      dateOfBirth: selectedDob.toString(),
      requestsThisMonth: auth.value.requestsThisMonth,
      notification_status: auth.value.notification_status,
      defaultAddress: auth.value.defaultAddress,
      location: auth.value.location,
    );
    auth.value = userModel;
    await userService.updateUserEdit(userModel).then(
      (value) {
        isLoading.value = false;
        Get.back();
      },
    );
  }
  Future<void> updateNotificationStatus()async {
    auth.value.notification_status=isNotification.value;
    await userService.updateNotificationStatus(isNotification.value);
  }

  Future<void> showLogoutConfirmationDialog(context) async {
    return showDialog<void>(
      context: context,
      builder: (BuildContext context) {
        return Scaffold(
          backgroundColor: Colors.transparent,
          body: Align(
            alignment: AlignmentDirectional.bottomCenter,
              child: Wrap(
                children: [
                  Container(
                    padding: const EdgeInsets.all(20),
                    margin: const EdgeInsets.only(bottom: 20,left: 20,right: 20),
                    decoration: const BoxDecoration(
                      color: Colors.white,
                    ),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        HeaderTxtWidget("Logout?"),
                        const SizedBox(height: 5,),
                        SubTxtWidget('Are you sure want to logout from the app?'),
                        const SizedBox(height: 10,),
                        Row(
                          children: [
                            Expanded(child: ButtonPrimaryWidget("Cancel",onTap: () {
                              Navigator.pop(context);
                            },height: 40,padding: 0,color: Colors.white,borderColor: Colors.grey,
                              marginHorizontal: 10,
                              txtColor: Colors.black,radius: 5,)),
                            Expanded(child: ButtonPrimaryWidget("Logout",onTap: () async {
                              await FirebaseAuth.instance.signOut();
                              auth.value=UserModel.fromJson({});
                              isProvider.value=false;
                              isGuest.value=false;
                              Logout();
                              Get.offAllNamed('/login');
                            },height: 40,padding: 0,color: Colors.red,
                              marginHorizontal: 10,radius: 5,)),
                          ],
                        )
                      ],
                    ),
                  )
                ],
              )),
        );
      },
    );
  }

  ///Read JSON country data from assets
  Future<dynamic> getResponse() async {
    var res = await rootBundle.loadString('assets/json/country.json');
    return jsonDecode(res);
  }

  ///get countries from json response
  void getCountries() async {
    countryList.clear();
    var countries = await getResponse() as List;
    countries.forEach((data) {
      countryList.add(data['name']);
    });
  }

  ///get states from json response
  void getStates() async {
    if (selectedCountry.value.isEmpty) {
      return;
    }
    stateList.clear();
    cityList.clear();
    var response = await getResponse();
    var takeState = response
        .map((map) => Country.fromJson(map))
        .where((item) => item.name == selectedCountry.value)
        .map((item) => item.state)
        .toList();
    var states = takeState as List;
    states.forEach((f) {
      var name = f.map((item) => item.name).toList();
      for (var stateName in name) {
        stateList.add(stateName.toString());
      }
    });
    stateList.sort((a, b) => a.compareTo(b));
  }

  ///get cities from json response
  void getCities() async {
    if (selectedCountry.value.isEmpty) {
      return;
    }
    if (selectedState.value.isEmpty) {
      return;
    }
    cityList.clear();
    var response = await getResponse();
    var takeCity = response
        .map((map) => Country.fromJson(map))
        .where((item) => item.name == selectedCountry.value)
        .map((item) => item.state)
        .toList();
    var cities = takeCity as List;
    cities.forEach((f) {
      var name = f.where((item) => item.name == selectedState.value);
      var cityName = name.map((item) => item.city).toList();
      cityName.forEach((ci) {
        var citiesName = ci.map((item) => item.name).toList();
        for (var cityName in citiesName) {
          cityList.add(cityName.toString());
        }
      });
    });
    cityList.sort((a, b) => a.compareTo(b));
  }

  void shareApp(){
    createDynamicLink(type: 'share',id: auth.value.uid).then((value) {
      Share.share("${auth.value.fullName} suggest you The groom app, follow link ${value.toString()}");
    },);
  }
}
