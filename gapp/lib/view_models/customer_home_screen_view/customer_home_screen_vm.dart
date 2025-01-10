import 'package:groom/data_models/user_model.dart';

import '../../data_models/provider_service_model.dart';

abstract class CustomerHomeScreenViewModel {
Future<List<ProviderServiceModel>> displayServiceList();
Future<List<UserModel>> displayAllFeaturedProvider();
}