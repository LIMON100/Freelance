import 'dart:io';
import 'package:carousel_slider/carousel_slider.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:cached_network_image/cached_network_image.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:image_picker/image_picker.dart';
import 'package:groom/data_models/provider_service_model.dart';
import 'package:groom/firebase/provider_service_firebase.dart';

class EditProviderServiceScreen extends StatefulWidget {
  final ProviderServiceModel service;

  const EditProviderServiceScreen({Key? key, required this.service})
      : super(key: key);

  @override
  _EditProviderServiceScreenState createState() =>
      _EditProviderServiceScreenState();
}

class _EditProviderServiceScreenState extends State<EditProviderServiceScreen> {
  late TextEditingController priceController;
  late TextEditingController descriptionController;
  List<String> serviceImages = [];
  List<File> newImages = [];

  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();

  @override
  void initState() {
    super.initState();
    priceController =
        TextEditingController(text: widget.service.servicePrice.toString());
    descriptionController =
        TextEditingController(text: widget.service.description);
    serviceImages = widget.service.serviceImages ?? [];
  }

  Future<void> pickImage() async {
    final pickedFile =
        await ImagePicker().pickImage(source: ImageSource.gallery);
    if (pickedFile != null) {
      setState(() {
        newImages.add(File(pickedFile.path));
      });
    }
  }

  Future<void> saveService() async {
    List<String> uploadedImages = [];

    for (var image in newImages) {
      String url = await providerServiceFirebase.uploadImage(
          image, FirebaseAuth.instance.currentUser!.uid);
      uploadedImages.add(url);
    }
    serviceImages.addAll(uploadedImages);

    ProviderServiceModel updatedService = ProviderServiceModel(
      serviceId: widget.service.serviceId,
      userId: widget.service.userId,
      serviceType: widget.service.serviceType,
      servicePrice: int.parse(priceController.text),
      description: descriptionController.text,
      serviceImages: serviceImages,
      serviceName: "",
    );

    bool result2 = await providerServiceFirebase.updateServiceEdit(
      widget.service.serviceId,
      serviceImages,
      descriptionController.text,
      int.parse(priceController.text),
    );

    // bool result = await providerServiceFirebase.writeServiceToFirebase(
    //     widget.service.serviceId, updatedService);
    if (result2) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Service updated successfully'),
        ),

      );
    } else {
      Get.back();
    }
    Get.back(closeOverlays: true);

  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: HeaderTxtWidget('Edit Service',color: Colors.white,),
        actions: [
          IconButton(
            icon: Icon(Icons.save),
            onPressed: saveService,
          ),
        ],
      ),
      body: SingleChildScrollView(
        child: Column(
          children: [
            CarouselSlider(
              items: [
                // Display the existing service images
                ...serviceImages.map((url) => Stack(
                  children: [
                    CachedNetworkImage(
                      imageUrl: url,
                      width: 1000.0,
                      fit: BoxFit.cover,
                    ),
                    Positioned(
                      top: 10,
                      right: 10,
                      child: IconButton(
                        icon: Icon(Icons.delete, color: Colors.red),
                        onPressed: () {
                          if (serviceImages.length <= 1 && newImages.isEmpty) {
                            ScaffoldMessenger.of(context).showSnackBar(
                              SnackBar(
                                content: Text('You can\'t remove all images'),
                                backgroundColor: Colors.red,
                              ),
                            );
                            return;
                          }
                          setState(() {
                            serviceImages.remove(url);
                          });
                        },
                      ),
                    ),
                  ],
                )),
                // Display the newly picked images
                ...newImages.map((file) => Stack(
                  children: [
                    Image.file(
                      file,
                      width: 1000.0,
                      fit: BoxFit.cover,
                    ),
                    Positioned(
                      top: 10,
                      right: 10,
                      child: IconButton(
                        icon: Icon(Icons.delete, color: Colors.red),
                        onPressed: () {
                          setState(() {
                            newImages.remove(file);
                          });
                        },
                      ),
                    ),
                  ],
                )),
              ],
              options: CarouselOptions(
                height: 300,
                aspectRatio: 16 / 9,
                viewportFraction: 0.8,
                initialPage: 0,
                enableInfiniteScroll: true,
                reverse: false,
                autoPlay: false,
                enlargeCenterPage: true,
                scrollDirection: Axis.horizontal,
              ),
            ),
            ElevatedButton.icon(
              onPressed: pickImage,
              icon: Icon(Icons.add),
              label: Text('Add Image'),
            ),
            Padding(
              padding: const EdgeInsets.all(8.0),
              child: TextField(
                controller: priceController,
                keyboardType: TextInputType.number,
                decoration: InputDecoration(
                  labelText: 'Service Price',
                  border: OutlineInputBorder(),
                ),
              ),
            ),
            Padding(
              padding: const EdgeInsets.all(8.0),
              child: TextField(
                controller: descriptionController,
                maxLines: 5,
                decoration: InputDecoration(
                  labelText: 'About Service',
                  border: OutlineInputBorder(),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
