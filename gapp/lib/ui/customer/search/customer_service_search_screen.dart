import 'package:cached_network_image/cached_network_image.dart';
import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/firebase/provider_service_firebase.dart';
import 'package:groom/firebase/user_firebase.dart';
import 'package:groom/repo/setting_repo.dart';
import 'package:groom/widgets/guest_widget.dart';
import 'package:groom/widgets/header_txt_widget.dart';
import 'package:groom/widgets/loading_widget.dart';
import '../../../generated/assets.dart';
import '../../../states/customer_service_state.dart';
import '../../../utils/colors.dart';
import '../../../widgets/app_bar_widget.dart';
import '../../../widgets/sub_txt_widget.dart';
import '../services/service_details_screen.dart';
import '../services/provider_details_screen.dart';
import 'search_controller.dart' as se;
class SearchScreen extends StatefulWidget {
  bool showBackButton=false;
   SearchScreen({Key? key,this.showBackButton=false}) : super(key: key);

  @override
  State<SearchScreen> createState() => _SearchScreenState();
}

class _SearchScreenState extends State<SearchScreen> {
  final con = Get.put(se.SearchController());
  final ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  CustomerServiceState customerServiceState = Get.put(CustomerServiceState());
  final UserFirebase userFirebase = UserFirebase();
  final List<String> filters = [
    "All",
    "Makeup",
    "Hair Style",
    "Nails",
    "Coloring",
    "Wax",
    "Spa",
    "Massage",
    "Facial"
  ];
  final List<String> sortOptions = ["Rating", "Price"];

  @override
  void initState() {
    super.initState();
  }



