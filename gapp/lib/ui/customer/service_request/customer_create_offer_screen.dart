import 'dart:io';
import 'package:easy_localization/easy_localization.dart';
import 'package:file_picker_pro/file_data.dart';
import 'package:file_picker_pro/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:google_places_autocomplete_text_field/google_places_autocomplete_text_field.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/custom_switch.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import '../../../utils/tools.dart';
import '../../../data_models/customer_offer_model.dart';
import '../../../generated/assets.dart';
import '../../../states/customer_offer_state.dart';
import '../../../utils/utils.dart';
import '../../../widgets/input_widget.dart';
import '../../../widgets/sub_txt_widget.dart';
import '../../../widgets/time_widget.dart';
import '../../provider/addService/provider_create_service_screen.dart';

class CustomerCreateOfferScreen extends StatefulWidget {
  CustomerOfferModel? data;
  CustomerCreateOfferScreen({super.key, this.data});

  @override
  State<CustomerCreateOfferScreen> createState() =>
      _CustomerCreateOfferScreenState();
}

class _CustomerCreateOfferScreenState extends State<CustomerCreateOfferScreen> {
  CustomerOfferStateController customerOfferState =
      CustomerOfferStateController();
  String? selectedService;
  String selectedRadius = "1mi";
  final List<File> _images = [];
  final bool _isLoading = false;
  bool _depositAccept = false;
  final _formKey = GlobalKey<FormState>();
  final TextEditingController _descriptionController = TextEditingController();
  final TextEditingController _locationController = TextEditingController();

  RangeValues _currentRangeValues = const RangeValues(5, 2000);
  final TextEditingController serviceNameCon = TextEditingController();
  String currentTime = DateFormat.jm().format(DateTime.now());
  DateTime? selectedDate;
  String? selectedTime;
  String? selectedTime2;
  String? date;
  List<String> timeList = [
    "Custom",
    "Open Now ${DateFormat.jm().format(DateTime.now())}",
    "Later Today",
    "3am - 7am",
    "10pm - 2am",
  ];
  LatLng _initialLocation = auth.value.location ?? defalultLatLng;
  GoogleMapController? mapController;
  Set<Circle> circles = {};

  @override
  void initState() {
    super.initState();
    initEdit();
  }

  void initEdit() {
    if (widget.data != null) {
      if (timeList.contains(widget.data!.selectedTime)) {
        selectedTime = widget.data!.selectedTime;
      } else {
        selectedTime = "Custom";
        selectedTime2 = widget.data!.selectedTime;
      }
      date = "Choose from calender";
      selectedDate = widget.data!.dateTime;
      serviceNameCon.text = widget.data!.serviceName!;
      _descriptionController.text = widget.data!.description;
      selectedService = widget.data!.serviceType!;
      _depositAccept = widget.data!.deposit ?? false;
      _locationController.text = widget.data!.address!;
      _initialLocation = widget.data!.location!;
      selectedRadius = widget.data!.radius!;
      try {
        _currentRangeValues = RangeValues(
            double.parse(widget.data!.priceRange!.split("-")[0]),
            double.parse(widget.data!.priceRange!.split("-")[1]));
      } catch (e, s) {
        print(s);
      }
      setState(() {});
    }
  }

  void getDateTime() {
    DateTime now = DateTime.now();
    if (date == "Today") {
      selectedDate = now;
    }
    if (date == "Tomorrow") {
      selectedDate = DateTime(now.year, now.month, now.day + 1);
    }
    if (date == "This week") {
      int week = 7 - now.weekday;
      selectedDate = DateTime(now.year, now.month, now.day + week);
    }
    print("selectedDate==>${selectedDate.toString()}");
  }

  Future<void> _saveCustomerServiceModel() async {
    if (_formKey.currentState!.validate()) {
      getDateTime();
      if (selectedTime == null) {
        Tools.ShowErrorMessage('Please select time');
        return;
      } else {
        if (selectedTime == "Custom" && selectedTime2 == null) {
          Tools.ShowErrorMessage('Please select time');
        }
      }
      if (selectedDate == null) {
        Tools.ShowErrorMessage('Please select date');
        return;
      }
      if (selectedService == null) {
        Tools.ShowErrorMessage('Please select service type');

        return;
      }
      if (_locationController.text.isEmpty) {
        Tools.ShowErrorMessage('Please select location');
        return;
      }
      if (selectedRadius.isEmpty) {
        Tools.ShowErrorMessage('Please select radius');
        return;
      }

      final customerService = CustomerOfferModel(
          userId: auth.value.uid,
          offerId:
              widget.data == null ? generateProjectId() : widget.data!.offerId,
          description: _descriptionController.text,
          serviceType: selectedService,
          priceRange:
              '${_currentRangeValues.start.round()}-${_currentRangeValues.end.round()}',
          dateTime: selectedDate,
          deposit: _depositAccept,
          location: _initialLocation,
          selectedTime: selectedTime == "Custom" ? selectedTime2 : selectedTime,
          radius: selectedRadius,
          address: _locationController.text,
          serviceName: serviceNameCon.text,
          offerImages: widget.data == null ? [] : widget.data!.offerImages!);

      Get.toNamed('/confirm_service_request_list',
          arguments: {"data": customerService, "images": _images});
    }
  }

