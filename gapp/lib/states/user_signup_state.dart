import 'package:flutter/material.dart';
import 'dart:io';
import 'dart:typed_data';
import 'package:get/get.dart';
import '../data_models/customer_plan.dart';
import '../data_models/user_model.dart';
import 'package:image_picker/image_picker.dart';
import 'package:intl/intl.dart';

class UserSignupState extends GetxController {
  File? userImage ;
  var selectedImage = Rxn<Uint8List>();
  String? countryValue;
  String? stateValue;
  String? cityValue;
  int? dateOfBirthInt;

  String? dateOfBirthText;
  String? temp;
  String? n;
  TextEditingController phoneController = TextEditingController();
  TextEditingController emailController = TextEditingController();
  TextEditingController fullNameController = TextEditingController();
  TextEditingController dateOfBirthController = TextEditingController();
  var signupUserModel = UserModel(
          uid: '',
          email: '',
          isblocked: false,
          photoURL: 'https://firebasestorage.googleapis.com/v0/b/groom202406-web.appspot.com/o/Unknown_person.jpg?alt=media&token=544af59a-c22b-4619-94c6-c2bb78217025',
          contactNumber: '',
          fullName: '',
          joinedOn: 0,
          country: '',
          state: '',
          city: '',
    requestsThisMonth: 0,)
      .obs;
  void disposeControllers() {
    phoneController.dispose();
    emailController.dispose();
    fullNameController.dispose();
    dateOfBirthController.dispose();
  }
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
      // Format the date as "day month year"
      final DateFormat formatter = DateFormat('dd MMMM yyyy');
      final String formattedDate = formatter.format(picked);

      dateOfBirthController.text = formattedDate;
      dateOfBirthInt = int.parse(
          "${picked.year}${picked.month.toString().padLeft(2, '0')}${picked.day.toString().padLeft(2, '0')}");
    }
  }

}
