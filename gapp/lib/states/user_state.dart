import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/data_models/user_model.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:image_picker/image_picker.dart';
import '../data_models/provider_service_model.dart';


class UserStateController extends GetxController {
  File? userImage;
  var selectedImage = Rxn<Uint8List>();
  String? temp;
  String? n;
  int? dateOfBirthInt;
  TextEditingController dateOfBirthController = TextEditingController();
  TextEditingController providerEmailController = TextEditingController();
  TextEditingController providerFullNameContoller = TextEditingController();
  TextEditingController phoneController = TextEditingController();
  RxString defaultAddress = RxString("");
  RxString defaultStreet = RxString("");
  LatLng? defaultLatLng;
  RxBool isLoading = false.obs;
  RxList<ProviderServiceModel> allService = RxList();
  @override
  void onInit() {
    // TODO: implement onInit
    super.onInit();
    defaultAddress.value = auth.value.defaultAddress!;

    defaultLatLng = auth.value.location;
  }

  var userInit = UserModel(
    uid: "uid",
    email: "email",
    isblocked: false,
    photoURL: "photoURL",
    contactNumber: "",
    fullName: "fullName",
    joinedOn: 2,
    country: "",
    state: "",
    city: "",
    requestsThisMonth: 0,
  ).obs;

  var homeUser = UserModel(
          uid: "uid",
          email: "email",
          isblocked: false,
          photoURL: "photoURL",
          contactNumber: "",
          fullName: "fullName",
          joinedOn: 2,
          country: "",
          state: "",
          city: "", requestsThisMonth: 0,
  ).obs;

  void selectImage() async {
    XFile? pickedFile =
        await ImagePicker().pickImage(source: ImageSource.gallery);
    if (pickedFile != null) {
      selectedImage.value = await pickedFile.readAsBytes();
      userImage = File(pickedFile.path);
    }
  }

  Future<void> selectDate(BuildContext context) async {
    final DateTime? picked = await showDatePicker(
      context: context,
      initialDate: DateTime.now(),
      firstDate: DateTime(1900),
      lastDate: DateTime.now(),
    );
    if (picked != null) {
      dateOfBirthController.text = "${picked.toLocal()}".split(' ')[0];
      dateOfBirthInt = int.parse(
          "${picked.year}${picked.month.toString().padLeft(2, '0')}${picked.day.toString().padLeft(2, '0')}");
    }
  }
}
