import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:geocoding/geocoding.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:google_places_autocomplete_text_field/google_places_autocomplete_text_field.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/states/customer_service_state.dart';
import 'package:groom/utils/colors.dart';
import 'package:groom/widgets/button_primary_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import 'dart:ui' as ui;
import '../../../data_models/provider_service_model.dart';
import '../../../data_models/user_model.dart';
import '../../../firebase/provider_service_firebase.dart';
import '../../../generated/assets.dart';
import '../../../states/map_controller.dart';
import '../../../states/provider_service_state.dart';
import '../services/provider_details_screen.dart';
import '../services/service_details_screen.dart';
import 'search_controller.dart' as se;
class SearchMapScreen extends StatefulWidget {
  const SearchMapScreen({super.key});

  @override
  State<SearchMapScreen> createState() => _GoogleMapScreenState();
}

class _GoogleMapScreenState extends State<SearchMapScreen> {
  ProviderServiceState providerServiceState = Get.put(ProviderServiceState());
  LatLng _initialLocation = defalultLatLng;
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  CustomerServiceState similarServiceState = Get.put(CustomerServiceState());
  final TextEditingController _locationController = TextEditingController();
  Placemark? placemark;
  String filterType="Sort by";
  String selectedFilter="All";
  String filterSortBy="Recommended";
  List<UserModel> allProviders=[];
  final con = Get.put(se.SearchController());
  @override
  void initState() {
    super.initState();
    _fetchUserLocation();
  }

  Future<void> initLocation() async {
    List<Placemark> placemarks = await placemarkFromCoordinates(_initialLocation.latitude, _initialLocation.longitude);
    setState(() {
      placemark=placemarks.first;
    });
  }

  Future<void> _fetchUserLocation() async {
    _initialLocation = auth.value.location??defalultLatLng;
    initLocation();
  }

