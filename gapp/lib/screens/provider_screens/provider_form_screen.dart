import 'dart:io';
import 'dart:typed_data';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/firebase/user_firebase.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/input_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import 'package:image_picker/image_picker.dart';
import '../../repo/session_repo.dart';
import '../../states/provider_state.dart';
import '../../ui/customer/customer_dashboard/customer_dashboard_controller.dart';
import '../../ui/customer/customer_dashboard/customer_dashboard_page.dart';
import '../../widgets/provider_form_widgets/services_list_widget.dart';
import '../google_maps_screen.dart';
import 'package:groom/data_models/provider_user_model.dart';
import 'package:groom/firebase/provider_user_firebase.dart';

class ProviderFormPage extends StatefulWidget {
  @override
  State<ProviderFormPage> createState() => _ProviderFormPageState();
}

class _ProviderFormPageState extends State<ProviderFormPage> {
  final FormController formController = Get.put(FormController());
  ProviderUserFirebase providerUserFirebase = ProviderUserFirebase();
  var salonTitle=TextEditingController();
  var descriptionTitle=TextEditingController();
  var addressTitle=TextEditingController();
  var basePriceCon=TextEditingController();
  LatLng? selectedLocation;
  Uint8List? mapScreenshot;
  List<File> _images = [];
  bool _isLoading = false;
  final providerFormKey = GlobalKey<FormState>();

  Future<void> _pickImage() async {
    final ImagePicker picker = ImagePicker();
    final XFile? pickedImage = await picker.pickImage(source: ImageSource.gallery);
    if (pickedImage != null) {
      setState(() {
        _images.add(File(pickedImage.path));
      });
    }
  }

  void _removeImage(int index) {
    setState(() {
      _images.removeAt(index);
    });
  }

