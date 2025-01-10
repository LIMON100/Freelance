import 'package:carousel_slider/carousel_slider.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:geolocator/geolocator.dart';
import 'package:get/get.dart';
import 'package:googleapis/firestore/v1.dart';
import 'package:groom/data_models/user_model.dart';
import 'package:groom/screens/newpayscree.dart';
import 'package:groom/ui/customer/customer_dashboard/widgets/ProviderSearchonMap.dart';
import 'package:groom/ui/customer/customer_dashboard/widgets/nearby_provider_widget.dart';
import 'package:groom/states/provider_service_state.dart';
import 'package:groom/states/user_state.dart';
import 'package:groom/widgets/loading_widget.dart';
import 'package:groom/widgets/network_image_widget.dart';
import 'package:groom/widgets/sub_txt_widget.dart';
import '../../../firebase/geomaps_firebase.dart';
import '../../../firebase/messaging_firebase.dart';
import '../../../generated/assets.dart';
import '../../../membership/purchase_api.dart';
import '../../../repo/setting_repo.dart';
import '../../../utils/colors.dart';
import '../../../widgets/app_bar_widget.dart';
import '../../../widgets/header_txt_widget.dart';
import '../services/provider_details_screen.dart';
import 'widgets/all_provider_widget.dart';
import 'widgets/all_service_widget.dart';
import 'widgets/nearby_service_widget.dart';
import '../../../widgets/search_input_widget.dart';
import '../../../firebase/user_firebase.dart';
import 'filter_screen.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
class CustomerHomeScreen extends StatefulWidget {
  const CustomerHomeScreen({super.key});

  @override
  State<CustomerHomeScreen> createState() => _CustomerHomeScreenState();
}

class _CustomerHomeScreenState extends State<CustomerHomeScreen> {
  ProviderServiceState providerServiceState = Get.put(ProviderServiceState());
  TextEditingController searchField = TextEditingController();
  UserFirebase userFirebase = UserFirebase();
  UserStateController userStateController = Get.put(UserStateController());
  RxInt selectedIndex = 0.obs;