  @override
  Widget build(BuildContext context) {
    Size size=MediaQuery.of(context).size;
    return Scaffold(
      resizeToAvoidBottomInset: false,
      appBar: AppBar(
        backgroundColor: Colors.white,
        title: HeaderTxtWidget('Find a barber nearby'),
        elevation: 0,
        iconTheme: IconThemeData(
          color: primaryColorCode,
        ),
      ),
      body: Container(
        height: size.height,
        width: size.width,
        child: Stack(
          children: [
            Positioned(
              top: 0,
              right: 0,
              left: 0,
              bottom: 0,
              child: GoogleMap(
                initialCameraPosition:
                CameraPosition(target: _initialLocation, zoom: 13),
                mapToolbarEnabled: true,
                compassEnabled: false,
                markers: providerServiceState.markers.value,
                onMapCreated: (controller) {
                  providerServiceState.mapController = controller;
                },
              ),
            ),
            Positioned(
              top: 20,
              right: 20,
              left: 20,
              child: GooglePlacesAutoCompleteTextFormField(
                textEditingController: _locationController,
                googleAPIKey: "AIzaSyAiF2WXvYTrnybDDn4EwvOL9RJAwJ_4Bi4",
                countries: ["IN","US"],
                debounceTime: 400,
                isLatLngRequired: true,
                decoration: const InputDecoration(
                    border: OutlineInputBorder(
                        borderRadius: BorderRadius.all(Radius.circular(12)),
                        borderSide: BorderSide.none),
                    prefixIcon: Icon(Icons.search),
                    fillColor: Colors.white,
                    filled: true),
                getPlaceDetailWithLatLng: (prediction) {
                  _initialLocation = LatLng(double.parse(prediction.lat!),
                      double.parse(prediction.lng!));
                  providerServiceState.mapController!.animateCamera(
                      CameraUpdate.newCameraPosition(
                          CameraPosition(target: _initialLocation, zoom: 12)));
                  initLocation();
                },
                itmClick: (prediction) {
                  _locationController.text = prediction.description!;
                  _locationController.selection = TextSelection.fromPosition(
                      TextPosition(offset: prediction.description!.length));
                },
                validator: (p0) {
                  if (p0!.isEmpty) {
                    return "Please search location";
                  }
                  return null;
                },
              ),
            ),

            Positioned(bottom: 0,
              right: 0,
              left: 0,child: Container(
                decoration: const BoxDecoration(
                    borderRadius: BorderRadius.only(topLeft: Radius.circular(20),topRight: Radius.circular(20)),
                    color: Colors.white
                ),
                padding: const EdgeInsets.symmetric(vertical: 10),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    if(placemark!=null)
                      Container(
                          padding: const EdgeInsets.all(10),
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Row(
                                children: [
                                  Expanded(child: Row(
                                    crossAxisAlignment: CrossAxisAlignment.start,
                                    children: [
                                      SvgPicture.asset(Assets.svgLocation,color: primaryColorCode,),
                                      const SizedBox(width: 5,),
                                      Expanded(child: SubTxtWidget(placemark!.name!),),
                                    ],
                                  ),),
                                  InkWell(
                                    child: Container(
                                      margin: const EdgeInsets.only(right: 10),
                                      padding: const EdgeInsets.all(10),
                                      decoration: BoxDecoration(
                                          color: primaryColorCode,
                                          borderRadius: const BorderRadius.all(Radius.circular(10))
                                      ),
                                      child: Row(
                                        children: [
                                          SvgPicture.asset(Assets.svgFilter,width: 12,),
                                          const SizedBox(width: 5,),
                                          SubTxtWidget("Filter",color: Colors.white,fontSize: 10,)
                                        ],
                                      ),
                                    ),
                                    onTap: () {
                                      showModalBottomSheet(context: context, builder: (context) => Container(
                                        height: MediaQuery.of(context).size.height*0.8,
                                        decoration: const BoxDecoration(
                                            borderRadius: BorderRadius.only(topLeft: Radius.circular(20),topRight: Radius.circular(20)),
                                            color: Colors.white
                                        ),
                                        child: _filter(),
                                      ),
                                          isScrollControlled: true);
                                    },
                                  ),
                                ],
                              ),
                              SubTxtWidget('${placemark!.street}, ${placemark!.locality}, ${placemark!.subAdministrativeArea}')
                            ],
                          )
                      ),
                    Divider(
                      color: Colors.grey.shade100,
                    ),
                    SizedBox(
                      height: 140,
                      child: ListView.builder(itemBuilder: (context, index) {
                        var data = allProviders[index];
                        return InkWell(
                          child: Container(
                            width: 300,
                            decoration: BoxDecoration(
                              color: Colors.grey.shade50,
                              borderRadius: BorderRadius.all(Radius.circular(15))
                            ),
                            margin: const EdgeInsets.only(top: 15),
                            child: Row(
                              children: [
                                ClipRRect(
                                  borderRadius: BorderRadius.circular(9),
                                  child: SizedBox(
                                    width: 100,
                                    height: 100,
                                    child: NetworkImageWidget(
                                      url: data.providerUserModel!.providerImages!.first,
                                      fit: BoxFit.fill,
                                    ),
                                  ),
                                ),
                                const SizedBox(
                                  width: 10,
                                ),
                                Expanded(
                                    child: Column(
                                      crossAxisAlignment: CrossAxisAlignment.start,
                                      children: [
                                        HeaderTxtWidget(data.providerUserModel!.providerType !=
                                            "Independent"
                                            ? data.providerUserModel!.salonTitle!
                                            : data.fullName),
                                        Row(
                                          children: [
                                            SvgPicture.asset(Assets.svgLocationGray),
                                            const SizedBox(
                                              width: 5,
                                            ),
                                            SubTxtWidget(
                                              isGuest.value
                                                  ? 'Login to view'
                                                  : '${data.providerUserModel!.addressLine.toMiniAddress(limit: 20)} (${data.distance})',
                                              color: "#8683A1".toColor(),
                                            ),
                                          ],
                                        ),
                                        Row(
                                          crossAxisAlignment: CrossAxisAlignment.start,
                                          mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                          children: [
                                            Row(
                                              children: [
                                                SvgPicture.asset(Assets.svgStar),
                                                const SizedBox(
                                                  width: 5,
                                                ),
                                                SubTxtWidget(
                                                  '${data.providerUserModel!.overallRating}',
                                                  color: "#8683A1".toColor(),
                                                ),
                                              ],
                                            ),
                                            Container(
                                              margin: const EdgeInsets.only(top: 20),
                                              padding: const EdgeInsets.symmetric(
                                                  horizontal: 10, vertical: 8),
                                              decoration: BoxDecoration(
                                                  borderRadius: const BorderRadius.only(
                                                      topLeft: Radius.circular(15),
                                                      bottomRight: Radius.circular(15)),
                                                  color: primaryColorCode),
                                              child: HeaderTxtWidget(
                                                isGuest.value ? "Login" : 'Provider Details',
                                                color: Colors.white,
                                                fontSize: 13,
                                              ),
                                            )
                                          ],
                                        ),
                                      ],
                                    ))
                              ],
                            ),
                          ),
                          onTap: () {
                            if (isGuest.value) {
                              Get.toNamed('/login');
                            } else {
                              Get.to(() => ProviderDetailsScreen(
                                provider: data,
                              ));
                            }
                          },
                        );
                      },itemCount: allProviders.length,scrollDirection: Axis.horizontal,
                        shrinkWrap: true,
                      padding: const EdgeInsets.symmetric(horizontal: 10),),
                    )
                  ],
                ),
              ),)
          ],
        ),
      ),
    );
  }
  Widget _filter(){
    return StatefulBuilder(builder: (context, setState) {
      return Container(
        padding: const EdgeInsets.symmetric(vertical: 15,horizontal: 20),
        child: Column(
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Expanded(child: InkWell(
                  onTap: () {
                    setState((){
                      filterType="Sort by";
                    });

                  },
                  child: Container(
                    alignment: AlignmentDirectional.center,
                    padding: const EdgeInsets.symmetric(vertical: 5),
                    margin: const EdgeInsets.symmetric(horizontal: 10),
                    decoration: BoxDecoration(
                        border: Border(bottom: BorderSide(
                            color:filterType=="Sort by"?primaryColorCode:Colors.grey.shade50
                        ))
                    ),
                    child: SubTxtWidget("Sort by"),
                  ),
                )),
                Expanded(child: InkWell(
                  onTap: () {
                    setState((){
                      filterType="Price";
                    });

                  },
                  child: Container(
                    alignment: AlignmentDirectional.center,
                    padding: const EdgeInsets.symmetric(vertical: 5),
                    margin: const EdgeInsets.symmetric(horizontal: 10),
                    decoration: BoxDecoration(
                        border: Border(bottom: BorderSide(
                            color:filterType=="Price"?primaryColorCode:Colors.grey.shade50
                        ))
                    ),
                    child: SubTxtWidget("Price"),
                  ),
                )),
              ],
            ),
            if(filterType=="Sort by")
              Column(children: [
                const SizedBox(height: 20,),
                InkWell(
                  onTap: (){
                   setState((){
                     con.filterSortBy.value="Recommended";
                   });
                  },
                  child: Container(
                    padding: const EdgeInsets.all(15),
                    margin: const EdgeInsets.symmetric(vertical: 10),
                    decoration: BoxDecoration(
                        borderRadius: const BorderRadius.all(Radius.circular(10)),
                        color: Colors.grey.shade100
                    ),
                    child: Row(
                      children: [
                        Icon(Icons.bookmark,color: Colors.grey.shade400,),
                        const SizedBox(width: 5,),
                        Expanded(child: SubTxtWidget('Recommended')),
                        Icon(Icons.check_circle,color: con.filterSortBy.value=="Recommended"?primaryColorCode:Colors.grey.shade400,),
                      ],
                    ),
                  ),
                ),
                InkWell(
                  onTap: (){
                    setState((){
                      con.filterSortBy.value="Most Popular";
                    });
                  },
                  child: Container(
                    padding: const EdgeInsets.all(15),
                    margin: const EdgeInsets.symmetric(vertical: 10),
                    decoration: BoxDecoration(
                        borderRadius: const BorderRadius.all(Radius.circular(10)),
                        color: Colors.grey.shade100
                    ),
                    child: Row(
                      children: [
                        Icon(Icons.recommend,color: Colors.grey.shade400,),
                        const SizedBox(width: 5,),
                        Expanded(child: SubTxtWidget('Most Popular')),
                        Icon(Icons.check_circle,color:con.filterSortBy.value=="Most Popular"?primaryColorCode:Colors.grey.shade400,),
                      ],
                    ),
                  ),
                ),
                InkWell(
                  onTap: (){
                    setState((){
                      con.filterSortBy.value="Distance";
                    });
                  },
                  child: Container(
                    padding: const EdgeInsets.all(15),
                    margin: const EdgeInsets.symmetric(vertical: 10),
                    decoration: BoxDecoration(
                        borderRadius: const BorderRadius.all(Radius.circular(10)),
                        color: Colors.grey.shade100
                    ),
                    child: Row(
                      children: [
                        Icon(Icons.social_distance,color: Colors.grey.shade400,),
                        const SizedBox(width: 5,),
                        Expanded(child: SubTxtWidget('Distance')),
                        Icon(Icons.check_circle,color:con.filterSortBy.value=="Distance"?primaryColorCode:Colors.grey.shade400,),
                      ],
                    ),
                  ),
                ),
              ],),
            if(filterType=="Price")
              Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const SizedBox(height: 20,),
                  HeaderTxtWidget('Maximum price fee'),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      HeaderTxtWidget("\$50.00"),
                      HeaderTxtWidget("\$${con.currentRangeValues.value.end.round()}.00"),
                    ],
                  ),
                  const SizedBox(height: 20,),
                  RangeSlider(
                    values: con.currentRangeValues.value,
                    min: 50,
                    max: 2000,
                    divisions: 2000,  // Divisions set to 2000 for integer steps
                    activeColor: primaryColorCode,
                    labels: RangeLabels(
                      "\$50.00",
                      "\$${con.currentRangeValues.value.end.round()}.00",
                    ),
                    onChanged: (RangeValues values) {
                      if (values.start <= values.end) {
                        setState((){
                          con.currentRangeValues.value = RangeValues(
                            50,
                            values.end.roundToDouble(),
                          );
                        });
                      }
                    },
                  ),
                ],),
            ButtonPrimaryWidget("filter",onTap: () {
              filterList();
              Navigator.pop(context);
            },color: '#641BB4'.toColor(),marginVertical: 20,marginHorizontal: 20,)
          ],
        ),
      );
    },);
  }
  void filterList(){
     allProviders = providerServiceState.allProviders;
      LatLng latLng = _initialLocation;
     for (var element in allProviders) {
       double distance=Geolocator.distanceBetween(
           element.providerUserModel!.location!.latitude,
           element.providerUserModel!.location!.latitude,
           latLng.latitude,
           latLng.longitude) /
           1000;
       if(radius>=distance){
         element.distance="${distance.toStringAsFixed(2)} mi";
       }else{
         allProviders.remove(element);
       }
     }
     if(filterType=="Sort by"){
       if(con.filterSortBy.value=="Distance"){
         allProviders.sort((a, b) => changeDisToDouble(a.distance!).compareTo(changeDisToDouble(b.distance!)),);
       }
     }
     if(filterType=="Price"){
       allProviders = allProviders
           .where((element) =>
       double.parse(element.providerUserModel!.basePrice.toString()) >
           con.currentRangeValues.value.start)
           .toList();
       allProviders = allProviders
           .where((element) =>
       double.parse(element.providerUserModel!.basePrice.toString()) <
           con.currentRangeValues.value.end)
           .toList();
     }
  }
  double changeDisToDouble(String d){
    return double.parse(d.replaceAll("mi", "").trim());
  }
}
