import 'dart:async';
import 'dart:io';
import 'dart:math';
import 'dart:typed_data';

import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:get/get_rx/src/rx_types/rx_types.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:image_picker/image_picker.dart';
import 'package:intl/intl.dart';

import '../data_models/user_model.dart';

String generateServiceBookingId() {
  final random = Random();

  final currentDateTime = DateTime.now();
  final formattedDate =
      "${currentDateTime.year}${currentDateTime.month.toString().padLeft(2, '0')}${currentDateTime.day.toString().padLeft(2, '0')}";
  final formattedTime =
      "${currentDateTime.hour.toString().padLeft(2, '0')}${currentDateTime.minute.toString().padLeft(2, '0')}${currentDateTime.second.toString().padLeft(2, '0')}";

  final randomNumbers = List.generate(10, (index) => random.nextInt(10)).join();

  final projectId = '$formattedDate$formattedTime$randomNumbers';
  return projectId;
}

String getRoomId(String a, String b) {
  if (a.compareTo(b) > 0) {
    return a + b;
  } else {
    return b + a;
  }
}

void autoScroll(ScrollController scrollController) {
  Timer(const Duration(microseconds: 100), () {
    scrollController.animateTo(scrollController.position.maxScrollExtent,
        duration: const Duration(microseconds: 100), curve: Curves.easeOut);
  });
}

void autoScrollReverse(ScrollController scrollController) {
  Timer(const Duration(microseconds: 100), () {
    scrollController.animateTo(0,
        duration: const Duration(microseconds: 100), curve: Curves.easeOut);
  });
}

String createName(UserModel user) {
  return "${user.fullName}";
}

String formatDate(DateTime date) {
  final DateFormat formatter = DateFormat('d MMMM');
  return formatter.format(date);
}

String formatTime(DateTime date) {
  final DateFormat timeFormatter = DateFormat('HH:mm');
  return timeFormatter.format(date);
}

String formatDateInt(int timestamp) {
  // Convert the timestamp to a DateTime object
  DateTime date = DateTime.fromMillisecondsSinceEpoch(timestamp);
  DateFormat dateFormat = DateFormat(globalTimeFormat);
  return dateFormat.format(date);
}

String formatTimeInt(int timestamp) {
  // Convert the timestamp to a DateTime object
  DateTime date = DateTime.fromMillisecondsSinceEpoch(timestamp);

  // Format the time as HH:mm
  DateFormat timeFormat = DateFormat('HH:mm');
  return timeFormat.format(date);
}

String generateProjectId() {
  final random = Random();

  final currentDateTime = DateTime.now();
  final formattedDate =
      "${currentDateTime.year}${currentDateTime.month.toString().padLeft(2, '0')}${currentDateTime.day.toString().padLeft(2, '0')}";
  final formattedTime =
      "${currentDateTime.hour.toString().padLeft(2, '0')}${currentDateTime.minute.toString().padLeft(2, '0')}${currentDateTime.second.toString().padLeft(2, '0')}";

  final randomNumbers = List.generate(10, (index) => random.nextInt(10)).join();

  final projectId = '$formattedDate$formattedTime$randomNumbers';
  return projectId;
}

String getServiceIcon(String serviceType) {
  switch (serviceType) {
    case 'Hair Style':
      return 'assets/haircutIcon.png';
    case 'Nails':
      return 'assets/nailsIcon.png';
    case 'Facial':
      return 'assets/facialIcon.png';
    case 'Coloring':
      return 'assets/coloringIcon.png';
    case 'Spa':
      return 'assets/spaIcon.png';
    case 'Wax':
      return 'assets/waxingIcon.png';
    case 'Makeup':
      return 'assets/makeupIcon.png';
    case 'Massage':
      return 'assets/massageIcon.png';
    default:
      return 'assets/spaIcon.png';
  }
}

Color getBorderColor(String serviceType) {
  switch (serviceType) {
    case 'Hair Style':
      return Colors.blue;
    case 'Nails':
      return Colors.pink;
    case 'Facial':
      return Colors.green;
    case 'Coloring':
      return Colors.purple;
    case 'Spa':
      return Colors.orange;
    case 'Wax':
      return Colors.red;
    case 'Makeup':
      return Colors.yellow;
    case 'Massage':
      return Colors.brown;
    default:
      return Colors.grey;
  }
}

void selectImage_tool(
    File? selectedImageOut, Rxn<Uint8List>? selectedImage) async {
  XFile? pickedFile =
      await ImagePicker().pickImage(source: ImageSource.gallery);
  if (pickedFile != null) {
    selectedImage!.value = await pickedFile.readAsBytes();
    selectedImageOut = File(pickedFile.path);
  }
}

Future<void> selectDate_tool(BuildContext context, Function(int) onSelectedDate, TextEditingController dateOfBirthController) async {
  final DateTime? picked = await showDatePicker(
    context: context,
    initialDate: DateTime.now(),
    firstDate: DateTime(1900),
    lastDate: DateTime.now(),
  );
  if (picked != null) {
    dateOfBirthController.text = "${picked.toLocal()}".split(' ')[0];
    int selectedDateInt = int.parse(
        "${picked.year}${picked.month.toString().padLeft(2, '0')}${picked.day.toString().padLeft(2, '0')}");
    onSelectedDate(selectedDateInt);
    print(selectedDateInt);
  }
}