  @override
  void initState() {
    super.initState();
    FirebaseApi().initNotifications();
    FirebaseApi().initInfo();
    GeoMapsFirebase().checkAndFetchLocation();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: CustomScrollView(
        slivers: [
          SliverAppBar(
              backgroundColor: Colors.white,
              pinned: false,
              automaticallyImplyLeading: false,
              floating: true,
              snap: true,
              toolbarHeight: 110,
              title: AppBarWidget(),
              surfaceTintColor: Colors.white,
              bottom: PreferredSize(
                preferredSize: const Size.fromHeight(70),
                child: Row(
                  children: [
                    Expanded(
                        child: SearchInputWidget(
                      hint: "Search barber’s, haircut service",
                      controller: searchField,
                      onDone: (p0) {
                        Get.toNamed('/search', arguments: {
                          "q": searchField.text,
                          "clearFilter": true
                        });
                      },
                    )),
                    InkWell(
                      child: Container(
                        margin: const EdgeInsets.only(right: 10),
                        padding: const EdgeInsets.all(14),
                        decoration: BoxDecoration(
                            color: primaryColorCode,
                            borderRadius:
                                const BorderRadius.all(Radius.circular(10))),
                        child: SvgPicture.asset(Assets.svgFilter),
                      ),
                      onTap: () {
                        showModalBottomSheet(
                            context: context,
                            builder: (context) => Container(
                                  height:
                                      MediaQuery.of(context).size.height * 0.85,
                                  decoration: const BoxDecoration(
                                      borderRadius: BorderRadius.only(
                                          topLeft: Radius.circular(20),
                                          topRight: Radius.circular(20)),
                                      color: Colors.white),
                                  child: FilterScreen(
                                    filter: () {
                                      setState(() {});
                                    },
                                  ),
                                ),
                            isScrollControlled: true);
                      },
                    )
                  ],
                ),
              )),
          SliverToBoxAdapter(
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 15, vertical: 10),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  _slider(),
                  const SizedBox(
                    height: 10,
                  ),
                  NearbyProviderWidget(),
                  const SizedBox(
                    height: 20,
                  ),
                  AllServiceWidget(),
                  const SizedBox(
                    height: 20,
                  ),
                  AllProviderWidget(),
                  const SizedBox(
                    height: 20,
                  ),
                  NearbyServiceWidget(),
                  SubTxtWidget(
                    'Map Search',

                    // Ensure the font is included in pubspec.yaml
                    fontSize: 18.0,
                    fontWeight: FontWeight.w700, // 700 corresponds to bold

                    color: Colors.black,
                  ),
                  const SizedBox(
                    height: 10,
                  ),
                  Obx(() => _mapWidget(),),
                  const SizedBox(
                    height: 20,
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _slider() {
    return SizedBox(
      height: 250,
      child: Stack(
        children: [
          CarouselSlider(
            items: [Assets.imgBanner, Assets.imgBanner2].map((e) {
              return ClipRRect(
                borderRadius: const BorderRadius.all(Radius.circular(10)),
                child: InkWell(
                  onTap: () {
                    if (providerServiceState.allProviders.isNotEmpty) {
                      if (isGuest.value) {
                        Get.toNamed('/login');
                      } else {
                        Get.to(() => ProviderDetailsScreen(
                              provider: providerServiceState.allProviders.first,
                            ));
                      }
                    }
                  },
                  child: Image.asset(
                    e,
                    fit: BoxFit.contain,
                    width: 1000.0,
                  ),
                ),
              );
            }).toList(),
            options: CarouselOptions(
              aspectRatio: 1,
              height: 250,
              viewportFraction: 1,
              initialPage: 0,
              enableInfiniteScroll: true,
              reverse: false,
              autoPlay: true,
              autoPlayInterval: const Duration(seconds: 3),
              autoPlayAnimationDuration: const Duration(milliseconds: 800),
              autoPlayCurve: Curves.fastOutSlowIn,
              enlargeCenterPage: true,
              scrollDirection: Axis.horizontal,
              onPageChanged: (index, reason) {
                selectedIndex.value = index;
              },
            ),
          ),
          Positioned(
            bottom: 10,
            right: 10,
            child: Obx(
              () => _dots(2),
            ),
          ),
        ],
      ),
    );
  }

  Widget _mapWidget() {
    if(!providerServiceState.isMarkerAdded.value){
      return LoadingWidget();
    }
    return SizedBox(
      height: 200,
      width: double.infinity,
      child: ClipRRect(
        borderRadius: const BorderRadius.all(Radius.circular(15)),
        child: Stack(
          children: [
            GoogleMap(
              initialCameraPosition: CameraPosition(target: auth.value.location??defalultLatLng, zoom: 13),
              mapToolbarEnabled: false,
              compassEnabled: false,
              myLocationEnabled: false,
              zoomControlsEnabled: false,
              markers: providerServiceState.markers.value,
              gestureRecognizers: Set()
                ..add(Factory<PanGestureRecognizer>(() => PanGestureRecognizer()))
                ..add(Factory<ScaleGestureRecognizer>(() => ScaleGestureRecognizer()))
                ..add(Factory<TapGestureRecognizer>(() => TapGestureRecognizer()))
                ..add(Factory<VerticalDragGestureRecognizer>(
                        () => VerticalDragGestureRecognizer())),
              onMapCreated: (controller) {
                providerServiceState.mapController=controller;
              },
            ),
            Positioned(
              bottom: 0,
                right: 0,
                child: Container(
                  width: 110,
              alignment: AlignmentDirectional.center,
              padding: const EdgeInsets.symmetric(
                  horizontal: 10, vertical: 8),
              decoration: BoxDecoration(
                  borderRadius: const BorderRadius.only(
                      topLeft: Radius.circular(15),
                      bottomRight: Radius.circular(15)),
                  color: primaryColorCode),
              child: InkWell(
                onTap: (){
                  Get.toNamed('/search_map_screen');
                },
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    HeaderTxtWidget(
                      isGuest.value ? 'Login' : 'Find Now',
                      color: Colors.white,
                      fontSize: 11,
                    ),
                    const SizedBox(
                      width: 10,
                    ),
                    SvgPicture.asset(Assets.svgSearch,width: 18,color: Colors.white,)
                  ],
                ),
              ),
            ))
          ],
        ),
      ),
    );
  }

  Widget _dots(int length) {
    List<Widget> list = [];
    for (int i = 0; i < length; i++) {
      list.add(AnimatedContainer(
        duration: const Duration(milliseconds: 500),
        height: 8,
        width: selectedIndex.value == i ? 28 : 8,
        margin: const EdgeInsets.symmetric(horizontal: 5),
        decoration: BoxDecoration(
            borderRadius: const BorderRadius.all(Radius.circular(10)),
            color:
                selectedIndex.value == i ? Colors.blue : Colors.grey.shade400),
      ));
    }
    return Row(
      children: list,
    );
  }
}
