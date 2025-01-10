import 'package:firebase_dynamic_links/firebase_dynamic_links.dart';

Future<Uri>createDynamicLink({required String type,required String id}) async {
  final dynamicLinkParams = DynamicLinkParameters(
    link: Uri.parse("https://www.joingroom.com?type=$type&id=$id"),
    uriPrefix: "https://groomapp.page.link",
    androidParameters: const AndroidParameters(packageName: "com.app.groomapp.groom"),
    iosParameters: const IOSParameters(bundleId: "com.app.groomapp.groom"),
  );
  final dynamicLink =
      await FirebaseDynamicLinks.instance.buildLink(dynamicLinkParams);
  return dynamicLink;
}