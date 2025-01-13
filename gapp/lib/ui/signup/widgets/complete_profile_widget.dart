import 'package:country_code_picker/country_code_picker.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_rounded_date_picker/flutter_rounded_date_picker.dart';
import 'package:flutter_svg/svg.dart';
import 'package:get/get.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/widgets/social_login_widget.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../generated/assets.dart';
import '../../../utils/colors.dart';
import '../../../widgets/dropdown_with_search.dart';
import '../../../widgets/input_widget.dart';
import '../signup_controller.dart';

class CompleteProfileWidget extends StatelessWidget {
  CompleteProfileWidget({Key? key}) : super(key: key);
  final _con = Get.put(SignupController());

  @override
  Widget build(BuildContext context) {
    return Scaffold(
        backgroundColor: Colors.white,
        appBar: AppBar(
          backgroundColor: Colors.white,
          surfaceTintColor: Colors.white,
          iconTheme: IconThemeData(
              color: primaryColorCode
          ),
          leading: IconButton(
              onPressed: () {
                _con.pageController.animateToPage(1,
                    duration: const Duration(milliseconds: 500),
                    curve: Curves.easeOut);
              },
              icon: const Icon(Icons.arrow_back)),
        ),
        body: Container(
          padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 10),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              HeaderTxtWidget(
                'Complete your info',
                fontSize: 28,
              ),
              const SizedBox(
                height: 20,
              ),
              Expanded(
                  child: Obx(
                () => AbsorbPointer(
                  absorbing: _con.isLoading.value,
                  child: SingleChildScrollView(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        InputWidget(
                          title: 'First Name',
                          hint: 'Enter first name',
                          controller: _con.firstName,
                        ),
                        InputWidget(
                          title: 'Last Name',
                          hint: 'Enter last name',
                          controller: _con.lastName,
                        ),
                        const SizedBox(
                          height: 10,
                        ),
                        Padding(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 5,
                          ),
                          child: SubTxtWidget(
                            'Phone Number',
                            fontSize: 12,
                          ),
                        ),
                        AbsorbPointer(
                          absorbing: true,
                          child: Padding(
                            padding: const EdgeInsets.symmetric(
                              horizontal: 5,
                            ),
                            child: Row(children: [
                              Container(
                                decoration: BoxDecoration(
                                    borderRadius: const BorderRadius.all(
                                        Radius.circular(10)),
                                    border: Border.all(
                                        color: primaryColorCode, width: 1)),
                                height: 53,
                                child: CountryCodePicker(
                                  onChanged: (value) {
                                    _con.countryCode = value;
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
                          ),
                        ),
                        const SizedBox(
                          height: 10,
                        ),
                        Padding(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 5,
                          ),
                          child: SubTxtWidget(
                            'Date of Birth',
                            fontSize: 12,
                          ),
                        ),
                        Container(
                            margin: const EdgeInsets.symmetric(
                                horizontal: 5, vertical: 10),
                            padding: const EdgeInsets.symmetric(
                                vertical: 15, horizontal: 10),
                            decoration: BoxDecoration(
                                borderRadius:
                                    const BorderRadius.all(Radius.circular(10)),
                                border: Border.all(
                                    color: primaryColorCode, width: 1)),
                            child: InkWell(
                              child: Row(
                                mainAxisAlignment:
                                    MainAxisAlignment.spaceBetween,
                                children: [
                                  SubTxtWidget(_con.selectedDob.value == null
                                      ? 'Select date of birth'
                                      : Tools.changeDateFormat(
                                          _con.selectedDob.value.toString(),
                                          globalTimeFormat)),
                                  SvgPicture.asset(Assets.svgCalender),
                                ],
                              ),
                              onTap: () {
                                CupertinoRoundedDatePicker.show(
                                  context,
                                  fontFamily: "Mali",
                                  textColor: primaryColorCode,
                                  background: Colors.white,
                                  borderRadius: 16,
                                  initialDate: DateTime(2005),
                                  minimumYear: 1900,
                                  maximumYear: 2006,
                                  initialDatePickerMode: CupertinoDatePickerMode.date,
                                  onDateTimeChanged: (newDateTime) {
                                    _con.selectedDob.value = newDateTime;
                                  },
                                );
                              },
                            )),
                        Row(
                          children: [
                            Expanded(
                              child: DropdownWithSearch(
                                title: 'Country',
                                placeHolder: 'Select country',
                                items: _con.countryList,
                                selected: _con.selectedCountry.value,
                                onChanged: (c) {
                                  _con.selectedCountry.value = c;
                                  _con.getStates();
                                },
                              ),
                            ),
                            Expanded(
                              child: DropdownWithSearch(
                                title: 'State',
                                placeHolder: 'Select state',
                                items: _con.stateList,
                                selected: _con.selectedState.value,
                                onChanged: (c) {
                                  _con.selectedState.value = c;
                                  _con.getCities();
                                },
                              ),
                            ),
                          ],
                        ),
                        Row(
                          children: [
                            Expanded(
                              child: DropdownWithSearch(
                                title: 'City',
                                placeHolder: 'Select city',
                                items: _con.cityList,
                                selected: _con.selectedCity.value,
                                onChanged: (c) {
                                  _con.selectedCity.value = c;
                                },
                              ),
                            ),
                            Expanded(
                                child: InputWidget(
                              title: "Zip",
                              hint: "Zip",
                              controller: _con.pinCode,
                                  inputType: TextInputType.number,
                            )),
                          ],
                        ),
                        InkWell(
                          child: Container(
                            margin: const EdgeInsets.only(top: 10, bottom: 20),
                            decoration: BoxDecoration(
                                borderRadius:
                                const BorderRadius.all(Radius.circular(20)),
                                color: Colors.grey.shade200),
                            padding: const EdgeInsets.all(10),
                            child: RichText(text: const TextSpan(
                                text: 'By selecting Next, i agree to Groom terms of service, ',
                                style: TextStyle(
                                  color: Colors.black,
                                  fontSize: 14,
                                ),
                                children: [
                                  TextSpan(
                                      text: 'Payment Terms of Service & Privacy Policy.',
                                      style: TextStyle(
                                          color: Colors.blue,
                                          fontSize: 14,
                                          fontWeight: FontWeight.w600
                                      )
                                  )
                                ]
                            )),
                          ),
                          onTap: (){
                            Get.toNamed('/term_condition');
                          },
                        ),
                        Center(
                          child: ButtonPrimaryWidget(
                            'Continue',
                            radius: 30,
                            onTap: () {
                              FocusScope.of(context).unfocus();
                              _con.registerUser();
                            },
                            isLoading: _con.isLoading.value,
                          ),
                        ),
                        const SizedBox(
                          height: 20,
                        ),
                        SocialLoginWidget(),
                      ],
                    ),
                  ),
                ),
              )),
            ],
          ),
        ));
  }
}
