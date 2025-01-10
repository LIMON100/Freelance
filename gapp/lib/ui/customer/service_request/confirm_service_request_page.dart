import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:get/get.dart';
import 'package:groom/Utils/tools.dart';
import 'package:groom/generated/assets.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';

import '../../../data_models/customer_offer_model.dart';
import '../../../firebase/customer_offer_firebase.dart';

class ConfirmServiceRequestPage extends StatefulWidget {
  const ConfirmServiceRequestPage({super.key});

  @override
  State<ConfirmServiceRequestPage> createState() =>
      _ConfirmServiceRequestPageState();
}

class _ConfirmServiceRequestPageState extends State<ConfirmServiceRequestPage> {
  List<File> _images = [];
  late CustomerOfferModel data;
  bool success = false;

  @override
  Widget build(BuildContext context) {
    data = Get.arguments['data'];
    _images = Get.arguments['images'];
    return Scaffold(
      backgroundColor: Colors.white,
      body: SingleChildScrollView(
        child: Container(
          padding: const EdgeInsets.only(left: 20, right: 20, top: 100),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              if (success) ...{
                Column(
                  children: [
                    SvgPicture.asset(Assets.svgSuccessCheck),
                    HeaderTxtWidget('Thank you for your request!'),
                    SubTxtWidget(
                      'Your service request has been posted and is now visible to all providers. You will be notified when providers respond',
                      textAlign: TextAlign.center,
                    ),
                    SubTxtWidget(
                      'We have received your service request. Here are the details you submitted:',
                      textAlign: TextAlign.center,
                    ),
                  ],
                )
              } else ...{
                Center(
                  child: SvgPicture.asset(Assets.svgCloudDown),
                ),
                const SizedBox(
                  height: 10,
                ),
                Center(
                  child: HeaderTxtWidget('Your Request'),
                ),
              },
              const SizedBox(
                height: 50,
              ),
              HeaderTxtWidget('Review Your Service Request'),
              SubTxtWidget('Here\'s a summary of your service request:'),
              Divider(
                color: Colors.grey.shade200,
              ),
              const SizedBox(
                height: 20,
              ),
              Row(
                children: [
                  // HeaderTxtWidget('Timing :'),
                  const SizedBox(
                    width: 20,
                  ),
                  SubTxtWidget('${data.selectedTime}'),
                ],
              ),
              const SizedBox(
                height: 10,
              ),
              Row(
                children: [
                  HeaderTxtWidget('Date :'),
                  const SizedBox(
                    width: 20,
                  ),
                  SubTxtWidget(Tools.changeDateFormat(
                      data.dateTime.toString(), "MM-dd-yyyy")),
                ],
              ),
              const SizedBox(
                height: 10,
              ),
              Row(
                children: [
                  HeaderTxtWidget('Service Name :'),
                  const SizedBox(
                    width: 20,
                  ),
                  SubTxtWidget('${data.serviceName}'),
                ],
              ),
              const SizedBox(
                height: 10,
              ),
              Row(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  HeaderTxtWidget('Description :'),
                  const SizedBox(
                    width: 20,
                  ),
                  Expanded(
                    child: SubTxtWidget('${data.description}'),
                  )
                ],
              ),
              const SizedBox(
                height: 10,
              ),
              Row(
                children: [
                  HeaderTxtWidget('Budget :'),
                  const SizedBox(
                    width: 20,
                  ),
                  SubTxtWidget('${data.priceRange}'),
                ],
              ),
              const SizedBox(
                height: 10,
              ),
              Row(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  HeaderTxtWidget('Address :'),
                  const SizedBox(
                    width: 20,
                  ),
                  Expanded(
                    child: SubTxtWidget('${data.address}'),
                  )
                ],
              ),
              const SizedBox(
                height: 10,
              ),
              if (_images.isNotEmpty)
                Center(
                  child: Image.file(
                    _images.first,
                    height: 150,
                  ),
                ),
              const SizedBox(
                height: 20,
              ),
              if (success) ...{
                Center(
                  child: ActionChip(
                    label: SubTxtWidget(
                      'Close',
                      color: Colors.white,
                    ),
                    backgroundColor: Colors.black,
                    onPressed: () {
                      Get.offAllNamed('/customer_dashboard');
                    },
                  ),
                ),
              } else ...{
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    ActionChip(
                      label:
                          SubTxtWidget('Submit Request', color: Colors.white),
                      backgroundColor: primaryColorCode,
                      onPressed: () {
                        submit();
                      },
                    ),
                    const SizedBox(
                      width: 20,
                    ),
                    ActionChip(
                      label: SubTxtWidget(
                        'Edit Request',
                        color: Colors.white,
                      ),
                      backgroundColor: Colors.black,
                      onPressed: () {
                        Get.back();
                      },
                    )
                  ],
                ),
              },
              const SizedBox(
                height: 20,
              ),
            ],
          ),
        ),
      ),
    );
  }

  Future<void> submit() async {
    _showLoadingDialog(context);
    List<String> imageUrls = [];
    if (_images.isNotEmpty) {
      for (File image in _images) {
        String imageUrl =
            await CustomerOfferFirebase().uploadImage(image, auth.value.uid);
        if (imageUrl.isNotEmpty) {
          imageUrls.add(imageUrl);
        }
      }
      data.offerImages = imageUrls;
    }
    await CustomerOfferFirebase().writeOfferToFirebase(data.offerId, data);
    setState(() {
      success = true;
    });
    _hideLoadingDialog(context);
  }

  void _showLoadingDialog(context) {
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (BuildContext context) {
        return const AlertDialog(
          content: Row(
            children: [
              CircularProgressIndicator(),
              SizedBox(width: 20),
              Text("Creating offer, Please wait..."),
            ],
          ),
        );
      },
    );
  }

  void _hideLoadingDialog(context) {
    Navigator.of(context).pop();
  }
}
