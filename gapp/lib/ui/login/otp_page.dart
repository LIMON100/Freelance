import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/ui/login/login_controller.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import 'package:pinput/pinput.dart';
import '../../generated/assets.dart';

class OtpPage extends StatelessWidget {
  OtpPage({Key? key}) : super(key: key);
  final _con = Get.put(LoginController());
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
        color: Colors.white,
        fontSize: 16
      ),
      decoration: BoxDecoration(
        color: fillColor,
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Colors.transparent),
      ),
    );

    return Scaffold(
      backgroundColor: "#2D2E2F".toColor(),
      appBar: AppBar(
        backgroundColor: "#2D2E2F".toColor(),
        title: HeaderTxtWidget('Phone Verification',color: Colors.white,),
        elevation: 1,
        leading: InkWell(
          onTap: () {
            Get.back();
          },
          child: Container(
            padding: const EdgeInsets.all(10),
            child: Container(
              height: 40,
              width: 40,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: Colors.grey.withOpacity(0.5),
              ),
              child: const Icon(Icons.arrow_back_ios_outlined,color: Colors.white,),
            ),
          ),
        ),
      ),
      body: Obx(() => AbsorbPointer(
        absorbing: _con.isLoading.value,
        child: Container(
          padding: const EdgeInsets.symmetric(horizontal: 20,vertical:20),
          child: Column(
            children: [
              Expanded(child: Column(
                children: [
                  HeaderTxtWidget('Enter 6 digit verification code sent to your phone number',
                    color: Colors.white,),
                  const SizedBox(height: 20,),
                  SizedBox(
                    height: 120,
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
                      validator: (value) {
                        if(value!.isEmpty){
                          return "Enter OTP";
                        }
                        return _con.errorMessage?.value;
                      },
                      focusedPinTheme: defaultPinTheme.copyWith(
                        height: 68,
                        width: 64,
                        decoration: defaultPinTheme.decoration!.copyWith(
                          border: Border.all(color: borderColor),
                        ),
                      ),
                      disabledPinTheme: defaultPinTheme.copyWith(
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
              ButtonPrimaryWidget("Verify", color: secoundryColorCode,onTap: () {
                _con.verifyOtp();
              },isLoading: _con.isLoading.value,)

            ],
          ),
        ),
      ),),
    );
  }
}