  @override
  Widget build(BuildContext context) {
    con.getResentSearch();
    return Scaffold(
      body: CustomScrollView(
        slivers: [
          SliverAppBar(
              backgroundColor: Colors.white,
              pinned: false,
              automaticallyImplyLeading: widget.showBackButton,
              iconTheme: IconThemeData(
                color: primaryColorCode
              ),
              floating: true,
              snap: true,
              toolbarHeight: 60,
              title:  AppBarWidget(
                hideIcon: true,
              ),
              surfaceTintColor: Colors.white,
              bottom:  PreferredSize(
                preferredSize: const Size.fromHeight(50),
                child: Container(
                  padding: const EdgeInsets.all(10),
                  margin: const EdgeInsets.symmetric(horizontal: 10,vertical: 10),
                  alignment: AlignmentDirectional.topCenter,
                  decoration: BoxDecoration(
                    borderRadius: const BorderRadius.all(Radius.circular(10)),
                    border: Border.all(color: Colors.grey,width: 1),
                  ),
                  child: Row(
                    children: [
                      SvgPicture.asset(Assets.svgSearch,),
                  const SizedBox(width: 10,),
                  Expanded(child: InkWell(
                    onTap: () {
                      Get.toNamed('/search',arguments: {
                        "q":"",
                        "clearFilter":true
                      });
                    },
                    child: SubTxtWidget('Shop name or service', fontWeight: FontWeight.w400,
                        fontSize: 16,
                        color: "#5E5E5E7A".toColor().withOpacity(0.5)),
                  )),
                      InkWell(
                        onTap: (){
                          Get.toNamed('/search_map_screen');
                        },
                        child: Row(
                          children: [
                            SvgPicture.asset(Assets.svgFilterGray),
                            SubTxtWidget('Map',),
                          ],
                        ),
                      )
                    ],
                  ),
                ),
              )
          ),
          SliverToBoxAdapter(
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 10),
            child: Obx(() => Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    HeaderTxtWidget('Recently searched'),
                    TextButton(onPressed: () {
                      con.deleteAllResentSearch();
                      con.resentSearchList.clear();
                    }, child: SubTxtWidget('Clear all',color: Colors.blue,))
                  ],
                ),
                ListView.builder(itemBuilder: (context, index) {
                  return Container(
                    padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 8),
                    child: InkWell(
                      child: Row(
                        children: [
                          Icon(Icons.search,color: Colors.grey.shade400,),
                          const SizedBox(width: 10,),
                          Expanded(child: SubTxtWidget(con.resentSearchList[index].query!),),
                          InkWell(onTap: (){
                            con.deleteResentSearch(con.resentSearchList[index].id);
                            con.resentSearchList.removeAt(index);
                          }, child: const Icon(Icons.close,size: 20,
                            color: Colors.blue,)),
                        ],
                      ),
                      onTap: () {
                        con.searchController.text=con.resentSearchList[index].query!;
                        con.search(con.resentSearchList[index].query!);
                        Get.toNamed('/search');
                      },
                    ),
                  );
                },itemCount: con.resentSearchList.length>5?5:con.resentSearchList.length,
                  shrinkWrap: true,primary: false,padding: EdgeInsets.zero,),
                const Divider(
                  height: 20,
                ),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    HeaderTxtWidget('Trending near you'),
                    InkWell(
                      onTap: (){
                        con.showFilter.value=true;
                        Get.toNamed('/search');
                      },
                      child: Row(
                        children: [
                          SvgPicture.asset(Assets.svgFilterGray,color: Colors.blue,),
                          SubTxtWidget('Filter',color: Colors.blue,),
                        ],
                      ),
                    )
                  ],
                ),
                const SizedBox(height: 20,),
                SizedBox(
                  height: 220,
                  child:con.isLoading.value?LoadingWidget(type: LoadingType.DASHBOARD):ListView.builder(
                    shrinkWrap: true,
                    scrollDirection: Axis.horizontal,
                    itemCount: con.oldList.length,
                    itemBuilder: (context, index) {
                      var service = con.oldList[index];
                      return InkWell(
                        onTap: (){
                          if(isGuest.value){
                            Get.to(GuestWidget(message: "Login to view details"));
                          }else {
                            Get.to(() => ProviderDetailsScreen(provider: service.provider!,));
                          }
                        },
                        child: Container(
                          width: 200,
                          margin: const EdgeInsets.symmetric(horizontal: 5),
                          child: Column(
                            mainAxisAlignment: MainAxisAlignment.start,
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              SizedBox(
                                width:200,
                                height:120,
                                child: ClipRRect(
                                  borderRadius:
                                  BorderRadius.circular(12),
                                  child: CachedNetworkImage(
                                    imageUrl:
                                    service.serviceImages!.first,
                                    fit: BoxFit.fill,
                                  ),
                                ),
                              ),
                              const SizedBox(
                                height: 5,
                              ),
                              Container(
                                padding: const EdgeInsets.symmetric(horizontal: 10),
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    SubTxtWidget(service.serviceType,color: "#8F90A6".toColor(),fontSize: 10,),
                                    HeaderTxtWidget(service.provider!.providerUserModel!.providerType != "Independent"?service.provider!.providerUserModel!.salonTitle!:service.provider!.fullName),
                                    SubTxtWidget(service.description,maxLines: 1,overflow: TextOverflow.ellipsis,),
                                    Row(
                                      crossAxisAlignment: CrossAxisAlignment.start,
                                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                      children: [
                                        Row(
                                          children: [
                                            SvgPicture.asset(Assets.svgStar),
                                            const SizedBox(width: 5,),
                                            SubTxtWidget('${service.provider!.providerUserModel!.overallRating}',color: "#8683A1".toColor(),),
                                          ],
                                        ),
                                        HeaderTxtWidget('\$${service.provider!.providerUserModel!.basePrice}',color: Colors.blue,),
                                        const SizedBox(width: 20,),
                                      ],
                                    ),
                                  ],
                                ),
                              )
                            ],
                          ),
                        ),
                      );
                    },
                  ),
                ),
                const Divider(),
                const SizedBox(height: 5,),
                HeaderTxtWidget('Try these services'),
                SizedBox(
                  height: 180,
                  width: MediaQuery.of(context).size.width,
                  child:con.isLoading.value?LoadingWidget(type: LoadingType.DASHBOARD,):ListView.builder(
                    primary: true,
                    scrollDirection: Axis.horizontal,
                    itemCount: con.oldList.length,
                    itemBuilder: (context, index) {
                      var service = con.oldList[index];
                      return InkWell(
                        onTap: (){
                          if(isGuest.value){
                            Get.toNamed('/login');
                          }else {
                            customerServiceState
                                .selectedService.value = con.oldList[index];
                            Get.to(() =>  ServiceDetailsScreen());
                          }
                        },
                        child: Container(
                          width: 150,
                          margin: const EdgeInsets.symmetric(horizontal: 10,vertical: 10),
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.center,
                            children: [
                              SizedBox(
                                  width: 90,
                                  height: 90,
                                  child: ClipRRect(
                                    borderRadius: const BorderRadius.all(Radius.circular(60)),
                                    child: CachedNetworkImage(
                                      imageUrl: service.serviceImages!.first,
                                      fit: BoxFit.fill,
                                      height: 90,
                                      width: 90,
                                    ),
                                  )
                              ),
                              const SizedBox(height: 10,),
                              HeaderTxtWidget(service.serviceName!),
                            ],
                          ),
                        ),
                      );
                    },
                    padding: EdgeInsets.zero,
                  ),
                )
              ],
            ),),),

          ),
        ],
      ),
    );
  }
}
