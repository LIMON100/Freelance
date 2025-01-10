import 'package:get/get.dart';
import 'package:firebase_auth/firebase_auth.dart';
import '../../firebase/user_firebase.dart';

class ServiceController extends GetxController {
  final UserFirebase userFirebase;
  final String serviceId;
  var isFavorite = false.obs;

  ServiceController({required this.userFirebase, required this.serviceId});

  @override
  void onInit() {
    super.onInit();
    _loadFavoriteStatus();
  }

  void _loadFavoriteStatus() async {
    bool favoriteStatus = await userFirebase.checkServiceFavorite(
        FirebaseAuth.instance.currentUser!.uid, serviceId);
    isFavorite.value = favoriteStatus;
  }

  void toggleFavorite() async {
    await userFirebase.addServiceToFavorites(
        FirebaseAuth.instance.currentUser!.uid, serviceId);
    isFavorite.value = !isFavorite.value;
  }
}
