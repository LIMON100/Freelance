import 'package:groom/data_models/provider_service_model.dart';
import 'package:groom/data_models/user_model.dart';
import 'package:groom/firebase/provider_service_firebase.dart';
import 'package:groom/firebase/provider_user_firebase.dart';
import 'package:groom/view_models/customer_home_screen_view/customer_home_screen_vm.dart';

class CustomerHomeScreenViewModelImp implements CustomerHomeScreenViewModel{

ProviderServiceFirebase providerServiceFirebase =ProviderServiceFirebase();
ProviderUserFirebase providerUserFirebase = ProviderUserFirebase();

  @override
  Future<List<ProviderServiceModel>> displayServiceList() {

    throw providerServiceFirebase.getAllServices();
  }

  @override
  Future<List<UserModel>> displayAllFeaturedProvider() {

    throw providerUserFirebase.getAllProviders();
  }


}