  @override
  Widget build(BuildContext context) {
    Size size = MediaQuery.of(context).size;
    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: Colors.white,
        iconTheme: IconThemeData(color: primaryColorCode),
        title: HeaderTxtWidget(widget.data != null
            ? "Edit Service request"
            : "Create your Service request"),
      ),
      body: Container(
        padding: const EdgeInsets.symmetric(horizontal: 20),
        child: SingleChildScrollView(
          child: Form(
            key: _formKey,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                if (_images.isEmpty && widget.data == null)
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
                      setState(() {
                        _images.add(File(fileData.path));
                      });
                    },
                    onCancel: (message, messageCode) {},
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
                    ),
                  ),
                if (_images.isNotEmpty || widget.data != null)
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
                      if (widget.data != null) ...{
                        for (var image in widget.data!.offerImages!)
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
                                  NetworkImageWidget(
                                    url: image,
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
                                              widget.data!.offerImages!
                                                  .remove(image);
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
                      },
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
                            setState(() {
                              _images.add(File(fileData.path));
                            });
                          },
                          onCancel: (message, messageCode) {},
                          child: Container(
                            width: 100,
                            height: 100,
                            color: Colors.transparent,
                            child: Image.asset("assets/addImage.png"),
                          )),
                    ],
                  ),
                Center(
                  child: SubTxtWidget(
                    'Support : JPG , PNG, JPEG',
                    fontSize: 12,
                    color: "#6C757D".toColor(),
                  ),
                ),
                HeaderTxtWidget('Ranges'),
                const SizedBox(
                  height: 5,
                ),
                Wrap(
                  children: timeList.map(
                    (e) {
                      return InkWell(
                        onTap: () {
                          if (date != "Today" && e == "Later Today") {
                            Tools.ShowErrorMessage(
                                '"Later Today" can only be selected when the date is "Today".');
                            return;
                          }

                          if (e == "Custom") {
                            showDialog(
                              context: context,
                              builder: (context) {
                                return TimeWidget(
                                  onChanged: (value) {
                                    selectedTime2 = value;
                                  },
                                  selectedTime: selectedTime2,
                                );
                              },
                            );
                          }
                          setState(() {
                            selectedTime = e;
                          });
                        },
                        child: Container(
                          margin: const EdgeInsets.symmetric(
                              horizontal: 5, vertical: 5),
                          padding: const EdgeInsets.symmetric(
                              horizontal: 10, vertical: 5),
                          decoration: BoxDecoration(
                              border: Border.all(
                                  color: Colors.grey.shade200, width: 1),
                              borderRadius:
                                  const BorderRadius.all(Radius.circular(10)),
                              color: selectedTime == e
                                  ? primaryColorCode
                                  : Colors.white),
                          child: SubTxtWidget(
                              e == "Custom" ? selectedTime2 ?? e : e,
                              color: selectedTime == e
                                  ? Colors.white
                                  : Colors.black),
                        ),
                      );
                    },
                  ).toList(),
                ),
                const SizedBox(
                  height: 10,
                ),
                HeaderTxtWidget('Date'),
                const SizedBox(
                  height: 5,
                ),
                Wrap(
                  children: [
                    InkWell(
                      onTap: () {
                        setState(() {
                          date = "Today";
                        });
                      },
                      child: Container(
                        margin: const EdgeInsets.symmetric(
                            horizontal: 5, vertical: 5),
                        padding: const EdgeInsets.symmetric(
                            horizontal: 10, vertical: 5),
                        decoration: BoxDecoration(
                            border: Border.all(
                              color: Colors.grey.shade200,
                              width: 1,
                            ),
                            borderRadius:
                                const BorderRadius.all(Radius.circular(10)),
                            color: date == "Today"
                                ? primaryColorCode
                                : Colors.white),
                        child: SubTxtWidget("Today",
                            color: date == "Today"
                                ? Colors.white
                                : btntextcolor2.withOpacity(1)),
                      ),
                    ),
                    InkWell(
                      onTap: () {
                        setState(() {
                          date = "Tomorrow";
                          // Disable "Later Today" in timeList when "Tomorrow" is selected
                          if (selectedTime == "Later Today") {
                            Tools.ShowErrorMessage(
                                'You cannot select "Later Today" when the date is not "Today".');
                            selectedTime = null; // Reset selectedTime
                          }
                        });
                      },
                      child: Container(
                        margin: const EdgeInsets.symmetric(
                            horizontal: 5, vertical: 5),
                        padding: const EdgeInsets.symmetric(
                            horizontal: 10, vertical: 5),
                        decoration: BoxDecoration(
                            border: Border.all(
                                color: Colors.grey.shade200, width: 1),
                            borderRadius:
                                const BorderRadius.all(Radius.circular(10)),
                            color: date == "Tomorrow"
                                ? btnColorCode
                                : Colors.white),
                        child: SubTxtWidget("Tomorrow",
                            color: date == "Tomorrow"
                                ? Colors.white
                                : btntextcolor2.withOpacity(1)),
                      ),
                    ),
                    InkWell(
                      onTap: () {
                        setState(() {
                          date = "This week";
                          // Disable "Later Today" in timeList when "This week" is selected
                          if (selectedTime == "Later Today") {
                            Tools.ShowErrorMessage(
                                'You cannot select "Later Today" when the date is not "Today".');
                            selectedTime = null; // Reset selectedTime
                          }
                        });
                      },
                      child: Container(
                        margin: const EdgeInsets.symmetric(
                            horizontal: 5, vertical: 5),
                        padding: const EdgeInsets.symmetric(
                            horizontal: 10, vertical: 5),
                        decoration: BoxDecoration(
                            border: Border.all(
                                color: Colors.grey.shade200, width: 1),
                            borderRadius:
                                const BorderRadius.all(Radius.circular(10)),
                            color: date == "This week"
                                ? primaryColorCode
                                : Colors.white),
                        child: SubTxtWidget("This week",
                            color: date == "This week"
                                ? Colors.white
                                : btntextcolor2.withOpacity(1)),
                      ),
                    ),
                    InkWell(
                      onTap: () {
                        showDatePicker(
                                context: context,
                                firstDate: DateTime.now(),
                                lastDate: DateTime(2030))
                            .then(
                          (value) {
                            setState(() {
                              selectedDate = value;
                              date = "Choose from calender";
                              // Disable "Later Today" in timeList when "Choose from calendar" is selected
                              if (selectedTime == "Later Today") {
                                Tools.ShowErrorMessage(
                                    'You cannot select "Later Today" when the date is not "Today".');
                                selectedTime = null; // Reset selectedTime
                              }
                            });
                          },
                        );
                      },
                      child: Container(
                        width: selectedDate != null ? 200 : 250,
                        margin: const EdgeInsets.symmetric(
                            horizontal: 5, vertical: 5),
                        padding: const EdgeInsets.symmetric(
                            horizontal: 10, vertical: 5),
                        decoration: BoxDecoration(
                            boxShadow: const [
                              BoxShadow(
                                color: Colors.black38, // Shadow color
                                offset: Offset(0,
                                    4), // Shadow offset (more pronounced at the bottom)
                                blurRadius: 1.0, // Shadow blur radius
                                spreadRadius: 0.5, // Shadow spread radius
                              ),
                            ],
                            border: Border.all(
                                color: Colors.grey.shade200, width: 1),
                            borderRadius:
                                const BorderRadius.all(Radius.circular(10)),
                            color: date == "Choose from calender"
                                ? primaryColorCode
                                : Colors.white),
                        child: Row(
                          children: [
                            SvgPicture.asset(
                              Assets.svgCalender,
                              color: selectedDate != null
                                  ? Colors.white
                                  : Colors.black,
                            ),
                            const SizedBox(
                              width: 10,
                            ),
                            Expanded(
                              child: SubTxtWidget(
                                  fontSize:
                                      14, // Replace with the value of var(--Font-size300)
                                  fontWeight: FontWeight
                                      .w400, // 400 corresponds to FontWeight.w400

                                  textAlign: TextAlign.right,
                                  selectedDate != null
                                      ? Tools.changeDateFormat(
                                          selectedDate.toString(), "dd-MM-yyyy")
                                      : "Choose from calender",
                                  color: date == "Choose from calender"
                                      ? Colors.white
                                      : Colors.black),
                            ),
                            const SizedBox(
                              width: 10,
                            ),
                            const SizedBox(
                              height: 16,
                              child: VerticalDivider(
                                color: Colors.grey,
                                width: 1,
                                thickness: 1,
                              ),
                            ),
                            const SizedBox(
                              width: 10,
                            ),
                            Icon(Icons.arrow_forward_ios_outlined,
                                size: 15,
                                color: selectedDate != null
                                    ? Colors.white
                                    : Colors.grey),
                          ],
                        ),
                      ),
                    ),
                  ],
                ),
                Container(
                  margin: const EdgeInsets.symmetric(vertical: 10),
                  padding: const EdgeInsets.all(10),
                  decoration: BoxDecoration(
                      borderRadius: const BorderRadius.all(Radius.circular(10)),
                      color: "#F6F7F9".toColor()),
                  child: Column(
                    children: [
                      const SizedBox(
                        height: 10,
                      ),
                      InputWidget(
                        title: 'Service Name',
                        hint: 'Service Name',
                        margin: const EdgeInsets.symmetric(vertical: 10),
                        controller: serviceNameCon,
                        validatorCallback: (value) =>
                            value == null || value.isEmpty
                                ? 'Please enter a service name'
                                : null,
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
                      ),
                    ],
                  ),
                ),
                const Padding(
                  padding: EdgeInsets.all(8.0),
                  child: Text(
                    "Select price range",
                    style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
                  ),
                ),
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 18.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          HeaderTxtWidget(
                              '\$${_currentRangeValues.start.round().toString()}'),
                          HeaderTxtWidget(
                              '\$${_currentRangeValues.end.round().toString()}'),
                        ],
                      ),
                      const SizedBox(
                          height: 10), // Space between headers and sliders
                      // Start Slider
                      Slider(
                        value: _currentRangeValues.start,
                        min: 5,
                        max: _currentRangeValues
                            .end, // Ensure the start slider does not exceed the end value
                        divisions: 2000,
                        label: _currentRangeValues.start.round().toString(),
                        onChanged: (double value) {
                          setState(() {
                            if (value + 50 < _currentRangeValues.end) {
                              _currentRangeValues = RangeValues(
                                value,
                                _currentRangeValues.end,
                              );
                            }
                          });
                        },
                      ),
                      // End Slider
                      Slider(
                        value: _currentRangeValues.end,
                        min: _currentRangeValues
                            .start, // Ensure the end slider does not go below the start value
                        max: 2000,
                        divisions: 2000,
                        label: _currentRangeValues.end.round().toString(),
                        onChanged: (double value) {
                          setState(() {
                            if (value - 50 > _currentRangeValues.start) {
                              _currentRangeValues = RangeValues(
                                _currentRangeValues.start,
                                value,
                              );
                            }
                          });
                        },
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 12),
                Row(
                  children: [
                    CustomSwitch(
                      value: _depositAccept,
                      onChanged: (newBool) {
                        setState(() {
                          _depositAccept = newBool;
                        });
                      },
                      showText: true,
                      positiveText: "Yes",
                      negativeText: " No",
                      borderColor: const Color(0x00363062).withOpacity(1),
                      innerColor: const Color(0x00363062).withOpacity(1),
                    ),
                    const SizedBox(
                      width: 10,
                    ),
                    HeaderTxtWidget(
                      "Will You Pay a Deposit?",
                      color: Colors.grey,
                    )
                  ],
                ),
                const SizedBox(height: 12),
                HeaderTxtWidget('Location'),
                Container(
                  margin: const EdgeInsets.symmetric(vertical: 10),
                  padding: const EdgeInsets.all(10),
                  decoration: BoxDecoration(
                      borderRadius: const BorderRadius.all(Radius.circular(10)),
                      color: "#F6F7F9".toColor()),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const SizedBox(
                        height: 10,
                      ),
                      SubTxtWidget('Radius'),
                      const SizedBox(
                        height: 5,
                      ),
                      DropdownButtonFormField<String>(
                        value: selectedRadius,
                        decoration: const InputDecoration(
                          border: OutlineInputBorder(
                            borderRadius: BorderRadius.all(Radius.circular(12)),
                            borderSide: BorderSide.none,
                          ),
                          fillColor: Colors.white,
                          filled: true,
                        ),
                        hint: SubTxtWidget("Select Radius"),
                        isExpanded: true,
                        items: [
                          // Generate specific options
                          for (int i = 1; i <= 200; i++)
                            if (i == 1 ||
                                i % 5 == 0) // Include 1 and every 5th number
                              DropdownMenuItem<String>(
                                value:
                                    "${i}mi", // Generate options like 1mi, 5mi, etc.
                                child: Text("$i mi"),
                              ),
                        ],
                        onChanged: (String? newValue) {
                          setState(() {
                            selectedRadius = newValue!;
                            mapController!.animateCamera(
                              CameraUpdate.newCameraPosition(
                                CameraPosition(
                                    target: _initialLocation, zoom: getZoom()),
                              ),
                            );
                            circles = {
                              Circle(
                                circleId: const CircleId("id"),
                                center: _initialLocation,
                                radius: double.parse(
                                        selectedRadius.replaceAll("mi", "")) *
                                    1609.34, // Convert miles to meters
                                fillColor: Colors.blue.shade50.withOpacity(0.8),
                                strokeColor: Colors.blue,
                                strokeWidth: 1,
                              ),
                            };
                            setState(() {});
                          });
                        },
                        validator: (value) =>
                            value == null ? 'Please select a radius' : null,
                      ),
                      const SizedBox(
                        height: 10,
                      ),
                      SubTxtWidget('Search location'),
                      const SizedBox(
                        height: 5,
                      ),
                      GooglePlacesAutoCompleteTextFormField(
                        textEditingController: _locationController,
                        googleAPIKey:
                            "AIzaSyAiF2WXvYTrnybDDn4EwvOL9RJAwJ_4Bi4", //GlobalConfiguration().getValue("apiKey"),
                        debounceTime: 400,
                        isLatLngRequired: true,
                        decoration: const InputDecoration(
                            border: OutlineInputBorder(
                                borderRadius:
                                    BorderRadius.all(Radius.circular(12)),
                                borderSide: BorderSide.none),
                            suffixIcon: Icon(Icons.search),
                            fillColor: Colors.white,
                            filled: true),
                        getPlaceDetailWithLatLng: (prediction) {
                          print(
                              "Coordinates: (${prediction.lat},${prediction.lng})");
                          _initialLocation = LatLng(
                              double.parse(prediction.lat!),
                              double.parse(prediction.lng!));
                          mapController!.animateCamera(
                              CameraUpdate.newCameraPosition(CameraPosition(
                                  target: _initialLocation, zoom: getZoom())));
                          circles = {
                            Circle(
                              circleId: const CircleId("id"),
                              center: _initialLocation,
                              radius: double.parse(
                                      selectedRadius.replaceAll("mi", "")) *
                                  1000,
                              fillColor: Colors.blue.shade50.withOpacity(0.8),
                              strokeColor: Colors.blue,
                              strokeWidth: 1,
                            )
                          };
                          setState(() {});
                        },
                        itmClick: (prediction) {
                          _locationController.text = prediction.description!;
                          _locationController.selection =
                              TextSelection.fromPosition(TextPosition(
                                  offset: prediction.description!.length));
                        },
                        validator: (p0) {
                          if (p0!.isEmpty) {
                            return "Please search location";
                          }
                          return null;
                        },
                      ),
                      Container(
                        height: 300,
                        margin: const EdgeInsets.symmetric(vertical: 10),
                        child: ClipRRect(
                            borderRadius:
                                const BorderRadius.all(Radius.circular(10)),
                            child: GoogleMap(
                              initialCameraPosition: CameraPosition(
                                  target: _initialLocation, zoom: 13),
                              scrollGesturesEnabled: false,
                              compassEnabled: false,
                              onMapCreated: (controller) {
                                mapController = controller;
                              },
                              circles: circles,
                            )),
                      ),
                      const SizedBox(
                        height: 5,
                      ),
                    ],
                  ),
                ),
                const SizedBox(
                  height: 20,
                ),
                Center(
                  child: ButtonPrimaryWidget(
                    color: const Color(0x005f60b9).withOpacity(1),
                    "SAVE",
                    onTap: _isLoading ? null : _saveCustomerServiceModel,
                    isLoading: _isLoading,
                  ),
                ),
                const SizedBox(
                  height: 20,
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

double getZoom() {
  // Parse the radius from the selected value
  int radius = int.parse(selectedRadius.replaceAll("mi", ""));

  // Calculate zoom level dynamically based on radius
  if (radius <= 5) return 14;           // Close-up view for small radii
  if (radius <= 10) return 12;          // Zoom out for medium range
  if (radius <= 25) return 11;          // Larger area
  if (radius <= 50) return 10;          // Farther zoom
  if (radius <= 100) return 8;          // Even farther zoom
  if (radius <= 200) return 6;          // Wide-area zoom

  return 5;
}
}
