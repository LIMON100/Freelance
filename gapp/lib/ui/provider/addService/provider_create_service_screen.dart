import 'dart:io';
import 'dart:typed_data';
import 'package:file_picker_pro/file_data.dart';
import 'package:file_picker_pro/file_picker.dart';
import 'package:file_picker_pro/files.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:font_awesome_flutter/font_awesome_flutter.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:groom/utils/tools.dart';
import 'package:groom/data_models/provider_service_model.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/firebase/provider_service_firebase.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/custom_switch.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/input_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import 'package:image_picker/image_picker.dart';
import '../../../generated/assets.dart';
import '../../../repo/setting_repo.dart';
import '../../../screens/google_maps_screen.dart';
import '../../../utils/utils.dart';
import 'model/cancelation_policy_model.dart';

class ProviderCreateServiceScreen extends StatefulWidget {
  ProviderServiceModel?data;
   ProviderCreateServiceScreen({super.key,this.data});

  @override
  State<ProviderCreateServiceScreen> createState() =>
      _ProviderCreateServiceScreenState();
}

List<String> services = [
  "Hair Style",
  "Nails",
  "Facial",
  "Coloring",
  "Spa",
  "Wax",
  "Makeup",
  "Massage"
];

class _ProviderCreateServiceScreenState extends State<ProviderCreateServiceScreen> {
  LatLng? selectedLocation;
  Uint8List? mapScreenshot;
  String? selectedService;
  List<File> _images = [];
  bool _isLoading = false;
  bool isSetCustom = false;

  List<CancelationPolicyModel> cancelationPolicy = [
    CancelationPolicyModel(title: 'Flexible',description: "(2 days out 50% refund)"),
    CancelationPolicyModel(title: 'Moderate',description: "(7 days out 50% refund)"),
    CancelationPolicyModel(title: 'Strict',description: "(14 days out 50% refund)"),
    CancelationPolicyModel(title: 'Non-refundable',description: "(0% immediately)"),
  ];

  final _ProviderformKey = GlobalKey<FormState>();
  final TextEditingController _descriptionController = TextEditingController();
  final TextEditingController _serviceNameController = TextEditingController();
  final TextEditingController _priceController = TextEditingController();
  final TextEditingController _depositPriceController = TextEditingController();
  final TextEditingController _workingHours = TextEditingController(text: "1");
  final TextEditingController _appointmentDuration = TextEditingController();
  double bufferTime = 10;
  double recommendedBooking = 1;


