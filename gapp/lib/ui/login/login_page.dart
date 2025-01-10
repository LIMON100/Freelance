import 'package:country_code_picker/country_code_picker.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../generated/assets.dart';
import '../../widgets/input_widget.dart';
import 'login_controller.dart';

class LoginPage extends StatelessWidget {
  LoginPage({Key? key}) : super(key: key);
  final _con = Get.put(LoginController());

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: Stack(
        fit: StackFit.expand,
        children: [
          Positioned(top: 0,right: 0,left: 0,bottom: 0,child: SingleChildScrollView(
            child: Column(
              children: [
                const SizedBox(height: 35,),
                Image.asset(
                  Assets.imgLoginImg,
                  width: double.infinity,
                  height: 350,
                  fit: BoxFit.cover,
                ),
                Obx(() => AbsorbPointer(
                  absorbing: _con.isLoading.value,
                  child: Container(
                    padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 20),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.center,
                      children: [
                        Align(
                          alignment: AlignmentDirectional.centerStart,
                          child: HeaderTxtWidget(
                            'Welcome back 👋',
                            fontSize: 28,
                          ),
                        ),
                        const SizedBox(
                          height: 5,
                        ),
                        Align(
                          alignment: AlignmentDirectional.centerStart,
                          child: SubTxtWidget(
                            'Please enter your login information below to access your account',
                          ),
                        ),
                        const SizedBox(
                          height: 20,
                        ),
                        HeaderTxtWidget(
                          'Login to Your Account',
                          fontSize: 20,
                        ),
                        Container(
                          decoration: BoxDecoration(
                              borderRadius:
                              const BorderRadius.all(Radius.circular(20)),
                              color: primaryColorCode),
                          alignment: AlignmentDirectional.center,
                          padding: const EdgeInsets.all(8),
                          margin: const EdgeInsets.symmetric(vertical: 10),
                          child: SubTxtWidget(
                            'Phone Number',
                            color: Colors.white,
                            fontSize: 14,
                          ),
                        ),
                        Row(children: [
                          Container(
                            decoration: BoxDecoration(
                                borderRadius: const BorderRadius.all(Radius.circular(10)),
                                border:
                                Border.all(color: primaryColorCode, width: 1)),
                            child: CountryCodePicker(
                              onChanged: (value) {
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
                        const SizedBox(height: 10,),
                        ButtonPrimaryWidget('Send OTP',color: secoundryColorCode,onTap: () {
                          _con.login();
                        },isLoading: _con.isLoading.value,),
                        const SizedBox(height: 10,),
                        InkWell(
                          child: _createAccount(),
                          onTap: (){
                            Get.toNamed('/signup');
                          },
                        ),

                      ],
                    ),
                  ),
                ),)
              ],
            ),
          ),),
          Positioned(top: 40,left: 10,child: IconButton(onPressed: (){
            Get.back();
          }, icon: const Icon(Icons.arrow_back,color: Colors.white,)),)
        ],
      ),
    );
  }
  Widget _createAccount(){
    return RichText(text: TextSpan(
      text: "Don’t have account? ",
        style: const TextStyle(
            color: Colors.black,
            fontSize: 12,
            fontWeight: FontWeight.w600
        ),
      children: [
        TextSpan(
          text: "Create Account",
          style: TextStyle(
            color: secoundryColorCode,
            fontSize: 12,
            fontWeight: FontWeight.w600
          )
        )
      ]
    ));
  }
}
