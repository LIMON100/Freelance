import 'dart:convert';

import 'package:country_code_picker/country_code_picker.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:get/get.dart';
import 'package:groom/ui/customer/customer_dashboard/customer_dashboard_page.dart';

import '../../utils/tools.dart';
import '../../data_models/user_model.dart';
import '../../firebase/user_firebase.dart';
import '../../main.dart';
import 'm/select_status_model.dart';

class SignupController extends GetxController {
  PageController pageController = PageController(initialPage: 0);
  CountryCode countryCode=CountryCode(code: "US",dialCode: "+1",name: "United States");
  UserFirebase userService = UserFirebase();
  var firstName =TextEditingController();
  var lastName =TextEditingController();
  var phoneNumber =TextEditingController();
  var pinCode =TextEditingController();
  var otpCon =TextEditingController();
  RxList<String>countryList=RxList([]);
  RxString selectedCountry=RxString("");
  RxString selectedState=RxString("");
  RxString selectedCity=RxString("");
  RxList<String>stateList=RxList([]);
  RxList<String>cityList=RxList([]);
  String? VERIFICATIONID;
  int? resendingToken;
  RxBool isLoading=false.obs;
  Rx<DateTime?> selectedDob=Rx(null);
  @override
  void onInit() {
    // TODO: implement onInit
    super.onInit();
  }
  ///Read JSON country data from assets
  Future<dynamic> getResponse() async {
    var res = await rootBundle
        .loadString('assets/json/country.json');
    return jsonDecode(res);
  }
  ///get countries from json response
  Future<List<String?>> getCountries() async {
    countryList.clear();
    var countries = await getResponse() as List;
      countries.forEach((data) {
        countryList.add(data['name']);
      });
      selectedCountry.value=countryCode.name!;
      getStates();
    return countryList;
  }
  ///get states from json response
  Future<List<String?>> getStates() async {
    stateList.clear();
    cityList.clear();
    var response = await getResponse();
    var takeState =  response
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
    return stateList;
  }
  ///get cities from json response
  Future<List<String?>> getCities() async {
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
    return cityList;
  }

  Future sendOtp() async {
    if(phoneNumber.value.text.isEmpty){
      Tools.ShowErrorMessage("Please enter phone number");
      return;
    }
    isLoading.value=true;
    String mobile="${countryCode.dialCode}${phoneNumber.value.text}";
    bool numberExists = await userService.checkIfPhoneExists(mobile);
    if(!numberExists) {
      FirebaseAuth _auth = FirebaseAuth.instance;
      _auth.verifyPhoneNumber(
          phoneNumber: mobile,
          timeout: const Duration(seconds: 60),
          verificationCompleted: (AuthCredential authCredential) async {
            isLoading.value = false;
          },
          verificationFailed: (FirebaseAuthException authException) {
            isLoading.value = false;
            print('verification failed i think');
            Tools.ShowErrorMessage(authException.message.toString());
            printInfo(info: authException.message!);
          },
          codeSent: (String verificationId, int? forceResendingToken) {
            print('codeSent===>$verificationId');
            isLoading.value = false;
            VERIFICATIONID = verificationId;
            resendingToken = forceResendingToken;
            pageController.animateToPage(1, duration: const Duration(milliseconds: 500), curve: Curves.easeIn);
            getCountries();
          },
          codeAutoRetrievalTimeout: (String verificationId) {});
    }else{
      isLoading.value=false;
      Tools.ShowErrorMessage("phone number is already register, please use another number");
    }
  }
  Future resendOtp() async {
    String mobile="${countryCode.dialCode}${phoneNumber.value.text}";
      FirebaseAuth _auth = FirebaseAuth.instance;
      _auth.verifyPhoneNumber(
          phoneNumber: mobile,
          timeout: const Duration(seconds: 60),
          verificationCompleted: (AuthCredential authCredential) async {
            isLoading.value = false;
          },
          verificationFailed: (FirebaseAuthException authException) {
            isLoading.value = false;
            print('verification failed i think');
            Tools.ShowErrorMessage(authException.message.toString());
            printInfo(info: authException.message!);
          },
          codeSent: (String verificationId, int? forceResendingToken) {
            isLoading.value = false;
            VERIFICATIONID = verificationId;
            resendingToken = forceResendingToken;
            Tools.ShowSuccessMessage("OTP re-send successfully");
            },
          codeAutoRetrievalTimeout: (String verificationId) {});
  }
  Future<void> verifyOtp() async {
    if (otpCon.value.text.length != 6) {
      Tools.ShowErrorMessage("Otp not valid");
    } else {
      isLoading.value=true;
      FirebaseAuth auth = FirebaseAuth.instance;
      try {
        await auth.signInWithCredential(
          PhoneAuthProvider.credential(
              verificationId: VERIFICATIONID!,
              smsCode: otpCon.value.text),
        );
        isLoading.value=false;
        if (FirebaseAuth.instance.currentUser != null) {
          pageController.animateToPage(2, duration: const Duration(milliseconds: 500), curve: Curves.easeIn);
        } else {
          Tools.ShowErrorMessage("phone number is not register, please sign up");
        }
      } on FirebaseAuthException catch (e) {
        if (e.code == 'invalid-verification-code') {
          Tools.ShowErrorMessage('The verification code from SMS/TOTP is invalid. Please check and enter the correct verification code again.');
        } else {
          Tools.ShowErrorMessage(e.message!);
        }
      }
    }
  }
  void registerUser(){
    if(firstName.value.text.isEmpty){
      Tools.ShowErrorMessage("Please enter first name");
      return;
    }if(lastName.value.text.isEmpty){
      Tools.ShowErrorMessage("Please enter last name");
      return;
    }if(selectedDob.value==null){
      Tools.ShowErrorMessage("Please select date of birth");
      return;
    }if(selectedCountry.value.isEmpty){
      Tools.ShowErrorMessage("Please select country");
      return;
    }if(selectedState.value.isEmpty){
      Tools.ShowErrorMessage("Please select state");
      return;
    }if(selectedCity.value.isEmpty){
      Tools.ShowErrorMessage("Please select city");
      return;
    }if(pinCode.value.text.isEmpty){
      Tools.ShowErrorMessage("Please enter zip code");
      return;
    }
    isLoading.value=true;

    UserModel userModel = UserModel(
      uid: "uid",
      fullName: '${firstName.value.text} ${lastName.value.text}',
      isblocked: false,
      country: selectedCountry.value,
      state: selectedState.value,
      city: selectedCity.value,
      photoURL: 'https://firebasestorage.googleapis.com/v0/b/groom202406-web.appspot.com/o/Unknown_person.jpg?alt=media&token=544af59a-c22b-4619-94c6-c2bb78217025', // Use the selected or default image URL
      contactNumber: "${countryCode.dialCode}${phoneNumber.value.text}",
      joinedOn: DateTime.now().microsecondsSinceEpoch,
      dateOfBirth: selectedDob.toString(),
      requestsThisMonth: 0,
      pincode: pinCode.value.text,
    );
    userService.addUser(userModel).then((value) {
      isLoading.value=false;
      Get.offAll(() =>  CustomerDashboardPage());
    },);
  }
}
