import 'package:country_code_picker/country_code_picker.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/widgets/social_login_widget.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../utils/colors.dart';
import '../../../widgets/input_widget.dart';
import '../signup_controller.dart';

class MobileWidget extends StatelessWidget {
  MobileWidget({Key? key}) : super(key: key);
  final _con = Get.put(SignupController());

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: Colors.white,
        iconTheme: IconThemeData(
          color: primaryColorCode
        ),
      ),
      body: Container(
        padding: const EdgeInsets.symmetric(horizontal: 20,vertical: 10),
        child: Obx(() => AbsorbPointer(
          absorbing: _con.isLoading.value,
          child: SingleChildScrollView(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                HeaderTxtWidget('Sign up to Groom',fontSize: 28,),
                const SizedBox(height: 20,),
                SubTxtWidget('Mobile Number',fontSize: 12,),
                Row(children: [
                  Container(
                    decoration: BoxDecoration(
                        borderRadius: const BorderRadius.all(Radius.circular(10)),
                        border:
                        Border.all(color: primaryColorCode, width: 1)),
                    child: CountryCodePicker(
                      onChanged: (value) {
                        print(value.code);
                        _con.countryCode=value;
                      },
                      showCountryOnly: false,
                      showOnlyCountryWhenClosed: false,
                      alignLeft: false,
                      initialSelection: _con.countryCode.code,
                    ),
                  ),
                  Expanded(
                    child: InputWidget(
                      inputType: TextInputType.phone,
                      controller: _con.phoneNumber,
                    ),
                  )
                ]),
                Container(
                  padding: const EdgeInsets.all(10),
                  margin: const EdgeInsets.symmetric(vertical: 20),
                  decoration: BoxDecoration(
                      borderRadius: const BorderRadius.all(Radius.circular(10)),
                      color: Colors.grey.shade200
                  ),
                  child: Row(
                    children: [
                      const Icon(Icons.info,size: 20,),
                      const SizedBox(width: 10,),
                      Expanded(child: SubTxtWidget('You will receive an OTP code from Groom to confirm your number.',
                        fontSize: 12,))
                    ],
                  ),
                ),
                Center(
                  child: ButtonPrimaryWidget('Continue',radius: 30,onTap: () {
                    FocusScope.of(context).unfocus();
                    _con.sendOtp();
                  },isLoading: _con.isLoading.value,),
                ),
                const SizedBox(height: 20,),
                SocialLoginWidget(),
              ],
            ),
          ),
        ),),
      )
    );
  }
}
