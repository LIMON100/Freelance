import 'dart:io';
import 'package:file_picker_pro/file_data.dart';
import 'package:file_picker_pro/file_picker.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_rounded_date_picker/flutter_rounded_date_picker.dart';
import 'package:flutter_svg/svg.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import '../../../utils/tools.dart';
import '../../../generated/assets.dart';
import '../../../repo/setting_repo.dart';
import '../../../utils/colors.dart';
import '../../../widgets/button_primary_widget.dart';
import '../../../widgets/dropdown_with_search.dart';
import '../../../widgets/input_widget.dart';
import '../../../widgets/sub_txt_widget.dart';
import 'customer_profile_controller.dart';

class EditProfileScreen extends StatefulWidget {
  @override
  _EditProfileScreenState createState() => _EditProfileScreenState();
}

class _EditProfileScreenState extends State<EditProfileScreen> {
  bool isLoading = false;
  final _con = Get.put(CustomerProfileController());
  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
        backgroundColor: Colors.white,
        appBar: AppBar(
          title: HeaderTxtWidget('Profile', color: primaryColorCode),
          iconTheme: IconThemeData(color: primaryColorCode),
          backgroundColor: Colors.white,
        ),
        body: Container(
            padding: const EdgeInsets.symmetric(
              horizontal: 20,
            ),
            child: Obx(
              () => AbsorbPointer(
                absorbing: _con.isLoading.value,
                child: SingleChildScrollView(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.center,
                    children: [
                      const SizedBox(
                        height: 20,
                      ),
                      FilePicker(
                        context: context,
                        height: 100,
                        fileData: FileData(),
                        crop: true,
                        maxFileSizeInMb: 2,
                        camera: true,
                        gallery: true,
                        otherDevice: false,
                        cropOnlySquare: true,
                        allowedExtensions: const ['jpg', 'jpeg', 'png'],
                        onSelected: (fileData) {
                          _con.imageFile.value = fileData;
                        },
                        onCancel: (message, messageCode) {},
                        child: _profile(),
                      ),
                      HeaderTxtWidget(
                        auth.value.fullName,
                        fontSize: 20,
                      ),
                      const SizedBox(
                        height: 10,
                      ),
                      InputWidget(
                        title: 'Phone Number',
                        controller: _con.phoneNumber,
                        enabled: false,
                      ),
                      InputWidget(
                        title: 'Full Name',
                        hint: 'Enter full name',
                        controller: _con.fullName,
                      ),
                      const SizedBox(
                        height: 10,
                      ),
                      Container(
                        alignment: AlignmentDirectional.centerStart,
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
                              mainAxisAlignment: MainAxisAlignment.spaceBetween,
                              children: [
                                SubTxtWidget(_con.selectedDob.value == null
                                    ? 'Select date of birth'
                                    : Tools.changeDateFormat(
                                        _con.selectedDob.toString(),
                                        "MM-dd-yyyy")),
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
                                initialDatePickerMode:
                                    CupertinoDatePickerMode.date,
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
                      InputWidget(
                        title: 'Bio / About you',
                        hint: 'I am the best because...',
                        maxLines: 5,
                        controller: _con.bio,
                      ),
                      if (isProvider.value)
                        TextButton(
                            onPressed: () {},
                            child: SubTxtWidget(
                              'View as Client',
                              color: "#2372C1".toColor(),
                            )),
                      ButtonPrimaryWidget(
                        'Save',
                        onTap: () {
                          FocusScope.of(context).unfocus();
                          _con.updateProfile();
                        },
                        isLoading: _con.isLoading.value,
                      ),
                      const SizedBox(
                        height: 20,
                      ),
                      _providerMenus()
                    ],
                  ),
                ),
              ),
            )));
  }

  Widget _profile() {
    if (_con.imageFile.value != null) {
      return Container(
        decoration: BoxDecoration(
          shape: BoxShape.circle,
          border: Border.all(color: primaryColorCode, width: 2),
        ),
        child: ClipRRect(
          borderRadius: const BorderRadius.all(Radius.circular(50)),
          child: Image.file(
            File(_con.imageFile.value!.path),
            height: 100,
            width: 100,
            fit: BoxFit.cover,
          ),
        ),
      );
    }
    return Container(
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        border: Border.all(color: primaryColorCode, width: 2),
      ),
      child: ClipRRect(
        borderRadius: const BorderRadius.all(Radius.circular(50)),
        child: NetworkImageWidget(
          url: auth.value.photoURL,
          height: 100,
          width: 100,
          fit: BoxFit.cover,
        ),
      ),
    );
  }

  Widget _providerMenus() {
    if (isProvider.value) {
      return Column(
        children: [
          Divider(
            height: 5,
            color: "#F4F4F5".toColor(),
          ),
          ListTile(
            title: HeaderTxtWidget('Services'),
            trailing: const Icon(
              Icons.arrow_forward_ios_outlined,
              size: 18,
            ),
            onTap: () {
              Get.toNamed('/provider_service_list');
            },
          ),
          Divider(
            height: 5,
            color: "#F4F4F5".toColor(),
          ),
          ListTile(
            title: HeaderTxtWidget('Reviews'),
            trailing: const Icon(
              Icons.arrow_forward_ios_outlined,
              size: 18,
            ),
            onTap: () {
              Get.toNamed('/review_list');
            },
          ),
          Divider(
            height: 5,
            color: "#F4F4F5".toColor(),
          ),
          ListTile(
            title: HeaderTxtWidget('Schedule'),
            trailing: const Icon(
              Icons.arrow_forward_ios_outlined,
              size: 18,
            ),
            onTap: () {
              Get.toNamed('/schedule');
            },
          ),
          Divider(
            height: 5,
            color: "#F4F4F5".toColor(),
          ),
          ListTile(
            title: HeaderTxtWidget('Categories'),
            trailing: const Icon(
              Icons.arrow_forward_ios_outlined,
              size: 18,
            ),
            onTap: () {
              Get.toNamed('/category');
            },
          ),
          Divider(
            height: 5,
            color: "#F4F4F5".toColor(),
          ),
          ListTile(
            title: HeaderTxtWidget('Skills'),
            trailing: const Icon(
              Icons.arrow_forward_ios_outlined,
              size: 18,
            ),
            onTap: () {
              Get.toNamed('/skills');
            },
          ),
          /* Divider(
            height: 5,
            color: "#F4F4F5".toColor(),
          ),
          ListTile(
            title: HeaderTxtWidget('Search Preferences'),
            trailing: const Icon(Icons.arrow_forward_ios_outlined,size: 18,),
            onTap: () {
              Tools.ShowSuccessMessage("Coming soon");
            },
          ),
          Divider(
            height: 5,
            color: "#F4F4F5".toColor(),
          ),
          ListTile(
            title: HeaderTxtWidget('Client Preferences'),
            trailing: const Icon(Icons.arrow_forward_ios_outlined,size: 18,),
            onTap: () {
              Tools.ShowSuccessMessage("Coming soon");
            },
          ),
          Divider(
            height: 5,
            color: "#F4F4F5".toColor(),
          ),*/
        ],
      );
    }
    return SizedBox();
  }
}
