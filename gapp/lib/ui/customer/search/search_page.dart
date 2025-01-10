import 'package:flutter/material.dart';
import 'package:flutter_svg/svg.dart';
import 'package:get/get.dart';
import 'package:groom/ext/hex_color.dart';
import 'package:groom/widgets/network_image_widget.dart';
import '../../../generated/assets.dart';
import '../../../repo/setting_repo.dart';
import '../../../states/customer_service_state.dart';
import '../../../utils/colors.dart';
import '../../../widgets/app_bar_widget.dart';
import '../../../widgets/button_primary_widget.dart';
import '../../../widgets/header_txt_widget.dart';
import '../../../widgets/sub_txt_widget.dart';
import '../services/provider_details_screen.dart';
import 'search_controller.dart' as se;
class SearchPage extends StatefulWidget {
  SearchPage({Key? key}) : super(key: key);

  @override
  State<SearchPage> createState() => _SearchPageState();
}

class _SearchPageState extends State<SearchPage> {
  final con = Get.put(se.SearchController());
  CustomerServiceState customerServiceState = Get.put(CustomerServiceState());
  @override
  void initState() {
    // TODO: implement initState
    super.initState();
    if(Get.arguments!=null){
      if(Get.arguments['clearFilter']){
        con.showFilter.value=false;
        con.selectedFilter.value="All";
      }
      if(Get.arguments['q'].toString().isNotEmpty){
       Future.delayed(const Duration(seconds: 1),() {
         con.searchController.text=Get.arguments['q'].toString();
         con.search(Get.arguments['q']);
       },);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: CustomScrollView(
        slivers: [
          SliverAppBar(
              backgroundColor: Colors.white,
              pinned: false,
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
                  margin: const EdgeInsets.only(bottom: 10,left: 10,right: 10),
                  alignment: AlignmentDirectional.topCenter,
                  child: TextFormField(
                    decoration: InputDecoration(
                      fillColor:Colors.white,
                      contentPadding:const EdgeInsets.symmetric(horizontal: 5, vertical: 5),
                      focusedBorder: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(10),
                        borderSide:  BorderSide(color: Colors.black.withOpacity(0.1),width: 1),
                      ),
                      errorBorder: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(10),
                        borderSide:  BorderSide(color: Colors.black.withOpacity(0.1),width: 1),
                      ),
                      errorStyle: TextStyle(color: errorColor),
                      enabledBorder: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(10),
                        borderSide:  BorderSide(color: Colors.black.withOpacity(0.1),width: 1),
                      ),
                      border:OutlineInputBorder(
                        borderRadius: BorderRadius.circular(10),
                        borderSide:  BorderSide(color: Colors.black.withOpacity(0.1),width: 1),
                      ),
                      prefixIcon:Padding(
                        padding: const EdgeInsets.all(10),
                        child: SvgPicture.asset(Assets.svgSearch,),
                      ),
                      suffixIcon: IconButton(onPressed: () {
                        con.searchController.text="";
                      }, icon: const Icon(Icons.close)),
                      focusedErrorBorder:OutlineInputBorder(
                        borderRadius: BorderRadius.circular(10),
                        borderSide:  BorderSide(color: Colors.black.withOpacity(0.1),width: 1),
                      ),
                      filled: true,
                      hoverColor: Colors.transparent,
                      hintText: "Shop name or service",
                      hintStyle:  TextStyle(
                        overflow: TextOverflow.ellipsis,
                        fontWeight: FontWeight.w400,
                        fontSize: 16,
                        color: "#5E5E5E7A".toColor().withOpacity(0.5),),
                    ),
                    controller: con.searchController,
                    onChanged: (value) {
                      con.search(value);
                    },
                    onFieldSubmitted: (value) {
                      if(value.isNotEmpty) {
                        con.updateResentSearch(value);
                      }
                    },
                  ),
                ),
              )
          ),
          SliverToBoxAdapter(
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 15,vertical: 10),
              child: Obx(() =>con.showFilter.value?_filter():Column(
                children: [
                  InkWell(
                    onTap: (){
                    con.showFilter.value=true;
                    },
                    child: Row(
                      children: [
                        SvgPicture.asset(Assets.svgFilterGray,color: Colors.blue,),
                        SubTxtWidget('Filter',color: Colors.blue,),
                      ],
                    ),
                  ),
                  ListView.builder(
                    scrollDirection: Axis.vertical,
                    itemCount: con.filterList.length,
                    shrinkWrap: true,
                    primary: false,
                    padding: EdgeInsets.zero,
                    itemBuilder: (context, index) {
                      var service = con.filterList[index];
                      return InkWell(
                        child: Container(
                          margin: const EdgeInsets.only(top: 15),
                          child: Row(
                            children: [
                              ClipRRect(
                                borderRadius: BorderRadius.circular(9),
                                child: NetworkImageWidget(url: service.provider!.photoURL,
                                  height: 100,
                                  width: 100,),
                              ),
                              const SizedBox(width: 10,),
                              Expanded(child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  HeaderTxtWidget(service.provider!.providerUserModel!.providerType != "Independent"?service.provider!.providerUserModel!.salonTitle!:service.provider!.fullName),
                                  Row(
                                    crossAxisAlignment: CrossAxisAlignment.start,
                                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                    children: [
                                      Expanded(child: Column(
                                        crossAxisAlignment: CrossAxisAlignment.start,
                                        children: [
                                          Row(
                                            mainAxisAlignment: MainAxisAlignment.spaceBetween,
                                            children: [
                                              SubTxtWidget(service.serviceType,color: "#8683A1".toColor(),),
                                              HeaderTxtWidget('\$${service.servicePrice}',fontSize: 18,
                                                color: Colors.blue,),
                                            ],
                                          ),
                                          SubTxtWidget(service.description,maxLines: 1,overflow: TextOverflow.ellipsis,),
                                        ],
                                      )),
                                      Container(
                                        margin: const EdgeInsets.only(top: 20),
                                        padding: const EdgeInsets.symmetric(horizontal: 10,vertical: 8),
                                        decoration: BoxDecoration(
                                            borderRadius: const BorderRadius.only(topLeft: Radius.circular(15),bottomRight: Radius.circular(15)),
                                            color: primaryColorCode
                                        ),
                                        child:Row(
                                          children: [
                                            HeaderTxtWidget(
                                              isGuest.value ? 'Login' : 'Book Now',
                                              color: Colors.white,
                                              fontSize: 11,
                                            ),
                                            const SizedBox(width: 10,),
                                            SvgPicture.asset(Assets.svgCalendarMark)
                                          ],
                                        ),
                                      )
                                    ],
                                  ),
                                ],
                              ))
                            ],
                          ),
                        ),
                        onTap: (){
                          if(isGuest.value){
                            Get.toNamed('/login');
                          }else {
                            Get.to(() =>  ProviderDetailsScreen(provider: service.provider!,));

                          }
                        },
                      );
                    },
                  ),
                  if(con.filterList.isEmpty)
                    Container(
                      padding: const EdgeInsets.all(50),
                      child: HeaderTxtWidget("No Data found"),
                    )
                ],
              ),),
            ),
          )
        ],
      ),
    );
  }

  Widget _filter(){
    return Container(
      padding: const EdgeInsets.symmetric(vertical: 15,horizontal: 20),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Expanded(child: InkWell(
                onTap: () {
                  con.filterType.value="Category";
                },
                child: Container(
                  alignment: AlignmentDirectional.center,
                  padding: const EdgeInsets.symmetric(vertical: 5),
                  margin: const EdgeInsets.symmetric(horizontal: 10),
                  decoration: BoxDecoration(
                      border: Border(bottom: BorderSide(
                          color:con.filterType.value=="Category"?primaryColorCode:Colors.grey.shade50
                      ))
                  ),
                  child: SubTxtWidget("Category"),
                ),
              )),
              Expanded(child: InkWell(
                onTap: () {
                  con.filterType.value="Sort by";
                },
                child: Container(
                  alignment: AlignmentDirectional.center,
                  padding: const EdgeInsets.symmetric(vertical: 5),
                  margin: const EdgeInsets.symmetric(horizontal: 10),
                  decoration: BoxDecoration(
                      border: Border(bottom: BorderSide(
                          color:con.filterType.value=="Sort by"?primaryColorCode:Colors.grey.shade50
                      ))
                  ),
                  child: SubTxtWidget("Sort by"),
                ),
              )),
              Expanded(child: InkWell(
                onTap: () {
                  con.filterType.value="Price";
                },
                child: Container(
                  alignment: AlignmentDirectional.center,
                  padding: const EdgeInsets.symmetric(vertical: 5),
                  margin: const EdgeInsets.symmetric(horizontal: 10),
                  decoration: BoxDecoration(
                      border: Border(bottom: BorderSide(
                          color:con.filterType.value=="Price"?primaryColorCode:Colors.grey.shade50
                      ))
                  ),
                  child: SubTxtWidget("Price"),
                ),
              )),
            ],
          ),
          if(con.filterType.value=="Category")
            GridView.builder(gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(crossAxisCount: 3,
            childAspectRatio: 0.9),
              itemBuilder: (context, index) {
              return Obx(() => Column(
                children: [
                  InkWell(
                    child: Container(
                      decoration: BoxDecoration(
                          shape: BoxShape.circle,
                          color: "#B28DDA".toColor(),
                          border:con.selectedFilter.value==con.category[index].name? Border.all(
                              color: "#641BB4".toColor(),
                              width: 2
                          ):null
                      ),
                      padding: const EdgeInsets.all(15),
                      height: 90,
                      child: Image.asset(con.category[index].image,color: Colors.white,),
                    ),
                    onTap: () {
                      con.selectedFilter.value=con.category[index].name;
                    },
                  ),
                  SubTxtWidget(con.category[index].name)
                ],
              ),);
            },primary: false,shrinkWrap: true,
            itemCount: con.category.length,),
          if(con.filterType.value=="Sort by")
            Column(children: [
              const SizedBox(height: 20,),
              InkWell(
                onTap: (){
                  con.filterSortBy.value="Recommended";
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
                  con.filterSortBy.value="Most Popular";
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
                  con.filterSortBy.value="Distance";
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
          if(con.filterType.value=="Price")
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
                      con.currentRangeValues.value = RangeValues(
                        50,
                        values.end.roundToDouble(),
                      );
                    }
                  },
                ),
              ],),
          ButtonPrimaryWidget("search",onTap: () {
            con.filterData();
          },color: '#641BB4'.toColor(),)
        ],
      ),
    );
  }
}
