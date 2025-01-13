import 'package:country_code_picker/country_code_picker.dart';
import 'package:easy_localization/easy_localization.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import '../../utils/tools.dart';
import '../../firebase/user_firebase.dart';

class LoginController extends GetxController {
  var otpCon=TextEditingController();
  var phoneNumber=TextEditingController();
  CountryCode countryCode=CountryCode(code: "US",dialCode: "+1",name: "United States");
  String? VERIFICATIONID;
  int? resendingToken;
  RxBool isLoading=false.obs;
  RxString? errorMessage;
   UserFirebase userService = UserFirebase();
  Future login() async {
    if(phoneNumber.value.text.isEmpty){
      Tools.ShowErrorMessage("Please enter phone number");
      return;
    }
    isLoading.value=true;
    String mobile="${countryCode.dialCode}${phoneNumber.value.text}";
    print(mobile);
    bool numberExists = await userService.checkIfPhoneExists(mobile);
    if(numberExists) {
      FirebaseAuth _auth = FirebaseAuth.instance;
      _auth.verifyPhoneNumber(
          phoneNumber: mobile,
          timeout: const Duration(seconds: 60),
          verificationCompleted: (AuthCredential authCredential) async {
            isLoading.value = false;
          },
          verificationFailed: (FirebaseAuthException authException) {
            isLoading.value = false;
            Tools.ShowErrorMessage(authException.message.toString());
            printInfo(info: authException.message!);
          },
          codeSent: (String verificationId, int? forceResendingToken) {
            isLoading.value = false;
            VERIFICATIONID = verificationId;
            resendingToken = forceResendingToken;
            Get.toNamed('/otp');
          },
          codeAutoRetrievalTimeout: (String verificationId) {});
    }else{
      isLoading.value=false;
      Tools.ShowErrorMessage("phone number is not registered, please sign up");
    }
  }
  Future<void> verifyOtp() async {
    if (otpCon.value.text.length != 6) {
      errorMessage=RxString("Otp not valid");
    } else {
      errorMessage=null;
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
          errorMessage=null;
          await userService.getUserDetails(FirebaseAuth.instance.currentUser!.uid);
          Get.offAllNamed('/login_success');
        } else {
          errorMessage=RxString("phone number is not register, please sign up");
          Tools.ShowErrorMessage("phone number is not register, please sign up");
        }
      } on FirebaseAuthException catch (e) {
        isLoading.value=false;
        if (e.code == 'invalid-verification-code') {
          errorMessage=RxString("Invalid OTP");
          Tools.ShowErrorMessage('The verification code from SMS/TOTP is invalid. Please check and enter the correct verification code again.');
        } else {
          Tools.ShowErrorMessage(e.message!);
        }
      }
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

}

