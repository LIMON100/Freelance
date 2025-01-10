import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:groom/generated/assets.dart';
import 'package:groom/ui/customer/customer_dashboard/model/resent_search_model.dart';

import '../../../data_models/provider_service_model.dart';
import '../../../firebase/provider_service_firebase.dart';
import '../../../firebase/user_firebase.dart';
import 'model/category_model.dart';

class SearchController extends GetxController {
  var searchController = TextEditingController();
  ProviderServiceFirebase providerServiceFirebase = ProviderServiceFirebase();
  RxList<ProviderServiceModel> oldList = RxList();
  RxList<ProviderServiceModel> filterList = RxList();
  RxList<ResentSearchModel> resentSearchList = RxList();
  UserFirebase userFirebase = UserFirebase();
  RxString selectedFilter = RxString("All");
  RxBool showFilter=false.obs;
  RxString filterType=RxString("Category");
  RxString filterSortBy=RxString("");
  Rx<RangeValues> currentRangeValues = Rx(const RangeValues(5, 2000));
  RxList<CategoryModel>category=RxList();
  RxBool isLoading=false.obs;
  @override
  void onInit() {
    super.onInit();
    _fetchServices();
    initCategory();
  }
  void initCategory(){
    category.add(CategoryModel(name: 'Hair Cut', image: Assets.imgHirecut));
    category.add(CategoryModel(name: 'Nails', image: Assets.imgNails));
    category.add(CategoryModel(name: 'Facial', image: Assets.imgFacial));
    category.add(CategoryModel(name: 'Coloring', image: Assets.imgColoringIcon));
    category.add(CategoryModel(name: 'Spa', image: Assets.imgSpaIcon));
    category.add(CategoryModel(name: 'Wax', image: Assets.imgWaxingIcon));
    category.add(CategoryModel(name: 'Make up', image: Assets.imgMakeupIcon));
    category.add(CategoryModel(name: 'Massage', image: Assets.imgMassageIcon));
  }

  void search(String q) {
    filterList.clear();
    for (var element in oldList) {
        if (element.serviceName!.toLowerCase().contains(q.toLowerCase()) ||element.serviceType.toLowerCase().contains(q.toLowerCase()) ||
            (element.provider!.providerUserModel!.providerType != "Independent"
                    ? element.provider!.providerUserModel!.salonTitle!
                    : element.provider!.fullName)
                .toLowerCase()
                .contains(q.toLowerCase())) {
          filterList.add(element);
        }
      }
  }
  void filterData() {
    showFilter.value=false;
    filterList.clear();
    for (var element in oldList) {
      if(filterType.value=="Category"){
        if (element.serviceType.toLowerCase().contains(selectedFilter.toLowerCase())) {
          filterList.add(element);
        }
      }
      if(filterType.value=="Price"){
        if (currentRangeValues.value.end.toInt()>=element.servicePrice!) {
          filterList.add(element);
        }
      }
      }
  }
  void _fetchServices() async {
    isLoading.value=true;
    var services = await providerServiceFirebase.getAllServices();
    oldList.value = services;
    isLoading.value=false;
  }
  void updateResentSearch(q){
    userFirebase.addResentSearch({
      "query":q
    });
  }
  void deleteResentSearch(id){
     userFirebase.deleteResentSearch(id);
  }
  void deleteAllResentSearch(){
     userFirebase.deleteAllResentSearch();
  }
  void getResentSearch(){
   userFirebase.getResentSearch().then((value) {
     resentSearchList.value=value.reversed.toList();
   },);
  }
  List<ProviderServiceModel> getFilteredServices() {
    List<ProviderServiceModel> filteredServices = oldList;
    if (selectedFilter != "All") {
      filteredServices = filteredServices
          .where((service) => service.serviceType == selectedFilter)
          .toList();
    }


    return filteredServices;
  }
}