  Future<void> _saveCustomerServiceModel() async {
    if (_ProviderformKey.currentState!.validate()) {
      if (_images.isEmpty&&widget.data==null) {
        Get.snackbar("Error", "Please add at least one image",
            backgroundColor: Colors.blue, colorText: Colors.black);
        return;
      }

      if (selectedLocation == null) {
        Get.snackbar("Error", "Please select a location",
            backgroundColor: Colors.blue, colorText: Colors.black);
        return;
      }
      int? servicePrice = int.tryParse(_priceController.text);
      int? serviceDeposit = int.tryParse(_depositPriceController.text);
      if (servicePrice == null) {
        Get.snackbar("Error", "Invalid price value");
        return;
      }
      if (serviceDeposit == null) {
        Get.snackbar("Error", "Invalid deposit value");
        return;
      }if (getCancelationPolicy().isEmpty) {
        Get.snackbar("Error", "Please select cancellation policy");
        return;
      }
      setState(() {
        _isLoading=true;
      });
      List<String> imageUrls = [];
      for (File image in _images) {
        String imageUrl = await ProviderServiceFirebase()
            .uploadImage(image,auth.value.uid);
        if (imageUrl.isNotEmpty) {
          imageUrls.add(imageUrl);
        }
      }
      if(widget.data!=null){
        for (String imageUrl in widget.data!.serviceImages!) {
          if (imageUrl.isNotEmpty) {
            imageUrls.add(imageUrl);
          }
        }
      }

      final providerService = ProviderServiceModel(
        userId: auth.value.uid,
        serviceId: widget.data!=null?widget.data!.serviceId:generateProjectId(),
        description: _descriptionController.text,
        serviceName: _serviceNameController.text,
        serviceType: selectedService!,
        location: selectedLocation,
        servicePrice: servicePrice,
        serviceDeposit: serviceDeposit,
        serviceImages: imageUrls,
        bufferTime: bufferTime.toInt(),
        cancelationPolicy: getCancelationPolicy(),
        maximumBookingPerDay: recommendedBooking.toInt(),
        workingHours: _workingHours.text,
        appointmentDuration: int.parse(_appointmentDuration.text),
          address: await Tools.getAddress(selectedLocation!)??""
      );
      await ProviderServiceFirebase().writeServiceToFirebase(providerService.serviceId, providerService);
      setState(() {
        _isLoading = false;
      });
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          backgroundColor: Colors.green,
          content: Text('Service Created successfully'),
        ),
      );
      Get.back();
    }
  }
  String getCancelationPolicy(){
    StringBuffer buffer=StringBuffer();
    for (var element in cancelationPolicy) {
      if(element.isSelected) {
        if (buffer.toString().isNotEmpty) {
          buffer.write("/n");
        }
        buffer.write("<b>");
        buffer.write(element.title);
        buffer.write("</b> ");
        buffer.write("<p>");
        buffer.write(element.description);
        buffer.write("</p> ");
      }
    }
    return buffer.toString();
  }
  @override
  void initState() {
    // TODO: implement initState
    super.initState();
    init();
  }
  void init(){
    if(widget.data!=null){
      _serviceNameController.text=widget.data!.serviceName??"";
      _descriptionController.text=widget.data!.description??"";
      _priceController.text=widget.data!.servicePrice.toString();
      _depositPriceController.text=widget.data!.serviceDeposit.toString();
      _workingHours.text=widget.data!.workingHours.toString();
      _appointmentDuration.text=widget.data!.appointmentDuration.toString();
      selectedService=widget.data!.serviceType;
      selectedLocation=widget.data!.location!;
    }
  }
  @override
  Widget build(BuildContext context) {
    Size size = MediaQuery.of(context).size;
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: Colors.white,
        elevation: 1,
        surfaceTintColor: Colors.white,
        iconTheme: IconThemeData(color: primaryColorCode),
        title: HeaderTxtWidget("Add Service"),
      ),
      body: Container(
        padding: const EdgeInsets.symmetric(vertical: 10, horizontal: 15),
        child: SingleChildScrollView(
          child: Form(
            key: _ProviderformKey,
            autovalidateMode: AutovalidateMode.always,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                if (_images.isEmpty&&widget.data==null)
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
                      allowedExtensions: const ['jpg','jpeg','png'],
                      onSelected: (fileData) {
                        setState(() {
                          _images.add(File(fileData.path));
                        });
                      },
                      onCancel: (message, messageCode) {

                      },
                      child: Container(
                        width: size.width,
                        height: 120,
                        margin: const EdgeInsets.symmetric(vertical: 10),
                        alignment: AlignmentDirectional.center,
                        decoration: const BoxDecoration(
                          image: DecorationImage(
                              image: AssetImage(Assets.imgDottedBg),
                              fit: BoxFit.fill),
                        ),
                        child: Image.asset(Assets.imgChooseImage),
                      )
                  ),
                  Wrap(
                    children: [
                      for (var image in _images)
                        Container(
                          height: 100,
                          width: 100,
                          margin: const EdgeInsets.symmetric(
                              horizontal: 5, vertical: 5),
                          child: ClipRRect(
                            borderRadius:
                                const BorderRadius.all(Radius.circular(5)),
                            child: Stack(
                              children: [
                                Image.file(
                                  image,
                                  width: 100,
                                  height: 100,
                                  fit: BoxFit.cover,
                                ),
                                Positioned(
                                    top: -5,
                                    right: -5,
                                    child: IconButton(
                                        onPressed: () {
                                          setState(() {
                                            _images.remove(image);
                                          });
                                        },
                                        icon: const Icon(
                                          Icons.cancel,
                                          color: Colors.white,
                                        )))
                              ],
                            ),
                          ),
                        ),
                      if(widget.data!=null)
                      for (var image in widget.data!.serviceImages!)
                        Container(
                          height: 100,
                          width: 100,
                          margin: const EdgeInsets.symmetric(
                              horizontal: 5, vertical: 5),
                          child: ClipRRect(
                            borderRadius:
                            const BorderRadius.all(Radius.circular(5)),
                            child: Stack(
                              children: [
                                NetworkImageWidget(url: image,
                                  width: 100,
                                  height: 100,
                                  fit: BoxFit.cover,
                                ),
                                Positioned(
                                    top: -5,
                                    right: -5,
                                    child: IconButton(
                                        onPressed: () {
                                          setState(() {
                                            _images.remove(image);
                                          });
                                        },
                                        icon: const Icon(
                                          Icons.cancel,
                                          color: Colors.white,
                                        )))
                              ],
                            ),
                          ),
                        ),
                      if(_images.isNotEmpty||widget.data!=null)
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
                            allowedExtensions: const ['jpg','jpeg','png'],
                            onSelected: (fileData) {
                              setState(() {
                                _images.add(File(fileData.path));
                              });
                            },
                            onCancel: (message, messageCode) {

                            },
                            child: Container(
                              width: 100,
                              height: 100,
                              color: Colors.transparent,
                              child: Image.asset("assets/addImage.png"),
                            )
                        )

                    ],
                  ),
                Center(
                  child: SubTxtWidget(
                    'Support : JPG , PNG, JPEG',
                    fontSize: 12,
                    color: "#6C757D".toColor(),
                  ),
                ),
                Container(
                  margin: const EdgeInsets.symmetric(vertical: 10),
                  padding: const EdgeInsets.all(10),
                  decoration: BoxDecoration(
                      borderRadius: const BorderRadius.all(Radius.circular(10)),
                      color: "#F6F7F9".toColor()),
                  child: Column(
                    children: [
                      InputWidget(
                        controller: _serviceNameController,
                        hint: "Service name",
                        color: Colors.white,
                        margin: const EdgeInsets.symmetric(vertical: 15),
                        validatorCallback: (value) {
                          if (value == null || value.isEmpty) {
                            return 'Please enter service name';
                          }
                          return null;
                        },
                      ),
                      DropdownButtonFormField<String>(
                        value: selectedService,
                        decoration: const InputDecoration(
                            border: OutlineInputBorder(
                              borderRadius:
                                  BorderRadius.all(Radius.circular(12)),
                              borderSide: BorderSide.none,
                            ),
                            fillColor: Colors.white,
                            filled: true),
                        hint: SubTxtWidget("Select Service"),
                        isExpanded: true,
                        items: services.map((String service) {
                          return DropdownMenuItem<String>(
                            value: service,
                            child: Text(service),
                          );
                        }).toList(),
                        onChanged: (String? newValue) {
                          setState(() {
                            selectedService = newValue!;
                          });
                        },
                        validator: (value) =>
                            value == null ? 'Please select a service' : null,
                      ),
                      Row(
                        children: [
                          Expanded(
                            child: InputWidget(
                              controller: _priceController,
                              hint: "Service price \$",
                              sufix: "\$",
                              inputType: TextInputType.number,
                              color: Colors.white,
                              validatorCallback: (value) {
                                if (value == null || value.isEmpty) {
                                  return 'Please enter a price';
                                }
                                double dep=double.parse(value);
                                if(dep==0){
                                  return 'Price 0 not allow';
                                }if(dep<0){
                                  return 'Price -ve not allow';
                                }
                                return null;
                              },
                            ),
                          ),
                          Expanded(
                              child: InputWidget(
                            controller: _depositPriceController,
                            hint: "Service Deposit \$",
                            sufix: "\$",
                            inputType: TextInputType.number,
                            color: Colors.white,
                            validatorCallback: (value) {
                              if (value == null || value.isEmpty) {
                                return 'Please enter a deposit';
                              }
                              double dep=double.parse(value);
                              if(dep==0){
                                return 'Deposit 0 not allow';
                              }if(dep<0){
                                return 'Deposit -ve not allow';
                              }
                              if(_priceController.text.isNotEmpty) {
                                double price = double.parse(_priceController.text);
                                if(dep>price){
                                  return 'Required less from price';
                                }
                              }
                              return null;
                            },
                          ))
                        ],
                      ),
                      InputWidget(
                        controller: _descriptionController,
                        hint: "Service Description",
                        maxLines: 4,
                        inputType: TextInputType.text,
                        color: Colors.white,
                        validatorCallback: (value) =>
                            value == null || value.isEmpty
                                ? 'Please enter a description'
                                : null,
                      )
                    ],
                  ),
                ),
                const SizedBox(
                  height: 10,
                ),
                HeaderTxtWidget('Appointment Duration'),
                const SizedBox(
                  height: 10,
                ),
                SubTxtWidget(
                  'Booked appointment settings',
                  fontWeight: FontWeight.w600,
                  fontSize: 15,
                  color: "#5B5C5E".toColor(),
                ),
                const SizedBox(
                  height: 5,
                ),
                SubTxtWidget(
                    'Manage the booked appointments that will appear on your calendar',
                    fontSize: 14,
                    color: "#A0A2A5".toColor()),
                const SizedBox(
                  height: 10,
                ),
                SubTxtWidget(
                  'Buffer time',
                  fontWeight: FontWeight.w600,
                  fontSize: 15,
                  color: "#5B5C5E".toColor(),
                ),
                SubTxtWidget(
                  'Add time between appointment slots',
                  fontSize: 14,
                  color: "#A0A2A5".toColor(),
                ),
                const SizedBox(height: 10,),
                Padding(padding: const EdgeInsets.symmetric(horizontal: 15),
                  child: SubTxtWidget('Buffer time: ${bufferTime.toInt()} minutes',
                  color: "#172B4D".toColor(),),),
                Slider(
                  value: bufferTime,
                  onChanged: (value) {
                    setState(() {
                      bufferTime = value;
                    });
                  },
                  min: 10,
                  max: 60,
                  divisions: 10,
                ),
                const SizedBox(
                  height: 10,
                ),
                SubTxtWidget(
                  'Maximum bookings per day',
                  fontWeight: FontWeight.w600,
                  fontSize: 15,
                  color: "#5B5C5E".toColor(),
                ),
                SubTxtWidget(
                  'Limit how many booked appointments to accept in a single day',
                  fontSize: 14,
                  color: "#A0A2A5".toColor(),
                ),
                Row(
                  children: [
                    Expanded(child: SubTxtWidget('Maximum daily hours')),
                    Container(
                      width: 150,
                      margin: const EdgeInsets.symmetric(horizontal: 5),
                      child: DropdownButtonFormField<String>(items: [
                        for(int i=1;i<19;i++)
                          DropdownMenuItem(value: '$i',child: SubTxtWidget(i==1?'$i hour':'$i hours'),),
                      ], onChanged: (value) {
                        setState(() {
                          _workingHours.text=value??"1";
                        });
                      },
                        decoration: InputDecoration(
                          enabledBorder: OutlineInputBorder(
                            borderSide:
                            BorderSide(width: 1, color:   "#CCCCCC".toColor()),
                            borderRadius: BorderRadius.circular(12.0),
                          ),
                          fillColor:  Colors.grey.shade100,
                          contentPadding:
                              const EdgeInsets.symmetric(horizontal: 10, vertical: 15),
                          focusedBorder: OutlineInputBorder(
                            borderSide:
                            BorderSide(width: 1, color:  "#CCCCCC".toColor()),
                            borderRadius: BorderRadius.circular( 12.0),
                          ),
                          errorBorder: OutlineInputBorder(
                            borderSide:
                            BorderSide(width: 1, color:  Colors.red),
                            borderRadius: BorderRadius.circular(12.0),
                          ),
                          errorStyle: const TextStyle(color: Colors.red),
                          border: OutlineInputBorder(
                            borderSide:
                            BorderSide(width: 1, color: primaryColorCode),
                            borderRadius: BorderRadius.circular( 12.0),
                          ),
                          focusedErrorBorder: OutlineInputBorder(
                            borderSide: BorderSide(width: 1, color:  Colors.red),
                            borderRadius: BorderRadius.circular(12.0),
                          ),
                          filled: true,
                        ),
                        value: _workingHours.text.isEmpty?"1":_workingHours.text,
                        validator: (value) {
                          if (value!.isEmpty) {
                            return "Required";
                          }
                          return null;
                        },
                      ),
                    ),
                  ],
                ),
                Row(
                  children: [
                    Expanded(child: SubTxtWidget('Average Appointment Duration (minutes)')),
                    InputWidget(
                      width: 150,
                      controller: _appointmentDuration,
                      contentPadding: const EdgeInsets.symmetric(horizontal: 10),
                      color: "#CCCCCC".toColor(),
                      fillColor: Colors.grey.shade100,
                      inputType: TextInputType.number,
                      validatorCallback: (value) {
                        if (value!.isEmpty) {
                          return "Required";
                        }
                        double div=double.parse(value);
                        if(div<15){
                          return "Min 15 min required";
                        }
                        return null;
                      },
                    ),
                  ],
                ),
                Row(
                  children: [
                  CustomSwitch(value: isSetCustom, onChanged: (value) {
                    setState(() {
                      isSetCustom=value;
                    });
                  }, borderColor: Colors.red, innerColor: Colors.red,),
                  const SizedBox(width: 10,),
                  HeaderTxtWidget('Set Maximum bookings per day',color: "#6C757D".toColor(),),
                  ],
                ),
                const SizedBox(
                  height: 10,
                ),
                Padding(padding: const EdgeInsets.symmetric(horizontal: 15),
                  child: SubTxtWidget('Recommended maximum : ${getMaxBooking()} bookings',
                    color: "#172B4D".toColor(),),),
                if(!isSetCustom)
                Slider(
                  value: recommendedBooking,
                  onChanged: (value) {
                    setState(() {
                      recommendedBooking = value;
                    });
                  },
                  min: 1,
                  max: getMaxBooking().toDouble(),
                ),
                Padding(padding: const EdgeInsets.symmetric(horizontal: 15),
                  child: SubTxtWidget('Current maximum bookings per day: ${recommendedBooking.toInt()}',
                    fontSize: 12,
                    color: "#172B4D".toColor(),),),
                const SizedBox(
                  height: 20,
                ),

                HeaderTxtWidget('Cancellation Policy',fontSize: 20,),
                const SizedBox(
                  height: 10,
                ),
                ListView.builder(
                  itemBuilder: (context, index) {
                    return InkWell(
                      onTap: () {
                        for (var element in cancelationPolicy) {
                          element.isSelected=false;
                        }
                        setState(() {
                          cancelationPolicy[index].isSelected=!cancelationPolicy[index].isSelected;
                        });
                      },
                      child: Container(
                        padding: const EdgeInsets.symmetric(vertical: 10),
                        child: Row(
                          children: [
                            cancelationPolicy[index].isSelected?const Icon(Icons.radio_button_checked,color: Colors.blue,):const Icon(Icons.radio_button_off,color: Colors.blue,),
                            const SizedBox(width: 10,),
                            SubTxtWidget(cancelationPolicy[index].title,fontWeight: FontWeight.bold,fontSize: 14,),
                            const SizedBox(width: 10,),
                            SubTxtWidget(cancelationPolicy[index].description,fontSize: 12,color: Colors.grey.shade400,),
                          ],
                        )
                      ),
                    );
                  },
                  itemCount: cancelationPolicy.length,
                  shrinkWrap: true,
                  primary: false,
                ),
                const SizedBox(
                  height: 10,
                ),
                HeaderTxtWidget('Location'),
                const SizedBox(
                  height: 10,
                ),
                Container(
                  child: mapScreenshot != null||widget.data!=null
                      ? Column(
                          children: [
                            InkWell(
                              child: Row(
                                children: [
                                  SvgPicture.asset(Assets.svgSelectLocation),
                                  const SizedBox(
                                    width: 10,
                                  ),
                                  Expanded(
                                      child: FutureBuilder(
                                        future: Tools.getAddress(selectedLocation!),
                                        builder: (context, snapshot) {
                                          return SubTxtWidget('${snapshot.data}');
                                        },
                                      ))
                                ],
                              ),
                              onTap: () async {
                                final result = await Get.to(() => const GoogleMapScreen());
                                if (result != null) {
                                  setState(() {
                                    selectedLocation = result['location'];
                                    mapScreenshot = result['screenshot'];
                                  });
                                }
                              },
                            ),
                            if(mapScreenshot!=null)
                            SizedBox(
                              width: size.width,
                              height: 200,
                              child: ClipRRect(
                                borderRadius:
                                    const BorderRadius.all(Radius.circular(10)),
                                child: Image.memory(
                                  mapScreenshot!,
                                  fit: BoxFit.cover,
                                ),
                              ),
                            ),
                          ],
                        )
                      : Center(
                          child: TextButton(
                            onPressed: () async {
                              final result =
                                  await Get.to(() => const GoogleMapScreen());
                              if (result != null) {
                                setState(() {
                                  selectedLocation = result['location'];
                                  mapScreenshot = result['screenshot'];
                                });
                              }
                            },
                            child: Column(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                FaIcon(
                                  FontAwesomeIcons.locationDot,
                                  size: 40,
                                  color: Colors.blue.shade200,
                                ),
                                const SizedBox(
                                  height: 12,
                                ),
                                const Text(
                                  "Location",
                                  style: TextStyle(
                                      fontSize: 17,
                                      fontWeight: FontWeight.w600),
                                ),
                              ],
                            ),
                          ),
                        ),
                ),
                Center(
                  child: ButtonPrimaryWidget(
                    "Submit",
                    marginVertical: 20,
                    onTap: _isLoading ? null : _saveCustomerServiceModel,
                    isLoading: _isLoading,
                    color: "#5F60B9".toColor(),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
  int getMaxBooking(){
    if(_workingHours.text.isEmpty||_appointmentDuration.text.isEmpty){
      return 1;
    }
    double workingHrs=double.parse(_workingHours.text);
    double workingMin=(workingHrs*60)-(bufferTime*workingHrs);
    double appointmentDuration=double.parse(_appointmentDuration.text);
    double duration=(workingMin/appointmentDuration);
    if(isSetCustom) {
      recommendedBooking=duration.toInt().toDouble();
    }
    if(duration<1)return 1;
    if(recommendedBooking>duration){
      recommendedBooking=1;
    }
    return duration.toInt();
  }
}
