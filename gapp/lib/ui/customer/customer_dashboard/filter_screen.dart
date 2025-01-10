import 'dart:io';
import 'package:flutter/material.dart';
import 'package:flutter_rating_bar/flutter_rating_bar.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:google_places_autocomplete_text_field/google_places_autocomplete_text_field.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import '../../../Utils/tools.dart';
import '../../../generated/assets.dart';
import '../../../repo/setting_repo.dart';
import '../../../widgets/sub_txt_widget.dart';
import '../../../widgets/time_widget.dart';

class FilterScreen extends StatefulWidget {
  Function() filter;
   FilterScreen({super.key, required this.filter});

  @override
  State<FilterScreen> createState() =>
      _ScreenState();
}

class _ScreenState extends State<FilterScreen> {
  String? selectedService;
  DateTime? _dateTime;
  double rating=0;
  final _formKey = GlobalKey<FormState>();
  final TextEditingController _locationController = TextEditingController();
  RangeValues _currentRangeValues = const RangeValues(5, 2000);
  DateTime? selectedDate = DateTime.now();
  String? selectedTime;
  String? selectedTime2;
  String? date;
  List<String>timeList=[
    "Custom","Now/ASAP","Later Today","Early Morning","Late Night",
  ];
  LatLng _initialLocation =auth.value.location?? defalultLatLng;
  GoogleMapController? mapController;
  Set<Marker> marker=Set();

  @override
  void initState() {
    super.initState();
    if(filter.value.isNotEmpty){
      if(filter.value.containsKey('min_price')){
        _currentRangeValues= RangeValues(filter.value['min_price'], filter.value['max_price']);
      }if(filter.value.containsKey('rating')){
        rating= filter.value['rating'];
      }if(filter.value.containsKey('location')){
        _initialLocation= filter.value['location'];
        _locationController.text=filter.value['address'];
      }
    }
  }


  void getDateTime(){
    DateTime now=DateTime.now();
    if(date=="Today"){
      _dateTime=now;
    }if(date=="Tomorrow"){
      _dateTime=DateTime(now.year,now.month,now.day+1);
    }
    if(date=="This week"){
      int week=7-now.weekday;
      print('week==>$week');
      _dateTime=DateTime(now.year,now.month,now.day+week);
    }
    print("_dateTime==>${_dateTime.toString()}");
  }
  Future<void> applyFilter() async {
    Map<String,dynamic>map={};
    map['min_price']=_currentRangeValues.start;
    map['max_price']=_currentRangeValues.end;
    map['rating']=rating;
    if(_locationController.text.isNotEmpty){
    map['location']=_initialLocation;
    map['address']=_locationController.text;
    }
    filter.value=map;
    Navigator.pop(context);
    widget.filter.call();
  }



