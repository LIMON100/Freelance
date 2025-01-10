import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ui/signup/signup_controller.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import 'package:pinput/pinput.dart';

class OtpWidget extends StatelessWidget {
  OtpWidget({Key? key}) : super(key: key);
  final _con = Get.put(SignupController());
  final focusNode = FocusNode();
  final focusEmailNode = FocusNode();

  @override
  Widget build(BuildContext context) {
    const length = 6;
    const borderColor = Color.fromRGBO(114, 178, 238, 1);
    const errorColor = Color.fromRGBO(255, 234, 238, 1);
    const fillColor = Color.fromRGBO(222, 231, 240, .57);
    final defaultPinTheme = PinTheme(
      width: 56,
      height: 60,
      textStyle: const TextStyle(
        color: Colors.black,
        fontSize: 16
      ),
      decoration: BoxDecoration(
        color: fillColor,
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Colors.transparent),
      ),
    );

    return Scaffold(
      backgroundColor:Colors.white,
      appBar: AppBar(
        backgroundColor: Colors.white,
        elevation: 1,
        surfaceTintColor: Colors.white,
        iconTheme: IconThemeData(
            color: primaryColorCode
        ),
        leading: IconButton(onPressed: () {
          _con.pageController.animateToPage(0, duration: const Duration(milliseconds: 500), curve:Curves.easeOut);
        }, icon: const Icon(Icons.arrow_back)),
      ),
      body: Obx(() => AbsorbPointer(
        absorbing: _con.isLoading.value,
        child: Container(
          padding: const EdgeInsets.symmetric(horizontal: 20,vertical:20),
          child: Column(
            children: [
              Expanded(child: Column(
                children: [
                  HeaderTxtWidget('Verify your phone number',fontSize: 28,),
                  const SizedBox(height: 5,),
                  SubTxtWidget('Enter the code that was sent to your number'),
                  const SizedBox(height: 5,),
                  Align(
                    alignment: AlignmentDirectional.centerStart,
                    child: HeaderTxtWidget('  ${_con.countryCode}${_con.phoneNumber.value.text}'),
                  ),
                  const SizedBox(height: 20,),
                  SizedBox(
                    height: 68,
                    child: Pinput(
                      length: length,
                      controller: _con.otpCon,
                      focusNode: focusNode,
                      defaultPinTheme: defaultPinTheme,
                      closeKeyboardWhenCompleted: true,
                      enableSuggestions: true,
                      keyboardType: TextInputType.number,
                      onCompleted: (pin) {
                        _con.verifyOtp();
                      },
                      focusedPinTheme: defaultPinTheme.copyWith(
                        height: 68,
                        width: 64,
                        decoration: defaultPinTheme.decoration!.copyWith(
                          border: Border.all(color: borderColor),
                        ),
                      ),
                      errorPinTheme: defaultPinTheme.copyWith(
                        decoration: BoxDecoration(
                          color: errorColor,
                          borderRadius: BorderRadius.circular(8),
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(height:20,),
                  Container(
                    alignment: AlignmentDirectional.center,
                    child: TextButton(onPressed: (){
                      _con.resendOtp();
                    }, child: SubTxtWidget('Resend Code',color: secoundryColorCode,)),
                  ),
                  const SizedBox(height:50,),
                ],
              )),
              ButtonPrimaryWidget('Continue',radius: 30,onTap: () {
                _con.verifyOtp();
              },isLoading: _con.isLoading.value,),
            ],
          ),
        ),
      ),),
    );
  }
}