  void _showLoadingDialog() {
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (BuildContext context) {
        return const AlertDialog(
          content: Row(
            children: [
              CircularProgressIndicator(),
              SizedBox(width: 20),
              Text("Creating Groomer Profile"),
            ],
          ),
        );
      },
    );
  }

  void _hideLoadingDialog() {
    Navigator.of(context).pop();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: HeaderTxtWidget('Groomer Profile',color: Colors.white,),
      actions: [
        ActionChip(label: SubTxtWidget("Exit"),onPressed: () {
          setIsGroomPartner(false);
          Get.offAllNamed('/customer_dashboard');
        },shape: const StadiumBorder(),)
      ],),
      body: Obx(() => AbsorbPointer(
        absorbing: _isLoading,
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Form(
            key: providerFormKey,
            child: SingleChildScrollView(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  HeaderTxtWidget("Groomer Type:"),
                  const SizedBox(height: 8.0),
                  Row(
                    children: [
                      ChoiceChip(
                        label: SubTxtWidget('Salon'),
                        selected: formController.selectedGroomerType.value == 'Salon',
                        onSelected: (selected) {
                          if (selected) {
                            formController.selectedGroomerType.value = 'Salon';
                          }
                        },
                      ),
                      const SizedBox(width: 8.0),
                      ChoiceChip(
                        label: SubTxtWidget('Independent'),
                        selected: formController.selectedGroomerType.value == 'Independent',
                        onSelected: (selected) {
                          if (selected) {
                            formController.selectedGroomerType.value = 'Independent';
                          }
                        },
                      ),
                    ],
                  ),
                  const SizedBox(height: 20,),
                  if (formController.selectedGroomerType.value == 'Salon')...{
                    HeaderTxtWidget("Salon Title"),
                    InputWidget(
                      controller: salonTitle,
                      validatorCallback: (value) {
                        if (formController.selectedGroomerType
                            .value == 'Salon' &&
                            (value == null || value.isEmpty)) {
                          return 'Salon title is required';
                        }
                        return null;
                      },
                      onChanged: (v) {
                        formController.salonDescription = v;
                      },
                      margin: const EdgeInsets.symmetric(vertical: 10),
                    ),
                  },
                  HeaderTxtWidget("Groomer Description:"),
                  InputWidget(
                    controller: descriptionTitle,
                    validatorCallback: (value) {
                      if (value == null || value.isEmpty) {
                        return 'Description is required';
                      }
                      return null;
                    },
                    onChanged:(value){
                      formController.providerUser.update((user) {
                        user?.about = value;
                      });},
                    maxLines: 5,
                    margin: const EdgeInsets.symmetric(vertical: 10),
                  ),
                  HeaderTxtWidget("Base Amount"),
                  InputWidget(
                    controller: basePriceCon,
                    inputType: TextInputType.number,
                    validatorCallback: (value) {
                      if (value == null || value.isEmpty) {
                        return 'Base amount is required';
                      }
                      return null;
                    },
                    margin: const EdgeInsets.symmetric(vertical: 10),
                  ),
                  HeaderTxtWidget("Address Line"),
                  InputWidget(
                    controller: addressTitle,
                    validatorCallback: (value) {
                      if (value == null || value.isEmpty) {
                        return 'Address is required';
                      }
                      return null;
                    },
                    onChanged:(value){
                      formController.providerUser.update((user) {
                        user?.addressLine = value;
                      });
                    },
                    maxLines: 5,
                    margin: const EdgeInsets.symmetric(vertical: 10),
                  ),
                  HeaderTxtWidget("Work Day From"),
                  Container(
                    margin: const EdgeInsets.symmetric(vertical: 10),
                    padding: const EdgeInsets.symmetric(horizontal: 10),
                    decoration:  BoxDecoration(
                      borderRadius: const BorderRadius.all(Radius.circular(12)),
                      border: Border.all(color: primaryColorCode,width: 1),
                    ),
                    child:DropdownButtonFormField<String>(
                      decoration: const InputDecoration(
                          border: InputBorder.none
                      ),
                      items: formController.daysOfWeek.map((String day) {
                        return DropdownMenuItem<String>(
                          value: day,
                          child: Text(day),
                        );
                      }).toList(),
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return 'Work Day From is required';
                        }
                        return null;
                      },
                      onChanged: (value) {
                        formController.providerUser.update((user) {
                          user?.workDayFrom = value ?? '';
                        });
                      },
                    ),
                  ),
                  HeaderTxtWidget("Work Day To"),
                  Container(
                    margin: const EdgeInsets.symmetric(vertical: 10),
                    padding: const EdgeInsets.symmetric(horizontal: 10),
                    decoration:  BoxDecoration(
                      borderRadius: const BorderRadius.all(Radius.circular(12)),
                      border: Border.all(color: primaryColorCode,width: 1),
                    ),
                    child:DropdownButtonFormField<String>(
                      decoration: const InputDecoration(
                          border: InputBorder.none
                      ),
                      items: formController.daysOfWeek.map((String day) {
                        return DropdownMenuItem<String>(
                          value: day,
                          child: Text(day),
                        );
                      }).toList(),
                      validator: (value) {
                        if (value == null || value.isEmpty) {
                          return 'Work Day To is required';
                        }
                        return null;
                      },
                      onChanged: (value) {
                        formController.providerUser.update((user) {
                          user?.workDayTo = value ?? '';
                        });
                      },
                    ),
                  ),
                  const SizedBox(height: 10),
                  CardListSwap(),
                  const SizedBox(height: 10),
                  HeaderTxtWidget("Select your location"),
                  Container(
                    width: 150,
                    height: 150,
                    margin: EdgeInsets.symmetric(vertical: 10),
                    decoration: BoxDecoration(
                        borderRadius: BorderRadius.circular(12),
                        border: Border.all(color: primaryColorCode,width: 1)
                    ),
                    child: Center(
                      child: InkWell(
                        onTap: () async {
                          final result = await Get.to(() => GoogleMapScreen());
                          if (result != null) {
                            setState(() {
                              selectedLocation = result['location'];
                              mapScreenshot = result['screenshot'];
                              formController.providerUser.value.location = selectedLocation;
                            });
                          }
                        },
                        child:mapScreenshot != null?ClipRRect(
                          borderRadius: const BorderRadius.all(Radius.circular(12)),
                          child: Image.memory(mapScreenshot!,
                            fit: BoxFit.cover,
                            height: 150,
                            width: 150,
                          ),
                        ):SubTxtWidget("Location"),
                      ),
                    ),
                  ),
                  HeaderTxtWidget('Provider Images'),
                  const SizedBox(height: 16),
                  Wrap(
                    children: [
                      for (var i = 0; i < _images.length; i++)
                        Stack(
                          children: [
                            Padding(
                              padding: const EdgeInsets.all(8.0),
                              child: Image.file(
                                _images[i],
                                width: 100,
                                height: 100,
                                fit: BoxFit.cover,
                              ),
                            ),
                            Positioned(
                              right: 0,
                              child: IconButton(
                                icon: Icon(Icons.remove_circle, color: Colors.red),
                                onPressed: () => _removeImage(i),
                              ),
                            ),
                          ],
                        ),
                      GestureDetector(
                        onTap: _pickImage,
                        child: Container(
                          margin: const EdgeInsets.only(top: 10),
                          width: 100,
                          height: 100,
                          color: Colors.grey[300],
                          child: Icon(Icons.add),
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 16),
                  Center(
                    child: ButtonPrimaryWidget('Submit',onTap: () async {
                      if (providerFormKey.currentState!.validate()) {
                        if (_images.isEmpty || selectedLocation == null) {
                          Get.snackbar('Error', 'Please fill all the fields correctly.');
                          return;
                        }
                        setState(() {
                          _isLoading = true;
                        });
                        _showLoadingDialog();
                        List<String> imageUrls = [];
                        for (var image in _images) {
                          String url = await providerUserFirebase.uploadImage(
                              image, FirebaseAuth.instance.currentUser!.uid);
                          if (url.isNotEmpty) {
                            imageUrls.add(url);
                          }
                        }
                        ProviderUserModel providerModel = ProviderUserModel(
                            providerType: formController.selectedGroomerType.toString(),
                            about: descriptionTitle.value.text,
                            workDayFrom: formController.providerUser.value.workDayFrom,
                            providerServices: formController.providerUser.value.providerServices,
                            salonTitle: salonTitle.value.text,
                            providerImages: imageUrls,
                            workDayTo: formController.providerUser.value.workDayTo,
                            location: selectedLocation,
                            addressLine: addressTitle.value.text,
                            basePrice: basePriceCon.value.text,
                            createdOn: DateTime.now().millisecondsSinceEpoch);
                        await providerUserFirebase.addProvider(
                            FirebaseAuth.instance.currentUser!.uid, providerModel);
                        auth.value.providerUserModel=providerModel;
                        await setIsGroomPartner(true);
                        setState(() {
                          _isLoading = false;
                        });
                        _hideLoadingDialog();
                        Get.back();
                        Get.find<CustomerDashboardController>().changePage(0);
                      } else {
                        Get.snackbar('Error', 'Please fill all the fields correctly.');
                      }
                    },isLoading: _isLoading,),
                  )

                ],
              ),
            ),
          ),
        ),
      ),),
    );
  }
}
