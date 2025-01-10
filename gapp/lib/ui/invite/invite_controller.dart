import 'package:aws_sns_api/sns-2010-03-31.dart';
import 'package:fast_contacts/fast_contacts.dart';
import 'package:geocoding/geocoding.dart';
import 'package:get/get.dart';
import 'package:groom/Constant/global_configuration.dart';
import 'package:groom/Utils/tools.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:twilio_flutter/twilio_flutter.dart';
import '../../repo/setting_repo.dart';
class InviteController extends GetxController {
  RxList<Contact> contactList = RxList();
  RxList<Contact> contactListOld = RxList();
  RxList<String> selectedContactList = RxList();
  RxBool isLoading = false.obs;
  RxBool isSelectAll = false.obs;
  RxBool showSelectAll = false.obs;
  RxBool showSearch = false.obs;
  String countryCode="";
  final String region = 'us-east-1';

  @override
  void onInit() {
    // TODO: implement onInit
    super.onInit();
    getContacts();
  }

  Future<void> getContacts() async {
    await getDeviceCountry();
    if (await Permission.contacts.request().isGranted) {
      isLoading.value = true;
      FastContacts.getAllContacts().then(
        (value) {
          contactListOld.addAll(value.where(
            (element) =>
                element.phones.isNotEmpty &&
                element.displayName.isNotEmpty &&
                element.phones.first.number.length > 7,
          ));
          contactList.addAll(contactListOld);
          isLoading.value = false;
        },
      );
    }else{
      isLoading.value = true;
      FastContacts.getAllContacts().then(
            (value) {
          contactListOld.addAll(value.where(
                (element) =>
            element.phones.isNotEmpty &&
                element.displayName.isNotEmpty &&
                element.phones.first.number.length > 7,
          ));
          contactList.addAll(contactListOld);
          isLoading.value = false;
        },
      );
    }

  }
  void searchContacts(String q){
    contactList.clear();
    for (var element in contactListOld) {
      if(element.displayName.toLowerCase().contains(q.toLowerCase())){
        contactList.add(element);
      }
    }
  }

  Future<void> sendInvite({String? phone}) async {
    String message="Join me on Groom for an amazing experience! Download now and let's connect, explore, and enjoy together!, ${GlobalConfiguration().getValue('appDynamicLink')}";
    if (phone != null) {
      try {
        // Create SNS client
        final credentials = AwsClientCredentials(
          accessKey: accessKeyId,
          secretKey: secretAccessKey,
        );

        final sns = SNS(region: region, credentials: credentials);

        // Send SMS
        final response = await sns.publish(
          message: message,
          phoneNumber: phone,
        );
        print('response==>${response}');
        if (response.messageId != null) {
          print('Message sent successfully! Message ID: ${response.messageId}');
          Tools.ShowSuccessMessage("Message sent successfully!");
        } else {
          print('Failed to send the message');
        }
      } catch (e) {
        print('Error sending SMS: $e');
      }
      }
    }
  Future<void> sendInviteAll() async {
    String message="Join me on Groom for an amazing experience! Download now and let's connect, explore, and enjoy together!, ${GlobalConfiguration().getValue('appDynamicLink')}";

    int count = 0;
    do {
      String phone=selectedContactList[count];
      if (phone != null) {
        try {
          // Create SNS client
          final credentials = AwsClientCredentials(
            accessKey: accessKeyId,
            secretKey: secretAccessKey,
          );

          final sns = SNS(region: region, credentials: credentials);

          // Send SMS
          final response = await sns.publish(
            message: message,
            phoneNumber: phone,
          );
          print('response==>${response}');
          if (response.messageId != null) {
            print('Message sent successfully! Message ID: ${response.messageId}');
          } else {
            print('Failed to send the message');
          }
        } catch (e) {
          print('Error sending SMS: $e');
        }
      }
      count++;
    } while (selectedContactList.length > count);
    Tools.ShowSuccessMessage('Message sent successfully!');
    selectedContactList.clear();
  }
  Future<String> getDeviceCountry() async {
    List<Placemark> placemarks = await placemarkFromCoordinates(auth.value.location!.latitude, auth.value.location!.longitude);
    countryCode=placemarks.first.isoCountryCode??"";
    return countryCode;
  }

  String addCountryCodeIfMissing(String phoneNumber, context) {
    String cc= countryCode=="US"?"+1":"+91";
    final regex = RegExp(r'^\+\d{1,3}');
    if (!regex.hasMatch(phoneNumber)) {
      return '$cc$phoneNumber';
    }
    return phoneNumber;
  }

}