  @override
  Widget build(BuildContext context) {
    Size size = MediaQuery.of(context).size;
    return Scaffold(
      backgroundColor: Colors.transparent,
      body: Column(
        children: [
          Container(
            padding: const EdgeInsets.symmetric(vertical: 15,horizontal: 15),
            decoration: BoxDecoration(
                borderRadius: const BorderRadius.only(topRight: Radius.circular(20),topLeft: Radius.circular(20)),
                color: Colors.grey.shade200
            ),
            child: Row(
              children: [
                Image.asset(Assets.imgLoginIcon),
                const SizedBox(width: 10,),
                Expanded(child: HeaderTxtWidget("Filter",)),
                IconButton(onPressed: () {
                  Navigator.pop(context);
                }, icon: const Icon(Icons.cancel_outlined))
              ],
            ),
          ),
          Expanded(child: Container(
            padding: const EdgeInsets.symmetric(horizontal: 20),
            child: SingleChildScrollView(
              child: Form(
                key: _formKey,
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  mainAxisAlignment: MainAxisAlignment.start,
                  children: [
                    const SizedBox(height: 15,),
                    HeaderTxtWidget('Time & Date'),
                    const SizedBox(height: 5,),
                    Wrap(
                      children:[
                        InkWell(
                          onTap: () {
                            setState(() {
                              date="Today";
                            });
                          },
                          child: Container(
                            margin: const EdgeInsets.symmetric(horizontal: 5,vertical: 5),
                            padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 5),
                            decoration: BoxDecoration(
                                border: Border.all(color: Colors.grey.shade200,width: 1),
                                borderRadius: const BorderRadius.all(Radius.circular(10)),
                                color:date=="Today"?primaryColorCode:Colors.white
                            ),
                            child: SubTxtWidget("Today",color:date=="Today"?Colors.white:Colors.black),
                          ),
                        ),
                        InkWell(
                          onTap: () {
                            setState(() {
                              date="Tomorrow";
                            });
                          },
                          child: Container(
                            margin: const EdgeInsets.symmetric(horizontal: 5,vertical: 5),
                            padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 5),
                            decoration: BoxDecoration(
                                border: Border.all(color: Colors.grey.shade200,width: 1),
                                borderRadius: const BorderRadius.all(Radius.circular(10)),
                                color:date=="Tomorrow"?primaryColorCode:Colors.white
                            ),
                            child: SubTxtWidget("Tomorrow",color:date=="Tomorrow"?Colors.white:Colors.black),
                          ),
                        ),
                        InkWell(
                          onTap: () {
                            setState(() {
                              date="This week";
                            });
                          },
                          child: Container(
                            margin: const EdgeInsets.symmetric(horizontal: 5,vertical: 5),
                            padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 5),
                            decoration: BoxDecoration(
                                border: Border.all(color: Colors.grey.shade200,width: 1),
                                borderRadius: const BorderRadius.all(Radius.circular(10)),
                                color:date=="This week"?primaryColorCode:Colors.white
                            ),
                            child: SubTxtWidget("This week",color:date=="This week"?Colors.white:Colors.black),
                          ),
                        ),
                        InkWell(
                          onTap: () {
                            showDatePicker(context: context, firstDate: DateTime.now(), lastDate: DateTime(2030)).then((value) {
                              setState(() {
                                _dateTime=value;
                                date="Choose from calender";
                              });
                            },);

                          },
                          child: Container(
                            width: _dateTime!=null?170:250,
                            margin: const EdgeInsets.symmetric(horizontal: 5,vertical: 5),
                            padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 5),
                            decoration: BoxDecoration(
                                border: Border.all(color: Colors.grey.shade200,width: 1),
                                borderRadius: const BorderRadius.all(Radius.circular(10)),
                                color:date=="Choose from calender"?primaryColorCode:Colors.white
                            ),
                            child: Row(
                              children: [
                                SvgPicture.asset(Assets.svgCalender,color: _dateTime!=null?Colors.white:Colors.blue,),
                                const SizedBox(width: 10,),
                                SubTxtWidget(_dateTime!=null?Tools.changeDateFormat(_dateTime.toString(), "dd-MM-yyyy"):"Choose from calender",color:date=="Choose from calender"?Colors.white:Colors.black),
                                const SizedBox(width: 10,),
                                Icon(Icons.arrow_forward_ios_outlined,size: 10,color: _dateTime!=null?Colors.white:Colors.blue),
                              ],
                            ),
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 5,),
                    Wrap(
                      children: timeList.map((e) {
                        return InkWell(
                          onTap: () {
                            if(e=="Custom"){
                              showDialog(context: context, builder: (context) {
                                return TimeWidget(onChanged: (value) {
                                  setState(() {
                                    selectedTime2=value;
                                  });
                                },selectedTime: selectedTime2,);
                              },);
                            }
                            setState(() {
                              selectedTime=e;
                            });
                          },
                          child: Container(
                            margin: const EdgeInsets.symmetric(horizontal: 5,vertical: 5),
                            padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 5),
                            decoration: BoxDecoration(
                                border: Border.all(color: Colors.grey.shade200,width: 1),
                                borderRadius: const BorderRadius.all(Radius.circular(10)),
                                color:selectedTime==e?primaryColorCode:Colors.white
                            ),
                            child: SubTxtWidget(e=="Custom"?selectedTime2??e:e,color:selectedTime==e?Colors.white:Colors.black),
                          ),
                        );
                      },).toList(),
                    ),
                    const SizedBox(height: 10,),
                    const Padding(
                      padding: EdgeInsets.all(8.0),
                      child: Text(
                        "Select price range",
                        style: TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
                      ),
                    ),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        HeaderTxtWidget("\$${_currentRangeValues.start.round()}.00"),
                        HeaderTxtWidget("\$${_currentRangeValues.end.round()}.00"),
                      ],
                    ),
                    RangeSlider(
                      values: _currentRangeValues,
                      min: 5,
                      max: 2000,
                      divisions: 2000,  // Divisions set to 2000 for integer steps
                      activeColor: primaryColorCode,
                      labels: RangeLabels(
                        "\$${_currentRangeValues.start.round()}.00",
                        "\$${_currentRangeValues.end.round()}.00",
                      ),
                      onChanged: (RangeValues values) {
                        if (values.start+50 <= values.end && values.end > values.start) {
                          setState(() {
                            _currentRangeValues = RangeValues(
                                values.start.roundToDouble(),
                              values.end.roundToDouble(),
                            );
                          });
                        }
                      },
                    ),
                    const SizedBox(height: 12),
                    HeaderTxtWidget('Rating Barber'),
                    const SizedBox(height: 12),
                    RatingBar.builder(itemBuilder: (context, index) {
                      return const Icon(Icons.star,color: Colors.amber,);
                    }, onRatingUpdate: (value) {
                      rating=value;
                    },
                      minRating: 0,
                      maxRating: 5,
                      updateOnDrag: true,
                      initialRating: rating,
                      itemCount: 5,
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
                          SubTxtWidget('Search location'),
                          const SizedBox(
                            height: 5,
                          ),
                          GooglePlacesAutoCompleteTextFormField(
                            textEditingController: _locationController,
                            googleAPIKey: "AIzaSyAiF2WXvYTrnybDDn4EwvOL9RJAwJ_4Bi4",//GlobalConfiguration().getValue("apiKey"),
                            debounceTime: 400,
                            isLatLngRequired: true,
                            decoration:const InputDecoration(
                                border: OutlineInputBorder(
                                    borderRadius:
                                    BorderRadius.all(Radius.circular(12)),
                                    borderSide: BorderSide.none
                                ),
                                suffixIcon: Icon(Icons.search),
                                fillColor: Colors.white,
                                filled: true),
                            getPlaceDetailWithLatLng: (prediction) {
                              print("Coordinates: (${prediction.lat},${prediction.lng})");
                              _initialLocation=LatLng(double.parse(prediction.lat!), double.parse(prediction.lng!));
                              mapController!.animateCamera(CameraUpdate.newCameraPosition(CameraPosition(target: _initialLocation,zoom: 12)));
                              marker={
                                Marker(markerId: const MarkerId("marker"),
                                    icon: BitmapDescriptor.defaultMarker,
                                    position: _initialLocation,
                                    infoWindow: InfoWindow(
                                        title: _locationController.text,
                                    )
                                )
                              };
                              setState(() {
                              });

                            },
                            itmClick: (prediction) {
                              _locationController.text = prediction.description!;
                              _locationController.selection = TextSelection.fromPosition(TextPosition(offset: prediction.description!.length));
                            },
                            validator: (p0) {
                              if(p0!.isEmpty){
                                return "Please select location";
                              }
                              return null;
                            },
                          ),
                          Container(
                            height: 300,
                            margin: const EdgeInsets.symmetric(vertical: 10),
                            child: ClipRRect(
                                borderRadius: const BorderRadius.all(Radius.circular(10)),
                                child: GoogleMap(
                                  initialCameraPosition: CameraPosition(target: _initialLocation, zoom: 13),
                                  scrollGesturesEnabled: false,
                                  compassEnabled: false,
                                  onMapCreated: (controller) {
                                    mapController=controller;
                                  },
                                  markers: marker,
                                )
                            ),
                          ),
                          const SizedBox(
                            height: 5,
                          ),

                        ],
                      ),
                    ),
                    const SizedBox(height: 20,),
                    Center(
                      child: ButtonPrimaryWidget("Apply",onTap: applyFilter),
                    ),
                    const SizedBox(height: 20,),
                  ],
                ),
              ),
            ),
          ))
        ],
      ),
    );
  }

}
